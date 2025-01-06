#pragma once
#include "Skateboard/Scene/Components.h"
//all you need for box2d
#include "box2d/box2d.h"

//all you need for jolt make sure jolt.h is declared first
//#include "Jolt/Jolt.h"

// Jolt includes
//#include <Jolt/Physics/Body/BodyCreationSettings.h>

template <class PBody>
struct RigidBodyComponent : Skateboard::BaseComponent
{
	RigidBodyComponent(PBody* body) : Body(body){};
	PBody* Body;
};

struct RigidBody2D : RigidBodyComponent<b2Body>
{
	RigidBody2D(b2Body* body) : RigidBodyComponent<b2Body>(body) {};
};

//struct RigidBody3D : RigidBodyComponent<JPH::Body>
//{
//	RigidBody3D(JPH::Body* body) : RigidBodyComponent<JPH::Body>(body) {};
//};