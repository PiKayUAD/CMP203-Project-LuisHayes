// includes all the basic structures and most standard library containers used throughout Skateboard
#include "sktbdpch.h"

#include "CMP203/OBJ_Loader.h"
#include "MainScene.h"
#include "Skateboard/Camera/Camera.h"
#include "Skateboard/Time/TimeManager.h"
#include "Skateboard/Input.h"
#include "Skateboard/Assets/AssetManager.h"



using namespace Skateboard;

MainScene::MainScene(const std::string& name): 
	Scene(name)
{
	m_Renderer.Init();

	//ball_rotation.World = glm::translate(float3(0, 0, 0));

	//camera = CamObj(float3(0.f, 0.f, 0.f), float3(0.f, 0.f, 0.f));
	
	AssetManager::LoadTexture(L"assets/tennis", "tennis");

	AssetManager::LoadTexture(L"assets/grass/grass1", "grass1");
	AssetManager::LoadTexture(L"assets/grass/grass2", "grass2");
	AssetManager::LoadTexture(L"assets/grass/grass3", "grass3");
	AssetManager::LoadTexture(L"assets/grass/grass4", "grass4");
	AssetManager::LoadTexture(L"assets/grass/grass5", "grass5");
	AssetManager::LoadTexture(L"assets/grass/grass6", "grass6");
	AssetManager::LoadTexture(L"assets/grass/grass7", "grass7");
	AssetManager::LoadTexture(L"assets/grass/grass8", "grass8");

	m_Renderer.AddLight(CMP203::Light());
	m_Renderer.SetAmbientLight(float3(0.1f, 0.1f, 0.1f)); //RGB
	lightObj = &m_Renderer.GetLight(0);




	lightObj->LightType = CMP203::LightType::LightPoint;
	lightObj->DiffuseColour = float4(1.f, 0.f, 0.f, 1.f); //RGBA
	lightObj->LightPosition = float3(8, 8, -1.f); //XYZ
	lightObj->Attenuation = float4(0.05f, 0.01f, 0.001f, 100.f);
	

	objl::Loader objLoader;
	bool success = objLoader.LoadFile("./assets/models/doog.obj");
	dog_mesh = objLoader.LoadedMeshes[0];

	if (success) {
		AssetManager::LoadTexture(L"assets/dog", "dog");
		
		std::cout << "xxxxxxxxxxxxxxxxxxxxxxxxxx\n\n\n\n";

		//flips winding order
		for (int i = 0; i < dog_mesh.Indices.size(); i += 3) {
			uint32_t buffer = dog_mesh.Indices[i];
			dog_mesh.Indices[i] = dog_mesh.Indices[i + 2];
			dog_mesh.Indices[i + 2] = buffer;
		}

		//converts verticies
		for (int i = 0; i < dog_mesh.Vertices.size(); i++) {
			CMP203::Vertex current_v;

			current_v.Position = float3(dog_mesh.Vertices[i].Position.X, dog_mesh.Vertices[i].Position.Y, dog_mesh.Vertices[i].Position.Z);
			current_v.Normal = float3(dog_mesh.Vertices[i].Normal.X, dog_mesh.Vertices[i].Normal.Y, dog_mesh.Vertices[i].Normal.Z);
			current_v.UV = float2(dog_mesh.Vertices[i].TextureCoordinate.X, 1 - dog_mesh.Vertices[i].TextureCoordinate.Y);


			dog_verticies.push_back(current_v);
		}
		dog_data.TextureIndex = GraphicsContext::Context->GetViewHeapIndex(AssetManager::GetTexture("dog"));
	}
	else {
		std::cout << "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee\n\n\n\n";
	}
	

}

void MainScene::OnHandleInput(TimeManager* time)
{
	Scene::OnHandleInput(time);
}

