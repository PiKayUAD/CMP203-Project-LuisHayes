#pragma once
#include "Skateboard/Renderer/GraphicsContext.h"
#include "Skateboard/Time/TimeManager.h"
#include "Skateboard/Events/Event.h"
#include "Skateboard/User.h"
#include "Skateboard/Input/DeviceManager.h"
#include "sktbdpch.h"

namespace Skateboard
{
	struct PlatformProperties
	{
		std::wstring Title = L"Skateboard Engine";
		uint32_t BackBufferWidth = 1280u, BackBufferHeight = 720u;
		//
		//  BackBufferFormat = DataFormat_R8G8B8A8_UNORM;
		//DataFormat_ DepthStencilBufferFormat = DataFormat_D24_UNORM_S8_UINT;
	};

	class Platform
	{
		DISABLE_COPY_AND_MOVE(Platform)
	public:
		Platform() {};
		virtual ~Platform() {}
		// look at https://refactoring.guru/design-patterns/singleton
		static Platform& GetPlatform();

		static TimeManager* GetTimeManager() { return GetPlatform().m_Timer.get(); }
		static UserManager* GetUserManager() { return GetPlatform().m_Users.get(); }
		static DeviceManager* GetDeviceManager() { return GetPlatform().m_Devices.get(); }
		static GraphicsContext* GetGraphicsContext() { return GetPlatform().p_GraphicsContext.get(); }

		virtual void Init(const PlatformProperties& props = PlatformProperties()) = 0;
		virtual bool Update() { m_Timer->Tick(); m_Users->Update(); m_Devices->Update();  return true; };

		//maybe restrict access to these

		virtual void SetOnEventCallback(std::function<void(Event&)> callback) { EventCallback = callback; };
		static  void PlatformDispatchEvent(Event& e) { Platform::GetPlatform().EventCallback(e); };

		virtual void OnEvent(Event& e) { m_Users->OnEvent(e); m_Devices->OnEvent(e); };

		virtual void InitImGui() = 0;
		virtual void BeginImGuiPass() = 0;
		virtual void EndImGuiPass() = 0;
		virtual void ShutdownImGui() = 0;

		inline static std::optional<User> m_GamePadImGuiControl = std::optional<User>(); // to navigate imgui with a controller // Works only on PS5 but could be extended using new Imgui Navigation features 

		const std::shared_ptr<GraphicsContext>& GetGraphicsContextPtr() const { return p_GraphicsContext; }

	protected:
		std::function<void(Event&)> EventCallback;

		std::shared_ptr<GraphicsContext> p_GraphicsContext;
		std::unique_ptr<TimeManager> m_Timer;
		std::unique_ptr<UserManager> m_Users;
		std::unique_ptr<DeviceManager> m_Devices;
	};
}