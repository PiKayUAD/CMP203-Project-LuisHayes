#include "sktbdpch.h"
#include "ImGuiLayer.h"
#include "Skateboard/Platform.h"

namespace Skateboard
{
	ImGuiLayer::ImGuiLayer() :
		Layer("ImGuiLayer")
	{
	}

	void ImGuiLayer::OnAttach()
	{
		Platform::GetPlatform().InitImGui();
	}

	void ImGuiLayer::OnDetach()
	{
		Platform::GetPlatform().ShutdownImGui();
	}

	void ImGuiLayer::OnImGuiRender()
	{
	}

	void ImGuiLayer::Begin()
	{
		Platform::GetPlatform().BeginImGuiPass();
	}

	void ImGuiLayer::End()
	{
		Platform::GetPlatform().EndImGuiPass();
	}
}
