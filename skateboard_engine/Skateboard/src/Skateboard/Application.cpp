#include "sktbdpch.h"

#define	SKTBD_LOG_COMPONENT "APPLICATION"
#include "Skateboard/Log.h"

#include "Application.h"

#include "Renderer/RenderCommand.h"
#include "Skateboard/Memory/MemoryManager.h"

namespace Skateboard
{
	Application* Application::s_Instance = nullptr;

	Application::Application() :
		m_Platform(Platform::GetPlatform()),
		p_ImGuiMasterOverlay(new ImGuiLayer())	// Ownership is transferred to the layer stack, no need to worry about delete
	{
		// Set the platforms event call back function
		m_Platform.SetOnEventCallback(BIND_EVENT(Application::OnEvent));
		m_Platform.Init();

		Skateboard::GraphicsContext::Context->Reset();

		// Initialise the renderer
		RenderCommand::Init();
		//TODO: Need a more graceful solution, for now should work.


		SKTBD_CORE_ASSERT(!s_Instance, "Cannot create two application instances. Consider using layers to create other windows!");
		s_Instance = this;

		PushOverlay(p_ImGuiMasterOverlay);
	}

	void Application::Run()
	{
		// Execute any GPU initialisation required from these layers
		SKTBD_CORE_INFO("Flushing GPU for post-initialisation");
		Skateboard::GraphicsContext::Context->Flush();

		bool running = true;
		while (running)
		{
			// Release some CPU consumption if the application is not used
			/*if (m_ApplicationPaused)
			{
				Sleep(100);
				continue;
			}*/

			// Update the platform (could be window messages, controller inputs, ...)
			// We will avoid uncessary rendering by quitting immediately if the context is destroyed
			if (!m_Platform.Update())
				break;

			// Get delta time from the platform (the procedure of getting time may differ!)
			TimeManager* timeManager = Platform::GetTimeManager();

			// Generic game loop
			for (Layer* layer : m_LayerStack)
				running &= layer->OnHandleInput(timeManager);
			for (Layer* layer : m_LayerStack)
				running &= layer->OnUpdate(timeManager);

			//TODO MAYBE IMPLEMENT A GLOBAL ENGINE UPDATE LAYER TO HANDLE THESE SORT OF THINGS?
			//void run an update on ECS components that might need to get signal to other systems that their state has changed, like Transforms 
			GlobalSystemsOnUpdate();

			//flushes the dirty data that might have been uploaded to the Buffer during Update to the respective buffers
			//MemoryManager::Update();

			// Prepare internal engine for rendering scene.
			Skateboard::GraphicsContext::Context->BeginFrame();

			// Render all the layers main graphics
			for (Layer* layer : m_LayerStack)
				layer->OnRender();

			// Render all the user interface
			p_ImGuiMasterOverlay->Begin();
			for (Layer* layer : m_LayerStack)
				layer->OnImGuiRender();
			p_ImGuiMasterOverlay->End();

			// Execute the rendering work recorded on the GPU
			Skateboard::GraphicsContext::Context->EndFrame();

			// Present the current back buffer
			Skateboard::GraphicsContext::Context->Present();

			//next frame
			Skateboard::GraphicsContext::Context->NextFrame();
		}

		// When exitting the app, ensure all GPU work has concluded
		SKTBD_CORE_INFO("Exiting App, GPU idle expected..");
		Skateboard::GraphicsContext::Context->WaitUntilIdle();
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatcher dispatcher(e);

		dispatcher.Dispatch<WindowCloseEvent>(BIND_EVENT(Application::OnWindowClose));
		dispatcher.Dispatch<WindowResizeEvent>(BIND_EVENT(Application::OnWindowResize));

		Platform::GetPlatform().OnEvent(e);

		// Go through each layer on the stack and pass the event forward to them
		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin(); )
		{
			(*--it)->OnEvent(e);

			if (e.IsHandled())
				break;
		}
	}

	void Application::GlobalSystemsOnUpdate()
	{
		//flush the data of the transform components to its respective FRAME RESOURCE buffers
		//{
		//	auto view = ECS::GetRegistry().view<TransformComponent>();
		//	view.each([](const auto entity, TransformComponent& TransformC)
		//		{
		//			auto trans = TransformC.Trans.AsMatrix();
		//			//MemoryManager::UploadData(TransformC.BufferId, 0, &trans);
		//		}
		//	);
		//}

		//{
		//	auto view = ECS::GetRegistry().view<MaterialComponent>();
		//	view.each([](const auto entity, MaterialComponent& MaterialC)
		//		{
		//			auto dat = MaterialC.Material.GetData() ;
		//			MaterialData M =
		//			{
		//				dat.m_Albedo,
		//				dat.m_AlbedoMapIndex,
		//				dat.padding,
		//				dat.m_FresnelR0,
		//				dat.m_Metallic,
		//				dat.m_Specular,
		//				dat.m_Roughness
		//			};
		//			
		//			//MemoryManager::UploadData(MaterialC.BufferId, 0, &M);
		//		}
		//	);
		//}

	}

	bool Application::OnWindowClose(WindowCloseEvent& e)
	{
		return false;
	}

	bool Application::OnWindowResize(WindowResizeEvent& e)
	{
		return false;
	}
}