void MainScene::OnUpdate(TimeManager* time)
{
	Scene::OnUpdate(time);
	
	ball_pos += ball_vel * time->DeltaTime();
	if (ball == THROWN) {
		ball_vel.y -= 5.f * time->DeltaTime();
		ball_vel *= 0.99f;

		if (ball_pos.y + ball_vel.y * time->DeltaTime() < -1.f) {
			ball_vel.y *= -1.f;
		}
	}
	
	//ball_rotation.World += glm::rotate(time->DeltaTime() * ball_vel.length(), glm::cross(ball_vel, float3(0, 1, 0)));
	


	//Dog AI

	float rotation = 0;
	float2 target_coords;
	float vel = 0;
	float length = 0;

	switch(ball) {
	
	case HELD:
		vel = 0;
		target_coords = { position.x, position.z };

		break;
	
	case THROWN:
		vel = 10;
		target_coords = { ball_pos.x, ball_pos.z };

		length = (ball_pos.y * 0.5) + (dog_pos.x - target_coords.x) * (dog_pos.x - target_coords.x) + (dog_pos.y - target_coords.y) * (dog_pos.y - target_coords.y);

		if (length < 1) {
			ball = DOG;
		}

		break;

	case DOG:
		vel = 4;
		target_coords = { position.x, position.z };

		length = (dog_pos.x - target_coords.x) * (dog_pos.x - target_coords.x) + (dog_pos.y - target_coords.y) * (dog_pos.y - target_coords.y);

		if (length < 4) {
			ball = HELD;
		}
		break;
	}

	float target_angle = atan((target_coords.x - dog_pos.x) / (target_coords.y - dog_pos.y));
	if ((target_coords.y - dog_pos.y) > 0) { target_angle += SKTBD_PI; }
	
	dog_pos += vel * float2(-sin(target_angle), -cos(target_angle)) * time->DeltaTime();

	dog_cam.set_position(float3(dog_pos.x, 2.f, dog_pos.y));
	dog_cam.set_rotation(0, target_angle + SKTBD_PI, 0);
	//dog_cam.set_directions(float3(0,1,0), float3(cos(target_angle), 0, sin(target_angle)));
	dog_cam.update();

	dog_data.World = glm::translate(float3(dog_pos.x, 1, dog_pos.y)) * glm::rotate(target_angle, float3(0.f, 1.f, 0.f)) * glm::scale(float3(2,2,2));
}

void MainScene::render_torus(int small_res, int big_res, float thickness, CMP203::InstanceData* torusData) {
	
	
	std::vector<CMP203::Vertex> torus_vertices;
	std::vector<uint32_t> torus_indicies;

	float small_radius = thickness;
	float big_radius = 1.f;


	for (int i = 0; i < big_res; i++) {
		for (int j = 0; j < small_res; j++) {
			CMP203::Vertex current_v;

			current_v.Colour = float3(1.f, 0.f, 0.f);

			float3 vector = float3(glm::cos(i * 2 * SKTBD_PI / big_res), glm::sin(i * 2 * SKTBD_PI / big_res), 0);

			current_v.Position += vector * (big_radius + small_radius * glm::cos(j * 2 * SKTBD_PI / small_res));
			current_v.Position.z = big_radius + small_radius * glm::sin(j * 2 * SKTBD_PI / small_res);



			/*current_v.Normal = vector * (small_radius * glm::cos(j * 2 * SKTBD_PI / small_res));
			current_v.Normal.z = big_radius + small_radius * glm::sin(j * 2 * SKTBD_PI / small_res);
			current_v.Normal /= current_v.Normal.length;
			*/


			torus_vertices.push_back(current_v);
			
			
			
			torus_indicies.push_back(i * small_res + j);
			torus_indicies.push_back(i * small_res + (j + 1) % small_res);
			torus_indicies.push_back((i + 1) % big_res * small_res + j);
			
			torus_indicies.push_back((i + 1) % big_res * small_res + j);
			torus_indicies.push_back(i * small_res + (j + 1) % small_res);
			torus_indicies.push_back((i + 1) % big_res * small_res + (j + 1) % small_res);
		}
	}

	for (CMP203::Vertex point : torus_vertices) {
		point.Colour = point.Position;
	}


	m_Renderer.Topology = SKTBD_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_Renderer.DrawVertices(torus_vertices.data(), torus_vertices.size(), torus_indicies.data(), torus_indicies.size(), torusData);
}


