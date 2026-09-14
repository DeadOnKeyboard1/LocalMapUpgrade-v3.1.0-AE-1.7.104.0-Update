#include "Settings.h"
#include "SettingsFile.h"
#include "utils/Logger.h"
#include "ExtraMarkersManager.h"
#include "ShaderManager.h"

namespace settings
{
	static std::string iniFileName;
	void Init(const std::string& name)
	{
		iniFileName = name;
		Reload();
	}

	void Reload()
	{
		const bool previousImmersive = mapmenu::localMapShowActorsOnlyWithDetectSpell;
		const bool previousColor = mapmenu::localMapColor;
		const bool previousFog = mapmenu::localMapFogOfWar;
		std::array<wchar_t, 32768> executable{};
		if (!GetModuleFileNameW(nullptr, executable.data(), static_cast<DWORD>(executable.size()))) {
			logger::error("Cannot resolve the game directory for settings");
			return;
		}
		const auto data = std::filesystem::path(executable.data()).parent_path() / "Data";
		const std::array paths{
			data / "MCM/Config/LocalMapUpgrade/settings.ini",
			data / "SKSE/Plugins" / iniFileName,
			data / "MCM/Settings/LocalMapUpgrade.ini"
		};
		// Read partial MCM overrides without relying on Skyrim's INI vtables.
		const auto read = [&](const wchar_t* section, const wchar_t* key, double fallback) {
			for (const auto& path : paths) fallback = ReadIniNumber(path, section, key, fallback);
			return fallback;
		};
		const auto boolean = [&](const wchar_t* key, bool fallback) {
			return read(L"MapMenu", key, fallback ? 1.0 : 0.0) != 0.0;
		};
		using namespace mapmenu;
		localMapColor = boolean(L"bMapLocalColor", true);
		localMapFogOfWar = boolean(L"bMapLocalFogOfWar", true);
		localMapKeyboardPanSpeed = static_cast<float>(std::clamp(read(L"MapMenu", L"fMapLocalKeyboardPanSpeed", 50.0), 0.0, 500.0));
		localMapShowEnemyActors = boolean(L"bMapLocalShowEnemyActors", true);
		localMapShowHostileActors = boolean(L"bMapLocalShowHostileActors", true);
		localMapShowGuardActors = boolean(L"bMapLocalShowGuardActors", true);
		localMapShowDeadActors = boolean(L"bMapLocalShowDeadActors", true);
		localMapShowTeammateActors = boolean(L"bMapLocalShowTeammateActors", true);
		localMapShowNeutralActors = boolean(L"bMapLocalShowNeutralActors", true);
		localMapShowActorsOnlyWithDetectSpell = boolean(L"bImmersiveMode", false);
		debug::logLevel = static_cast<logger::level>(static_cast<int>(std::clamp(read(L"Debug", L"uLogLevel", 2.0), 0.0, 5.0)));
		logger::set_level(debug::logLevel, debug::logLevel);
		if (auto markers = LMU::ExtraMarkersManager::GetSingleton(); markers && previousImmersive != localMapShowActorsOnlyWithDetectSpell) {
			markers->ResetDisplayRadii();
		}
		if (auto shader = LMU::ShaderManager::GetSingleton(); shader &&
			(previousColor != localMapColor || previousFog != localMapFogOfWar)) {
			shader->ApplySettings();
		}
		logger::info("Settings applied (UTF-8 INI): color={}, fog={}, immersive={}, pan={}, enemy={}, hostile={}, guard={}, dead={}, teammate={}, neutral={}",
			localMapColor, localMapFogOfWar, localMapShowActorsOnlyWithDetectSpell, localMapKeyboardPanSpeed,
			localMapShowEnemyActors, localMapShowHostileActors, localMapShowGuardActors,
			localMapShowDeadActors, localMapShowTeammateActors, localMapShowNeutralActors);
	}
}
