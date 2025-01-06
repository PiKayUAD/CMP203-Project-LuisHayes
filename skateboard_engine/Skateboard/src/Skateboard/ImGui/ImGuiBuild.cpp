#include "sktbdpch.h"

#if defined(SKTBD_PLATFORM_WINDOWS)
	#include "Skateboard/ImGui/backends/Windows/imgui_impl_win32.cpp"
	#include "Skateboard/ImGui/backends/Windows/imgui_impl_dx12.cpp"
#elif defined(SKTBD_PLATFORM_PLAYSTATION)
	#include "Skateboard/ImGui/backends/Playstation/imgui_impl_ps.cpp"
	#include "Skateboard/ImGui/backends/Playstation/imgui_libfont_ps.cpp"
#endif