void MainScene::render_quad(CMP203::InstanceData* quadData) {
	std::vector<CMP203::Vertex> quad_verticies;
	uint32_t quad_indicies[] = { 3, 1, 0, 0, 2, 3};

	for (int i = -1; i <= 1; i += 2) {
		for (int j = -1; j <= 1; j += 2) {
			CMP203::Vertex current_v(float3(i, j, 0));
			current_v.UV = float2((i + 1) / 2, (j + 1) / 2);
			current_v.Normal = float3(0, 0, -1);
			quad_verticies.push_back(current_v);
		}
	}

	m_Renderer.Topology = SKTBD_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_Renderer.DrawVertices(quad_verticies.data(), quad_verticies.size(), quad_indicies, 6, quadData);

}


void MainScene::render_sphere(int resolution, CMP203::InstanceData* sphereData) {
	std::vector<CMP203::Vertex> sphere_vertices;
	std::vector<uint32_t> sphere_indicies;


	for (int i = 0; i <= resolution; i++) {
		for (int j = 0; j <= resolution * 2; j++) {
			CMP203::Vertex current_v;

			current_v.Colour = float3(1.f, 0.f, 0.f);

			current_v.Position.z = glm::cos(SKTBD_PI * (float)i / (float)(resolution));
			current_v.Position.x = glm::cos(SKTBD_PI * (float)j / (float)resolution) * glm::sin(SKTBD_PI * (float)i / (float)(resolution));
			current_v.Position.y = glm::sin(SKTBD_PI * (float)j / (float)resolution) * glm::sin(SKTBD_PI * (float)i / (float)(resolution));

			current_v.Normal = current_v.Position;

			current_v.UV = float2((float)j / (resolution * 2), (float)i / resolution);

			sphere_vertices.push_back(current_v);


				
				sphere_indicies.push_back(i * resolution * 2 + j);
				sphere_indicies.push_back(i * resolution * 2 + (j + 1));
				sphere_indicies.push_back((i + 1) * resolution * 2 + j + 1);

			sphere_indicies.push_back((i + 1) * resolution * 2 + j + 1);
			sphere_indicies.push_back((i + 1) * resolution * 2 + j);
			sphere_indicies.push_back(i * resolution * 2 + j);
			
		
		}
	}


	m_Renderer.Topology = SKTBD_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	m_Renderer.DrawVertices(sphere_vertices.data(), sphere_vertices.size(), sphere_indicies.data(), sphere_indicies.size(), sphereData);
}



void MainScene::OnRender()
{
	CMP203::InstanceData torusData;
	CMP203::InstanceData quadData;
	quadData.World = sphereData.World = glm::translate(float3(0, -1, 0)) *
		glm::rotate(glm::radians(90.f), float3(1.f, 0.f, 0.f)) *
		glm::scale(float3(10.f, 10.f, 10.f));

	//glm::mat4 rotation, translation, scale;

	torusData.World = glm::translate(position) * glm::rotate(glm::radians(90.f), float3(1.f, 0.f, 0.f));
	
	
	sphereData.ColourScale = float4(.1f, .1f, .1f, .1f);
	sphereData.SpecularColor = float4(1.f, 1.f, 1.f, 1.f);
	sphereData.SpecularWeight = 5.f;
	sphereData.SpecularPower = 0.5f;
	

	
	torusData.SpecularPower = 0.5f;
	

	switch (ball) {
	case HELD:
		sphereData.World = glm::translate(camera.get_position()) * glm::rotate(glm::radians(45.f), float3(0.f, 1.f, 0.f)) * glm::translate(-camera.get_cam_forward() + float3(0, -.5f, 0)) * glm::scale(float3(0.2f, 0.2f, 0.2f));
		break;

	case THROWN:
		sphereData.World = glm::translate(ball_pos) * glm::scale(float3(0.2f, 0.2f, 0.2f));
		break;

	case DOG:
		sphereData.World = dog_data.World * glm::translate(float3(0, -.1f, -.3f)) * glm::scale(float3(0.1f, 0.1f, 0.1f));
		break;
	}
	
	

	//sphereData.World += ball_rotation;

	sphereData.TextureIndex = GraphicsContext::Context->GetViewHeapIndex(AssetManager::GetTexture("tennis"));


	


	Scene::OnRender();

	m_Renderer.Begin();

	render_torus(6, 12, 0.2f, &torusData);
	
	m_Renderer.Topology = SKTBD_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	
	
	m_Renderer.DrawVertices(dog_verticies.data(), dog_verticies.size(), dog_mesh.Indices.data(), dog_mesh.Indices.size(), &dog_data);

	render_sphere(8, &sphereData);

	//render_quad(&quadData);

	CMP203::InstanceData grassData;

	m_Renderer.SetPipelineFlags(CMP203::ALPHA_BLEND_TRANSPARENCY);

	for (int k = 1; k <= 8; k++) {
		for (int i = 0; i < 4; i++) {
			for (int j = 0; j < 4; j++) {
				grassData.World = glm::translate(float3(10 * i, -1 + 0.035f * k, 10 * j)) * 
					glm::rotate(glm::radians(90.f), float3(1.f, 0.f, 0.f)) * glm::scale(float3(5.f, 5.f, 10.f));

					grassData.TextureIndex = GraphicsContext::Context->GetViewHeapIndex(AssetManager::GetTexture("grass" + std::to_string(k)));
					render_quad(&grassData);
			}
		}
	}

	
	m_Renderer.UnsetPipelineFlags(CMP203::ALPHA_BLEND_TRANSPARENCY);

	m_Renderer.End();
}

