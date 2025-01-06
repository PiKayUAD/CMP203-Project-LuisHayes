#pragma once
#include "Skateboard/Scene/ECS.h"
#include "BaseSystem.h"
#include "Skateboard/Camera/CameraController.h"

class CameraSystem final : public BaseSystem {
private:
	// Add member variables here!
	Skateboard::CameraController m_CameraController;
	virtual void UpdateCamPosition(Transform& trans, float deltaTime);
	
public:
	virtual void Init(Skateboard::Scene* scn = NULL) override;
	virtual void HandleInput(Skateboard::TimeManager* time, Skateboard::Scene* scn = NULL) override;
	virtual void RunUpdate(Skateboard::TimeManager* time, Skateboard::Scene* scn = NULL) override;
};
