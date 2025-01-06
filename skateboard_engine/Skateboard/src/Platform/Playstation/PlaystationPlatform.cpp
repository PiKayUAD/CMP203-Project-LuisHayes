#include <sktbdpch.h>
#include "PlaystationPlatform.h"
#include "Skateboard/Input.h"
#include "Platform/Playstation/PlaystationUserManager.h"
#include "Platform/Playstation/PlaystationDeviceManager.h"
#include "Platform/AGC/TimeManager/AGCTimeManager.h"
#include "Platform/AGC/AGCGraphicsContext.h"
#include "Skateboard/ImGui/backends/Playstation/imgui_impl_ps.h"


// Link the user service function so we can use the user service library
#pragma comment(lib,"libSceUserService_stub_weak.a")
// Link the sce pad library so we can use the pad library
#pragma comment(lib,"libScePad_stub_weak.a")
// Link the sce parser library 
//#pragma comment(lib,"libScePackParser.a")

namespace Skateboard
{
	Skateboard::PlaystationPlatform::PlaystationPlatform() :
			m_ClockFrequency(0.f)
		,	m_UIScale(2.f)
	{
	}

	PlaystationPlatform::~PlaystationPlatform()
	{
#ifndef SKTBD_SHIP
		sceMatUninitialize();
#endif
		//// Clean up the user service library.
		//if (sceUserServiceTerminate() == SCE_USER_SERVICE_ERROR_NOT_INITIALIZED)
		//{
		//	printf("User service library was not been initialised!");
		//}
	}

	void PlaystationPlatform::Init(const PlatformProperties& props)
	{
#ifndef SKTBD_SHIP
		//sceMatInitialize(SCEMAT_INIT_DEFAULT);
		sceMatInitialize(SCEMAT_INIT_DEFAULT);
#endif
		//super lazy ddgey fix
		SetOnEventCallback(BIND_EVENT(PlaystationPlatform::OnEvent));

		//initialise device libraries
		m_Devices = std::make_unique<PlaystationDeviceManager>();
		m_Devices->Init();

		Input::RegisterDeviceManager(m_Devices.get());

		//Login Users
		m_Users = std::make_unique<PlaystationUserManager>();
		m_Users->Init();

		// Initialise the time manager
		m_Timer = std::make_unique<AGCTimeManager>();
		

		// Initialise the game pad manager
		//m_GamePadManager = std::make_unique<PlaystationGamePadManager>();
		//
		//if (m_GamePadManager->Init(*m_Users.get()) < SCE_OK)
		//{
		//	printf("Failed to initialise scePad library!");
		//}
		
		// Create the AGC graphics context
		p_GraphicsContext = std::make_shared<AGCGraphicsContext>(props);
	
		m_UIScale = static_cast<float>(gAGCContext->GetClientWidth()) / 1920.f;
		
	}
	
	bool PlaystationPlatform::Update()
	{
		Platform::Update();

#ifndef SKTBD_SHIP
		sceMatNewFrame();
#endif
		//m_Devices->Update()

		// Read the game pads for input updates
		//m_GamePadManager->QueryControllerInput();

		return true;
	}

	void PlaystationPlatform::OnEvent(Event& e)
	{
		Platform::OnEvent(e);
	}

	void PlaystationPlatform::InitImGui()
	{
		ImGui::CreateContext();
		//const ImGui_PS::AllocateFunc imguiAllocateFunc = [](void* allocator, size_t size, size_t alignment) {
		//	return static_cast<TemplatedGPUMemoryPoolAllocator*>(allocator)->AlignedByteAllocate(size);
		//};
		//const ImGui_PS::ReleaseFunc imguiReleaseFunc = [](void* allocator, void* ptr) {
		//	static_cast<TemplatedGPUMemoryPoolAllocator*>(allocator)->AlignedByteFree(ptr);
		//};
		ImGui_PS::initialize(1, m_UIScale);

		// Apply degamma to the default theme for gamma-correction enabled render target. (from samples)
		ImGuiStyle style, styleWithGamma;
		ImGui::StyleColorsDark(&style);
		styleWithGamma = style;
		const auto degamma = [](const ImVec4& color) {
			const auto sRGB_degamma_s = [](float value) {
				if (value <= 0.04045f)
					return value / 12.92f;
				return std::pow((value + 0.055f) / 1.055f, 2.4f);
			};
			return ImVec4(sRGB_degamma_s(color.x),
				sRGB_degamma_s(color.y),
				sRGB_degamma_s(color.z),
				color.w);
		};
		for (int i = 0; i < ImGuiCol_COUNT; ++i) {
			styleWithGamma.Colors[i] = degamma(styleWithGamma.Colors[i]);
		}
		ImGui::GetStyle() = styleWithGamma;
	}

