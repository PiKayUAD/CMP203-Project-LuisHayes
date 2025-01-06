#include "CamObj.h"

CamObj::CamObj() {

}

//CamObj::CamObj(float3 pos, float3 angles) {

//}

void CamObj::rotate_global(float pitch, float yaw, float roll) {
	angles += float3(pitch, yaw, roll);
}

void CamObj::move_local(float3 direction) {
	camera_pos += cam_right * direction.x;
	camera_pos += cam_right * direction.y;
	camera_pos += cam_forward * direction.z;
}

void CamObj::set_rotation(float pitch, float yaw, float roll) {
	angles = float3(pitch, yaw, roll);
}

void CamObj::set_directions(float3 up, float3 forward) {
	cam_up = up;
	cam_forward = forward;
}


void CamObj::move_global(float3 direction) {
	camera_pos += direction;
}

void CamObj::set_position(float3 position) {
	camera_pos = position;
}




float3 CamObj::get_position() {
	return camera_pos;
}

float3 CamObj::get_cam_forward() {
	return cam_forward;
}


void CamObj::update() {
	float cosP = glm::cos(glm::radians(angles.r));
	float cosY = glm::cos(glm::radians(angles.g));
	float cosR = glm::cos(glm::radians(angles.b));
	float sinP = glm::sin(glm::radians(angles.r));
	float sinY = glm::sin(glm::radians(angles.g));
	float sinR = glm::sin(glm::radians(angles.b));

	cam_forward.x = sinY * cosP;
	cam_forward.y = sinP;
	cam_forward.z = cosP * cosY;
	cam_forward = - glm::normalize(cam_forward);
	
	cam_up.x = -cosY * sinR - sinY * sinP * cosR;
	cam_up.y = cosP * cosR;
	cam_up.z = sinY * sinR - sinP * cosR * cosY;
	cam_up = glm::normalize(cam_up);
	cam_right = glm::cross(cam_up, cam_forward);
}

CMP203::FrameData CamObj::get_parented_coords(CMP203::InstanceData parent) {
	return CMP203::FrameData();
}

CMP203::FrameData CamObj::get_cam_data() {
	CMP203::FrameData camera_data;

	camera_data.ProjectionMatrix = glm::perspectiveLH(glm::radians(60.f), GraphicsContext::Context->GetClientAspectRatio(), 0.01f, 100.f);
	camera_data.ViewMatrix = glm::lookAt(camera_pos, camera_pos + cam_forward, cam_up);


	return camera_data;
}

CMP203::FrameData CamObj::look_at(float3 target) {
	CMP203::FrameData camera_data;
	camera_data.ProjectionMatrix = glm::perspectiveLH(glm::radians(60.f), GraphicsContext::Context->GetClientAspectRatio(), 0.01f, 100.f);
	camera_data.ViewMatrix = glm::lookAt(camera_pos, 2 * camera_pos - target, cam_up);

	return camera_data;
}