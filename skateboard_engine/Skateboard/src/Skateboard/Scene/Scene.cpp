#include "sktbdpch.h"
#include "Scene.h"
#include "Components.h"
#include "Entity.h"

#include "Skateboard/Camera/Camera.h"
#include "Skateboard/Scene/SceneBuilder.h"
#include "Skateboard/Memory/MemoryManager.h"
#include "Skateboard/Renderer/RenderCommand.h"
#include "Skateboard/Renderer/MeshletEngine/MeshEngine.h"
#include "Skateboard/Time/TimeManager.h"
#include "Skateboard/Assets/AssetManager.h"
#include "Skateboard/Input.h"
#include "Skateboard/Utilities/StringConverters.h"
//#include "Skateboard/Renderer/Renderer.h"
//#include "Skateboard/Renderer/Renderer2D.h"
#include "Skateboard/Renderer/SpriteComponent.h"



namespace Skateboard
{
	Scene::Scene(const std::string& name)
	:
		sceneName(name)
	{
	}

	Scene::~Scene()
	{
	}
	
	void Scene::OnHandleInput(TimeManager* time)
	{
	}

	void Scene::OnUpdate(TimeManager* time)
	{
	}

	void Scene::OnRender()
	{
	}

	

	
}
