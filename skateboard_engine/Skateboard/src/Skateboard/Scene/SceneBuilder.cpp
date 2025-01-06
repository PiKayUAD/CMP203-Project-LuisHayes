#include <sktbdpch.h>
#include "SceneBuilder.h"
#include "Skateboard/Renderer/Renderer.h"
#include "Skateboard/Renderer/GraphicsContext.h"
#include "Skateboard/Scene/Components.h"
#include "Skateboard/Assets/AssetManager.h"
#include "Skateboard/Utilities/StringConverters.h"

namespace Skateboard
{
	static std::list<std::string> keyStr;
	static uint8_t modelNum = 0;
	std::string SceneBuilder::CreateDefaultCubeMesh()
	{
		Primitive ret;
		MeshData cubeData = {};
		Singleton().BuildCube(cubeData);
		//ret.VertexResource = VertexBuffer::Create(L"CubeVBuffer",cubeData.Vertices.data(),cubeData.Vertices.size(), GetVertexLayout());
		//ret.IndexResource = IndexBuffer::Create(L"CubeIBuffer", cubeData.Indices.data(), cubeData.Indices.size());
		//ret.VertexCount = cubeData.Vertices.size();
		//ret.IndexCount = cubeData.Indices.size();

		std::string key = "DefaultCube";
		std::unique_ptr<Mesh> m = std::make_unique<Mesh>();
	//	m->AddPrimitive(ret);
		//add default texture
		keyStr.push_back(key);

		//FIX ME : add to model uses std move on the key thus invalidating the reference passed into add function. keystr.back becomes invalid, intended usage of key str back is unclear.
		AssetManager::AddModelToMap(m, keyStr.back());
		return key;
		
	}
	std::string SceneBuilder::CreateDefaultMesh(ShapeType s, float size )
	{
		Primitive meshTemp;
		MeshData data = {};
		std::string st;
		switch (s)
		{
		case Skateboard::ShapeType::Cube:
			Singleton().BuildCube(data, size);
			break;
		case Skateboard::ShapeType::Sphere:
			Singleton().BuildSphere(data, size);
			break;
		case Skateboard::ShapeType::CubeSphere:
			Singleton().BuildCubeSphere(data);
			break;
		case Skateboard::ShapeType::Cylinder:
			Singleton().BuildCylinder(data);
			break;
		case Skateboard::ShapeType::Cone:
			Singleton().BuildCone(data);
			break;
		case Skateboard::ShapeType::Terrain:
			Singleton().BuildTerrain(data);
			break;
		}

		
		//meshTemp.VertexResource = VertexBuffer::Create(ToWString(st).append(L"VBuffer"), data.Vertices.data(), data.Vertices.size(), GetVertexLayout());
		//meshTemp.IndexResource = IndexBuffer::Create(ToWString(st).append(L"IBuffer"), data.Indices.data(), data.Indices.size());
		//meshTemp.VertexCount = data.Vertices.size();
		//meshTemp.IndexCount = data.Indices.size();

		std::string key = "Default"  + ToString(data.Name)  + std::to_string(modelNum);
		keyStr.push_back(key);
		std::unique_ptr<Mesh> m = std::make_unique<Mesh>();
//		m->AddTexture(AssetManager::GetDefaultTexture());
//		m->AddPrimitive(meshTemp);
		AssetManager::AddModelToMap(m, keyStr.back());
		modelNum++;
		//invalid return FIX ME
		return key;
	}

	std::string SceneBuilder::CreateDefaultSphereMesh()
	{
		Primitive meshTemp;
		MeshData data = {};
		Singleton().BuildSphere(data);
		//meshTemp.VertexResource = VertexBuffer::Create(L"SphereVBuffer", data.Vertices.data(), data.Vertices.size(), GetVertexLayout());
		//meshTemp.IndexResource = IndexBuffer::Create(L"SphereIBuffer", data.Indices.data(), data.Indices.size());
		//meshTemp.VertexCount = data.Vertices.size();
		//meshTemp.IndexCount = data.Indices.size();

		std::string keyStr = "DefaultSphere";
		std::unique_ptr<Mesh> m = std::make_unique<Mesh>();
//		m->AddPrimitive(meshTemp);
		AssetManager::AddModelToMap(m, keyStr);
		//invalid return FIX ME
		return keyStr;

	}


