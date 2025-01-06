#pragma once
#include "Skateboard/Scene/Scene.h"
#include "Skateboard/ComponentSystems/Systems/BaseSystem.h"

#include "CMP203/Renderer203.h"
#include "CMP203/OBJ_Loader.h"
#include "CamObj.h"


class MainScene : public Skateboard::Scene {
public:
	explicit MainScene(const std::string& name);

	MainScene() = delete;

	virtual void OnHandleInput(Skateboard::TimeManager* time) override;
	virtual void OnUpdate(Skateboard::TimeManager* time) override;
	virtual void OnRender() override;

	virtual void OnImGuiRender() override;

	virtual void render_torus(int small_res, int big_res, float thickness, CMP203::InstanceData* torusData);

	virtual void render_sphere(int resolution, CMP203::InstanceData* sphereData);
	virtual void render_quad(CMP203::InstanceData* quadData);

private:
	CMP203::Renderer203 m_Renderer;

	float mouseX = 0.f;
	float mouseY = 0.f;

	ImVec2 cursorPos = ImVec2(0.f, 0.f);
	
	CamObj camera;
	CamObj dog_cam;

	float3 ball_pos = float3(0.5f, 1.f, 0.f);
	float3 ball_vel = float3(0.f, 0.f, 0.f);

	float3 rotation = float3(0.f, 0.f, 0.f);
	float3 position = float3(0.f, -1.f, 0.f);
	float4 bgColour;

	enum BallState {
		THROWN,
		HELD,
		DOG
	};

	bool playerCam = true;

	BallState ball = HELD;


	CMP203::Light* lightObj;

	CMP203::InstanceData sphereData;
	bool is_dog_cam = false;


	float2 dog_pos = { 0, 5 };
	float dog_rot = SKTBD_PI;
	objl::Mesh dog_mesh;
	CMP203::InstanceData dog_data;
	std::vector<CMP203::Vertex> dog_verticies;

	


};