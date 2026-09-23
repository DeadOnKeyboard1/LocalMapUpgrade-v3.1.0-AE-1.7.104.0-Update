#pragma once

#include "Settings.h"

namespace LMU
{
	struct ExtraMarker
	{
		enum Type
		{
			kEnemy,
			kHostile,
			kGuard,
			kDead,
			kTeammate,
			kNeutral,
			kTotal
		};
	};

	class ExtraMarkersManager
	{
	public:
		static constexpr inline std::string_view extensionPath = "_level0.WorldMap.LocalMapMenu.IconDisplayExtension";
		static constexpr inline float feetToUnits = 21.3333333F;

		static void InitSingleton()
		{
			static ExtraMarkersManager instance;
			singleton = &instance;
		}

		static ExtraMarkersManager* GetSingleton() { return singleton; }

		void AddExtraMarkers(RE::LocalMapMenu& a_localMapMenu);
		void PostCreateMarkers(RE::GFxValue& a_iconDisplay);

		void ResetDisplayRadii()
		{
			const float radius = settings::mapmenu::localMapShowActorsOnlyWithDetectSpell ?
				0.0F : std::numeric_limits<float>::max();
			aliveActorsDisplayRadius = radius;
			undeadActorsDisplayRadius = radius;
			deadActorsDisplayRadius = radius;
		}

	private:
		static void AddExtraMarker(RE::ActorHandle& a_actorHandle, RE::Actor* a_actor,
			RE::BSTArray<RE::MapMenuMarker>& a_mapMarkers);
		void RefreshDisplayRadii(RE::PlayerCharacter* a_player);
		static bool IsDetectDeadEffect(const RE::ActiveEffect* a_effect);
		static bool IsAuraWhisperEffect(const RE::ActiveEffect* a_effect);

		static inline ExtraMarkersManager* singleton = nullptr;

		float aliveActorsDisplayRadius = settings::mapmenu::localMapShowActorsOnlyWithDetectSpell ?
			0.0F : std::numeric_limits<float>::max();
		float undeadActorsDisplayRadius = settings::mapmenu::localMapShowActorsOnlyWithDetectSpell ?
			0.0F : std::numeric_limits<float>::max();
		float deadActorsDisplayRadius = settings::mapmenu::localMapShowActorsOnlyWithDetectSpell ?
			0.0F : std::numeric_limits<float>::max();
	};
}
