#include <sktbdpch.h>
#include "AGCPipeline.h"
#include "AGCBuffer.h"
//#include "AGCSrtSignature.h"

#define SKTBD_LOG_COMPONENT "PIPELINE_IMPL"
#include "Skateboard/Log.h"

namespace Skateboard
{
	struct SRT
	{
		sce::Agc::Core::Buffer gPassBuffer;
		sce::Agc::Core::Buffer gVertices;
	};

	void LoadShader(const wchar_t* filename, AGCShader& out_shader)
	{
		// Sanity checks
		if (!filename)
			return;
		SKTBD_CORE_ASSERT(!out_shader.pShader, "The shader stage has already been loaded.\
			Make sure you use only one of the Vertex, Domain or Geometry entries in your description!");

		// Check that the shader file exists
		std::filesystem::path path(SanitizeFilePath(filename));
		path.replace_extension(".ags");
		SKTBD_CORE_INFO("loading shader from {}", path.c_str());
		SKTBD_CORE_ASSERT(std::filesystem::exists(path), "Shader file does not exist!");

		// Read raw data
		size_t size = std::filesystem::file_size(path);
		char* streamBinary = new char[size];
		std::ifstream file(path, std::ios::binary);
		file.read(streamBinary, size);
		file.close();

		// Retrieve the shader header and GPU code
		// We will also retrieve the SRT signature from the shader metadata so that we can make the appropriate bindings
		// Read: https://p.siedev.net/resources/documents/SDK/7.000/Shader_Reflection-Overview/0002.html#__document_toc_00000006
		const SceShaderBinaryHandle sl = sceShaderGetBinaryHandle(streamBinary);
		GetSrtSignature(sl, out_shader);
		const size_t headerSize = sceShaderGetProgramHeaderSize(sl);
		const size_t codeSize = sceShaderGetProgramSize(sl);
		out_shader.Binary.Header = gAGCContext->GetMemAllocator()->TypedAlignedAllocate<uint8_t, sce::Agc::Alignment::kShaderHeader>(headerSize);
		out_shader.Binary.Code = gAGCContext->GetMemAllocator()->TypedAlignedAllocate<uint8_t, sce::Agc::Alignment::kShaderCode>(codeSize);
		memcpy(out_shader.Binary.Header.data, sceShaderGetProgramHeader(sl), headerSize);
		memcpy(out_shader.Binary.Code.data, sceShaderGetProgram(sl), codeSize);
		delete[] streamBinary;

		// Create the shader on AGC
		SceError error = sce::Agc::createShader(&out_shader.pShader, out_shader.Binary.Header.data, out_shader.Binary.Code.data);
		SCE_AGC_ASSERT(error == SCE_OK);

#ifndef SKTBD_SHIP
		sce::Agc::Core::registerResource(out_shader.pShader, path.filename().c_str());
#endif
	}

