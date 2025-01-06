#pragma once
#include "Core.h"
#include "Platform.h"
#include "Renderer/GraphicsContext.h"
#include "LayerStack.h"
#include "ImGui/ImGuiLayer.h"
#include "Renderer/Buffer.h"
#include "Renderer/Pipeline.h"
#include "Scene/SceneBuilder.h"
#include "Audio/AudioRenderer.h"

#include "Skateboard/Events/AppEvents.h"

namespace Skateboard
{
	class SKTBD_API Application
	{
	public:
		Application();
		DISABLE_COPY_AND_MOVE(Application);

		~Application() = default;
		static Application* Singleton() { return s_Instance; }

		void Run();

		void OnEvent(Event& e);

		void PushLayer(Layer* layer) { m_LayerStack.PushLayer(layer); }
		void PushOverlay(Layer* overlay) { m_LayerStack.PushOverlay(overlay); }

		void GlobalSystemsOnUpdate();

	protected:
		Platform& m_Platform;
		
		LayerStack m_LayerStack;
		ImGuiLayer* p_ImGuiMasterOverlay;	// Raw pointer as ownership will be transferred to LayerStack

	private:
		bool OnWindowClose(WindowCloseEvent& e);
		bool OnWindowResize(WindowResizeEvent& e);

	private:
		static Application* s_Instance;
	};

	Application* CreateApplication(int argc, char** argv);
}


