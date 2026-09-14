#include "../include/SettingsFile.h"
#include "../include/MarkerInput.h"
#include <cassert>
#include <iostream>

int wmain(int argc, wchar_t** argv)
{
	assert(argc == 3 || argc == 4);
	const std::filesystem::path defaults = std::filesystem::absolute(argv[1]);
	const std::filesystem::path overrides = std::filesystem::absolute(argv[2]);
	const auto read = [&](const wchar_t* section, const wchar_t* key, double fallback) {
		return settings::ReadIniNumber(overrides, section, key,
			settings::ReadIniNumber(defaults, section, key, fallback));
	};
	for (const auto key : { L"bMapLocalColor", L"bMapLocalFogOfWar", L"bMapLocalShowEnemyActors",
		L"bMapLocalShowHostileActors", L"bMapLocalShowGuardActors", L"bMapLocalShowDeadActors",
		L"bMapLocalShowTeammateActors", L"bMapLocalShowNeutralActors" }) {
		assert(settings::ReadIniNumber(defaults, L"MapMenu", key, -1) == 1);
		assert(read(L"MapMenu", key, 1) == 0);
		if (argc == 4) assert(settings::ReadIniNumber(argv[3], L"MapMenu", key, 1) == 0);
	}
	assert(read(L"MapMenu", L"fMapLocalKeyboardPanSpeed", 50) == 125.5);
	assert(read(L"MapMenu", L"bImmersiveMode", 0) == 1);
	assert(read(L"Debug", L"uLogLevel", 2) == 4);
	assert(read(L"Invalid", L"missing", 42) == 42);
	assert(read(L"Invalid", L"bad", 42) == 42);
	assert(read(L"Invalid", L"notfinite", 42) == 42);
	assert(read(L"Invalid", L"suffix", 42) == 42);
	assert(settings::ReadIniNumber(defaults.parent_path() / L"missing-file.ini", L"MapMenu", L"bMapLocalColor", 1) == 1);
	// Opening transitions, release-only events and repeated presses must never place.
	static_assert(!LMU::ShouldPlaceMarker(false, true, true, true, 1, 0));
	static_assert(!LMU::ShouldPlaceMarker(true, false, true, true, 1, 0));
	static_assert(!LMU::ShouldPlaceMarker(true, true, true, true, 0, 0));
	static_assert(!LMU::ShouldPlaceMarker(true, true, true, true, 0, 0.2F));
	static_assert(!LMU::ShouldPlaceMarker(true, true, true, true, 1, 0.2F));
	static_assert(!LMU::ShouldPlaceMarker(true, true, false, true, 1, 0));
	static_assert(!LMU::ShouldPlaceMarker(true, true, true, false, 1, 0));
	static_assert(LMU::ShouldPlaceMarker(true, true, true, true, 1, 0));
	std::cout << "PASS: all 11 INI overrides, whitespace, invalid/missing values, marker opening/release/held/bounds/fresh press.\n";
}
