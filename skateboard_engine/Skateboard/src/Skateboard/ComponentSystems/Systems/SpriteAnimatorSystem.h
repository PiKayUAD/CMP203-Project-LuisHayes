#pragma once

#include "BaseSystem.h"

class SpriteAnimatorSystem :
    public BaseSystem
{
    void Init(Skateboard::Scene* scn = NULL) override;

    void HandleInput(Skateboard::TimeManager* time, Skateboard::Scene* scn = NULL) override;
    void RunUpdate(Skateboard::TimeManager* time, Skateboard::Scene* scn = NULL) override;
};

