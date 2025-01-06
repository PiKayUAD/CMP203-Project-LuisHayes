#pragma once


#include <string>
#include <vector>

#include "Skateboard/Camera/Camera.h"
#include "Skateboard/Mathematics.h"
#include "Skateboard/Scene/InstanceData.h"
#include "Skateboard/Renderer/Materials/Material.h"
#include "Skateboard/Renderer/Animation/Animator/Animator.h"
#include "entt.hpp"
#include "ECS.h"



namespace Skateboard
{
	class Entity;

	struct SpriteFrame 
	{
		float2 uv_tl;
		float2 uv_br;
	};
	
	struct SpriteComponentTest 
	{
		std::string s;
		entt::registry* reg_;
		SpriteComponentTest(entt::registry* reg) : reg_(reg){};
		SpriteComponentTest(const SpriteComponentTest&) = default;
	};


	class BaseComponent 
	{
	public:
		BaseComponent() = default;
		BaseComponent(entt::entity entityId): m_entityId(entityId){};
		BaseComponent(const BaseComponent&) = default;
		
		template<typename T> T&  GetComponent() const { return ECS::GetRegistry().get<T>(m_entityId); };
		template<typename T> bool HasComponent() const { return ECS::GetRegistry().any_of<T>(m_entityId); }
		virtual void OnComponentAdded(Entity* e) {};
		virtual void OnComponentRemoved(Entity* e) {};

		entt::entity m_entityId;


	};


	


	struct TagComponent: public BaseComponent 
	{
		std::string tag;

		TagComponent() = default;
		TagComponent(const TagComponent&) = default;
		TagComponent(const std::string& string)
			:
			tag(string) {}
	};

	struct TransformComponent : public BaseComponent
	{
		/*
		float3 Translation	= { 0.f,0.f,0.f };
		float3 Rotation		= { 0.f, 0.f,0.f };
		float3 Scale		= { 1.f, 1.f, 1.f };
		*/

		Transform Trans;

		TransformComponent() = default;
		~TransformComponent();
		TransformComponent(const TransformComponent&) = default;
		TransformComponent(const glm::vec3& trans)
		{
			Trans.Translation = trans;
		}

		void OnComponentAdded(Entity* E) override;

		operator Transform& () { return Trans; };
		operator const Transform () { return Trans; };
		operator const Transform& () { return Trans; };
		operator const glm::mat4x4 () { return Trans.AsMatrix(); };
		
		[[nodiscard]] glm::mat4x4 GetTransform() const
		{
			/*auto euler = glm::eulerAngles(Trans.Rotation);
			return	glm::translate(Trans.Translation) *
					glm::yawPitchRoll(glm::radians(euler.y), glm::radians(euler.x), glm::radians(euler.z)) *
					glm::scale(Trans.Scale);
		*/
			return Trans.AsMatrix();
		}
		uint32_t BufferId;
	};

	struct CameraComponent : public BaseComponent
	{
		CameraComponent() = default;
		DISABLE_COPY(CameraComponent)
		ENABLE_MOVE(CameraComponent)


		std::unique_ptr<Camera> Camera;
		bool Primary = true;
		bool FixedAspectRatio = false;

	};




	//Forward Declarations
	class BaseNativeScript;
	class TimeManager;


	struct NativeScriptComponent
	{
		BaseNativeScript* Instance = nullptr;

		std::function <void()> InstantiateFunction;
		std::function <void()> DestroyInstanceFunction;

		std::function<void()> OnCreateFunction;
		std::function<void()> OnDestroyFunction;
		std::function<void(TimeManager*)> OnHandleInputFunction;
		std::function<void(TimeManager*)> OnUpdateFunction;

		void (*DestoryScript)(NativeScriptComponent*);

		template<typename T>
		void Bind()
		{
			//TODO: I Removed this for compilation on the PS5! It's giving massive warnings
			InstantiateFunction = [&]() {Instance = new T; };
			DestroyInstanceFunction = [&]() {delete (T*)Instance; Instance = nullptr; };

			OnCreateFunction = [&]() {((T*)Instance)->OnCreate();};
			OnDestroyFunction = [&]() {((T*)Instance)->OnDestroy();};
			OnUpdateFunction = [&](TimeManager* tm) {((T*)Instance)->OnUpdate(tm);};
			OnHandleInputFunction = [&](TimeManager* tm) {((T*)Instance)->OnHandleInput(tm);};

		}
	};

	struct MaterialComponent : public BaseComponent
	{
		MaterialComponent() = default;
		~MaterialComponent();

		void OnComponentAdded(Entity* E) override;

		Material Material;
		uint32_t BufferId;
	};

	struct StaticMeshComponent : public BaseComponent
	{
		StaticMeshComponent() = default;
		StaticMeshComponent(const std::string_view str) : ModelName(str) {};
		StaticMeshComponent(const StaticMeshComponent&) = default;

		std::string ModelName;
		Transform offset;
	};

	struct RendererComponent : public BaseComponent
	{
		RendererComponent() = default;
		RendererComponent(const RendererComponent&) = default;
		int32_t RendererId;
		int32_t Textures[32];
	};

	struct StaticMeshInstanceComponent
	{
		StaticMeshInstanceComponent() : MeshId(UINT32_MAX), InstanceId(UINT32_MAX), MaterialId(-1) {}
		StaticMeshInstanceComponent(uint32_t meshID) : MeshId(meshID), InstanceId(UINT32_MAX), MaterialId(-1) {}
		StaticMeshInstanceComponent(const StaticMeshInstanceComponent&) = default;
		auto operator=(const StaticMeshInstanceComponent&)  ->StaticMeshInstanceComponent & = default;
		StaticMeshInstanceComponent(StaticMeshInstanceComponent&&) = default;
		~StaticMeshInstanceComponent() = default;

		MeshID MeshId;
		InstanceID InstanceId;
		int32_t MaterialId;
	};

	struct AnimatorComponent
	{
		AnimatorComponent() = default;
		AnimatorComponent(const AnimatorComponent&) = default;
		AnimatorComponent(AnimatorComponent&&) = default;
		~AnimatorComponent() = default;

		//AnimationUpdateMethod AnimUpdateMethod;
		//Animator AnimController;
		//Armature ArmatureId;
		bool ApplyRootMotion;
		
	};

	

}


