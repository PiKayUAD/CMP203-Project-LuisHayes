#include "sktbdpch.h"
#include "CameraSystem.h"
#include "Skateboard/Scene/Components.h"
#include "Skateboard/Scene/Scene.h"
#include "Skateboard/Input.h"

void CameraSystem::UpdateCamPosition(Transform& trans, float deltaTime)
{
	auto lstick = Skateboard::Input::GetLeftStick();
	auto rstick = Skateboard::Input::GetRightStick();

	trans.Translation += float3(lstick.x, 0, lstick.y) * trans.GetForwardVector()*deltaTime;
	
}

void CameraSystem::Init(Skateboard::Scene* scn)
{
}

void CameraSystem::HandleInput(Skateboard::TimeManager* time, Skateboard::Scene* scn)
{

	// Control camera from input manager
}


void CameraSystem::RunUpdate(Skateboard::TimeManager* time, Skateboard::Scene* scn)
{
	auto& registry = Skateboard::ECS::GetRegistry();


	auto group = registry.view<Skateboard::TransformComponent, Skateboard::CameraComponent>();
	for (auto entity : group) {
		auto[transform, camera] = group.get<Skateboard::TransformComponent, Skateboard::CameraComponent>(entity);
		
		if (scn && camera.Primary)
		{
			// If the Scene has been provided, then update camera buffers for that scene with the primary camera
			
			
			Skateboard::PerspectiveCamera* perspCam = dynamic_cast<Skateboard::PerspectiveCamera*>(camera.Camera.get());

			// If the current camera is of the type perspective camera then:
			if (perspCam) 
			{
				UpdateCamPosition(transform, time->DeltaTime());
				
			}
			//else if (OrthogrpahicCamera)
				// Update Orthographic camera ...

			// Update the camera view matrix  based on the transform component of the entity
			camera.Camera->UpdateViewMatrix(transform);
		}
	}

}
