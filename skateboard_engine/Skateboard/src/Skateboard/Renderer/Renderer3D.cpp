#include "../Memory/MemoryManager.h"
#include "../Assets/AssetManager.h"
#include "Skateboard/Scene/SceneBuilder.h"
#include "Skateboard/Renderer/RenderCommand.h"
#include "Renderer3D.h"

namespace Skateboard 
{

	Renderer3D::Renderer3D()
	{


	}

	void Renderer3D::Create() {
		Singleton().Init();
	}

	void Renderer3D::Init()
	{
		m_DescriptorTable.reset(DescriptorTable::Create(DescriptorTableDesc::Init(ShaderDescriptorTableType_CBV_SRV_UAV)));
		m_TextureDescriptorTable.reset(DescriptorTable::Create(DescriptorTableDesc::Init(ShaderDescriptorTableType_CBV_SRV_UAV)));
		//m_MaterialDescriptorTable.reset(DescriptorTable::Create(DescriptorTableDesc::Init(ShaderDescriptorTableType_CBV_SRV_UAV)));

		//m_SceneBuffer.reset(FrameBuffer::Create(L"Main Frame Buffer", FrameBufferDesc::InitAsFullRenderTarget(1920, 1080)));
		m_PassBufferId = 0u;

		// TODO make the 3d renderer a static class with no instances -- even across scenes we don't need  more than one.
		//generalPassCBV = MemoryManager::CreateConstantBuffer<PassBuffer>(L"Pass Buffer");
		//lightCBV = MemoryManager::CreateConstantBuffer<LightsData>(L"Light Buffer");
		//materialSBV = MemoryManager::CreateStructuredBuffer<MaterialData>(L"Material Buffer",1);


		//m_TextureDescriptorTable->AddDescriptor(Descriptor::ShaderResourceView(AssetManager::GetDefaultTexture(), 0, 0));
		//m_TextureDescriptorTable->AddDescriptor(Descriptor::ShaderResourceView(MemoryManager::GetUploadBuffer(materialSBV), 1, 0));
		//m_TextureDescriptorTable->GenerateTable();

		//m_MaterialDescriptorTable->GenerateTable();

#pragma region Create and pass Camera info to shaders

		// Create a pass buffer
		//m_PassBufferId = MemoryManager::CreateConstantBuffer<PassBuffer>(L"Pass Constant Buffer");

#pragma endregion

		auto inLayout = SceneBuilder::GetVertexLayout();

		//auto worldBuf = MemoryManager::CreateConstantBuffer<WorldBuffer>(L"WorldBuffer-");
		//m_DescriptorTable->AddDescriptor(Descriptor::ConstantBufferView(MemoryManager::GetUploadBuffer(worldBuf), 1, 0));
		//m_DescriptorTable->GenerateTable();

		//Create a pipeline
		RasterizationPipelineDesc pipelineDesc = {};
		pipelineDesc.SetInputLayout(inLayout);
		pipelineDesc.SetType(RasterizationPipelineType_Default);
		pipelineDesc.SetWireFrame(false);
		pipelineDesc.SetDepthBias(PIPELINE_SETTINGS_DEFAULT_DEPTH_BIAS);
		pipelineDesc.SetVertexShader(L"default_vertexshader_vv", L"main");
		pipelineDesc.SetPixelShader(L"default_pixelshader_p", L"main");
		//pipelineDesc.AddConstantBufferView(MemoryManager::GetUploadBuffer(m_PassBufferId), 0, 0, ShaderVisibility_VertexShader);
		//pipelineDesc.AddConstantBufferView(MemoryManager::GetUploadBuffer(lightCBV), 3, 0, ShaderVisibility_PixelShader);
		//pipelineDesc.AddShaderResourceView(MemoryManager::GetUploadBuffer(materialSBV), 3, 0, ShaderVisibility_PixelShader);
		pipelineDesc.AddDescriptorTable(m_DescriptorTable.get(), ShaderVisibility_VertexShader);
		pipelineDesc.AddDescriptorTable(m_TextureDescriptorTable.get(), ShaderVisibility_PixelShader);
		//pipelineDesc.AddDescriptorTable(m_MaterialDescriptorTable.get(), ShaderVisibility_PixelShader);
		pipelineDesc.AddSampler(SamplerDesc::InitAsDefaultTextureSampler(0, 0, ShaderVisibility_PixelShader));
		p_Pipeline.reset(RasterizationPipeline::Create(L"Graphics Pipeline", pipelineDesc));

		m_PipelineMap.AddRasterizationPipeline("Albedo", p_Pipeline.get());


		// Set the default material
		defaultMaterialData = {
			float4(1,1,1,1),
			0,
			int3(0,0,0),
			float3(0.01, 0.01, 0.01),
			0.1f,
			float3(0.2f,0.2f,0.2f),
			0.4f
		};
		//MemoryManager::UploadData(materialSBV, 0, &defaultMaterialData);

	}

