#include "sktbdpch.h"
#include "Pipeline.h"
#include "Renderer.h"

#if defined (SKTBD_PLATFORM_WINDOWS)
	#include "Platform/DirectX12/D3DPipeline.h"
#elif defined(SKTBD_PLATFORM_PLAYSTATION)
	#include "Platform/AGC/AGCPipeline.h"
	#include "Platform/AGC/MeshletPipeline/AGCMeshletPipeline.h"
#endif

#define SKTBD_LOG_COMPONENT "PIPELINE"
#include "Skateboard/Log.h"

namespace Skateboard
{
	SamplerDesc SamplerDesc::InitAsDefaultTextureSampler()
	{
		SamplerDesc desc = {};
		desc.Filter = SamplerFilter_Min_Mag_Mip_Linear;
		desc.ModeU = SamplerMode_Wrap;
		desc.ModeV = SamplerMode_Wrap;
		desc.ModeW = SamplerMode_Wrap;
		desc.MipMapLevelOffset = 0.f;
		desc.MipMapMinSampleLevel = 0.f;
		desc.MipMapMaxSampleLevel = FLT_MAX;
		desc.MaxAnisotropy = 16u;
		desc.ComparisonFunction = SamplerComparisonFunction_Less_Equal;
		desc.BorderColour = SamplerBorderColour_White;
		desc.Flags = SamplerFlags_NONE;
		return desc;
	}

	SamplerDesc SamplerDesc::InitAsDefaultShadowSampler()
	{
		SamplerDesc desc = {};
		desc.Filter = SamplerFilter_Min_Mag_Linear_Mip_Point;
		desc.ModeU = SamplerMode_Border;
		desc.ModeV = SamplerMode_Border;
		desc.ModeW = SamplerMode_Border;
		desc.MipMapLevelOffset = 0.f;
		desc.MipMapMinSampleLevel = 0.f;
		desc.MipMapMaxSampleLevel = FLT_MAX;
		desc.MaxAnisotropy = 16u;
		desc.ComparisonFunction = SamplerComparisonFunction_Less_Equal;
		desc.BorderColour = SamplerBorderColour_Black;
		return desc;
	}

	void  MeshletPipelineDesc::SetWireFrame(bool isWire)
	{
		IsWireFrame = isWire;
	}

	void ShaderInputsLayoutDesc::AddRootConstant(uint32_t num32bitvalues, uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility)
	{
		ShaderResourceDesc desc;
		desc.ShaderElementType = ShaderElementType_RootConstant;
		desc.Constant.Num32BitValues = num32bitvalues;
		desc.Constant.ShaderRegister = shaderRegister;
		desc.Constant.RegisterSpace = shaderRegisterSpace;
		desc.ShaderVisibility = shaderVisibility;
		vPipelineInputs.emplace_back(std::move(desc));
	}


	void ShaderInputsLayoutDesc::AddConstantBufferView(uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility)
	{
		ShaderResourceDesc desc;

		desc.ShaderElementType = ShaderElementType_ConstantBufferView;
		desc.Descriptor.ShaderRegister = shaderRegister;
		desc.Descriptor.RegisterSpace = shaderRegisterSpace;
		desc.ShaderVisibility = shaderVisibility;

		vPipelineInputs.emplace_back(desc);
	}

	void ShaderInputsLayoutDesc::AddShaderResourceView(uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility)
	{
		ShaderResourceDesc desc;
		desc.ShaderElementType = ShaderElementType_ReadResourceView;
		desc.Descriptor.ShaderRegister = shaderRegister;
		desc.Descriptor.RegisterSpace = shaderRegisterSpace;
		desc.ShaderVisibility = shaderVisibility;
		vPipelineInputs.emplace_back(desc);
	}

	void ShaderInputsLayoutDesc::AddUnorderedAccessView(uint32_t shaderRegister, uint32_t shaderRegisterSpace, ShaderVisibility_ shaderVisibility)
	{
		ShaderResourceDesc desc;
		desc.ShaderElementType = ShaderElementType_ReadWriteResourceView;
		desc.Descriptor.ShaderRegister = shaderRegister;
		desc.Descriptor.RegisterSpace = shaderRegisterSpace;
		desc.ShaderVisibility = shaderVisibility;
		vPipelineInputs.emplace_back(desc);
	}

	void ShaderInputsLayoutDesc::AddDescriptorTable(DescriptorTableLayout DescTable, ShaderVisibility_ shaderVisibility)
	{
		ShaderResourceDesc desc = {};
		desc.ShaderElementType = ShaderElementType_DescriptorTable;
		desc.ShaderVisibility = shaderVisibility;
		desc.DescriptorTable = DescTable;
		vPipelineInputs.emplace_back(std::move(desc));
	}

	void ShaderInputsLayoutDesc::AddStaticSampler(const SamplerDesc& desc, uint32_t ShaderRegister, uint32_t ShaderRegisterSpace, ShaderVisibility_ ShaderVisibility)
	{
		SamplerSlotDesc Slot = { desc, ShaderRegister, ShaderRegisterSpace, ShaderVisibility };

		vStaticSamplers.emplace_back(Slot);
	}

