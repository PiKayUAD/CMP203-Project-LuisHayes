#include "sktbdpch.h"
#include "PhysicsSystem.h"
#include "Skateboard/Scene/ECS.h"

#define SKTBD_LOG_COMPONENT "PHYSICS SYSTEM"
#include "Skateboard/Log.h" 

//Physics System sample Box2D

void PhysicsSystem2D::Init(Skateboard::Scene* scn)
{

}

void PhysicsSystem2D::HandleInput(Skateboard::TimeManager* time, Skateboard::Scene* scn)
{

}

void PhysicsSystem2D::RunUpdate(Skateboard::TimeManager* time, Skateboard::Scene* scn)
{
	//update physics World
	PhysWorld->Step(timeStep, velocityIterations, positionIterations);

	auto physicsObjects2D = Skateboard::ECS::GetRegistry().view<RigidBody2D, Skateboard::TransformComponent>();

	for (auto entity : physicsObjects2D)
	{
		auto [rigidbody, transform] = physicsObjects2D.get<RigidBody2D, Skateboard::TransformComponent>(entity);

		b2Vec2 translation	= rigidbody.Body->GetPosition();
		float  Zrotation	= rigidbody.Body->GetAngle();

		//SKTBD_APP_TRACE("position x:{0},y:{1}", translation.x, translation.y);

		transform.Trans.Translation = float3(translation.x, translation.y, transform.Trans.Translation.z);
		transform.Trans.SetEulerZ(Zrotation);
	}

	//auto Contacts = MainWorld->GetContactList();
}
