#pragma once
#include "Skateboard/Scene/ECS.h"
#include "Skateboard/Time/TimeManager.h"

class Entity;

class BaseSystem {
public:
	virtual ~BaseSystem() = default;

	virtual void Init(Skateboard::Scene* scn = NULL) = 0;

	// These are just two of functions I have added in
	// You can add in an initialize function here too if that helps your project organisation!
	virtual void HandleInput(Skateboard::TimeManager* time, Skateboard::Scene* scn = NULL) = 0;
	virtual void RunUpdate(Skateboard::TimeManager* time, Skateboard::Scene* scn = NULL) = 0;
};

//class A {};
//class B {};
//
//template<class ...Components>
//class System
//{
//	template<typename ...Args>
//	void OnComponentAdded(Args&&...);
//
//	
//	void UpdateComponents();
//};
//
//class b2BodyDef{};
//
//
//class PhysicsSystem : System<A, B>
//{
//	void OnComponentAdded(b2BodyDef* def) {
//	
//	};
//};