#pragma once

#include <stdio.h>
#include <memory>
#include <utility>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <filesystem>

#include <array>
#include <vector>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <string>
#include <stack>

#include <functional>
#include <optional>
#include <mutex>
#include <future>
#include <stddef.h>
#include <variant>
#include <any>

#include "imgui/imgui.h"

#include <Flossy.hpp> // python style string formatting
#include "Skateboard/Core.h"

#ifdef SKTBD_PLATFORM_WINDOWS
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif // !WIN32_LEAN_AND_MEAN
	#include <Windows.h>
	#include <windowsx.h>
#elif defined(SKTBD_PLATFORM_PLAYSTATION)
	#include <kernel.h>
#endif // SKTBD_PLATFORM_WINDOWS
