#include "Renderer2D.h"
#include "Skateboard/Memory/MemoryManager.h"
#include "Skateboard/Renderer/RenderCommand.h"
#include "Skateboard/Camera/Camera.h"
#include "Skateboard/Assets/AssetManager.h"
#include "SpriteComponent.h"
#include "Skateboard/Renderer/GPUResource.h"
#include "sktbdpch.h"

namespace Skateboard {

	Renderer2D& Renderer2D::Singleton()
	{
		// TODO: insert return statement here
		static std::unique_ptr<Renderer2D> singleton(new Renderer2D());
		return *singleton;
	}


	void Renderer2D::Create()
	{
		Singleton().Init();
	}

	void Renderer2D::Init()
	{

		//m_SceneBuffer = Renderer3D::GetSceneFrameSharedPtr();
		m_PassBufferId = 0u;

		float quadVertices[20] = {
			-0.5f, -0.5f, 0.0f,		1.f, 1.f,
			 0.5f, -0.5f, 0.0f,		0.f, 1.f,
			 0.5f,  0.5f, 0.0f,		0.f, 0.f,
			-0.5f,  0.5f, 0.0f,		1.f, 0.f
		};
		//p_QuadVertices.reset(VertexBuffer::Create(L"Quad2DVertexB", quadVertices, 4, m_2DLayout));
		//
		//uint32_t quadIndices[6] = { 0,1,2,2,3,0 };

		//p_QuadIndices.reset(IndexBuffer::Create(L"Quad2DIndexB", quadIndices, 6));

		//m_PassBufferId = MemoryManager::CreateConstantBuffer<PassBuffer2D>(L"PassBuffer2D");

		//// Create descriptor table for World buffer so that we can draw more than one object
		//m_WorldBufferId = MemoryManager::CreateConstantBuffer<WorldBuffer2D>(L"Renderer2D_WorldBuffer2D");
		//m_ColorBufferId = MemoryManager::CreateConstantBuffer<float4>(L"Renderer2D_ColourBuffer");
		//m_ScaleBufferId = MemoryManager::CreateConstantBuffer<float4>(L"Renderer2D_ScaleBuffer");
		//m_OffsetBufferId = MemoryManager::CreateConstantBuffer<float2>(L"Renderer2D_OffsetBuffer");

		//m_ScaleBufferId = MemoryManager::CreateConstantBuffer<float2>(L"Renderer2D_ScaleBuffer");
		//m_OffsetBufferId = MemoryManager::CreateConstantBuffer<float2>(L"Renderer2D_OffsetBuffer");

		

		//Create a pipeline
		RasterizationPipelineDesc pipelineDesc = {};
		//pipelineDesc.SetInputLayout(m_2DLayout);
		pipelineDesc.SetType(RasterizationPipelineType_Default);
		pipelineDesc.SetWireFrame(false);
		pipelineDesc.SetDepthBias(PIPELINE_SETTINGS_DEFAULT_DEPTH_BIAS);

		pipelineDesc.SetVertexShader(L"DefaultVertexShader2D_vv", L"main");
		pipelineDesc.SetPixelShader(L"DefaultPixelShader2D_p", L"main");


		//cbvs

		/*pipelineDesc.AddConstantBufferView(MemoryManager::GetUploadBuffer(m_PassBufferId), 0, 0, ShaderVisibility_VertexShader);
		pipelineDesc.AddConstantBufferView(MemoryManager::GetUploadBuffer(m_WorldBufferId), 1, 0, ShaderVisibility_VertexShader);
		pipelineDesc.AddConstantBufferView(MemoryManager::GetUploadBuffer(m_ScaleBufferId), 2, 0, ShaderVisibility_VertexShader);
		pipelineDesc.AddConstantBufferView(MemoryManager::GetUploadBuffer(m_OffsetBufferId), 3, 0, ShaderVisibility_VertexShader);

		pipelineDesc.AddConstantBufferView(MemoryManager::GetUploadBuffer(m_ColorBufferId), 0, 0, ShaderVisibility_PixelShader);*/
		
		//srvs

		pipelineDesc.AddShaderResourceView(AssetManager::GetTexture(DefaultTexKey), 0, 0, ShaderVisibility_PixelShader);

		//default sampler


		pipelineDesc.AddSampler(SamplerDesc::InitAsDefaultTextureSampler(0, 0, ShaderVisibility_PixelShader));
		p_Pipeline.reset(RasterizationPipeline::Create(L"2D Pipeline", pipelineDesc));

		float4 tempcolor = { 1,1,1,1 };

		//MemoryManager::UploadData(m_ColorBufferId, 0, &tempcolor);

		float4x4 tempworld = glm::identity<float4x4>();

		//MemoryManager::UploadData(m_WorldBufferId, 0, &tempworld);

		//m_PipelineMap.AddRasterizationPipeline("Albedo", p_Pipeline.get());
	}


	void Renderer2D::UpdateCamera(const Camera& cam)
	{
		PassBuffer2D passData = {};
		passData.ViewMatrix = cam.GetViewMatrix();
		passData.ProjectionMatrix = cam.GetProjectionMatrix();
		//MemoryManager::UploadData(Singleton().m_PassBufferId, 0, &passData);
	}

	void Renderer2D::StartDraw()
	{
		//Singleton().m_SceneBuffer->Bind();
	}

	void Renderer2D::EndDraw()
	{
		//Singleton().m_SceneBuffer->Unbind();
	}

	void Renderer2D::DrawQuad(TextureView* tex, GPUResource* worldbuffer, GPUResource* ScaleBuffer, GPUResource* OffsetBuffer, GPUResource* ColourBuffer)
	{
		//swap descriptors
		Singleton().p_Pipeline.get()->GetReadResourceSlots()[0].pResource = tex;
		Singleton().p_Pipeline.get()->GetConstantBuffersSlots()[1].pResource = worldbuffer;
		Singleton().p_Pipeline.get()->GetConstantBuffersSlots()[2].pResource = ScaleBuffer;
		Singleton().p_Pipeline.get()->GetConstantBuffersSlots()[3].pResource = OffsetBuffer;
		Singleton().p_Pipeline.get()->GetConstantBuffersSlots()[4].pResource = ColourBuffer;
	    

		// Drawcall
		RenderCommand::DrawIndexed(Singleton().p_Pipeline.get(), Singleton().p_QuadVertices.get(), Singleton().p_QuadIndices.get());
	}

	void Renderer2D::DrawSprite(SpriteComponent& sprite)
	{
		// Create and update a uniform that changes the UVs to match the sprites.
		
		//Transform& trans = sprite.GetComponent<TransformComponent>().Trans;
		//auto& transform = sprite.GetComponent<TransformComponent>();
		//auto world_buffer = //MemoryManager::GetUploadBuffer(transform.BufferId);
		//auto colour_buffer = //MemoryManager::GetUploadBuffer(sprite.ColourBufferId);
		//auto scale_buffer = //MemoryManager::GetUploadBuffer(sprite.SizeBufferId);
		//auto offset_buffer = //MemoryManager::GetUploadBuffer(sprite.OffsetBufferId);

		//DrawQuad(sprite.GetRenderTexture(), world_buffer,scale_buffer,offset_buffer, colour_buffer);
	}

	//Texture* Renderer2D::GetDefaultWhiteTexture()
	//{
	//	return AssetManager::GetTexture(DefaultTexKey);
	//}

	
}