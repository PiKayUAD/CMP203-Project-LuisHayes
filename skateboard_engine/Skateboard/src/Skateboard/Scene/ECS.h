#pragma once
#include <cstdint>
#include "entt.hpp"

namespace Skateboard {
	
	class Scene;
	class BaseComponent;
	class Entity;



	class ECS 
	{
	public:

		ECS() = default;
		~ECS() = default;

		static ECS& Singleton();
		static entt::registry& GetRegistry() { return Singleton().Registry; };

		static Entity CreateEntity(const std::string_view& name, entt::registry& reg = Singleton().GetRegistry()) ;

		static Entity GetEntity(const BaseComponent& b);
		static Entity GetEntity(const std::string_view& tag, entt::registry& reg = Singleton().GetRegistry());

	private:

		// Creates an entity and adds the default transform component
		Entity CreateEntityImpl(const std::string_view& name, entt::registry& reg );
		entt::registry Registry;


	};

	typedef ECS ECSSystem;
}