	void SceneBuilder::BuildCone(MeshData& data, uint32_t resolution, bool capMesh)
	{
		// Generate the vertices and indices for a cone following a triangle list primitive type
		if (resolution < 3u)
			resolution = 3u;

		// Create writable index and vertex buffers
		const uint32_t vertexCount = resolution + 2u + capMesh; // A base of resolution + 1 (to roll back) + 1 (top vertex) + 1 (middle vertex)
		const uint32_t indexCount = (capMesh ? 2u : 1u) * 3u * resolution;
		data.Vertices.resize(vertexCount, {});
		data.Indices.resize(indexCount, 0u);
		data.Name = L"Cone";

		// Create the vertices
		const float deltaAngle = 2.f * SKTBD_PI / resolution;
		const float deltaTexCoord = 1.f / (resolution + 1u);
		float currentTexCoord = 0.f;
		const glm::quat roationQuaternion = glm::angleAxis(deltaAngle, float3(0.f, 1.f, 0.f));
		float3 currentVertexPos = float3(0.f, 0.f, -1.f);
		float3 currentTangent = float3(1.f, 0.f, 0.f);
		for (uint32_t i = 0u; i < resolution + 1u; ++i)
		{
			// Round up the last vertex of the circle to the first vertex, to avoid float imprecision errors
			if (i == resolution)
			{
				data.Vertices[i] = data.Vertices[0];
				data.Vertices[i].texCoord = float2(0.f, 1.f);
			}
			else
			{
				data.Vertices[i].position = currentVertexPos;
				data.Vertices[i].normal = glm::normalize(currentVertexPos);
				data.Vertices[i].tangent = glm::normalize(currentTangent);
				data.Vertices[i].bitangent = glm::cross(glm::normalize(currentVertexPos), glm::normalize(currentTangent));
				data.Vertices[i].texCoord = float2(1.f - currentTexCoord, 1.f);
			}
			currentTexCoord += deltaTexCoord;
			currentVertexPos = std::move(glm::rotate(roationQuaternion, std::move(currentVertexPos)));
			currentTangent = std::move(glm::rotate(roationQuaternion, std::move(currentTangent)));
		}
		// The forlast vertex is the top of the cone
		data.Vertices[resolution + 1u] = { float3(0.f, 1.f, 0.f), float2(.5f, 0.f), float3(0.f, 1.f, 0.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f) };
		// The last vertex is the middle of the cone
		if (capMesh) data.Vertices[resolution + 2u] = { float3(0.f, 0.f, 0.f), float2(.5f, 0.f), float3(0.f, -1.f, 0.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, -1.f) };

		// Create the indices
		uint32_t currentIndex = 0u;
		for (uint32_t i = 0u; i < resolution; ++i, currentIndex += 3u)
		{
			data.Indices[currentIndex] = i;
			data.Indices[currentIndex + 1u] = i + 1u;
			data.Indices[currentIndex + 2u] = resolution + 1u;
		}
		for (uint32_t i = 0u; i < resolution && capMesh; ++i, currentIndex += 3u)
		{
			data.Indices[currentIndex] = resolution + 2u;
			data.Indices[currentIndex + 1u] = i + 1u;
			data.Indices[currentIndex + 2u] = i;
		}
	}