	void GetSrtSignature(const _SceShaderBinaryHandle* sl, AGCShader& out_shader)
	{
		// md = MetaData
		// lh = list handle
		// r = resource
		// rc = resource class
		// th = type handle
		// tc = type class
		// m = member
		const SceShaderMetadataSectionHandle md = sceShaderGetMetadataSection(sl);
		const SceShaderResourceListHandle lh = sceShaderGetResourceList(md);

		// Loop through all resources from the list
		for (SceShaderResourceHandle r = sceShaderGetFirstResource(md, lh); r != nullptr; r = sceShaderGetNextResource(md, r))
		{
			const SceShaderResourceClass rc = sceShaderGetResourceClass(md, r);
			/*int32_t resourceSlot = sceShaderGetResourceApiSlot(md, r);
			if (rc == SceShaderSrv || rc == SceShaderCb || rc == SceShaderUav || rc == SceShaderSamplerState)
			{
				const SceShaderTypeHandle th = sceShaderGetResourceType(md, r);
				const SceShaderTypeClass tc = sceShaderGetTypeClass(md, th);

				SceShaderTypeHandle const bufferElemType = sceShaderGetBufferElementType(md, th);
				SceShaderBufferClass const bufferClass = sceShaderGetBufferClass(md, bufferElemType);
				int bufferSize = sceShaderGetStructSize(md, bufferElemType);

				SignatureParameter param = {};
				param.MemberSize = bufferSize;
				param.RegisterSlot = resourceSlot;

			}*/
			if (rc != SceShaderSrt)
				continue;

			const SceShaderTypeHandle th = sceShaderGetResourceType(md, r);
			const SceShaderTypeClass tc = sceShaderGetTypeClass(md, th);
			if (tc != SceShaderStructType)
				continue;

			for (SceShaderMemberHandle m = sceShaderGetFirstMember(md, th); nullptr != m; m = sceShaderGetNextMember(md, m))
			{
				SignatureParameter param = {};
				param.MemberOffset = sceShaderGetMemberOffset(md, m);
				param.MemberSize = sceShaderGetMemberSize(md, m);

				// Descriptor tables are just pointers, so they wont contain a type, register slot or register space
				if (!sceShaderIsSrtSignatureResource(md, m))
				{
					// TODO:
					const SceShaderTypeHandle mt = sceShaderGetMemberType(md, m);
					const SceShaderTypeClass mc = sceShaderGetTypeClass(md, mt);
					if (mc != SceShaderPointerType)	// Ensure this is a descriptor table
						continue;
					param.ElementType = ShaderElementType_DescriptorTable;
				}
				// Other entries provide all the necessary information
				else
				{
					param.ElementType = AGCResourceClassToShaderElementType(sceShaderGetSrtSignatureResourceClass(md, m));
					param.RegisterSlot = sceShaderGetSrtSignatureResourceSlotIndex(md, m);
					param.RegisterSpace = sceShaderGetSrtSignatureResourceSpaceIndex(md, m);
				}

				out_shader.SRT.SignatureRef().SizeInBytes += param.MemberSize;
				//SKTBD_CORE_ASSERT(out_shader.SRT.SignatureRef().SizeInBytes < 16u * 4u, "The SRT signature is too large! Did the PSSL even let it compile? (max 16 DWORDS)");
				out_shader.SRT.SignatureRef().vParameters.emplace_back(param);
			}
		}
	}

	AGCRasterizationPipeline::AGCRasterizationPipeline(const std::wstring& debugName, const RasterizationPipelineDesc& desc) :
		RasterizationPipeline(debugName, desc),
		m_HullShader{},
		m_PrimitiveShader{},
		m_PixelShader{},
		m_VertexBufferSlot(0u)
	{
		// Load Shaders and Link them
		LoadAllShaders();
		
		// The previous stage also retrieved the SRT signature from the shader metadata,
		// so we can now create the Shader Resource Table from the input resources in the
		// description and verify that all resources are bound at the correct locations
		CreateShaderResourceTable();


		// Completely describe how we want our primitives to be rendered.
		ConfigurePipelineSetup();

		// Initialise Binders (avoid redundancy)
		m_HullBinder.init();
		m_PrimitiveBinder.init();
		m_PixelBinder.init();

	}

	AGCRasterizationPipeline::~AGCRasterizationPipeline()
	{
		// Release the shaders direct memory
		if (m_HullShader.pShader) gAGCContext->GetMemAllocator()->TypedAlignedFree(m_HullShader.Binary.Header), gAGCContext->GetMemAllocator()->TypedAlignedFree(m_HullShader.Binary.Code);
		if (m_PrimitiveShader.pShader) gAGCContext->GetMemAllocator()->TypedAlignedFree(m_PrimitiveShader.Binary.Header), gAGCContext->GetMemAllocator()->TypedAlignedFree(m_PrimitiveShader.Binary.Code);
		if (m_PixelShader.pShader) gAGCContext->GetMemAllocator()->TypedAlignedFree(m_PixelShader.Binary.Header), gAGCContext->GetMemAllocator()->TypedAlignedFree(m_PixelShader.Binary.Code);
	}

