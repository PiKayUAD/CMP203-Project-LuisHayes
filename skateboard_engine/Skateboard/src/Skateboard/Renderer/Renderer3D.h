#pragma once

#include "../Scene/PipelineMap.h"

namespace Skateboard {
	struct PassBuffer
	{
		float4x4 ViewMatrix;
		float4x4 ProjectionMatrix;
	};

	struct WorldBuffer
	{
		float4x4 Transform;
	};

	struct LightData
	{
		float3 Diffuse;
		float FalloffStart;
		float3 Direction;
		float FalloffEnd;
		float3 Position;
		float SpotPower;
		float3 Radiance;
		float pad;
		float4x4 ViewProj;
	};

	struct LightsData
	{
		LightData light;
		float3 CameraPosition;
		float Pad1;
	};

	class Renderer3D 
	{
	public:

		~Renderer3D() = default;

		void Init();
		static void Create();
		static Renderer3D& Singleton();
		//int32_t GetAlbedoPipelineId() { return m_PipelineMap.GetPipelineId("Albedo"); };
		static void SetRenderTexture(TextureView* tex);

		static void UpdateLightData(const Light& , const Camera& cam);
		static void UpdateCamera(const Camera& cam);
		static void UpdateMaterialData(uint32_t MaterialBuffer);
		static void StartDraw();
		static void EndDraw();

		// Draws a single mesh
		static void DrawMesh(const Primitive* mesh, const TransformComponent& transform);
		// Draws the model as is with the provided transform
		static void DrawModel(Mesh* model, const TransformComponent& transform);

	//	static FrameBuffer* GetSceneAlbedo() { return Singleton().m_SceneBuffer.get(); }
//		static auto& GetSceneFrameSharedPtr() { return Singleton().m_SceneBuffer; }


	private:
		Renderer3D();

		PipelineMap m_PipelineMap;

		std::unique_ptr<DescriptorTable> m_DescriptorTable;
		std::unique_ptr<DescriptorTable> m_TextureDescriptorTable;
		std::unique_ptr<DescriptorTable> m_MaterialDescriptorTable;
//		std::shared_ptr<FrameResource<Texture, TextureMemHandle>> m_SceneBuffer;

		uint32_t m_PassBufferId;
		std::unique_ptr<Skateboard::RasterizationPipeline> p_Pipeline;


		uint32_t generalPassCBV, shadowPassCBV, lightCBV, materialSBV;
		MaterialData defaultMaterialData;
	};
}