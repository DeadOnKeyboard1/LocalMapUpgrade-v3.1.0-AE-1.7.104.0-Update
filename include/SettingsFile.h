#pragma once
#include <Windows.h>
#include "third_party/SimpleIni.h"
#include <cmath>
#include <cwchar>
#include <cwctype>
#include <filesystem>

namespace settings
{
	class IniFile
	{
	public:
		explicit IniFile(const std::filesystem::path& a_path)
		{
			ini.SetUnicode();
			loaded = ini.LoadFile(a_path.c_str()) >= 0;
		}

		[[nodiscard]] double ReadNumber(const wchar_t* a_section, const wchar_t* a_key, double a_fallback) const
		{
			if (!loaded) {
				return a_fallback;
			}

			const wchar_t* text = ini.GetValue(a_section, a_key);
			if (!text) {
				return a_fallback;
			}

			wchar_t* end = nullptr;
			const double value = std::wcstod(text, &end);
			if (end == text || !std::isfinite(value)) {
				return a_fallback;
			}
			while (std::iswspace(*end)) {
				++end;
			}
			return *end == L'\0' ? value : a_fallback;
		}

	private:
		CSimpleIniW ini;
		bool loaded = false;
	};
	inline double ReadIniNumber(const std::filesystem::path& a_path, const wchar_t* a_section,
		const wchar_t* a_key, double a_fallback)
	{
		return IniFile(a_path).ReadNumber(a_section, a_key, a_fallback);
	}

}