	void SceneBuilder::BuildCube(MeshData& data, float size)
	{
		// Generate the vertices and indices for a cube
		//const uint32_t vertexCount = 24u;
		//const uint32_t indexCount = 36u;
		size *= .5f;
		// Declare the vertex data and the size of the vertex buffer to be created
		data.Vertices.assign({
			// Top
			{ float3(-1.f, 1.f, -1.f) * size, float2(0.f, 1.f), float3(0.f, 1.f, 0.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f) },
			{ float3(1.f, 1.f, -1.f)*size, float2(1.f, 1.f), float3(0.f, 1.f, 0.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(1.f, 1.f, 1.f)	*size, float2(1.f, 0.f), float3(0.f, 1.f, 0.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(-1.f, 1.f, 1.f)*size, float2(0.f, 0.f), float3(0.f, 1.f, 0.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },

			// Bottom
			{ float3(1.f, -1.f, 1.f)	*size, float2(0.f, 1.f), float3(0.f, -1.f, 0.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(-1.f, -1.f, 1.f)	*size, float2(1.f, 1.f), float3(0.f, -1.f, 0.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(-1.f, -1.f, -1.f)	*size, float2(1.f, 0.f), float3(0.f, -1.f, 0.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(1.f, -1.f, -1.f)	*size, float2(0.f, 0.f), float3(0.f, -1.f, 0.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },

			// Front
			{ float3(-1.f, -1.f, -1.f)	*size, float2(0.f, 1.f), float3(0.f, 0.f, -1.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(1.f, -1.f, -1.f)	*size, float2(1.f, 1.f), float3(0.f, 0.f, -1.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(1.f, 1.f, -1.f)	*size, float2(1.f, 0.f), float3(0.f, 0.f, -1.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(-1.f, 1.f, -1.f)	*size, float2(0.f, 0.f), float3(0.f, 0.f, -1.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },

			// Back
			{ float3(1.f, -1.f, 1.f)	*size, float2(0.f, 1.f), float3(0.f, 0.f, 1.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(-1.f, -1.f, 1.f)	*size, float2(1.f, 1.f), float3(0.f, 0.f, 1.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(-1.f, 1.f, 1.f)	*size, float2(1.f, 0.f), float3(0.f, 0.f, 1.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },
			{ float3(1.f, 1.f, 1.f)		*size, float2(0.f, 0.f), float3(0.f, 0.f, 1.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f)  },

			// Left
			{ float3(-1.f, -1.f, 1.f)	*size, float2(0.f, 1.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, -1.f), float3(0.f, 0.f, 1.f)  },
			{ float3(-1.f, -1.f, -1.f)	*size, float2(1.f, 1.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, -1.f), float3(0.f, 0.f, 1.f)  },
			{ float3(-1.f, 1.f, -1.f)	*size, float2(1.f, 0.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, -1.f), float3(0.f, 0.f, 1.f)  },
			{ float3(-1.f, 1.f, 1.f)	*size, float2(0.f, 0.f), float3(-1.f, 0.f, 0.f), float3(0.f, 0.f, -1.f), float3(0.f, 0.f, 1.f)  },

			// Right
			{ float3(1.f, -1.f, -1.f)	*size, float2(0.f, 1.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f), float3(0.f, 0.f, 1.f)  },
			{ float3(1.f, -1.f, 1.f)	*size, float2(1.f, 1.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f), float3(0.f, 0.f, 1.f)  },
			{ float3(1.f, 1.f, 1.f)		*size, float2(1.f, 0.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f), float3(0.f, 0.f, 1.f)  },
			{ float3(1.f, 1.f, -1.f)	*size, float2(0.f, 0.f), float3(1.f, 0.f, 0.f), float3(0.f, 0.f, 1.f), float3(0.f, 0.f, 1.f)  }
			}
		);

		// Declare the indices
		data.Indices.assign({
			// Top face
			2, 1, 0,
			3, 2, 0,

			// Bottom face
			4, 5, 6,
			4, 6, 7,

			// Front face
			10, 9, 8,
			11, 10, 8,

			// Back face
			14, 13, 12,
			15, 14, 12,

			// Left face
			18, 17, 16,
			19, 18, 16,

			// Right face
			22, 21, 20,
			23, 22, 20
			}
		);

		data.Name = L"Cube";
	}

	void SceneBuilder::BuildCubeSphere(MeshData& data, uint32_t meshResolution)
	{
		// Generate the vertices and indices for a cube sphere
		if (meshResolution == 0u)
			meshResolution = 1u;

		// Create a writable vertex buffer for all the vertices
		const uint32_t vertexCount = 6u * (meshResolution + 1u) * (meshResolution + 1u); // a face is of resolution * resolution vertices
		const uint32_t indexCount = 6u * 6u * meshResolution * meshResolution;
		data.Vertices.resize(vertexCount);
		data.Indices.resize(indexCount);
		data.Name = L"CubeSphere";

		// Define the radius of the sphere
		const float radius = 1.f;	// 1.f since the Normalise() operation on vectors gives a length of 1 (don't change this)

		// Create the vertices and indices
		uint32_t currentVertex = 0u;
		float fRes = static_cast<float>(meshResolution);	//fRes -> float Resolution
		uint32_t currentIndex = 0u;
		uint32_t verticesPerFace = vertexCount / 6u;

		// Top, bottom, front, back, left, right
		for (uint8_t faceNum = 0u; faceNum < 6u; ++faceNum)
		{
			// Generate the vertices for a face
			// Each plane is made of dimension (m_MeshResolution + 1) * (m_MeshResolution + 1) vertices
			// x and y identify the coordinates of a vertex in this plane
			for (uint32_t y = 0u; y <= meshResolution; ++y)
			{
				for (uint32_t x = 0u; x <= meshResolution; ++x, ++currentVertex)
				{
					// Get the position of the vertex in the [-1, 1] plane
					// x and y divided by the resolution go from [0, 1], so double them [0, 2] and substract 1 to get [-1, 1]
					// Store the results as floats
					float fX = static_cast<float>(x) * 2.f / fRes - 1.f;
					float fY = static_cast<float>(y) * 2.f / fRes - 1.f;

					// This is how you can rll 6 faces into a loop, each face needs a different input for each argument
					// For instance, faceNum 0u is the top face
					//	- The X position should be fX
					//	- The Y position should be positive radius
					//	- The Z position should be fY
					// This result with the plane being on the XZ plane at a distance 'radius' from the origin
					// The bottom face would be similar, but the Y position should be below the origin (-radius)
					// Repeat the logic for front, back, left and right faces
					float arg1 = (faceNum < 4u) * fX + (faceNum == 4u) * radius + (faceNum == 5u) * -radius;
					float arg2 = (faceNum == 0u) * radius + (faceNum == 1u) * -radius + (faceNum > 1u) * fY;
					float arg3 = (faceNum < 2u) * fY + (faceNum == 2u) * radius + (faceNum == 3u) * -radius + (faceNum > 3u) * fX;

					// The VECTOR object is the target position of the vertex if this was a flat plane
					// The position of the vertex must be in the direction of the targeted position, but of length (radius) 1, thus normalised
					// The normal is therefore equals to the position as it is a unit vector from the center
					float3 target = glm::normalize(float3(arg1, arg2, arg3));
					data.Vertices[currentVertex].position = target;
					data.Vertices[currentVertex].normal = target;

					// Calculate the tangent vector
					// Since this is a unit sphere, the cartesian equation of the sphere is x^2 + y^2 + z^2 = 1
					// Thus, the parametric equation becomes r(t, p) = cos(t)*sin(p)*i + sin(t)*sin(p)*j + cos(p)*k, 0 <= t <= 2*PI && 0 <= p <= PI
					// The tangent is then stated with dr/dt. The bitagent would be dr/dp
					// dr/dt = -sin(t)*sin(p)*i + cos(t)*sin(p)*j + 0
					// t (theta) is obtained using atan2 (https://en.wikipedia.org/wiki/Atan2) which in essence calculates the angle between a point P(x,y) and the x-axis
					// p (phi) is obtained using trigonometry: cos(p) = adjacent / hypothenuse. The hypothenuse is the radius of the unit sphere, thus p = acos(adjacent)
					float theta = atan2f(data.Vertices[currentVertex].position.z, data.Vertices[currentVertex].position.x); // P(Z,X) makes an point on the horizontal plane on the sphere, since Y is up
					if (theta < 0) theta += 2.f * SKTBD_PI;
					float phi = acosf(data.Vertices[currentVertex].position.y);
					/*data.Vertices[currentVertex].tangent.x = -sinf(theta) * sinf(phi);	// In essence, these are the calculations. But it needs to be normalised to ensure it is a unit vector
					data.Vertices[currentVertex].tangent.y = 0.f;
					data.Vertices[currentVertex].tangent.z = cosf(theta) * sinf(phi);*/
					float3 tangent = glm::normalize(float3(-sinf(theta) * sinf(phi), 0.f, cosf(theta) * sinf(phi)));
					data.Vertices[currentVertex].tangent = tangent;
					data.Vertices[currentVertex].bitangent = glm::cross(target, tangent);

					// The texture coordinates should match the coordinates on the plane in the range [0, 1]
					bool inverseCoord = faceNum == 1u || faceNum == 2u || faceNum == 5u;
					arg1 = static_cast<float>(x) / fRes;
					arg2 = 1.f - static_cast<float>(y) / fRes;	// V goes from top (0) to bottom (1), but the generation from top to bottom
					data.Vertices[currentVertex].texCoord = float2(inverseCoord ? 1.f - arg1 : arg1, arg2);
				}
			}

			// Generate the indices for a face
			// Determine whether the face bust be rendered in a clockwise order
			// For instance, the top face is clock wise, but the bottom face should be anti-clockwise (otherwise it would render inside the sphere)
			// This simply depends on how the vertices have been generated
			bool clockWiseIndexing = faceNum == 0u || faceNum == 3u || faceNum == 4u;
			for (uint32_t y = 0u; y < meshResolution; ++y)
			{
				for (uint32_t x = 0u; x < meshResolution; ++x, currentIndex += 6u)
				{
					data.Indices[currentIndex + 0u] = (x + 0u) + (y + 0u) * (meshResolution + 1u) + faceNum * verticesPerFace;
					data.Indices[currentIndex + 1u] = (x + 1u) + (y + clockWiseIndexing * 1u) * (meshResolution + 1u) + faceNum * verticesPerFace;
					data.Indices[currentIndex + 2u] = (x + 1u) + (y + !clockWiseIndexing * 1u) * (meshResolution + 1u) + faceNum * verticesPerFace;
					data.Indices[currentIndex + 3u] = (x + 0u) + (y + 0u) * (meshResolution + 1u) + faceNum * verticesPerFace;
					data.Indices[currentIndex + 4u] = (x + !clockWiseIndexing * 1u) + (y + 1u) * (meshResolution + 1u) + faceNum * verticesPerFace;
					data.Indices[currentIndex + 5u] = (x + clockWiseIndexing * 1u) + (y + 1u) * (meshResolution + 1u) + faceNum * verticesPerFace;
				}
			}
		}
	}

	void SceneBuilder::BuildCylinder(MeshData& data, uint32_t resolution, uint32_t stackCount, bool capMesh)
	{
		// Capping yet unsupported.
		SKTBD_CORE_ASSERT(!capMesh, "Cylinder cap mesh not yet supported! (FIXME)");
		// FIXME: Reflections on cylinders are broken! Normals seem correct however, not sure what is going wrong..

		// Generate the vertices and indices for a cylinder
		if (resolution < 3u)
			resolution = 3u;

		// Create a writable vertex buffer for all the vertices
		const uint32_t vertexCount = (stackCount + 1u) * (resolution + 1u); // A base of resolution + 1 (to roll back) times n stacks
		const uint32_t indexCount = 2u * 3u * resolution * stackCount;
		data.Vertices.resize(vertexCount);
		data.Indices.resize(indexCount);
		data.Name = L"Cylinder";

		// Create the vertices
		const float deltaTexCoord = 2.f / resolution;
		float currentTexCoord = 0.f;
		const glm::quat roationQuaternion = glm::angleAxis(2.f * SKTBD_PI / resolution, float3(0.f, 1.f, 0.f));
		float3 currentVertexPos = float3(0.f, -1.f, -1.f);
		uint32_t currentVertex = 0u;
		float3 currentTangent = float3(1.f, 0.f, 0.f);
		for (float height = -1.f; height < 1.01f; height += 2.f / static_cast<float>(stackCount))
		{
			currentVertexPos = float3(0.f, height, -1.f);
			for (uint32_t i = 0u; i < resolution + 1u; ++i, ++currentVertex)
			{
				// Round up the last vertex of the circle to the first vertex, to avoid float imprecision errors
				if (i == resolution)
				{
					data.Vertices[currentVertex] = data.Vertices[currentVertex - resolution];
					data.Vertices[currentVertex].texCoord = float2(-1.f, 1.f - (height + 1.f) / 2.f);
				}
				else
				{
					data.Vertices[currentVertex].position = currentVertexPos;
					float3 normal = glm::normalize(float3(data.Vertices[currentVertex].position.x, 0.f, data.Vertices[currentVertex].position.z));
					float3 tangent = glm::normalize(currentTangent);
					data.Vertices[currentVertex].normal = normal;
					data.Vertices[currentVertex].tangent = tangent;
					data.Vertices[currentVertex].bitangent = glm::cross(normal, tangent);
					data.Vertices[currentVertex].texCoord = float2(1.f - currentTexCoord, 1.f - (height + 1.f) / 2.f);
				}
				currentTexCoord += deltaTexCoord;
				currentVertexPos = std::move(glm::rotate(roationQuaternion, std::move(currentVertexPos)));
				currentTangent = std::move(glm::rotate(roationQuaternion, std::move(currentTangent)));
			}
			currentTexCoord = 0.f;
			currentTangent = float3(1.f, 0.f, 0.f);
		}

		// Create the indices
		uint32_t currentIndex = 0u;
		for (uint32_t h = 0u; h < stackCount; ++h)
		{
			for (uint32_t i = 0; i < resolution; ++i, currentIndex += 6u)
			{
				data.Indices[currentIndex + 0u] = h * (resolution + 1u) + i;
				data.Indices[currentIndex + 1u] = h * (resolution + 1u) + i + 1;
				data.Indices[currentIndex + 2u] = h * (resolution + 1u) + (i + 1) + (resolution + 1u);
				data.Indices[currentIndex + 3u] = h * (resolution + 1u) + i;
				data.Indices[currentIndex + 4u] = h * (resolution + 1u) + (i + 1) + (resolution + 1u);
				data.Indices[currentIndex + 5u] = h * (resolution + 1u) + i + (resolution + 1u);
			}
		}
	}

	void SceneBuilder::BuildSphere(MeshData& data, float size,uint32_t resolution)
	{
		// Generate the vertices and indices for a traditional sphere
		// The least resolution contains 3 layers of both lats and longs to make a sphere
		if (resolution < 3u)
			resolution = 3u;

		// Create a writable vertex buffer for all the vertices
		const uint32_t vertexCount = 2u + (resolution - 2u) * (resolution + 1u); // Top and bottom vertex + (m_MeshResolution - 2u) discs of (m_MeshResolution + 1u) vertices
		const uint32_t indexCount = 6u * resolution + 6u * (resolution - 3u) * resolution;
		data.Vertices.resize(vertexCount);
		data.Indices.resize(indexCount);
		data.Name = L"Sphere";

		// Build the vertices
		// Using longitude and latitude to create the vertices of the sphere
		const float deltaTheta = 2.f * SKTBD_PI / resolution;
		const float deltaGamma = SKTBD_PI / (resolution - 1u);	// Resolution includes the bottom vertex, remove it
		const float deltaTexCoordX = 1.f / resolution;	// There will be an extra vertex to go around the circle properly
		const float deltaTexCoordY = 1.f / (resolution - 1u);
		const glm::quat latitudeRoationQuaternion = glm::angleAxis(deltaTheta, float3(0.f, 1.f, 0.f));
		// First vertex is the top of the sphere
		data.Vertices[0].position = data.Vertices[0].normal = float3(0.f, 1.f, 0.f);
		data.Vertices[0].position *= size;
		data.Vertices[0].texCoord = float2(.5f, 0.f);
		// Last vertex is the bottom of the sphere
		data.Vertices[vertexCount - 1u].position = data.Vertices[vertexCount - 1u].normal = float3(0.f, -1.f, 0.f);
		data.Vertices[vertexCount - 1u].position *= size;
		data.Vertices[vertexCount - 1u].texCoord = float2(.5f, 1.f);
		// Other vertices are a series of discs
		float2 currentVertexTex = float2(1.f, deltaTexCoordY);
		for (uint32_t longitude = 0u; longitude < resolution - 2u; ++longitude)	// Exclude top and bottom vertices
		{
			const glm::quat longitudeRoationQuaternion = glm::angleAxis(static_cast<float>(longitude + 1u) * deltaGamma, float3(1.f, 0.f, 0.f));
			float3 currentVertexPos = glm::rotate(longitudeRoationQuaternion, float3(0.f, 1.f, 0.f));
			for (uint32_t latitude = 0u; latitude < resolution + 1u; ++latitude)
			{
				const uint32_t vertexIndex = latitude + longitude * (resolution + 1u) + 1u;	// Add 1 to consider the top vertex

				// Round up the last vertex of the circle to the first vertex, to avoid float imprecision errors
				if (latitude == resolution)
				{
					data.Vertices[vertexIndex] = data.Vertices[vertexIndex - resolution];
					data.Vertices[vertexIndex].texCoord = float2(0.f, (longitude + 1u) * deltaTexCoordY);
				}
				else
				{
					data.Vertices[vertexIndex].position = currentVertexPos*size;
					data.Vertices[vertexIndex].normal = currentVertexPos;
					float theta = atan2f(data.Vertices[vertexIndex].position.z, data.Vertices[vertexIndex].position.x); // See cube sphere to understand how this works
					if (theta < 0) theta += 2.f * SKTBD_PI;
					float phi = acosf(data.Vertices[vertexIndex].position.y);
					float3 tangent = glm::normalize(float3(-sinf(theta) * sinf(phi), 0.f, cosf(theta) * sinf(phi)));
					data.Vertices[vertexIndex].tangent = tangent;
					data.Vertices[vertexIndex].bitangent = glm::cross(currentVertexPos, tangent);
					data.Vertices[vertexIndex].texCoord = currentVertexTex;
				}
				currentVertexTex.x -= deltaTexCoordX;
				currentVertexPos = std::move(glm::rotate(latitudeRoationQuaternion, std::move(currentVertexPos)));
			}
			currentVertexTex.x = 1.f;
			currentVertexTex.y += deltaTexCoordY;
		}

		// Create the indices
		// Index the top cone
		uint32_t currentIndex = 0u;
		for (uint32_t i = 0u; i < resolution; ++i, currentIndex += 3u)
		{
			data.Indices[currentIndex + 0u] = i + 1u;
			data.Indices[currentIndex + 1u] = i + 2u;
			data.Indices[currentIndex + 2u] = 0u;
		}
		// Index the disc quads
		for (uint32_t j = 1u; j < resolution - 2u; ++j)
		{
			for (uint32_t i = 0u; i < resolution; ++i, currentIndex += 6u)
			{
				data.Indices[currentIndex + 0u] = i + 2u + (j - 1u) * (resolution + 1u);
				data.Indices[currentIndex + 1u] = i + 1u + (j - 1u) * (resolution + 1u);
				data.Indices[currentIndex + 2u] = i + 1u + (j - 0u) * (resolution + 1u);
				data.Indices[currentIndex + 3u] = i + 2u + (j - 1u) * (resolution + 1u);
				data.Indices[currentIndex + 4u] = i + 1u + (j - 0u) * (resolution + 1u);
				data.Indices[currentIndex + 5u] = i + 2u + (j - 0u) * (resolution + 1u);
			}
		}
		// Index the bottom cone
		for (uint32_t i = 0u; i < resolution; ++i, currentIndex += 3u)
		{
			data.Indices[currentIndex + 0u] = i + 1u + (resolution - 3u) * (resolution + 1u);
			data.Indices[currentIndex + 1u] = vertexCount - 1u;
			data.Indices[currentIndex + 2u] = i + 2u + (resolution - 3u) * (resolution + 1u);
		}
	}

	void SceneBuilder::BuildTerrain(MeshData& data, uint32_t resolution)
	{
		// Generate the vertices and indices for a traditional terrain
		if (resolution < 1u) resolution = 1u;

		// Create a writable vertex buffer for all the vertices
		const uint32_t vertexCount = (resolution + 1u) * (resolution + 1u);
		const uint32_t indexCount = 6u * (resolution) * (resolution);
		data.Vertices.resize(vertexCount);
		data.Indices.resize(indexCount);
		data.Name = L"Terrain";

		//Create vertex buffer and the index buffer
		const float fWidht = static_cast<float>(resolution);
		const float fHeight = static_cast<float>(resolution);
		for (uint32_t z = 0u; z < resolution + 1u; ++z)
		{
			for (uint32_t x = 0u; x < resolution + 1u; ++x)
			{
				const float fX = static_cast<float>(x) / fWidht;
				const float fZ = static_cast<float>(z) / fHeight;
				const uint32_t idx = x + z * (resolution + 1u);
				data.Vertices[idx].position = float3(fX * 2.f - 1.f, 0.f, fZ * 2.f - 1.f);	// x2 -1 to convert from [0,1] to [-1,1]
				data.Vertices[idx].texCoord = float2(fX, 1.f - fZ);
				data.Vertices[idx].normal = float3(0.f, 1.f, 0.f);
				data.Vertices[idx].tangent = float3(1.f, 0.f, 0.f);
				data.Vertices[idx].bitangent = float3(0.f, 0.f, 1.f);
			}
		}

		uint32_t currentIndex = 0u;
		for (uint32_t z = 0u; z < resolution; ++z)
		{
			for (uint32_t x = 0u; x < resolution; ++x, currentIndex += 6u)
			{
				const uint32_t idx = x + z * (resolution + 1u);
				data.Indices[currentIndex + 0u] = idx;
				data.Indices[currentIndex + 1u] = idx + 1u + (resolution + 1u);
				data.Indices[currentIndex + 2u] = idx + 1u;
				data.Indices[currentIndex + 3u] = idx;
				data.Indices[currentIndex + 4u] = idx + 0u + (resolution + 1u);
				data.Indices[currentIndex + 5u] = idx + 1u + (resolution + 1u);
			}
		}
	}
}