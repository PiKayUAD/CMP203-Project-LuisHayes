#pragma once

//#include "Skateboard/Renderer/GraphicsContext.h"
#include "Skateboard/Renderer/ResourceFactory.h"
#include "Skateboard/Renderer/RenderCommand.h"
#include "Skateboard/Renderer/Renderer.h"

using namespace Skateboard;

namespace CMP203
{
	//buffer layouts

	struct Vertex
	{
		float3 Position;
		float3 Colour;
		float2 UV;
		float3 Normal;

		//default colour is white
		Vertex() : Position(0, 0, 0), Colour(0, 0, 0), UV(0, 0), Normal(0,0,0) { }
		Vertex(float3 pos) : Vertex() { Position = pos; }
		Vertex(float3 pos, float3 col) : Vertex(pos) { Colour = col; }
		Vertex(float3 pos, float3 col, float2 uv) : Vertex(pos, col) { UV = uv; }
		Vertex(float3 pos, float3 col, float2 uv, float3 normal) : Vertex(pos, col, uv) {  Normal = normal; }

		static BufferLayout VertexLayout()
		{
			return {
				{ "POSITION", ShaderDataType_::Float3 },
				{ "COLOR",	  ShaderDataType_::Float3 },
				{ "TEXCOORD", ShaderDataType_::Float2 },
				{ "NORMAL",   ShaderDataType_::Float3 },
			};
		}
	};

	enum LightType : uint32_t
	{
		LightDirectional = 0,
		LightPoint = 1,
		LightSpot = 2,
	};

	struct InstanceData
	{
		matrix World;
		float4 ColourScale;

		int TextureIndex;

		float4 SpecularColor;
		float SpecularPower;
		float SpecularWeight;

		InstanceData() { World = glm::identity<glm::mat4x4>(); ColourScale = float4(1, 1, 1, 1); SpecularColor = float4(1, 1, 1, 1);  TextureIndex = 1; SpecularPower = 1.f; SpecularWeight = 1; }
	};

	struct Light
	{
	public:
		float4 DiffuseColour;
		float4 Attenuation;

		float3 LightPosition;
		float InnerCone;

		float3 LightDirection;
		float OuterCone;

	private:
		float2 Padding;
	public:
		float FalloffPower;
		uint32_t LightType;

		static float4 AttenuationDefaults() { return { 0.05f, 0.01f, 0.001f, 100.f }; }
	};

	struct  FrameData
	{
		glm::mat4x4 ViewMatrix;
		glm::mat4x4 ProjectionMatrix;
	};

	struct Frame
	{
		FrameData Matrices;

		matrix CameraMatrix;
		uint32_t LightCount;
		float3 AmbientLight;
	};

	static SamplerDesc AnisotropicSampler(uint8_t anisotropy, SamplerMode_ U, SamplerMode_ V)
	{
		return
		{
			.Filter = SamplerFilter_::SamplerFilter_Anisotropic,
			.ModeU = U,
			.ModeV = V,
			.ModeW = V,
			.MipMapLevelOffset = 0.f,
			.MipMapMinSampleLevel = 0.f,
			.MipMapMaxSampleLevel = 10.f,
			.MaxAnisotropy = anisotropy,	// Valid range 1 - 16 -> uint32_t cause padding anyways
			.ComparisonFunction = SamplerComparisonFunction_Less_Equal,
			.BorderColour = SamplerBorderColour_TransparentBlack,
			.Flags = 0,
		};
	}

	static SamplerDesc LinearSampler(SamplerMode_ U, SamplerMode_ V)
	{
		auto sampler = SamplerDesc::InitAsDefaultTextureSampler();
		sampler.ModeU = U;
		sampler.ModeV = V;
		return sampler;
	}

	static SamplerDesc PointSampler(SamplerMode_ U, SamplerMode_ V)
	{
		auto sampler = SamplerDesc::InitAsDefaultTextureSampler();
		sampler.Filter = SamplerFilter_::SamplerFilter_Comaprison_Min_Mag_Mip_Point;
		sampler.ModeU = U;
		sampler.ModeV = V;
		return sampler;
	}

	enum PipelineFlags : uint8_t
	{
		DEPTH_TEST = 1 << 0,
		ALPHA_BLEND_TRANSPARENCY = 1 << 1,
		WIREFRAME = 1 << 2,
		LIT = 1 << 3 
	};
	ENUM_FLAG_OPERATORS(PipelineFlags);

	constexpr static PipelineFlags DEFAULT_FLAGS = DEPTH_TEST | LIT;
	constexpr static uint8_t PIPELINE_PERMUTATIONS = (DEPTH_TEST | ALPHA_BLEND_TRANSPARENCY | WIREFRAME | LIT) + 1;

	class Renderer203 : public Skateboard::IRenderer
	{
		Skateboard::ShaderInputLayout RootSig;

		//pipeline permutations
		std::array<Skateboard::Pipeline, PIPELINE_PERMUTATIONS> Pipelines;