	void AGCRasterizationPipeline::Bind()
	{
		// Get API objects
		sce::Agc::DrawCommandBuffer& dcb = gAGCContext->GetDrawCommandBuffer();
		sce::Agc::Core::StateBuffer& sb = gAGCContext->GetStateBuffer();

		// Bind the shaders
		// We need to first supply the linkages. These do not supply the shaders, but rather describe the pipeline that will be executed
		sb.setState(m_CxLinkage);
		sb.setState(m_UcLinkage);

		//recreate a resource table
		CreateShaderResourceTable();
		
		// Now we can bind the shaders to the state buffer & to the binders
		// When using a particular binder, we will also allocate memory on the draw command buffer for the shader resources
		if (m_HullShader.pShader)
		{
			// Set the shader on the state
			sb.setShader(m_HullShader.pShader);

			// Reset the binder with the shader to associate with
			m_HullBinder.reset().setShader(m_HullShader.pShader);

			if (m_HullBinder.getUserDataSizeInBytes())
			{
				// Allocate memory on the dcb for user data
				sce::Agc::ShRegister* pHullUserData = (sce::Agc::ShRegister*)dcb.allocateTopDown({ m_HullBinder.getUserDataSizeInBytes(), sce::Agc::Alignment::kRegister });
				m_HullBinder.setUserDataPointer(pHullUserData);

				// Set the Shader Resource Table (contains all the expected resources)
				m_HullShader.SRT.GenerateTableOnDCB(dcb);
				m_HullBinder.setUserSrtBuffer(m_HullShader.SRT.GetTable(), m_HullShader.SRT.GetSizeInDwords());

				// Since the blocks have already been allocated and filled with our shader resource of time the only
				// thing we have to do each draw is just insert the packets to set the indirect register arrays.
				dcb.setShRegistersIndirect(pHullUserData, m_HullBinder.getUserDataSizeInElements());
			}
		}
		if (m_PrimitiveShader.pShader)
		{
			// Set the shader on the state
			sb.setShader(m_PrimitiveShader.pShader);

			// Reset the binder with the shader to associate with
			m_PrimitiveBinder.reset().setShader(m_PrimitiveShader.pShader);

			if (m_PrimitiveBinder.getUserDataSizeInBytes())
			{
				// Allocate memory on the dcb for user data
				sce::Agc::ShRegister* pPrimitiveUserData = (sce::Agc::ShRegister*)dcb.allocateTopDown({ m_PrimitiveBinder.getUserDataSizeInBytes(), sce::Agc::Alignment::kRegister });
				m_PrimitiveBinder.setUserDataPointer(pPrimitiveUserData);

				// Set the Shader Resource Table (contains all the expected resources)
				m_PrimitiveShader.SRT.GenerateTableOnDCB(dcb);
				m_PrimitiveBinder.setUserSrtBuffer(m_PrimitiveShader.SRT.GetTable(), m_PrimitiveShader.SRT.GetSizeInDwords());

				// Since the blocks have already been allocated and filled with our shader resource of time the only
				// thing we have to do each draw is just insert the packets to set the indirect register arrays.
				dcb.setShRegistersIndirect(pPrimitiveUserData, m_PrimitiveBinder.getUserDataSizeInElements());
			}
		}
		if (m_PixelShader.pShader)
		{
			// Set the shader on the state
			sb.setShader(m_PixelShader.pShader);

			// Reset the binder with the shader to associate with
			m_PixelBinder.reset().setShader(m_PixelShader.pShader);

			if (m_PixelBinder.getUserDataSizeInBytes())
			{
				// Allocate memory on the dcb for user data
				sce::Agc::ShRegister* pPixelUserData = (sce::Agc::ShRegister*)dcb.allocateTopDown({ m_PixelBinder.getUserDataSizeInBytes(), sce::Agc::Alignment::kRegister });
				m_PixelBinder.setUserDataPointer(pPixelUserData);

				// Set the Shader Resource Table (contains all the expected resources)
				m_PixelShader.SRT.GenerateTableOnDCB(dcb);
				m_PixelBinder.setUserSrtBuffer(m_PixelShader.SRT.GetTable(), m_PixelShader.SRT.GetSizeInDwords());

				// Since the blocks have already been allocated and filled with our shader resource of time the only
				// thing we have to do each draw is just insert the packets to set the indirect register arrays.
				dcb.setShRegistersIndirect(pPixelUserData, m_PixelBinder.getUserDataSizeInElements());
			}
		}
	}

