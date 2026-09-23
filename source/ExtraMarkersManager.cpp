#include "ExtraMarkersManager.h"
#include "Settings.h"
#include "RE/M/MapMenuMarker.h"

namespace RE
{
	std::int32_t TESObjectREFR_GetInventoryCount(TESObjectREFR* a_object, bool a_useDataHandlerInventory = false,
		bool a_unk03 = false)
	{
		if (!a_object) {
			return 0;
		}
		using func_t = decltype(&TESObjectREFR_GetInventoryCount);
		static REL::Relocation<func_t> func{ REL::VariantID{ 19274, 19700, 0x29F980 } };
		return func(a_object, a_useDataHandlerInventory, a_unk03);
	}

	std::int32_t ExtraDataList_GetDroppedWeapon(ExtraDataList* a_extraList, TESObjectREFRPtr& a_weapon)
	{
		using func_t = decltype(&ExtraDataList_GetDroppedWeapon);
		static REL::Relocation<func_t> func{ REL::VariantID{ 11616, 11762, 0x1266A0 } };
		return a_extraList ? func(a_extraList, a_weapon) : 0;
	}

	std::int32_t ExtraDataList_GetDroppedUtil(ExtraDataList* a_extraList, TESObjectREFRPtr& a_util)
	{
		using func_t = decltype(&ExtraDataList_GetDroppedUtil);
		static REL::Relocation<func_t> func{ REL::VariantID{ 11617, 11763, 0x126870 } };
		return a_extraList ? func(a_extraList, a_util) : 0;
	}

	bool TESObjectREFR_HasAnyDroppedItem(TESObjectREFR* a_ref)
	{
		if (!a_ref) {
			return false;
		}
		if (TESObjectREFR_GetInventoryCount(a_ref) > 0) {
			return true;
		}
		if (a_ref->formType != FormType::ActorCharacter) {
			return false;
		}

		TESObjectREFRPtr droppedWeapon;
		ExtraDataList_GetDroppedWeapon(&a_ref->extraList, droppedWeapon);
		if (droppedWeapon) {
			return true;
		}

		TESObjectREFRPtr droppedUtil;
		ExtraDataList_GetDroppedUtil(&a_ref->extraList, droppedUtil);
		return static_cast<bool>(droppedUtil);
	}

	bool Actor__IsDead(Actor* a_actor, bool a_notEssential = true)
	{
		if (!a_actor) {
			return false;
		}
		auto* state = a_actor->AsActorState();
		if (!state) {
			return false;
		}
		const ACTOR_LIFE_STATE lifeState = state->actorState1.lifeState;
		if (lifeState == ACTOR_LIFE_STATE::kDying ||
			lifeState == ACTOR_LIFE_STATE::kDead ||
			lifeState == ACTOR_LIFE_STATE::kRecycle) {
			return true;
		}
		return !a_notEssential && lifeState == ACTOR_LIFE_STATE::kEssentialDown;
	}

	bool Actor__IsUndead(Actor* a_actor)
	{
		auto* race = a_actor ? a_actor->GetRace() : nullptr;
		if (!race) {
			return false;
		}

		static std::unordered_map<FormID, bool> raceUndeadCache;
		const FormID raceID = race->GetFormID();
		if (const auto it = raceUndeadCache.find(raceID); it != raceUndeadCache.end()) {
			return it->second;
		}

		bool undead = false;
		static constexpr std::array<std::string_view, 4> keywords{
			"ActorTypeDaedra", "ActorTypeDwarven", "NoDetectLife", "ActorTypeUndead"
		};
		for (const auto keyword : keywords) {
			if (race->HasKeywordString(keyword)) {
				undead = true;
				break;
			}
		}
		raceUndeadCache.emplace(raceID, undead);
		return undead;
	}
}

namespace LMU
{
	bool ExtraMarkersManager::IsDetectDeadEffect(const RE::ActiveEffect* a_effect)
	{
		if (!a_effect || !a_effect->effect || !a_effect->effect->baseEffect) {
			return false;
		}

		auto* condition = a_effect->effect->baseEffect->conditions.head;
		while (condition) {
			if (condition->data.functionData.function == RE::FUNCTION_DATA::FunctionID::kGetDead) {
				return true;
			}
			condition = condition->next;
		}
		return false;
	}

	bool ExtraMarkersManager::IsAuraWhisperEffect(const RE::ActiveEffect* a_effect)
	{
		if (!a_effect || !a_effect->spell) {
			return false;
		}
		static constexpr std::array<RE::FormID, 3> auraWhisperIDs{ 0x8AFCC, 0x8AFCD, 0x8AFCE };
		const RE::FormID id = a_effect->spell->GetFormID();
		return std::ranges::find(auraWhisperIDs, id) != auraWhisperIDs.end();
	}

