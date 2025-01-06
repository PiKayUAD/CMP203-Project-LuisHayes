#pragma once
#include "InstanceData.h"
#include "Skateboard/Mathematics.h"
#include "Skateboard/Scene/Entity.h"
#include "AABB.h"

#ifndef SKTBD_SCENEBUILDER_NUM_PRIMITIVES
#define SKTBD_SCENEBUILDER_NUM_PRIMITIVES 6ui64
#endif // !SKTBD_SCENEBUILDER_NUM_PRIMITIVES

namespace Skateboard
{
	class Scene;

	enum class ShapeType {
		Cube,
		Sphere,
		CubeSphere,
		Cylinder,
		Cone,
		Terrain
	};

	class SceneBuilder
	{
		friend Scene;
	protected:
		struct VertexType
		{
			float3 position;
			float2 texCoord;
			float3 normal;
			float3 tangent;
			float3 bitangent;
		};
		const BufferLayout m_Layout = {
			{ "POSITION", Skateboard::ShaderDataType_::Float3 },
			{ "TEXCOORD", Skateboard::ShaderDataType_::Float2 },
			{ "NORMAL", Skateboard::ShaderDataType_::Float3 },
			{ "TANGENT", Skateboard::ShaderDataType_::Float3 },
			{ "BINORMAL", Skateboard::ShaderDataType_::Float3 }
		};

		struct MeshData
		{
			std::wstring Name;
			std::vector<VertexType> Vertices;
			std::vector<uint32_t> Indices;
		};

	protected:
		SceneBuilder() {}

	public:
		static SceneBuilder& Singleton()
		{
			static SceneBuilder builder;
			return builder;
		}
		~SceneBuilder() {}

		static const BufferLayout& GetVertexLayout() { return Singleton().m_Layout; }

		static std::string CreateDefaultMesh(ShapeType s, float size = 1 );
	protected:
		static std::string CreateDefaultSphereMesh();
		static std::string CreateDefaultCubeMesh();
		void BuildCone(MeshData& data, uint32_t resolution = 20u, bool capMesh = false);
		void BuildCube(MeshData& data, float size = 1);
		void BuildCubeSphere(MeshData& data, uint32_t meshResolution = 20u);
		void BuildCylinder(MeshData& data, uint32_t resolution = 20u, uint32_t stackCount = 1, bool capMesh = false);
		void BuildSphere(MeshData& data, float size = 1.0f, uint32_t resolution = 20u);
		void BuildTerrain(MeshData& data, uint32_t resolution = 20u);

	private:
	};
}