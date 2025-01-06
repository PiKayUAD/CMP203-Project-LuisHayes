#pragma once
// AGCF for AGC Framework since agc.h is taken T.T

// Enable AGC asserts
#if !defined(SKTBD_SHIP) && !defined(SCE_AGC_DEBUG)
#define SCE_AGC_DEBUG
#endif
// Enable warnings on _NODISCARD functions
#define SCE_AGC_CHECK_ERROR_CODES

#include <agc.h>
#include <psr/psr.h>

#include <mat.h>
#include <video_out.h>
#include <agc/gnmp/extras/dataformats.h>
#include <shader.h>

#ifndef ROUND_UP
#define ROUND_UP(v, powerOf2Alignment) (((v) + (powerOf2Alignment)-1) & ~((powerOf2Alignment)-1))
#endif

namespace Skateboard
{
	class AGCGraphicsContext;
	extern AGCGraphicsContext* gAGCContext;

	inline std::string wstring2string(const wchar_t* wstr)
	{
		std::wstring a;
		std::string b;
		a.assign(wstr);
		b.resize(a.size());
		std::wcstombs(&b[0], a.c_str(), a.size());
		return b;
	}

	inline std::string SanitizeFilePath(const char* path)
	{
		std::string ret("/app0/");
		ret.append(path);
		return ret;
	}

	inline std::string SanitizeFilePath(const wchar_t* path)
	{
		std::string ret("/app0/");
		std::string p = wstring2string(path);
		if(p.substr(0, 6) !=ret)
			p=ret.append(p);
		return p;
	}
}