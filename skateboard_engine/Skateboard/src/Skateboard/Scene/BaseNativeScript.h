#pragma once
#include "Entity.h"
#include "Skateboard/Time/TimeManager.h"
namespace Skateboard
{
	class BaseNativeScript
	{
	public:

		friend class Scene;

	public:
		virtual ~BaseNativeScript() = default;

		template<typename T>
		T& GetComponent()
		{
			return m_Entity.GetComponent<T>();
		}
		virtual void SetEntity(Entity e) { m_Entity = e; };
		virtual void OnHandleInput(TimeManager* time) =0;
		virtual void OnUpdate(TimeManager* time) =0;
		virtual void OnCreate() =0;
		virtual void OnDestroy() =0;
	protected:
	private:
		Entity m_Entity;
	};
}