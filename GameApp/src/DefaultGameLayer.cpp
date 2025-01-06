#include "DefaultGameLayer.h"
#include "Skateboard/Assets/AssetManager.h"
#include "Skateboard/Scene/SceneBuilder.h"
#include "MainScene.h"

DefaultGameLayer::DefaultGameLayer()
{
	// Initialise the scene
	p_Scene = std::make_unique<MainScene>("Demo Scene");
}

DefaultGameLayer::~DefaultGameLayer()
{
}

void DefaultGameLayer::OnEvent(Skateboard::Event& e)
{
	p_Scene->OnEvent(e);
}

bool DefaultGameLayer::OnHandleInput(Skateboard::TimeManager* time)
{
	p_Scene->OnHandleInput(time);
	return true;
}

bool DefaultGameLayer::OnUpdate(Skateboard::TimeManager* time)
{
	// Prepare for rendering
	p_Scene->OnUpdate(time);
	return true;
}

void DefaultGameLayer::OnRender()
{
	// Render the scene
	p_Scene->OnRender();
}

void DefaultGameLayer::OnImGuiRender()
{
	p_Scene->OnImGuiRender();
}


