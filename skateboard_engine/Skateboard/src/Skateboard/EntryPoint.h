#pragma once
// The goal of this file is to have the engine handle the entry point for the client
// This is mainly because of the different platforms
// We can ignore the errors (if any)

#ifdef SKTBD_PLATFORM_WINDOWS
#ifdef SKTBD_API_D3D12
extern "C" { __declspec(dllexport) extern const UINT D3D12SDKVersion = 614; }
extern "C" { __declspec(dllexport) extern const char* D3D12SDKPath = "..\\packages\\"; }
#endif// SKTBD_API_D3D12

extern Skateboard::Application* Skateboard::CreateApplication(int argc, char** argv);

int main(int argc, char** argv)
{
	Skateboard::Log::Init();
	//Create Platform
	Skateboard::Platform::GetPlatform();

	std::unique_ptr<Skateboard::Application> app(Skateboard::CreateApplication(argc, argv));
	app->Run();
	return 0;
}
#elif defined(SKTBD_PLATFORM_PLAYSTATION)
#include <stdlib.h>
// Ensure that the heap size will be extended when running out of memory on an allocation
unsigned int sceLibcHeapExtendedAlloc = 1;
// Disable the upper limit of the heap area. Alternatively, this could be set to a value such as 1*1024*1024 for an upper limit of 1MB.
// Read: https://p.siedev.net/resources/documents/SDK/7.000/C_and_Cpp_standard_libraries/stdlib.html#malloc
size_t sceLibcHeapSize = SCE_LIBC_HEAP_SIZE_EXTENDED_ALLOC_NO_LIMIT;

// We need to check cloning the project works first time for new clones, project configuration should work
int main(int argc, char** argv)
{
	Skateboard::PlatformProperties props = {};
	props.BackBufferWidth = 1920;
	props.BackBufferHeight = 1080;
	//props.BackBufferWidth = 3840;
	//props.BackBufferHeight = 2160;
	props.Title = L"Playstation - Skateboard Engine";
	Skateboard::Platform::GetPlatform().Init(props);

	std::unique_ptr<Skateboard::Application> app(Skateboard::CreateApplication(argc, argv));
	app->Run();
	return 0;
}
#else
#error Skateboard only supports windows for now!
#endif