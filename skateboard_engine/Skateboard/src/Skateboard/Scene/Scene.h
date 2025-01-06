#pragma once
#include "entt.hpp"

#include "Skateboard/Scene/InstanceData.h"
#include "Skateboard/SizedPtr.h"
#include "Skateboard/Scene/AABB.h"

#include "Skateboard/Camera/CameraController.h"
#include "Skateboard/Renderer/Lights/Light.h"
#include "Skateboard/Scene/Components.h"
#include "Skateboard/Input.h"
#include "Skateboard/Events/Event.h"
#include "PipelineMap.h"
#include "ECS.h"

namespace Skateboard
{
	class Entity;
	class TimeManager;

	struct SKTBDPassBuffer
	{
		float4x4 ViewMatrix;
		float4x4 ProjectionMatrix;
		float4x4 ViewMatrixInverse;
		float4x4 ProjectionMatrixInverse;
		float3 CameraPosition;
		float Padding;
	};

	struct SKTBDInstanceBuffer
	{
		float4x4 WorldMatrix;
		uint32_t MeshID;
		int32_t MaterialIndex;
	};

	struct SKTBDLightBuffer
	{
		float3	Diffuse{ 0.9f, 0.9f, 0.9f };
		float	FalloffStart{ 1.0f };
		float3	Direction{ 0.f, -1.0f, 0.0f };
		float	FalloffEnd{ 10.0f };
		float3	Position{ 0.f, 5.0f, 0.f };
		float	SpotPower{ 64.0f };
		float3	Radiance{ 0.01f, 0.01f, 0.01f };
		float	Pad0{ 0.f };
		float4x4 LightViewProjTex;
	};

	struct SKTBDProceduralPrimitiveBuffer
	{
		float Radius;
	};

#define MAX_GEOMETRY 8
#define MAX_INSTANCES 64
#define MAX_BONE_COUNT 128

	struct AnimWorld
	{
		glm::mat4x4 WorldTransform;
		glm::mat4x4 BoneTransforms[MAX_BONE_COUNT];
	};

	class Scene
	{
		friend class Entity;
		friend class SceneBuilder;
	public:
		explicit Scene(const std::string& name);

		Scene() = delete;
		DISABLE_COPY_AND_MOVE(Scene);
		virtual ~Scene();

		virtual void OnHandleInput(TimeManager* time);
		virtual void OnUpdate(TimeManager* time);
		virtual void OnRender();
		virtual void OnImGuiRender() {};

		virtual void OnSceneEnter() {};
		virtual void OnSceneExit() {};

		virtual void OnEvent(Skateboard::Event& e) {};

		inline const int GetSceneIndex() { return sceneIndex; };

	protected:
		std::string sceneName;
		int sceneIndex;
	};
}