		//normal rendering
		Skateboard::Pipeline NormalVisualizer;

		size_t DynamicBufferSize = 64 * 1024;
		size_t InstanceDataBufferSize = 64 * 1024;

		Skateboard::MultiResource<Skateboard::Buffer> TriangleDataBuffer;
		Skateboard::MultiResource<Skateboard::Buffer> InstanceDataBuffer;
		Skateboard::MultiResource<Skateboard::Buffer> LightDataBuffer;

		//inline view desc
		BufferViewDesc sbvdesc;
		BufferViewDesc lightsbvdesc;
		BufferViewDesc cbvdesc;
	
		//resterizer
		SamplerDesc StaticSamplerDesc = SamplerDesc::InitAsDefaultTextureSampler();

		//counts offsets for dynamic data uploaded every frame
		size_t m_Offset = ROUND_UP(sizeof(FrameData), GraphicsConstants::CONSTANT_BUFFER_ALIGNMENT);;

		//forked on each call with a unique data pointer/ otherwise default instance data is used
		uint32_t m_InstanceDataForks = 0;

		Frame m_Frame{ .AmbientLight = { 1, 1, 1} };
		FrameData m_CameraData{};

		std::vector<Light> m_Lights;

		InstanceData m_DefaultInstanceData = InstanceData();

		uint32_t m_DefaultTextureIDX = 0;

		PipelineFlags m_PipelineSelection = DEFAULT_FLAGS;
		bool m_Pipeline_dirty;

		bool m_VisualiseNormals = false;
		
	public:
		bool WireFrame = false;

		SKTBD_PRIMITIVE_TOPOLOGY Topology = SKTBD_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		virtual void Init() override;

		virtual void Init(SamplerDesc Sampler)
		{
			StaticSamplerDesc = Sampler;
			Init();
		}

		virtual void Init(size_t TriangleBuffersizeOverride, size_t InstanceBufferSizeOverride)
		{
			DynamicBufferSize = ROUND_UP(TriangleBuffersizeOverride, GraphicsConstants::DEFAULT_RESOURCE_ALIGNMENT);
			InstanceDataBufferSize = ROUND_UP(InstanceBufferSizeOverride, GraphicsConstants::DEFAULT_RESOURCE_ALIGNMENT);
			Init();
		}

		virtual void Init(size_t TriangleBufferSizeOverride, size_t InstanceBufferSizeOverride, SamplerDesc Sampler)
		{
			DynamicBufferSize = ROUND_UP(TriangleBufferSizeOverride, GraphicsConstants::DEFAULT_RESOURCE_ALIGNMENT);
			InstanceDataBufferSize = ROUND_UP(InstanceBufferSizeOverride, GraphicsConstants::DEFAULT_RESOURCE_ALIGNMENT); 

			StaticSamplerDesc = Sampler;
			Init();
		}

		virtual void Begin();
		virtual void End();
		virtual void Shutdown();

		void SetLights(std::vector<Light> Lights) { m_Lights = Lights; };

		void AddLight(Light light) { m_Lights.push_back(light); }
		Light& GetLight(uint32_t idx) { return m_Lights[idx]; }

		//RendererConfiguration

		PipelineFlags GetPipelineFlags() const { return m_PipelineSelection; };

		void SetPipelineFlags(PipelineFlags Flags = DEFAULT_FLAGS)
		{
			m_PipelineSelection |= Flags;
			m_Pipeline_dirty = true;
		}

		void UnsetPipelineFlags(PipelineFlags Flags = DEFAULT_FLAGS)
		{
			m_PipelineSelection &= ~Flags;
			m_Pipeline_dirty = true;
		}

		void ResetPipelineFlags(PipelineFlags Flags = DEFAULT_FLAGS)
		{
			m_PipelineSelection = Flags;
			m_Pipeline_dirty = true;
		}

		//Debug normals
		void SetDrawDebugNormals(bool DrawNormals) { m_VisualiseNormals = DrawNormals; }

		void SetAmbientLight(float3 light_colour) {
			m_Frame.AmbientLight = light_colour;
		}
		void SetFrameData(FrameData newframeData);
		//Copies and draws Triangle Data from the CPU to the GPU
		void DrawVertices(Vertex* vertxBuffer, size_t vertexcount, uint32_t* indexbuffer, size_t indexcount, InstanceData* data = nullptr);

		void DrawSphere(float radius, int n_stacks, int n_slices, float3 v_colour, CMP203::InstanceData* transformData);

		// DOESNT Copy Data Over and just draws SpecifiedVertex and Index Buffers, Use with assets from asset manager
		void DrawVBIB(VertexBufferView* vb, IndexBufferView* ib, InstanceData* data = nullptr);

		//void DrawSphere(float radius = 3.f, int n_stacks = 16, int n_slices = 16, CMP203::InstanceData* transformData = nullptr);
	};

};
