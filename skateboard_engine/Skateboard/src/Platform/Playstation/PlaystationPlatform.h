#pragma once
#include "Platform/AGC/AGCF.h"
#include "Skateboard/Platform.h"
#include <scebase_common.h>

namespace Skateboard
{
	class PlaystationPlatform final : public Platform
	{
	public:
		DISABLE_COPY_AND_MOVE(PlaystationPlatform);

		PlaystationPlatform();
		virtual ~PlaystationPlatform() final override;

		void Init(const PlatformProperties& props) final override;

		// Bind main event call back function
		//void SetOnEventCallback(std::function<void(Event&)> callback) final override;

		// Update the system app
		virtual bool Update() final override;
		virtual void OnEvent(Event& e) final override;

		virtual void InitImGui() final override;
		virtual void BeginImGuiPass() final override;
		virtual void EndImGuiPass() final override;
		virtual void ShutdownImGui() final override;

	private:

		void SetupImGuiViewport(const sce::Agc::CxRenderTarget* renderTarget);

	private:

		struct PlatformData
		{
			std::string Title;
			uint32_t BackBufferWidth, BackBufferHeight;
			bool VSync;
			bool Fullscreen;
			bool Windowed;
			bool IsResizing;
			bool IsClosing;
		};
		
		PlatformData m_PlatformData;

		float m_ClockFrequency;
		float m_UIScale;
	};
}