	void PlaystationPlatform::BeginImGuiPass()
	{
		// ??
		SetupImGuiViewport(&gAGCContext->GetRenderTarget());

		static ImVec2 g_lastMousePosition = ImVec2(
			float(gAGCContext->GetClientWidth() / 2) / m_UIScale,
			float(gAGCContext->GetClientHeight() / 2) / m_UIScale);

		//g_controller.update();
		//InputState inputState = {};
		//g_controller.getState(&inputState);

		//ImGui_PS::ControlData controlData;
		//ImGui_PS::translate(g_GUIMode ? &inputState.rawData : nullptr,
		//	g_GUIMode && g_controller.isMouseAvailable() ? ImGui_PS::PadUsage_Navigation : ImGui_PS::PadUsage_MouseEmulation,
		//	g_GUIMode && g_controller.isMouseAvailable() ? &inputState.mouseData : nullptr,
		//	g_lastMousePosition, &controlData);

		//Controll Imgui with Controller-------------------------

		//bool g_GUIMode = false;
		ImGui_PS::ControlData controlData = {};
		size_t datasize;

		if (m_GamePadImGuiControl.has_value())
		{
			ImGui_PS::translate(static_cast<ScePadData*>(m_Devices->GetRawData(m_GamePadImGuiControl.value().id, DeviceType::SKTB_GamePad, datasize)), // if gui controll is enabled clamp to 4 and use respective gamepad 
				ImGui_PS::PadUsage_Navigation,
				nullptr,
				g_lastMousePosition, &controlData);
		}

		ImGui_PS::newFrame(
			uint32_t(gAGCContext->GetClientWidth()),
			uint32_t(gAGCContext->GetClientHeight()),
			controlData);
		ImGui::NewFrame();

		g_lastMousePosition = ImGui::GetIO().MousePos;
	}

	void PlaystationPlatform::EndImGuiPass()
	{
		ImGui::Render();
		ImGui_PS::renderDrawData(gAGCContext->GetDrawCommandBuffer(), ImGui::GetDrawData());
	}

	void PlaystationPlatform::ShutdownImGui()
	{
		ImGui_PS::shutdown();
		ImGui::DestroyContext();
	}

	void PlaystationPlatform::SetupImGuiViewport(const sce::Agc::CxRenderTarget* renderTarget)
	{
		struct AdditionalCxRegisters {
			sce::Agc::CxRenderTarget renderTarget;
			sce::Agc::CxRenderTargetMask renderTargetMask;
			sce::Agc::Core::CxScreenViewport screenViewport;
			sce::Agc::CxScreenScissor screenScissor;
		};
		struct CxRegistersToDisableDepthStencil {
			sce::Agc::CxDepthStencilControl depthStencilControl;
			sce::Agc::CxDbRenderControl dbRenderControl;
		};

		sce::Agc::DrawCommandBuffer& dcb = gAGCContext->GetDrawCommandBuffer();
		sce::Agc::TwoSidedAllocator& mem = dcb;

		AdditionalCxRegisters* const addCxRegs =
			(AdditionalCxRegisters*)mem.allocateTopDown(
				sizeof(AdditionalCxRegisters),
				alignof(sce::Agc::CxRegister));
		const uint32_t width = renderTarget->getWidth();
		const uint32_t height = renderTarget->getHeight();

		addCxRegs->renderTarget = *renderTarget;
		addCxRegs->renderTarget.setSlot(0);
		addCxRegs->renderTargetMask.init().setMask(0, 0xF);
		sce::Agc::Core::setupScreenViewport(&addCxRegs->screenViewport, 0, 0, width, height, 0.5f, 0.5f);
		addCxRegs->screenScissor.init().setLeft(0).setTop(0).setRight(int32_t(width)).setBottom(int32_t(height));

		dcb.setCxRegistersIndirect(
			(sce::Agc::CxRegister*)addCxRegs,
			sizeof(AdditionalCxRegisters) / sizeof(sce::Agc::CxRegister));

		CxRegistersToDisableDepthStencil* const cxRegsDisableDS =
			(CxRegistersToDisableDepthStencil*)mem.allocateTopDown(
				sizeof(CxRegistersToDisableDepthStencil),
				alignof(sce::Agc::CxRegister));
		cxRegsDisableDS->depthStencilControl.init();
		cxRegsDisableDS->dbRenderControl.init();
		dcb.setCxRegistersIndirect(
			(sce::Agc::CxRegister*)cxRegsDisableDS,
			sizeof(CxRegistersToDisableDepthStencil) / sizeof(sce::Agc::CxRegister));
	}

}