#pragma once

#include "../Scene/PipelineMap.h"


namespace Skateboard {

	//TODO: MOVE THIS INTO SOME SORT OF CONFIG FOR SKATEBOARD WIDE USE
	
	class Camera;
	class SpriteComponent;

	struct PassBuffer2D
	{
		float4x4 ViewMatrix;
		float4x4 ProjectionMatrix;
	};

	struct WorldBuffer2D
	{
		float4x4 Transform;
	};

	struct SpriteColour 
	{
		float4 colour;
	};

	struct SpriteSize
	{
		float2 size;
		float2 UvScale;
	};

	struct SpriteUvOffset
	{
		float2 UvOffset;
	};

	class Renderer2D
	{
	protected:
		const BufferLayout m_2DLayout = {
			{ "POSITION", Skateboard::ShaderDataType_::Float3 },
			{ "TEXCOORD", Skateboard::ShaderDataType_::Float2 }
		};
	public:

		~Renderer2D() = default;

		void Init();
		static void Create();
		static Renderer2D& Singleton();

		static void UpdateCamera(const Camera& cam);

		static void StartDraw();
		static void EndDraw();

		static void DrawQuad(TextureView* tex, GPUResource* worldbuffer, GPUResource* ScaleBuffer, GPUResource* OffsetBuffer, GPUResource* ColourBuffer);


		static void DrawSprite(SpriteComponent& sprite);
		
		static FrameBuffer* GetSceneAlbedo() { return Singleton().m_SceneBuffer.get(); }
		static TextureView* GetDefaultWhiteTexture();


	private:
		Renderer2D() {} ;

		std::shared_ptr<FrameBuffer> m_SceneBuffer;	
		
		uint32_t m_PassBufferId;
		uint32_t m_WorldBufferId;	//shared worldbuffer
		uint32_t m_ColorBufferId;	//shared colorbuffer
		uint32_t m_ScaleBufferId;	//shared Scalebuffer //uv and sprite size
		uint32_t m_OffsetBufferId;	//shared UvOffsetBuffer

		std::unique_ptr<Skateboard::Pipeline> p_Pipeline;
		std::unique_ptr<Skateboard::VertexBuffer> p_QuadVertices;
		std::unique_ptr<Skateboard::IndexBuffer> p_QuadIndices;
		std::unique_ptr<DescriptorTable> m_TextureDescriptorTable;


		uint32_t generalPassCBV;
	};
}