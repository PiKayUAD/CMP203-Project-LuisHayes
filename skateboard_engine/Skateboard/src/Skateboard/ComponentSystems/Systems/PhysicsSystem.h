#pragma once

#include "BaseSystem.h"
#include "sktbdpch.h"

#include "box2d/box2d.h"
#include "Jolt/Jolt.h"

#include "Skateboard/Scene/ECS.h"
#include "Skateboard/ComponentSystems/Components/PhysicsBodyComponent.h"

class PhysicsSystem2D :         //BOX2D
    public BaseSystem
{
public:
    PhysicsSystem2D(std::shared_ptr<b2World> world) : PhysWorld(world) {};
    
    void Init(Skateboard::Scene* scn = NULL) override;

    void HandleInput(Skateboard::TimeManager* time, Skateboard::Scene* scn = NULL) override;
    void RunUpdate(Skateboard::TimeManager* time, Skateboard::Scene* scn = NULL) override;
private:
    std::shared_ptr<b2World> PhysWorld;

    const float timeStep = 1.f / 60.f;
    const int32 velocityIterations = 8;
    const int32 positionIterations = 3;
};

class PhysicsSystem3D :         //JOLT
    public BaseSystem
{
    void Init(Skateboard::Scene* scn = NULL) override;
    void HandleInput(Skateboard::TimeManager* time, Skateboard::Scene* scn = NULL) override;
    void RunUpdate(Skateboard::TimeManager* time, Skateboard::Scene* scn = NULL) override;

};

