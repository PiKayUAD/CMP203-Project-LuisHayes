#pragma once
#include "Skateboard/Scene/Scene.h"
#include "Skateboard/ComponentSystems/Systems/BaseSystem.h"

#include "CMP203/Renderer203.h"

class CamObj
{
public:
	CamObj();
	CamObj(float3 pos, float3 angles);

	void rotate_local(float pitch, float yaw, float roll);
	void rotate_global(float pitch, float yaw, float roll);
	void set_rotation(float pitch, float yaw, float roll);

	void set_directions(float3 up, float3 forward);

	void move_local(float3 direction);
	void move_global(float3 direction);
	void set_position(float3 position);


	float3 get_position();
	float3 get_cam_forward();

	void update();

	CMP203::FrameData get_parented_coords(CMP203::InstanceData parent);
	CMP203::FrameData get_cam_data();
	CMP203::FrameData look_at(float3 target);


private:
	float3 camera_pos = { 0.f, 0.f, 0.f };
	float3 angles = { 0.f, 0.f, 0.f };

	float3 cam_forward = { 0.f, 0.f, -1.f };
	float3 cam_right = {1.f, 0.f, 0.f};
	float3 cam_up = {0.f, 1.f, 1.f};


	
};

