#pragma once
#include <Windows.h>
#include "third_party/SimpleIni.h"
#include <cmath>
#include <cwchar>
#include <cwctype>
#include <filesystem>

namespace settings
{
	inline double ReadIniNumber(const std::filesystem::path& path, const wchar_t* section,
		const wchar_t* key, double fallback)
	{
		CSimpleIniW ini;
		ini.SetUnicode();
		if (ini.LoadFile(path.c_str()) < 0) return fallback;
		const wchar_t* text = ini.GetValue(section, key);
		if (!text) return fallback;
		wchar_t* end = nullptr;
		const double value = std::wcstod(text, &end);
		if (end == text || !std::isfinite(value)) return fallback;
		while (std::iswspace(*end)) ++end;
		return *end == L'\0' ? value : fallback;
	}
}
