#pragma once
#include "sktbdpch.h"
// <summary>
// Informs the compiler not to generate a copy constructor and copy assignment operator.
// </summary>
#ifndef DISABLE_COPY
#define DISABLE_COPY(T)\
	    T(const T&) = delete;\
		auto operator=(const T&) noexcept -> T& = delete;
#endif

// <summary>
// Informs the compiler not to generate a move constructor and move assignment operator.
// </summary>
#ifndef DISABLE_MOVE
#define DISABLE_MOVE(T)\
	    T(T&&) = delete;\
		auto operator=(T&&) noexcept -> T& = delete;
#endif

#ifndef ENABLE_MOVE
#define ENABLE_MOVE(T)\
	    T(T&&) = default;\
		auto operator=(T&&) noexcept -> T& = default;
#endif


#ifndef DISBALE_DEFAULT
#define DISABLE_DEFAULT(T)\
		T() = delete;
#endif

#define BIT(x) 1 << x

#ifndef ROUND_UP
#define ROUND_UP(v, powerOf2Alignment) (((v) + (powerOf2Alignment)-1) & ~((powerOf2Alignment)-1))
#endif

#ifndef GENERATE_DEFAULT_CLASS
#define GENERATE_DEFAULT_CLASS(T)\
	T() = default;\
	T(const T&) = default;\
	T(T&&) noexcept = default;\
	auto operator=(const T&) -> T& = default;\
	auto operator=(T&&) noexcept -> T& = default;
#endif

#ifndef DISABLE_COPY_AND_MOVE
#define DISABLE_COPY_AND_MOVE(T) DISABLE_COPY(T) DISABLE_MOVE(T)
#endif

//copy move default
#ifndef DISABLE_C_M_D
#define DISABLE_C_M_D(T) DISABLE_COPY_AND_MOVE(T) DISABLE_DEFAULT(T)
#endif // !DISABLE_C_M_D

#if SKTBD_DYNAMIC_LINK
	#ifdef SKTBD_BUILD_DLL 
		#define SKTBD_API __declspec(dllexport)		
	#else 
		#define SKTBD_API __declspec(dllimport)
	#endif
#else 
	#define SKTBD_API
#endif

namespace Skateboard::GraphicsConstants
{
	extern size_t DEFAULT_RESOURCE_ALIGNMENT;
	extern size_t SMALL_RESOURCE_ALIGNMENT;
	extern size_t MSAA_RESOURCE_ALIGNEMNT;
	extern size_t SMALL_MSAA_RESOURCE_ALIGNMENT;
	extern size_t CONSTANT_BUFFER_ALIGNMENT;
	extern size_t BUFFER_ALIGNMENT;
	extern size_t RAYTRACING_STRUCT_ALIGNMENT;
}

//SKATEBOARD RESULT basically winerror
typedef uint32_t SKTBDR;

//everything.. seems to be in order
#define OKEYDOKEY 0;

//OOPSIE WOOPSIE
constexpr auto OOPS_OUTOFMEMORY = -1;
constexpr auto OOPS_INVALIDARG = -2;
constexpr auto OOPS_INVALIDPTR = -3;
constexpr auto OOPS_INVALIDHANDLE = -4;
constexpr auto OOPS_FAIL = -5;
constexpr auto OOPS_ACCESSDENIED =-6;
constexpr auto OOPS_UNEXPECTED =-7;
constexpr auto OOPS_RESOURCESIZEMISSMATCH =-8;

#if !defined(SKTBD_SHIP)
#define ASSERTIONS_ENABLED 1
#endif

#if ASSERTIONS_ENABLED
// check the expression and fail if it is false 
#define ASSERT(expr, ...) if(expr){}else{ SKTBD_CORE_ERROR("Assertion failed: {0} in {1} at {2}", #expr, __FILE__, __LINE__); __debugbreak(); __VA_ARGS__ }
#else
// evaluates to nothing
#define ASSERT(expr) 
#endif

#define SKTBD_ASSERT(A,...) ASSERT(A)
#define SKTBD_ASSERT_M(A,...) ASSERT(A, SKTBD_APP_ERROR(__VA_ARGS__)) 

#define SKTBD_CORE_ASSERT(A,...) ASSERT(A, SKTBD_CORE_CRITICAL(__VA_ARGS__)) 


// Uncomment this definition to log the CPU idles (warning: may flood console as currently the CPU always
// waits on the GPU -> not enough CPU tasks!
//#define SKTBD_LOG_CPU_IDLE