void MainScene::OnImGuiRender()
{


	if (ImGui::IsKeyPressed(ImGuiKey_A)) {
		
	}

	
	ImGui::Begin("ImGui");//creates new window

	

	//ImGui::Text((std::to_string(mousePos.x)).c_str());
	ImGui::SliderFloat3("TorusPos", (float*)&position, -5.f, 5.f);
	ImGui::SliderFloat3("SphereRot", (float*)&rotation, -5.f, 5.f);


	if (playerCam) {
		if (ImGui::IsKeyDown(ImGuiKey_W)) {
			camera.move_local(float3(0.f, 0.f, -0.1f));
		}
		if (ImGui::IsKeyDown(ImGuiKey_S)) {
			camera.move_local(float3(0.f, 0.f, 0.1f));
		}
		if (ImGui::IsKeyDown(ImGuiKey_A)) {
			camera.move_local(float3(0.1f, 0.f, 0.f));
		}
		if (ImGui::IsKeyDown(ImGuiKey_D)) {
			camera.move_local(float3(-0.1f, 0.f, 0.f));
		}
		if (ImGui::IsKeyDown(ImGuiKey_Q)) {
			camera.move_local(float3(0.f, -0.1f, 0.f));
		}
		if (ImGui::IsKeyDown(ImGuiKey_E)) {
			camera.move_local(float3(0.f, 0.1f, 0.f));
		}
		camera.move_global(float3(0, 2 - camera.get_position().y, 0));

		if (ImGui::IsKeyDown(ImGuiKey_I)) {
			camera.rotate_global(0.5f, 0.f, 0);
		}
		if (ImGui::IsKeyDown(ImGuiKey_K)) {
			camera.rotate_global(-0.5f, 0.f, 0);
		}
		if (ImGui::IsKeyDown(ImGuiKey_J)) {
			camera.rotate_global(0., -0.5f, 0);
		}
		if (ImGui::IsKeyDown(ImGuiKey_L)) {
			camera.rotate_global(0.f, 0.5f, 0);
		}

		if (ImGui::IsKeyPressed(ImGuiKey_Enter)) {
			lightObj = &m_Renderer.GetLight(0);
			lightObj->Attenuation.w = 100 - lightObj->Attenuation.w;
		}


		if (ball == HELD && ImGui::IsKeyPressed(ImGuiKey_Space)) {
			ball = THROWN;
			ball_vel = -20 * camera.get_cam_forward();
			ball_pos = position;
		}
		else if (ball != HELD && ImGui::IsKeyPressed(ImGuiKey_Space)) {
			is_dog_cam = true;
		}

		camera.update();

		position = camera.get_position();
	}
	

	


	ImGui::ColorPicker4("colour", (float*)&bgColour);
	GraphicsContext::Context->SetBackBufferClearColour(bgColour);

	ImGui::End();//ends that window
	
	if (!is_dog_cam) {
		m_Renderer.SetFrameData(camera.get_cam_data());
	}
	else {
		switch (ball)
		{
		case HELD:
			is_dog_cam = false;
			break;

		case THROWN:
			m_Renderer.SetFrameData(dog_cam.look_at(ball_pos));
			break;
		case DOG:
			m_Renderer.SetFrameData(dog_cam.look_at(float3(position.x, 0, position.z)));
			break;
		}
	}

	
	
}
