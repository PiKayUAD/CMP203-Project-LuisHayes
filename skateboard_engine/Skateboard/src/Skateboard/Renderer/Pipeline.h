#pragma once
#include "Skateboard/Mathematics.h"
#include "Buffer.h"
#include "InternalFormats.h"
#include "Skateboard/Memory/DescriptorTable.h"
#include "Skateboard/Renderer/Model/Model.h"

#define PIPELINE_SETTINGS_DEFAULT_DEPTH_BIAS 0

namespace Skateboard
{
	enum PipelineType_
	{
		PipelineType_Graphics,
		PipelineType_Compute,
	//	PipelineType_Mesh,
	//	PipelineType_Ray
	};

	enum FillMode_ : uint8_t
	{
		FillMode_Fill		= 0b00000000,
		FillMode_Wireframe	= 0b00000001,
		FillMode_Point		= 0b00000010
	};

	enum WindingOrder_ : uint8_t
	{
		WindingOrder_CCW	= 0b00000100,
		WindingOrder_CW		= 0b00001000,
	};

	enum CullMode_ : uint8_t
	{
		CullMode_Back		= 0b00010000,
		CullMode_Front		= 0b00100000,
	};

	struct Viewport
	{
		float TopLeftX;
		float TopLeftY;
		float Width;
		float Height;
		float MinZ;
		float MaxZ;
	};

	struct Rect
	{
		uint64_t left;
		uint64_t top;
		uint64_t right;
		uint64_t bottom;
	};

	struct ShaderModuleDesc
	{
		const wchar_t* FileName;
		const wchar_t* EntryPoint;
	};

	struct RaytracingHitGroup
	{
		const wchar_t* HitGroupName;
		const wchar_t* AnyHitShaderEntryPoint;
		const wchar_t* ClosestHitShaderEntryPoint;
		const wchar_t* IntersectionShaderEntryPoint;
		RaytracingHitGroupType_ Type;
	};

	struct RaytracingShaderLibrary
	{
		const wchar_t* FileName;
		const wchar_t* RayGenShaderEntryPoint;
		std::vector<RaytracingHitGroup> HitGroups;
		std::vector<const wchar_t*> MissShaderEntryPoints;
		std::vector<const wchar_t*> CallableShaders;
	};

	struct DescriptorRange
	{
		ShaderElementType_ DescriptorType;
		uint32_t NumOfDescriptors;
		uint32_t BaseRegister;
		uint32_t RegisterSpace;
		uint32_t OffsetInDescriptorsFromTableStart;
	};

	struct DescriptorTableLayout
	{
		uint32_t NumberOfRanges;
		DescriptorRange* Ranges;

		DescriptorRange CreateDescriptorInfo(ShaderElementType_ Type, uint32_t BaseRegister, uint32_t NumOfDesc = 1, uint32_t RegisterSpace = 0, uint32_t OffsetInDescriptorsFromTableStart = 0xffffffff) { return { Type,NumOfDesc,BaseRegister,RegisterSpace, OffsetInDescriptorsFromTableStart }; };
	};

	struct ShaderResourceDesc
	{
		ShaderElementType_ ShaderElementType;
		ShaderVisibility_ ShaderVisibility;
		union
		{
			struct {
				uint32_t ShaderRegister;
				uint32_t RegisterSpace;
			} Descriptor;
			struct {
				uint32_t ShaderRegister;
				uint32_t RegisterSpace;
				uint32_t Num32BitValues;
			} Constant ;
			DescriptorTableLayout DescriptorTable;
		};
	};

	enum SamplerFlags_
	{
		SamplerFlags_NONE = 0,
		SamplerFlags_UINT_BORDER_COLOR = 0x1,
		SamplerFlags_NON_NORMALIZED_COORDINATES = 0x2
	};

	struct SamplerDesc
	{
		SamplerFilter_ Filter;
		SamplerMode_ ModeU;
		SamplerMode_ ModeV;
		SamplerMode_ ModeW;
		float MipMapLevelOffset;
		float MipMapMinSampleLevel;
		float MipMapMaxSampleLevel;
		uint32_t MaxAnisotropy;	// Valid range 1 - 16 -> uint32_t cause padding anyways
		SamplerComparisonFunction_ ComparisonFunction;
		SamplerBorderColour_ BorderColour;
		uint32_t Flags;