	void ExtraMarkersManager::RefreshDisplayRadii(RE::PlayerCharacter* a_player)
	{
		if (!settings::mapmenu::localMapShowActorsOnlyWithDetectSpell) {
			aliveActorsDisplayRadius = undeadActorsDisplayRadius = deadActorsDisplayRadius =
				std::numeric_limits<float>::max();
			return;
		}

		aliveActorsDisplayRadius = undeadActorsDisplayRadius = deadActorsDisplayRadius = 0.0F;
		if (!a_player) {
			return;
		}

		auto* effects = a_player->GetActiveEffectList();
		if (!effects) {
			return;
		}

		for (auto* activeEffect : *effects) {
			if (!activeEffect || !activeEffect->effect || !activeEffect->effect->baseEffect ||
				activeEffect->flags.any(RE::ActiveEffect::Flag::kInactive) ||
				activeEffect->flags.any(RE::ActiveEffect::Flag::kDispelled)) {
				continue;
			}

			const float radius = static_cast<float>(activeEffect->effect->GetArea()) * feetToUnits;
			if (radius <= 0.0F) {
				continue;
			}

			if (skyrim_cast<RE::DetectLifeEffect*>(activeEffect)) {
				if (IsDetectDeadEffect(activeEffect)) {
					undeadActorsDisplayRadius = std::max(undeadActorsDisplayRadius, radius);
					deadActorsDisplayRadius = std::max(deadActorsDisplayRadius, radius);
				} else {
					aliveActorsDisplayRadius = std::max(aliveActorsDisplayRadius, radius);
				}
			} else if (skyrim_cast<RE::ScriptEffect*>(activeEffect) && IsAuraWhisperEffect(activeEffect)) {
				aliveActorsDisplayRadius = std::max(aliveActorsDisplayRadius, radius);
				undeadActorsDisplayRadius = std::max(undeadActorsDisplayRadius, radius);
			}
		}
	}

	void ExtraMarkersManager::AddExtraMarker(RE::ActorHandle& a_actorHandle, RE::Actor* a_actor,
		RE::BSTArray<RE::MapMenuMarker>& a_mapMarkers)
	{
		if (!a_actor || !a_actorHandle) {
			return;
		}

		RE::MapMenuMarker mapMarker{};
		mapMarker.fullName = nullptr;
		mapMarker.ref = a_actorHandle.native_handle();
		mapMarker.customMarker = a_actor->GetDisplayFullName();
		mapMarker.type = 0;  // kLocation in the engine's local-map marker table
		mapMarker.door = 0;
		mapMarker.index = -1;
		mapMarker.form = nullptr;
		mapMarker.unk30 = 1;
		a_mapMarkers.push_back(mapMarker);
	}

	void ExtraMarkersManager::AddExtraMarkers(RE::LocalMapMenu& a_localMapMenu)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* processLists = RE::ProcessLists::GetSingleton();
		if (!player || !processLists) {
			return;
		}

		RefreshDisplayRadii(player);

		RE::GFxValue extraMarkersData;
		auto& runtimeData = a_localMapMenu.GetRuntimeData();
		if (!runtimeData.mapMovie.GetMember("ExtraMarkerData", &extraMarkersData) || !extraMarkersData.IsArray()) {
			return;
		}
		extraMarkersData.ClearElements();

		auto& mapMarkers = a_localMapMenu.mapMarkers;
		auto& actorHandles = processLists->highActorHandles;
		auto& enemyHandles = player->GetInfoRuntimeData().actorsToDisplayOnTheHUDArray;

		std::unordered_set<std::uint32_t> enemies;
		enemies.reserve(enemyHandles.size());
		for (const auto& enemyHandle : enemyHandles) {
			if (enemyHandle) {
				enemies.insert(enemyHandle.native_handle());
			}
		}

		const RE::NiPoint3 playerPosition = player->GetPosition();
		for (auto& actorHandle : actorHandles) {
			auto actorPtr = actorHandle.get();
			auto* actor = actorPtr.get();
			if (!actor || actor == player) {
				continue;
			}

			const float distance = actor->GetPosition().GetDistance(playerPosition);
			if (RE::Actor__IsDead(actor)) {
				if (distance <= deadActorsDisplayRadius && settings::mapmenu::localMapShowDeadActors &&
					RE::TESObjectREFR_HasAnyDroppedItem(actor)) {
					AddExtraMarker(actorHandle, actor, mapMarkers);
					extraMarkersData.PushBack(ExtraMarker::Type::kDead);
				}
				continue;
			}

			const bool isUndead = RE::Actor__IsUndead(actor);
			if ((!isUndead && distance > aliveActorsDisplayRadius) ||
				(isUndead && distance > undeadActorsDisplayRadius)) {
				continue;
			}

			if (enemies.contains(actorHandle.native_handle())) {
				if (settings::mapmenu::localMapShowEnemyActors) {
					AddExtraMarker(actorHandle, actor, mapMarkers);
					extraMarkersData.PushBack(ExtraMarker::Type::kEnemy);
				}
			} else if (actor->IsPlayerTeammate()) {
				if (settings::mapmenu::localMapShowTeammateActors) {
					AddExtraMarker(actorHandle, actor, mapMarkers);
					extraMarkersData.PushBack(ExtraMarker::Type::kTeammate);
				}
			} else if (actor->IsHostileToActor(player)) {
				if (settings::mapmenu::localMapShowHostileActors) {
					AddExtraMarker(actorHandle, actor, mapMarkers);
					extraMarkersData.PushBack(ExtraMarker::Type::kHostile);
				}
			} else if (actor->IsGuard()) {
				if (settings::mapmenu::localMapShowGuardActors) {
					AddExtraMarker(actorHandle, actor, mapMarkers);
					extraMarkersData.PushBack(ExtraMarker::Type::kGuard);
				}
			} else if (settings::mapmenu::localMapShowNeutralActors) {
				AddExtraMarker(actorHandle, actor, mapMarkers);
				extraMarkersData.PushBack(ExtraMarker::Type::kNeutral);
			}
		}
	}
	void ExtraMarkersManager::PostCreateMarkers(RE::GFxValue& a_iconDisplay)
	{
		a_iconDisplay.Invoke("PostCreateMarkers");
	}

}
