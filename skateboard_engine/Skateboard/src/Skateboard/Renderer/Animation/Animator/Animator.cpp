#include "sktbdpch.h"
#include "Animator.h"
#include "Skateboard/Renderer/Renderer.h"

//#include "Platform/DirectX12/Animator.h"
#include "Platform/AGC/Animation/AGCAnimator.h"

#define SKTBD_LOG_COMPONENT "ANIMATOR"
#include "Skateboard/Log.h"

namespace Skateboard
{
	Animator* Animator::Create()
	{

#if defined SKTBD_PLATFORM_WINDOWS
	//		return new D3DAnimator();
			return nullptr;
#elif defined SKTBD_PLATFORM_PLAYSTATION
		case RendererAPI_::RendererAPI_AGC:
			return new AGCAnimator();
#endif
	}

	Animator* Animator::Singleton()
	{
		static std::unique_ptr<Animator> singleton(Create());
		return singleton.get();
	}
}


