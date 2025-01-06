#include "sktbdpch.h"
#include "ECS.h"
#include "Entity.h"
#include "Components.h"

#define SKTBD_LOG_COMPONENT "ECS SYSTEM"
#include "Skateboard/Log.h" 

namespace Skateboard 
{
	ECS& ECS::Singleton()
	{
		// TODO: insert return statement here
		static std::unique_ptr<ECS> singleton(new ECS());
		return *singleton;
	}



	Entity ECS::CreateEntity(const std::string_view& name, entt::registry& reg)
	{
		return  Singleton().CreateEntityImpl(name, reg); 
	}

	Entity ECS::GetEntity(const BaseComponent& b)
	{

		Entity e = b.m_entityId;
		return e;
		// TODO: insert return statement here
	}

	Entity ECS::GetEntity(const std::string_view& tag, entt::registry& reg)
	{
		auto group = reg.view<TagComponent>();
		for (auto entity : group) 
		{
			auto& tagComp = group.get<TagComponent>(entity);
			if (tagComp.tag == tag) {
				return tagComp.m_entityId;
			}
		}
		
		return Entity();
	}

	Entity ECS::CreateEntityImpl(const std::string_view& name, entt::registry& reg) {
		Entity entity =  reg.create() ;
		TagComponent& tag = entity.AddComponent<TagComponent>();
		tag.tag = name.empty() ? "Entity" : name;
		entity.AddComponent<TransformComponent>();
		return entity;
	}

	


}