	Renderer3D& Renderer3D::Singleton()
	{
		// TODO: insert return statement here
		static std::unique_ptr<Renderer3D> singleton(new Renderer3D());
		return *singleton;
	}

	void Renderer3D::SetRenderTexture(TextureView* tex)
	{
		if (tex)
			Singleton().m_TextureDescriptorTable->UpdateDescriptor(Descriptor::ShaderResourceView(tex, 0, 0));
	}

	void Renderer3D::UpdateLightData(const Light& light, const Camera& cam)
	{
		LightsData lightData =
		{
			{
				light.GetDiffuse(),
				light.GetFalloffStart(),
				light.GetDirection(),
				light.GetFalloffEnd(),
				light.GetPosition(),
				light.GetSpotPower(),
				light.GetRadiance(),
				0.0f,
				cam.GetProjectionMatrix()
			},
			cam.GetPosition(),
			0.0f
		};
		//MemoryManager::UploadData(Singleton().lightCBV, 0, &lightData);
	}

	void Renderer3D::UpdateCamera(const Camera& cam)
	{
		PassBuffer passData = {};
		passData.ViewMatrix = cam.GetViewMatrix();
		passData.ProjectionMatrix = cam.GetProjectionMatrix();
		//MemoryManager::UploadData(Singleton().m_PassBufferId, 0, &passData);
	}

	void Renderer3D::UpdateMaterialData(uint32_t MaterialBuffer)
	{
	//	Singleton().m_TextureDescriptorTable->UpdateDescriptor(Descriptor::ShaderResourceView(MemoryManager::GetUploadBuffer(MaterialBuffer), 1, 0));
	}

	void Renderer3D::StartDraw()
	{
	//	RenderCommand::
	}

	void Renderer3D::EndDraw()
	{
//		Singleton().m_SceneBuffer->Unbind();
	}

	void Renderer3D::DrawMesh(const Primitive* mesh, const TransformComponent& transform)
	{
		if (mesh) {
			glm::mat4x4 matrix = transform.GetTransform();
			//auto world = MemoryManager::GetUploadBuffer(transform.BufferId);
			//world->CopyData(0, 0, &matrix);
			//Singleton().m_DescriptorTable->UpdateDescriptor(Descriptor::ConstantBufferView(world, 1, 0));

			RenderCommand::DrawIndexed(Singleton().p_Pipeline.get(), mesh->VertexResource, mesh->IndexResource);
		}
	}

	void Renderer3D::DrawModel(Mesh* model, const TransformComponent& transform)
	{
		if (model) {
			glm::mat4x4 matrix = transform.GetTransform() * model->Offset.AsMatrix();
			//auto world = MemoryManager::GetUploadBuffer(transform.BufferId);
			//world->CopyData(0, 0, &matrix);
			//Singleton().m_DescriptorTable->UpdateDescriptor(Descriptor::ConstantBufferView(world, 1, 0));
			
			
			if (transform.HasComponent<MaterialComponent>()) {
				auto& mat = transform.GetComponent<MaterialComponent>();
				UpdateMaterialData(mat.BufferId);
			}
			else {
				UpdateMaterialData(Singleton().materialSBV);
			}
			

			for (int32_t i = 0; i < model->GetMeshCount(); ++i)
			{
				if(model->GetTextureCount()>0)
					SetRenderTexture(model->GetTexture(model->GetMesh(i)->TextureIndex)); // Use this to render correct texture on a mesh of a model
			//	RenderCommand::DrawIndexed(Singleton().p_Pipeline.get(), model->GetVertexBuffer(i), model->GetIndexBuffer(i));
			}
		}
	}
	

}