	void AGCRasterizationPipeline::Unbind()
	{
		sce::Agc::Core::StateBuffer& sb = gAGCContext->GetStateBuffer();

		// We need to call postDraw on the components. We don't need to call postBatch on the
		// IndirectStageBinders since we didn't use them at all between draw calls.
		sb.postDraw();
	}

	void AGCRasterizationPipeline::Release()
	{
	}

	void AGCRasterizationPipeline::SetVertexBuffer(AGCVertexBuffer* pVB, uint32_t vertexOffset)
	{
		// TODO: I dont like this, find a better way
		//m_PrimitiveBinder.setBuffers(m_PrimitiveShader.SRT.GetVertexBufferSlot(), 1, &pVB->GetResource());

		/*if (vertexOffset != 0)
			m_PrimitiveBinder.setVertexOffset(vertexOffset);*/
	}

	void AGCRasterizationPipeline::LoadAllShaders()
	{
		LoadShader(m_Desc.HullShader.FileName, m_HullShader);
		LoadShader(m_Desc.VertexShader.FileName, m_PrimitiveShader);		// All of these 3 are combined in the same stage! only provide one!
		LoadShader(m_Desc.DomainShader.FileName, m_PrimitiveShader);		//
		LoadShader(m_Desc.GeometryShader.FileName, m_PrimitiveShader);		//
		LoadShader(m_Desc.PixelShader.FileName, m_PixelShader);

		
		// Link our GS/PS ahead of time so that we don't have to do it every frame.
		SceError error = sce::Agc::Core::linkShaders(
			&m_CxLinkage,
			&m_UcLinkage,
			m_HullShader.pShader,
			m_PrimitiveShader.pShader,
			m_PixelShader.pShader,
			sce::Agc::UcPrimitiveType::Type::kTriList
		);
		SCE_AGC_ASSERT(error == SCE_OK);
	}

