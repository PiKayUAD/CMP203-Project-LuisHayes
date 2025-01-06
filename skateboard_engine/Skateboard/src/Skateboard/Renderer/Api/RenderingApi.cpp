#include "sktbdpch.h"
#include "RenderingApi.h"
#include "Skateboard/Renderer/GraphicsContext.h"
#include "Skateboard/Scene/Scene.h"

namespace Skateboard
{
	void RenderingApi::WaitUntilIdle()
	{
		Skateboard::GraphicsContext::Context->WaitUntilIdle();
	}

	void RenderingApi::Flush()
	{
		Skateboard::GraphicsContext::Context->Flush();
	}

	//void RenderingApi::BeginScene(Scene* pScene)
	//{
	//	p_ActiveScene = pScene;
	//}

	//void RenderingApi::EndScene()
	//{
	//	p_ActiveScene = nullptr;

	//	// When ending a scene, if the scene rendered to different frame buffers we 
	//	// need to set the back buffer to be the current render target again
	//	Skateboard::GraphicsContext::Context->SetRenderTargetToBackBuffer();
	//}
}
