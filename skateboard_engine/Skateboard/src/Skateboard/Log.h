#pragma once
#include "Core.h"

#ifdef SKTBD_LOG_COMPONENT
	#ifdef SKTBD_PLATFORM_PLAYSTATION
		#define SCE_DBG_LOG_COMPONENT SKTBD_LOG_COMPONENT
	#endif // SKTBD_PLATFORM_PLAYSTATION
#endif // SKTBD_LOG_COMPONENT

#if defined(SKTBD_SHIP) //|| defined(SKTBD_PLATFORM_PLAYSTATION)
	// Core log macros
	#define SKTBD_CORE_CRITICAL(...)
	#define SKTBD_CORE_ERROR(...)
	#define SKTBD_CORE_WARN(...)
	#define SKTBD_CORE_INFO(...)
	#define SKTBD_CORE_TRACE(...)
	
	// Client log macros
	#define SKTBD_APP_CRITICAL(...)
	#define SKTBD_APP_ERROR(...)
	#define SKTBD_APP_WARN(...)
	#define SKTBD_APP_INFO(...)
	#define SKTBD_APP_TRACE(...)

#elif defined(SKTBD_PLATFORM_PLAYSTATION)

#pragma comment(lib, "SceDbg_nosubmission_stub_weak")
#include <libdbg.h>
#include "Flossy.hpp"

//use flossy to convert from python to suitable format

#define GET(...) flossy::format(__VA_ARGS__).c_str()

//#define W(X,...) X
 
#define SCE_DBG_LOG_PREFIX                 //redifine to empty as we are using our own formatting

// Core log macros
#define SKTBD_CORE_CRITICAL(...)	SCE_DBG_LOG_ERROR(GET(__VA_ARGS__));
#define SKTBD_CORE_ERROR(...)		SCE_DBG_LOG_ERROR(GET(__VA_ARGS__));
#define SKTBD_CORE_WARN(...)		SCE_DBG_LOG_WARNING(GET(__VA_ARGS__));
#define SKTBD_CORE_INFO(...)		SCE_DBG_LOG_INFO(GET(__VA_ARGS__));
#define SKTBD_CORE_TRACE(...)		SCE_DBG_LOG_TRACE(GET(__VA_ARGS__));
									
// Client log macros				
#define SKTBD_APP_CRITICAL(...)     SCE_DBG_LOG_ERROR(GET(__VA_ARGS__)); 
#define SKTBD_APP_ERROR(...)		SCE_DBG_LOG_ERROR(GET(__VA_ARGS__));
#define SKTBD_APP_WARN(...)			SCE_DBG_LOG_WARNING(GET(__VA_ARGS__));
#define SKTBD_APP_INFO(...)			SCE_DBG_LOG_INFO(GET(__VA_ARGS__));
#define SKTBD_APP_TRACE(...)		SCE_DBG_LOG_TRACE(GET(__VA_ARGS__));

#else
// This ignores all warnings raised inside External headers
#pragma warning(push)
#pragma warning(disable : 4616)
#pragma warning(disable : 6285)
#pragma warning(disable : 26437)
#pragma warning(disable : 26450)
#pragma warning(disable : 26451)
#pragma warning(disable : 26495)
#pragma warning(disable : 26498)
#pragma warning(disable : 26800)
#pragma warning(disable : 4996)
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#pragma warning(pop)
namespace Skateboard
{
	class Log
	{
	public:
		static void Init();
		inline static std::shared_ptr<spdlog::logger>& GetCoreLogger() { return s_CoreLogger; }
		inline static std::shared_ptr<spdlog::logger>& GetAppLogger() { return s_AppLogger; }


	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_AppLogger;
	};
}

	// Core log macros
	#define SKTBD_CORE_CRITICAL(...)		{::Skateboard::Log::GetCoreLogger()->critical(__VA_ARGS__);	}
	#define SKTBD_CORE_ERROR(...)			{::Skateboard::Log::GetCoreLogger()->error(__VA_ARGS__);	}
	#define SKTBD_CORE_WARN(...)			{::Skateboard::Log::GetCoreLogger()->warn(__VA_ARGS__);		}
	#define SKTBD_CORE_INFO(...)			{::Skateboard::Log::GetCoreLogger()->info(__VA_ARGS__);		}
	#define SKTBD_CORE_TRACE(...)		    {::Skateboard::Log::GetCoreLogger()->trace(__VA_ARGS__);	}
	
	// Client log macros
	#define SKTBD_APP_CRITICAL(...)			{::Skateboard::Log::GetAppLogger()->critical(__VA_ARGS__);	}
	#define SKTBD_APP_ERROR(...)			{::Skateboard::Log::GetAppLogger()->error(__VA_ARGS__);		}
	#define SKTBD_APP_WARN(...)				{::Skateboard::Log::GetAppLogger()->warn(__VA_ARGS__);		}
	#define SKTBD_APP_INFO(...)				{::Skateboard::Log::GetAppLogger()->info(__VA_ARGS__);		}
	#define SKTBD_APP_TRACE(...)			{::Skateboard::Log::GetAppLogger()->trace(__VA_ARGS__);		}
#endif