	void AGCRasterizationPipeline::CreateShaderResourceTable()
	{
		// THIS IS REDUNDANT, WE COULD JUST COPPY DESCRIPTORS STRAIGHT INTO THE SRT AND THEN POINT TO THEM
		// BIND  UNBIND CONCEPT?
		// BIND ALL DESCRIPTORS
		// DRAW DRAW
		// SWAP DESCRIPTOR TABLE / DESCRIPTOR
		// DRAW DRAW DRAW DRAW DRAW DRAW DRAW DRAW
		// UNBIND


		// POSSIBLE OPTIMIZATIONS
		// CONCEPT OF CONST RESOURCES 
		// PER FRAME
		// PER Instance
		// PER PerObject

		// UNSUPPPORTED: Root Constants -> Need to be an inlined CBV!
		// CBVs
		// SRVs
		// UAVs
		// Descriptor Tables

		// I do not want to allocate the SRTs!
		// What I need to do is to sort my input buffers based on the signature (kinda).
		// That way, when calling Bind(), I can create the SRT on the top-down DCB with my input resources.
		// It may be nice to create the SRTs on here to avoid expensive operations, but the problem will be (as always) frame resources.
		// In short:
		// 1) Create a system to match an input resource with a shader.Signature.param
		//    -> If it matches with any param, then hoorays
		//    -> If it does not match with any param, then assert(false)
		//    -> This system should also probably put the input resources in the Signature order for easy binding
		// 2) Create the SRTs on the top-down DCB and memcpy the step 1)
		// 3) Figure out the frame resource problem
		auto AddToShaderSRTBasedOnVisibility = [&](const ShaderResourceDesc& desc) -> void {
			switch (desc.ShaderVisibility)
			{
			case ShaderVisibility_All:
				m_HullShader.SRT.AddResource(desc);
				m_PrimitiveShader.SRT.AddResource(desc);
				m_PixelShader.SRT.AddResource(desc);
				break;
			case ShaderVisibility_HullShader:
				m_HullShader.SRT.AddResource(desc);
				break;
			case ShaderVisibility_VertexShader:
			case ShaderVisibility_DomainShader:
			case ShaderVisibility_GeometryShader:
				m_PrimitiveShader.SRT.AddResource(desc);
				break;
			case ShaderVisibility_PixelShader:
				m_PixelShader.SRT.AddResource(desc);
				break;
			default:
				SKTBD_CORE_ASSERT(false, "The input Shader Visibility for this resource is invalid. Make sure you are not using raytracing visibilities!");
				break;
			}
		};
		auto AddSamplerToShaderSRTBasedOnVisibility = [&](const SamplerDesc& desc) -> void {
			switch (desc.ShaderVisibility)
			{
			case ShaderVisibility_All:
				m_HullShader.SRT.AddSampler(desc);
				m_PrimitiveShader.SRT.AddSampler(desc);
				m_PixelShader.SRT.AddSampler(desc);
				break;
			case ShaderVisibility_HullShader:
				m_HullShader.SRT.AddSampler(desc);
				break;
			case ShaderVisibility_VertexShader:
			case ShaderVisibility_DomainShader:
			case ShaderVisibility_GeometryShader:
				m_PrimitiveShader.SRT.AddSampler(desc);
				break;
			case ShaderVisibility_PixelShader:
				m_PixelShader.SRT.AddSampler(desc);
				break;
			default:
				SKTBD_CORE_ASSERT(false, "The input Shader Visibility for this resource is invalid. Make sure you are not using raytracing visibilities!");
				break;
			}
		};

		for (const ShaderResourceDesc desc : m_Desc.vRootConstants)
		{
			SKTBD_CORE_ASSERT(false, "");
		}

		for (const ShaderResourceDesc& desc : m_Desc.vCBV)
			AddToShaderSRTBasedOnVisibility(desc);

		for (const ShaderResourceDesc& desc : m_Desc.vSRV)
			AddToShaderSRTBasedOnVisibility(desc);

		for (const ShaderResourceDesc& desc : m_Desc.vUAV)
			AddToShaderSRTBasedOnVisibility(desc);

		for (const ShaderResourceDesc& desc : m_Desc.vDescriptorTables)
			AddToShaderSRTBasedOnVisibility(desc);

		for (const SamplerDesc& desc : m_Desc.vStaticSamplers)
			AddSamplerToShaderSRTBasedOnVisibility(desc);

		m_HullShader.SRT.VerifyTable();
		m_PrimitiveShader.SRT.VerifyTable();
		m_PixelShader.SRT.VerifyTable();
	}

	void AGCRasterizationPipeline::ConfigurePipelineSetup()
	{

		if (m_Desc.Wireframe){
			m_PrimitiveSetup.setFrontPolygonMode(sce::Agc::CxPrimitiveSetup::FrontPolygonMode::kLine);
		}
		else{
			m_PrimitiveSetup.setFrontPolygonMode(sce::Agc::CxPrimitiveSetup::FrontPolygonMode::kFill);
		}

		// TODO: Switch to using flags? Can describe pipeline in a more compact way...
		/*if (m_Desc.PipelineFlags & FillMode_Fill) {
			m_PrimitiveSetup.setFrontPolygonMode(sce::Agc::CxPrimitiveSetup::FrontPolygonMode::kFill);
		}
		else if (m_Desc.PipelineFlags & FillMode_Wireframe) {
			m_PrimitiveSetup.setFrontPolygonMode(sce::Agc::CxPrimitiveSetup::FrontPolygonMode::kLine);
		}
		else if (m_Desc.PipelineFlags & FillMode_Point) {
			m_PrimitiveSetup.setFrontPolygonMode(sce::Agc::CxPrimitiveSetup::FrontPolygonMode::kPoint);
		}

		if (m_Desc.PipelineFlags & CullMode_Front){
			if (m_Desc.PipelineFlags & WindingOrder_CCW) {
				m_PrimitiveSetup.setFrontFace(sce::Agc::CxPrimitiveSetup::FrontFace::kCcw);
			}
			else if (m_Desc.PipelineFlags & WindingOrder_CW) {
				m_PrimitiveSetup.setFrontFace(sce::Agc::CxPrimitiveSetup::FrontFace::kCw);
			}
			m_PrimitiveSetup.setCullFace(sce::Agc::CxPrimitiveSetup::CullFace::kBack);
		}
		else{
			if (m_Desc.PipelineFlags & CullMode_Back) {
				m_PrimitiveSetup.setCullFace(sce::Agc::CxPrimitiveSetup::CullFace::kFront);
			}
		}*/
		
	}