	void RaytracingPipelineDesc::SetRaytracingLibrary(const wchar_t* libraryFilename, const wchar_t* raygenEntryPoint)
	{
		RaytracingShaders.FileName = libraryFilename;
		RaytracingShaders.RayGenShaderEntryPoint = raygenEntryPoint;
	}
	void RaytracingPipelineDesc::AddHitGroup(const wchar_t* hitGroupName, const wchar_t* anyHitEntryPoint, const wchar_t* closestHitEntryPoint, const wchar_t* intersectionEntryPoint, RaytracingHitGroupType_ type)
	{
		RaytracingHitGroup group = {};
		group.HitGroupName = hitGroupName;
		group.AnyHitShaderEntryPoint = anyHitEntryPoint;
		group.ClosestHitShaderEntryPoint = closestHitEntryPoint;
		group.IntersectionShaderEntryPoint = intersectionEntryPoint;
		group.Type = type;
		RaytracingShaders.HitGroups.emplace_back(std::move(group));
	}
	void RaytracingPipelineDesc::AddMissShader(const wchar_t* missEntryPoint)
	{
		RaytracingShaders.MissShaderEntryPoints.push_back(missEntryPoint);
	}
	void RaytracingPipelineDesc::AddCallableShader(const wchar_t* shaderEntryPoint)
	{
		RaytracingShaders.CallableShaders.push_back(shaderEntryPoint);
	}
	void RaytracingPipelineDesc::SetConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize, uint32_t maxRecursionDepth, uint32_t maxCallableShaderRecursionDepth)
	{
		MaxPayloadSize = maxPayloadSize;
		MaxAttributeSize = maxAttributeSize;
		MaxTraceRecursionDepth = maxRecursionDepth;
		MaxCallableShaderRecursionDepth = maxCallableShaderRecursionDepth;
	}

	/*RasterizationPipeline* RasterizationPipeline::Create(const std::wstring& debugName, const RasterizationPipelineDesc& desc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI_::RendererAPI_None:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#if defined SKTBD_PLATFORM_WINDOWS
		case RendererAPI_::RendererAPI_DirectX12:
			return new D3DRasterizationPipeline(debugName, desc);
		case RendererAPI_::RendererAPI_Vulkan:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#elif defined SKTBD_PLATFORM_PLAYSTATION
		case RendererAPI_::RendererAPI_AGC:
			return new AGCRasterizationPipeline(debugName, desc);
#endif
		default:
			SKTBD_CORE_ASSERT(false, "Could not create a pipeline, the input API does not exist!");
			return nullptr;
		}
	}*/

	/*ComputePipeline* ComputePipeline::Create(const std::wstring& debugName, const ComputePipelineDesc& desc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI_::RendererAPI_None:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#if defined SKTBD_PLATFORM_WINDOWS
		case RendererAPI_::RendererAPI_DirectX12:
			return new D3DComputePipeline(debugName, desc);
		case RendererAPI_::RendererAPI_Vulkan:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#elif defined SKTBD_PLATFORM_PLAYSTATION
		case RendererAPI_::RendererAPI_AGC:
			return new AGCComputePipeline(debugName, desc);
#endif
		default:
			SKTBD_CORE_ASSERT(false, "Could not create a pipeline, the input API does not exist!");
			return nullptr;
		}
	}*/

	/*RaytracingPipeline* RaytracingPipeline::Create(const std::wstring& debugName, const RaytracingPipelineDesc& desc)
	{
		switch (Renderer::GetAPI())
		{
		case RendererAPI_::RendererAPI_None:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#if defined SKTBD_PLATFORM_WINDOWS
		case RendererAPI_::RendererAPI_DirectX12:
			return new D3DRaytracingPipeline(debugName, desc);
		case RendererAPI_::RendererAPI_Vulkan:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#elif defined SKTBD_PLATFORM_PLAYSTATION
		case RendererAPI_::RendererAPI_AGC:
			SKTBD_CORE_ASSERT(false, "API not yet supported.");
			return nullptr;
#endif
		default:
			SKTBD_CORE_ASSERT(false, "Could not create a pipeline, the input API does not exist!");
			return nullptr;
		}
	}*/

//	MeshletPipeline* MeshletPipeline::Create(const std::wstring& debugName, const MeshletPipelineDesc& desc)
//	{
//		switch (Renderer::GetAPI())
//		{
//		case RendererAPI_::RendererAPI_None:
//			SKTBD_CORE_ASSERT(false, "API not yet supported.");
//			return nullptr;
//#if defined SKTBD_PLATFORM_WINDOWS
//		case RendererAPI_::RendererAPI_DirectX12:
//			//return new D3DRaytracingPipeline(debugName, desc);
//			SKTBD_CORE_ASSERT(false, "I dont know what's going on, but you had it create a raytracing pipeline here??");
//			return nullptr;
//		case RendererAPI_::RendererAPI_Vulkan:
//			SKTBD_CORE_ASSERT(false, "API not yet supported.");
//			return nullptr;
//#elif defined SKTBD_PLATFORM_PLAYSTATION
//		case RendererAPI_::RendererAPI_AGC:
//			return new AGCMeshletPipeline(debugName, desc);
//#endif
//		default:
//			SKTBD_CORE_ASSERT(false, "Could not create a pipeline, the input API does not exist!");
//			return nullptr;
//		}
//	}


}