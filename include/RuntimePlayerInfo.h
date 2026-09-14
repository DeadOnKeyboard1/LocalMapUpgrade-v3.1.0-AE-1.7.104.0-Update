#pragma once

namespace LMU
{
	inline RE::PlayerCharacter::INFO_RUNTIME_DATA& GetPlayerInfo(RE::PlayerCharacter* player)
	{
		if (REL::Module::get().version() >= REL::Version{ 1, 7, 104, 0 }) {
			// ID 40535 reads/writes the marker at 0x934 and its path at 0x938.
			static_assert(offsetof(RE::PlayerCharacter::INFO_RUNTIME_DATA, playerMapMarker) == 0x54);
			static_assert(offsetof(RE::PlayerCharacter::INFO_RUNTIME_DATA, playerMarkerPath) == 0x58);
			return *reinterpret_cast<RE::PlayerCharacter::INFO_RUNTIME_DATA*>(
				reinterpret_cast<std::uintptr_t>(player) + 0x8E0);
		}
		return player->GetInfoRuntimeData();
	}
}