	AGCComputePipeline::AGCComputePipeline(const std::wstring& debugName, const ComputePipelineDesc& desc) :
		ComputePipeline(debugName, desc),
		m_ComputeShader{}
	{
		// Load the shader
		LoadShader(m_Desc.ComputeShader.FileName, m_ComputeShader);

		// Create the shader resource table
		CreateShaderResourceTable();

		// Init binder
		m_Binder.init();
	}

	AGCComputePipeline::~AGCComputePipeline()
	{
	}

	void AGCComputePipeline::Bind()
	{
		// Get API objects
		sce::Agc::DrawCommandBuffer& dcb = gAGCContext->GetDrawCommandBuffer();
		sce::Agc::Core::StateBuffer& sb = gAGCContext->GetStateBuffer();

		// Set the shader
		sb.setShader(m_ComputeShader.pShader);
		m_Binder.reset().setShader(m_ComputeShader.pShader);

		// Allocate memory on the dcb for user data
		sce::Agc::ShRegister* pPixelUserData = (sce::Agc::ShRegister*)dcb.allocateTopDown({ m_Binder.getUserDataSizeInBytes(), sce::Agc::Alignment::kRegister });
		m_Binder.setUserDataPointer(pPixelUserData);

		// Set the Shader Resource Table (contains all the expected resources)
		m_ComputeShader.SRT.GenerateTableOnDCB(dcb);
		m_Binder.setUserSrtBuffer(m_ComputeShader.SRT.GetTable(), m_ComputeShader.SRT.GetSizeInDwords());

		// Since the blocks have already been allocated and filled with our shader resource of time the only
		// thing we have to do each draw is just insert the packets to set the indirect register arrays.
		dcb.setShRegistersIndirect(pPixelUserData, m_Binder.getUserDataSizeInElements());
	}

	void AGCComputePipeline::CreateShaderResourceTable()
	{
		for (const ShaderResourceDesc desc : m_Desc.vRootConstants)
		{
			SKTBD_CORE_ASSERT(false, "");
		}

		for (const ShaderResourceDesc desc : m_Desc.vCBV)
			m_ComputeShader.SRT.AddResource(desc);

		for (const ShaderResourceDesc desc : m_Desc.vSRV)
			m_ComputeShader.SRT.AddResource(desc);

		for (const ShaderResourceDesc desc : m_Desc.vUAV)
			m_ComputeShader.SRT.AddResource(desc);

		for (const ShaderResourceDesc desc : m_Desc.vDescriptorTables)
			m_ComputeShader.SRT.AddResource(desc);

		for (const SamplerDesc desc : m_Desc.vStaticSamplers)
			m_ComputeShader.SRT.AddSampler(desc);
	}

	void AGCComputePipeline::Release()
	{
	}

	/*AGCRaytracingPipeline::AGCRaytracingPipeline(const std::wstring& debugName, const RaytracingPipelineDesc& desc) :
		RaytracingPipeline(debugName, desc)
	{
	}

	AGCRaytracingPipeline::~AGCRaytracingPipeline()
	{
	}

	void AGCRaytracingPipeline::Bind()
	{
	}

	void AGCRaytracingPipeline::Release()
	{
	}

	void AGCRaytracingPipeline::ResizeDispatchAndOutputUAV(uint32_t newWidth, uint32_t newHeight, uint32_t newDepth)
	{
	}*/
}