		static SamplerDesc InitAsDefaultTextureSampler();
		static SamplerDesc InitAsDefaultShadowSampler();
	};

	struct SamplerSlotDesc
	{
		SamplerDesc SamplerDesc;
		uint32_t ShaderRegister;
		uint32_t ShaderRegisterSpace;
		ShaderVisibility_ ShaderVisibility;
	};

	//max root signature size on DX12 is 64 Dwords max root signature (BUT ON SOME HARDWARE ITS 16 ACTUALLY (PROBABLY SMILES IN AMD)) on PS5 is 16 Dwords
	struct ShaderInputsLayoutDesc
	{
		//only these for now 
		void AddRootConstant(uint32_t num32bitvalues, uint32_t shaderRegister, uint32_t shaderRegisterSpace = 0u, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
		void AddConstantBufferView(uint32_t shaderRegister, uint32_t shaderRegisterSpace = 0u, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
		void AddShaderResourceView(uint32_t shaderRegister, uint32_t shaderRegisterSpace = 0u, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
		void AddUnorderedAccessView(uint32_t shaderRegister, uint32_t shaderRegisterSpace = 0u, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
		void AddStaticSampler(const SamplerDesc& desc, uint32_t ShaderRegister, uint32_t ShaderRegisterSpace =0, ShaderVisibility_ ShaderVisibility = ShaderVisibility_PixelShader);

		bool DescriptorsDirctlyAddresssed;
		bool SamplersDirectlyAddressed;
		bool CanUseInputAssembler;

		std::vector<ShaderResourceDesc> vPipelineInputs;
		std::vector<SamplerSlotDesc> vStaticSamplers; 
		// We'll only use static samplers for now, TODO: support non-static samplers if necessary
		//std::vector<SamplerDesc> vSamplers; 

	private:
		void AddDescriptorTable(DescriptorTableLayout DescTable, ShaderVisibility_ shaderVisibility = ShaderVisibility_All);
	};

	class ShaderInputLayout : public SingleResource<GPUResource>
	{
	public:
		ShaderInputLayout() : SingleResource<GPUResource>(GpuResourceType_ShaderInputsLayout) {};
		ShaderInputLayout(std::wstring debugname) : SingleResource<GPUResource>(debugname, GpuResourceType_ShaderInputsLayout) {}

	//	const ShaderInputsLayoutDesc GetLayout() const { return m_Desc; };
	protected:
	//	ShaderInputsLayoutDesc m_Desc;
	};

	//Shamelesly stolen from D3D and extended with ps5 options
	//FIXME: add annotations to options
	enum CullMode
	{
		Cull_NONE = 1,
		Cull_FRONT = 2,
		Cull_BACK = 3,
	};

	enum ConservativeRasterMode
	{
		Underestimate,
		Overestimate
	};

	struct RasterizerConfig
	{
		bool Wireframe;
		CullMode Cull;

		bool FrontCC;
		float DepthBias;
		float DepthBiasClamp;
		float SlopeScaledDepthBias;
		bool DepthClipEnable;
		bool MultisampleEnable;
		uint32_t ForcedSampleCount;
		bool ConservativeRasterEnable;
		bool AntialiasedLineEnable;

		ConservativeRasterMode ConservativeRasterization;

		static RasterizerConfig Default()
		{
			return
			{
			.Wireframe = false,
			.Cull = Cull_BACK,
			.FrontCC = true,
			.DepthBias = 0.f,
			.DepthBiasClamp = 0.f,
			.SlopeScaledDepthBias = 0.f,
			.DepthClipEnable = false,
			.MultisampleEnable = false,
			.ForcedSampleCount = 0,
			.ConservativeRasterEnable = false,
			.AntialiasedLineEnable = false,
			//config.ConservativeRasterization; not set
			};
		}
	};

	enum SKTBD_Blend
	{
		SKTBD_Blend_ZERO = 1,
		SKTBD_Blend_ONE = 2,
		SKTBD_Blend_SRC_COLOR = 3,
		SKTBD_Blend_INV_SRC_COLOR = 4,
		SKTBD_Blend_SRC_ALPHA = 5,
		SKTBD_Blend_INV_SRC_ALPHA = 6,
		SKTBD_Blend_DEST_ALPHA = 7,
		SKTBD_Blend_INV_DEST_ALPHA = 8,
		SKTBD_Blend_DEST_COLOR = 9,
		SKTBD_Blend_INV_DEST_COLOR = 10,
		SKTBD_Blend_SRC_ALPHA_SAT = 11,
		SKTBD_Blend_BLEND_FACTOR = 14,
		SKTBD_Blend_INV_BLEND_FACTOR = 15,
		SKTBD_Blend_SRC1_COLOR = 16,
		SKTBD_Blend_INV_SRC1_COLOR = 17,
		SKTBD_Blend_SRC1_ALPHA = 18,
		SKTBD_Blend_INV_SRC1_ALPHA = 19,
		SKTBD_Blend_ALPHA_FACTOR = 20,
		SKTBD_Blend_INV_ALPHA_FACTOR = 21
	};

	enum SKTBD_LogicOp
	{
		SKTBD_LogicOp_CLEAR = 0,
		SKTBD_LogicOp_SET =				(SKTBD_LogicOp_CLEAR + 1),
		SKTBD_LogicOp_COPY =			(SKTBD_LogicOp_SET + 1),
		SKTBD_LogicOp_COPY_INVERTED =	(SKTBD_LogicOp_COPY + 1),
		SKTBD_LogicOp_NOOP =			(SKTBD_LogicOp_COPY_INVERTED + 1),
		SKTBD_LogicOp_INVERT =			(SKTBD_LogicOp_NOOP + 1),
		SKTBD_LogicOp_AND =				(SKTBD_LogicOp_INVERT + 1),
		SKTBD_LogicOp_NAND =			(SKTBD_LogicOp_AND + 1),
		SKTBD_LogicOp_OR =				(SKTBD_LogicOp_NAND + 1),
		SKTBD_LogicOp_NOR =				(SKTBD_LogicOp_OR + 1),
		SKTBD_LogicOp_XOR =				(SKTBD_LogicOp_NOR + 1),
		SKTBD_LogicOp_EQUIV =			(SKTBD_LogicOp_XOR + 1),
		SKTBD_LogicOp_AND_REVERSE =		(SKTBD_LogicOp_EQUIV + 1),
		SKTBD_LogicOp_AND_INVERTED =	(SKTBD_LogicOp_AND_REVERSE + 1),
		SKTBD_LogicOp_OR_REVERSE =		(SKTBD_LogicOp_AND_INVERTED + 1),
		SKTBD_LogicOp_OR_INVERTED =		(SKTBD_LogicOp_OR_REVERSE + 1)
	};

	enum SKTBD_BlendOp
	{
		SKTBD_BlendOp_ADD = 1,
		SKTBD_BlendOp_SUBTRACT = 2,
		SKTBD_BlendOp_REV_SUBTRACT = 3,
		SKTBD_BlendOp_MIN = 4,
		SKTBD_BlendOp_MAX = 5
	};

	struct RT_BlendConfig
	{
		bool BlendEnable;
		bool LogicOpEnable;
		SKTBD_Blend SrcBlend;
		SKTBD_Blend DestBlend;
		SKTBD_BlendOp BlendOp;
		SKTBD_Blend SrcBlendAlpha;
		SKTBD_Blend DestBlendAlpha;
		SKTBD_BlendOp BlendOpAlpha;
		SKTBD_LogicOp LogicOp;
		UINT8 RenderTargetWriteMask;
	};

	struct BlendConfig
	{
		bool AlphaToCoverage;
		bool IndependentBlendEnable;
		std::array<RT_BlendConfig, 8> RTBlendConfigs;
	};

	enum SKTBD_CompareOp
	{
		SKTBD_CompareOp_NONE = 0,
		SKTBD_CompareOp_NEVER = 1,
		SKTBD_CompareOp_LESS = 2,
		SKTBD_CompareOp_EQUAL = 3,
		SKTBD_CompareOp_LESS_EQUAL = 4,
		SKTBD_CompareOp_GREATER = 5,
		SKTBD_CompareOp_NOT_EQUAL = 6,
		SKTBD_CompareOp_GREATER_EQUAL = 7,
		SKTBD_CompareOp_ALWAYS = 8
	};

	enum SKTBD_StencilOp
	{
		SKTBD_StencilOp_KEEP = 1,
		SKTBD_StencilOp_ZERO = 2,
		SKTBD_StencilOp_REPLACE = 3,
		SKTBD_StencilOp_INCR_SAT = 4,
		SKTBD_StencilOp_DECR_SAT = 5,
		SKTBD_StencilOp_INVERT = 6,
		SKTBD_StencilOp_INCR = 7,
		SKTBD_StencilOp_DECR = 8
	};

	struct DepthStencilConfig
	{
		bool DepthEnable;
		bool DepthWriteAll;
		SKTBD_CompareOp DepthFunc;

		BOOL StencilEnable;
		UINT8 StencilReadMask;
		UINT8 StencilWriteMask;

		struct SKTBD_DepthStencilOpDesc
		{
			SKTBD_StencilOp StencilFailOp, StencilDepthFailOp, StencilPassOp;
			SKTBD_CompareOp StencilFunc;
		};

		SKTBD_DepthStencilOpDesc FrontFace, BackFace;
	};

	enum SKTBD_IBStripCutValue {
		SKTBD_IBStripCutValue_none = 0,
		SKTBD_IBStripCutValue_0xFFFF = 1,
		SKTBD_IBStripCutValue_0xFFFFFFFF = 2
	};

	struct RasterizationPipelineDesc
	{
		//Input Assembly Stage Configuration;
		BufferLayout InputVertexLayout;				// IA
		PrimitiveTopologyType_ InputPrimitiveType;	// IA
		SKTBD_IBStripCutValue TriangleStripCutValue;

		//Rasterization Stage Configuration
		RasterizerConfig Rasterizer;				// RASTERIZER
		
		//Output Merger Stage 
		DepthStencilConfig DepthStencil;			// OM
		BlendConfig Blend;							// OM
		uint32_t RenderTargetCount;					// OM
		std::array<DataFormat_,8> RenderTargetDataFormats; // OM
		DataFormat_ DepthstencilTargetFormat; // OM
		
		uint32_t SampleMask;	// MSAA OM?	
		uint32_t SampleCount;	// MSAA OM?
		uint32_t SampleQuality;	// MSAA OM?

		ShaderModuleDesc VertexShader;
		ShaderModuleDesc HullShader;
		ShaderModuleDesc DomainShader;
		ShaderModuleDesc GeometryShader;
		ShaderModuleDesc PixelShader;

		//uint8_t PipelineFlags;

		//void SetInputPrimitiveType(PrimitiveTopologyType_ type) { InputPrimitiveType = type; }
		//void SetInputLayout(const BufferLayout& layout) { InputVertexLayout = layout; }
		//void SetWireFrame(bool wf) { Rasterizer.Wireframe = wf; }
		//void SetDepthBias(int bias) { Rasterizer.DepthBias = bias; }

		//void SetFlags(uint8_t flags) { PipelineFlags |= flags; }

		void SetVertexShader(const wchar_t* filename, const wchar_t* entrypoint = L"main") { VertexShader.FileName = filename, VertexShader.EntryPoint = entrypoint; }
		void SetHullShader(const wchar_t* filename, const wchar_t* entrypoint = L"main") { HullShader.FileName = filename, HullShader.EntryPoint = entrypoint; }
		void SetDomainShader(const wchar_t* filename, const wchar_t* entrypoint = L"main") { DomainShader.FileName = filename, DomainShader.EntryPoint = entrypoint; }
		void SetGeometryShader(const wchar_t* filename, const wchar_t* entrypoint = L"main") { GeometryShader.FileName = filename, GeometryShader.EntryPoint = entrypoint; }
		void SetPixelShader(const wchar_t* filename, const wchar_t* entrypoint = L"main") { PixelShader.FileName = filename, PixelShader.EntryPoint = entrypoint; }
	};

	struct ComputePipelineDesc
	{
		ShaderModuleDesc ComputeShader;
		void SetComputeShader(const wchar_t* filename, const wchar_t* entrypoint) { ComputeShader.FileName = filename, ComputeShader.EntryPoint = entrypoint; }
	};

	//subject to REDO
	struct MeshletPipelineDesc
	{
		MeshletPipelineDesc()
			:
			DepthBias(PIPELINE_SETTINGS_DEFAULT_DEPTH_BIAS)
			, AmplificationShaderDesc()
			, MeshShaderDesc()
			, PixelShaderDesc()
			, Layout()
			, ShaderResourceIndex(0)
			, InstanceCount(0)
			, Flags(0)
			, Type()
			, UseDefaultPipelinePresets(false)
			, IsWireFrame(false)
			, UseShaderRootSignatureDefinition(false)
			, UseModelAttribsAsShaderDispatchDesc(false)
		{

		}
		~MeshletPipelineDesc() {}

		void SetInputLayout(const BufferLayout& layout) { Layout = layout; }
		void SetDepthBias(int bias) { DepthBias = bias; }
		int32_t DepthBias;

		void SetMeshShader(const wchar_t* filename, const wchar_t* entrypoint) { MeshShaderDesc.FileName = filename, MeshShaderDesc.EntryPoint = entrypoint; }
		void SetPixelShader(const wchar_t* filename, const wchar_t* entrypoint) { PixelShaderDesc.FileName = filename, PixelShaderDesc.EntryPoint = entrypoint; }
		void SetAmplificationShader(const wchar_t* filename, const wchar_t* entrypoint) { AmplificationShaderDesc.FileName = filename, AmplificationShaderDesc.EntryPoint = entrypoint; }

		void SetType(MeshletPipelineType_ type) { Type = type; }

		void SetWireFrame(bool isWire);

		ShaderModuleDesc AmplificationShaderDesc;
		ShaderModuleDesc MeshShaderDesc;
		ShaderModuleDesc PixelShaderDesc;
		BufferLayout Layout;
		uint32_t ShaderResourceIndex;
		uint32_t InstanceCount;
		uint8_t Flags;

		MeshletPipelineType_ Type;

		bool UseDefaultPipelinePresets;
		bool IsWireFrame;
		bool UseShaderRootSignatureDefinition;
		bool UseModelAttribsAsShaderDispatchDesc;
	};



	struct RaytracingPipelineDesc
	{
		RaytracingShaderLibrary RaytracingShaders;
		uint32_t MaxPayloadSize;
		uint32_t MaxAttributeSize;
		uint32_t MaxTraceRecursionDepth;
		uint32_t MaxCallableShaderRecursionDepth;

		void SetRaytracingLibrary(const wchar_t* libraryFilename, const wchar_t* raygenEntryPoint);
		void AddHitGroup(const wchar_t* hitGroupName, const wchar_t* anyHitEntryPoint, const wchar_t* closestHitEntryPoint, const wchar_t* intersectionEntryPoint, RaytracingHitGroupType_ type);
		void AddMissShader(const wchar_t* missEntryPoint);
		void AddCallableShader(const wchar_t* shaderEntryPoint);
		void SetConfig(uint32_t maxPayloadSize, uint32_t maxAttributeSize, uint32_t maxRecursionDepth, uint32_t maxCallableShaderRecursionDepth = 1u);
	};

	struct PipelineDesc
	{
		PipelineType_ Type;
		ShaderInputLayout GlobalLayoutSignature;

		std::variant<RasterizationPipelineDesc*, ComputePipelineDesc*> TypeDesc;

	//	union
	//	{
	//		RasterizationPipelineDesc Raster;
	//		ComputePipelineDesc Compute;
	////		MeshletPipelineDesc Mesh;
	////		RaytracingPipelineDesc Ray;
	//	}TypeDesc;
	};

	class Pipeline : public SingleResource<GPUResource>
	{
	public:
		Pipeline() : SingleResource<GPUResource>(GpuResourceType_PipelineState) {};
		Pipeline(std::wstring debugname, PipelineType_ type) : SingleResource<GPUResource>(debugname, GpuResourceType_PipelineState)
		{
			m_Type = type;
		};

		PipelineType_ GetType() const { return m_Type; };
	//	ShaderInputLayout const GetInputLayout() { return m_RootSignature; };
	protected:
		PipelineType_ m_Type;
	};

//	template <typename T> // TYPE OF PIPELINEDESC
//	class PipelineBase  : public Pipeline
//	{
//		static_assert(std::is_base_of<PipelineDesc, T>::value); //prevent pipelines being created without correct descriptions
//
//	protected:
//#ifndef SKTBD_SHIP
//		PipelineBase(const std::wstring& debugName,const T &desc) : m_DebugName(debugName), m_Desc(desc) {}
//		const std::wstring& GetDebugName() const { return m_DebugName; }
//#else
//		Pipeline(const std::wstring& debugName,const T &desc) : m_Des(desc) {}
//		const std::wstring& GetDebugName() const { return L""; }
//#endif
//	public:
//		//access to slots
//		std::vector<ShaderResourceDesc>& GetDescriptorTableSlots()	{ return m_Desc.Inputs; };
//
//		//access to descriptor
//		const T& GetDesc() { return m_Desc; };
//	protected:
//#ifndef SKTBD_SHIP
//		std::wstring m_DebugName;
//#endif // !SKTBD_SHIP
//
//		//Descriptor of the pipline
//		T m_Desc;
//	};
//
//	class RasterizationPipeline : public PipelineBase<RasterizationPipelineDesc>
//	{
//	protected:
//		RasterizationPipeline(const std::wstring& debugName, const RasterizationPipelineDesc& desc) : PipelineBase(debugName, desc) {}
//
//	public:
//	//	static RasterizationPipeline* Create(const std::wstring& debugName, const RasterizationPipelineDesc& desc);
//	};
//
//	
//	class ComputePipeline : public PipelineBase<ComputePipelineDesc>
//	{
//	protected:
//		ComputePipeline(const std::wstring& debugName, const ComputePipelineDesc& desc) : PipelineBase(debugName, desc) {}
//
//	public:
//	//	static ComputePipeline* Create(const std::wstring& debugName, const ComputePipelineDesc& desc);
//	};
//
//	class MeshletPipeline : public PipelineBase<MeshletPipelineDesc>
//	{
//	protected:
//		MeshletPipeline(const std::wstring& debugName, const MeshletPipelineDesc& desc) : PipelineBase(debugName, desc) {}
//	public:
//	//	static MeshletPipeline* Create(const std::wstring& debugName, const MeshletPipelineDesc& desc);
//
//		virtual ~MeshletPipeline() {}
//
//		virtual void SetInputLayout(const BufferLayout& layout) = 0;
//		virtual void SetDefaultInputLayout() = 0;
//		virtual void SetResource(BufferView* buffer, uint32_t shaderRegister, uint32_t registerSpace = 0) = 0;
//
//	//	virtual void BindMeshPipeline(MeshletModel* model = nullptr) = 0;
//	//	virtual void Bind() = 0;
//	//	virtual void Unbind() {};
//
//	protected:
//		virtual void LoadMeshShader(const wchar_t* filename, void** blob) = 0;
//	};


	//class RaytracingPipeline : public Pipeline<RaytracingPipelineDesc>
	//{
	//protected:
	//	RaytracingPipeline(const std::wstring& debugName, const RaytracingPipelineDesc& desc) : Pipeline(debugName, desc) {}

	//public:
	//	static RaytracingPipeline* Create(const std::wstring& debugName, const RaytracingPipelineDesc& desc);

	//	virtual void Bind() = 0;
	//	virtual void Unbind() {}

	//	virtual void ResizeDispatchAndOutputUAV(uint32_t newWidth, uint32_t newHeight, uint32_t newDepth = 1u) = 0;

	//	//const RaytracingPipelineDesc& GetDesc() const { return m_Desc; }
	//	Buffer* GetOutputUAV() const { return m_RaytracingOutput.get(); }

	//protected:
	////	RaytracingPipelineDesc m_Desc;
	//	std::unique_ptr<Skateboard::Buffer> m_RaytracingOutput;
	//};
}

