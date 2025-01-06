#pragma once
#include "AGCF.h"
#include "Skateboard/Renderer/Pipeline.h"
#include "Platform/AGC/AGCGraphicsContext.h"
#include "Skateboard/SizedPtr.h"
#include "ShaderResourceTable.h"

namespace Skateboard
{
	struct AGSBinary
	{
		MemoryHandle<uint8_t,sce::Agc::Alignment::kShaderHeader> Header;
		MemoryHandle<uint8_t, sce::Agc::Alignment::kShaderCode> Code;
	};

	struct AGCShader
	{
		AGSBinary Binary;
		sce::Agc::Shader* pShader;
		ShaderResourceTable SRT;
	};

	void LoadShader(const wchar_t* filename, AGCShader& out_shader);
	void GetSrtSignature(const struct _SceShaderBinaryHandle* sl, AGCShader& out_shader);

	class AGCRasterizationPipeline final : public RasterizationPipeline
	{
	public:
		AGCRasterizationPipeline(const std::wstring& debugName, const RasterizationPipelineDesc& desc);
		virtual ~AGCRasterizationPipeline() final override;

		virtual void Bind() final override;
		virtual void Unbind() final override;

		void Release() final override;

		void SetVertexBuffer(class AGCVertexBuffer* pVB, uint32_t vertexOfset = 0u);
		sce::Agc::DrawModifier GetDrawModifier() const { return m_PrimitiveShader.pShader->m_specials->m_drawModifier; }

	private:
		void LoadAllShaders();
		void CreateShaderResourceTable();
		void ConfigurePipelineSetup();
	private:
		AGCShader								m_HullShader,				// These are the shader objects containing the executable GPU code
												m_PrimitiveShader,
												m_PixelShader;
		sce::Agc::CxShaderLinkage				m_CxLinkage;				// 
		sce::Agc::UcPrimitiveState				m_UcLinkage;				// 
		sce::Agc::Core::IndirectStageBinder		m_HullBinder,				// The binders are used to bind resources to the different shader stages
												m_PrimitiveBinder,
												m_PixelBinder;

		sce::Agc::CxPrimitiveSetup				m_PrimitiveSetup;
		

		uint32_t m_VertexBufferSlot;
	};

	class AGCComputePipeline final : public ComputePipeline
	{
	public:
		AGCComputePipeline(const std::wstring& debugName, const ComputePipelineDesc& desc);
		virtual ~AGCComputePipeline() final override;

		virtual void Bind() final override;

		sce::Agc::DispatchModifier GetDispatchModifier() const { return m_ComputeShader.pShader->m_specials->m_dispatchModifier; }

	private:
		void CreateShaderResourceTable();
		void Release() final override;

	private:
		AGCShader								m_ComputeShader;
		sce::Agc::Core::IndirectStageBinder		m_Binder;
	};

	/*class AGCRaytracingPipeline final : public RaytracingPipeline
	{
	public:
		AGCRaytracingPipeline(const std::wstring& debugName, const RaytracingPipelineDesc& desc);
		virtual ~AGCRaytracingPipeline() final override;

		virtual void Bind() final override;
		void Release() final override;

		virtual void ResizeDispatchAndOutputUAV(uint32_t newWidth, uint32_t newHeight, uint32_t newDepth = 1u) final override;

	private:

	};*/
}