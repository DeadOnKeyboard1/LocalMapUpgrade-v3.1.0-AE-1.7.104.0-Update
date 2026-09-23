#include "PlayerSetMarkerManager.h"
#include "LocalMapAccess.h"

#undef MessageBox

namespace RE
{
	void PlayerCharacter__SetMarkerTeleportData(PlayerCharacter* a_player, TESObjectREFR* a_marker,
		TeleportPath* a_teleportPath, bool a_ignoreLocks)
	{
		if (!a_player || !a_marker || !a_teleportPath) {
			return;
		}
		using func_t = decltype(&PlayerCharacter__SetMarkerTeleportData);
		static REL::Relocation<func_t> func{ REL::VariantID(39441, 40517, 0x6C4D30) };
		func(a_player, a_marker, a_teleportPath, a_ignoreLocks);
	}

	void PlayerCharacter__SetPlayerMapMarker(PlayerCharacter* a_player, const NiPoint3& a_position,
		const TESForm* a_worldOrCell)
	{
		if (!a_player || !a_worldOrCell) {
			return;
		}
		using func_t = decltype(&PlayerCharacter__SetPlayerMapMarker);
		static REL::Relocation<func_t> func{ REL::VariantID(39458, 40535, 0x6C74D0) };
		func(a_player, a_position, a_worldOrCell);
	}

	void PlayerCharacter__RemovePlayerMapMarker(PlayerCharacter* a_player)
	{
		if (!a_player) {
			return;
		}
		using func_t = decltype(&PlayerCharacter__RemovePlayerMapMarker);
		static REL::Relocation<func_t> func{ REL::VariantID(39459, 40536, 0x6C7630) };
		func(a_player);
	}

	bool NiCamera__WindowPointToRay(NiCamera* a_camera, std::int32_t a_x, std::int32_t a_y,
		NiPoint3& a_origin, NiPoint3& a_dir, float a_windowWidth, float a_windowHeight)
	{
		if (!a_camera || a_windowWidth <= 0.0F || a_windowHeight <= 0.0F) {
			return false;
		}
		using func_t = bool (*)(NiCamera*, std::int32_t, std::int32_t, NiPoint3&, NiPoint3&, float, float);
		static REL::Relocation<func_t> func{ RELOCATION_ID(69263, 70630) };
		return func(a_camera, a_x, a_y, a_origin, a_dir, a_windowWidth, a_windowHeight);
	}

	std::uint32_t UI__OpenMessageBox(const BSString& a_title, const BSTSmartPointer<IMessageBoxCallback>& a_callback,
		std::uint8_t a_arg3, std::uint32_t a_arg4, std::int32_t a_arg5, const BSTArray<BSString>& a_options)
	{
		using func_t = decltype(&UI__OpenMessageBox);
		static REL::Relocation<func_t> func{ REL::ID(442726) };
		return func(a_title, a_callback, a_arg3, a_arg4, a_arg5, a_options);
	}

	RefHandle AddPlayerMapMarkerToMap(BSTArray<MapMenuMarker>& a_mapMarkers)
	{
		using func_t = RefHandle& (*)(RefHandle&, BSTArray<MapMenuMarker>&);
		static REL::Relocation<func_t> func{ REL::VariantID(52186, 53078, 0x913AB0) };
		RefHandle refHandle;
		func(refHandle, a_mapMarkers);
		return refHandle;
	}
}

namespace LMU
{
	[[nodiscard]] static bool HasPickResult(const RE::NiPick& a_pick)
	{
		return a_pick.pickResults.resultsCount > 0 && a_pick.pickResults[0] != nullptr;
	}

	bool PickObjectInNode(RE::NiPick* a_pick, RE::NiNode* a_node, const RE::NiPoint3& a_rayOrigin,
		const RE::NiPoint3& a_rayDir)
	{
		if (!a_pick || !a_node) {
			return false;
		}

		// NiPointer's raw-pointer constructor is explicit in CommonLib 9.x.
		a_pick->root.reset(a_node);
		if (a_pick->PickObjects(a_rayOrigin, a_rayDir) && HasPickResult(*a_pick)) {
			return true;
		}

		for (auto& child : a_node->GetChildren()) {
			if (!child) {
				continue;
			}
			if (auto* node = child->AsNode(); node && PickObjectInNode(a_pick, node, a_rayOrigin, a_rayDir)) {
				return true;
			}
		}
		return false;
	}

	bool PickObjectInCell(RE::NiPick* a_pick, RE::TESObjectCELL* a_cell, const RE::NiPoint3& a_rayOrigin,
		const RE::NiPoint3& a_rayDir)
	{
		if (!a_pick || !a_cell || !a_cell->IsAttached()) {
			return false;
		}

		auto& runtimeData = a_cell->GetRuntimeData();
		if (!runtimeData.loadedData || !runtimeData.loadedData->cell3D) {
			return false;
		}
		return PickObjectInNode(a_pick, runtimeData.loadedData->cell3D.get(), a_rayOrigin, a_rayDir);
	}

	bool GetRayCollisionPosition(const RE::NiPoint3& a_rayOrigin, const RE::NiPoint3& a_rayDir,
		RE::NiPoint3& a_rayCollision)
	{
		auto pick = RE::NiPick::Create();
		if (!pick) {
			SKSE::log::warn("Cannot place local marker: NiPick allocation failed");
			return false;
		}
		pick->observeAppCullFlag = true;
		pick->pickType = RE::NiPick::PickType::FIND_FIRST;

		auto* tes = RE::TES::GetSingleton();
		if (!tes) {
			return false;
		}

		auto takeHit = [&]() {
			if (!HasPickResult(*pick)) {
				return false;
			}
			a_rayCollision = pick->pickResults[0]->intersect;
			return true;
		};

		if (auto* interiorCell = tes->interiorCell) {
			if (PickObjectInCell(pick.get(), interiorCell, a_rayOrigin, a_rayDir) && takeHit()) {
				return true;
			}
		} else {
			auto* worldSpace = tes->GetRuntimeData2().worldSpace;
			if (worldSpace) {
				if (auto* skyCell = worldSpace->GetSkyCell();
					skyCell && PickObjectInCell(pick.get(), skyCell, a_rayOrigin, a_rayDir) && takeHit()) {
					return true;
				}

				if (tes->gridCells) {
					const int length = tes->gridCells->length;
					for (int x = 0; x < length; ++x) {
						for (int y = 0; y < length; ++y) {
							if (auto* cell = tes->gridCells->GetCell(x, y);
								cell && PickObjectInCell(pick.get(), cell, a_rayOrigin, a_rayDir) && takeHit()) {
								return true;
							}
						}
					}
				}
			}
		}

		if (tes->lodLandRoot) {
			for (auto& child : tes->lodLandRoot->GetChildren()) {
				if (!child) {
					continue;
				}
				pick->root = child;
				if (pick->PickObjects(a_rayOrigin, a_rayDir) && takeHit()) {
					return true;
				}
			}
		}
		return false;
	}

	void PlaceMarkerImmediate(RE::LocalMapMenu* a_localMapMenu, float a_wndPointX, float a_wndPointY)
	{
		if (!a_localMapMenu) {
			return;
		}

		auto* localMapState = a_localMapMenu->localCullingProcess.GetLocalMapCamera();
		auto* localMapCamera = localMapState ? localMapState->camera.get() : nullptr;
		const auto& topLeft = GetLocalMapTopLeft(*a_localMapMenu);
		const auto& bottomRight = GetLocalMapBottomRight(*a_localMapMenu);
		const float localMapViewWidth = bottomRight.x - topLeft.x;
		const float localMapViewHeight = bottomRight.y - topLeft.y;
		if (!localMapCamera || localMapViewWidth <= 0.0F || localMapViewHeight <= 0.0F) {
			SKSE::log::warn("Cannot place local marker: camera or map dimensions unavailable");
			return;
		}

		const float wndPointX = a_wndPointX - topLeft.x;
		const float wndPointY = a_wndPointY - topLeft.y;
		RE::NiPoint3 rayOrigin;
		RE::NiPoint3 rayDir;
		if (!RE::NiCamera__WindowPointToRay(localMapCamera, static_cast<std::int32_t>(wndPointX),
			static_cast<std::int32_t>(wndPointY), rayOrigin, rayDir, localMapViewWidth, localMapViewHeight)) {
			SKSE::log::warn("Cannot place local marker: cursor could not be projected onto the map");
			return;
		}

		RE::NiPoint3 markerPos;
		if (!GetRayCollisionPosition(rayOrigin, rayDir, markerPos)) {
			SKSE::log::warn("Cannot place local marker: no map geometry under the cursor");
			return;
		}

		auto* player = RE::PlayerCharacter::GetSingleton();
		auto* tes = RE::TES::GetSingleton();
		if (!player || !tes) {
			return;
		}
		const RE::TESForm* worldOrCell = tes->interiorCell ?
			static_cast<const RE::TESForm*>(tes->interiorCell) : static_cast<const RE::TESForm*>(player->GetWorldspace());
		if (!worldOrCell) {
			SKSE::log::warn("Cannot place local marker: no current cell/worldspace");
			return;
		}

		RE::PlayerCharacter__SetPlayerMapMarker(player, markerPos, worldOrCell);
		auto& playerInfo = player->GetInfoRuntimeData();
		const RE::ObjectRefHandle playerMapMarker = playerInfo.playerMapMarker;
		auto* teleportPath = playerInfo.playerMarkerPath;
		SKSE::log::info("Local marker placed: handle={:08X}, position=({:.1f}, {:.1f}, {:.1f})",
			playerMapMarker.native_handle(), markerPos.x, markerPos.y, markerPos.z);

		if (!playerMapMarker || !teleportPath) {
			return;
		}
		auto markerPtr = playerMapMarker.get();
		if (!markerPtr) {
			return;
		}
		RE::PlayerCharacter__SetMarkerTeleportData(player, markerPtr.get(), teleportPath, true);
		RE::AddPlayerMapMarkerToMap(a_localMapMenu->mapMarkers);

		if (auto* ui = RE::UI::GetSingleton()) {
			if (auto mapMenu = ui->GetMenu<RE::MapMenu>(); mapMenu) {
				if (auto* data2 = mapMenu->GetRuntimeData2()) {
					RE::AddPlayerMapMarkerToMap(data2->mapMarkers);
				}
			}
		}
	}

	PlayerSetMarkerManager::MessageBox::MessageBox()
	{
		auto getText = [](const char* a_name, const char* a_fallback) -> const char* {
			if (auto* collection = RE::GameSettingCollection::GetSingleton()) {
				if (auto* setting = collection->GetSetting(a_name); setting && setting->data.s) {
					return setting->data.s;
				}
			}
			return a_fallback;
		};
		title = getText("sMoveMarkerQuestion", "Move the marker?");
		options.push_back(getText("sMoveMarker", "Move"));
		options.push_back(getText("sLeaveMarker", "Leave"));
		options.push_back(getText("sRemoveMarker", "Remove"));
	}

	void PlayerSetMarkerManager::MessageBox::Callback::Run(std::uint8_t a_optionIndex)
	{
		auto* player = RE::PlayerCharacter::GetSingleton();
		switch (a_optionIndex) {
		case 0:  // Move
			// Reacquire the current MapMenu at callback time instead of retaining a
			// LocalMapMenu pointer across the asynchronous message box.
			if (auto* ui = RE::UI::GetSingleton()) {
				if (auto mapMenu = ui->GetMenu<RE::MapMenu>(); mapMenu) {
					if (auto* runtime = mapMenu->GetRuntimeData()) {
						PlaceMarkerImmediate(std::addressof(runtime->localMapMenu), wndPointX, wndPointY);
					}
				}
			}
			break;
		case 2:  // Remove
			if (player) {
				RE::PlayerCharacter__RemovePlayerMapMarker(player);
			}
			break;
		default:  // Leave
			break;
		}
		ClearData();
	}

	void PlayerSetMarkerManager::PlaceMarker(RE::LocalMapMenu* a_localMapMenu, float a_wndPointX, float a_wndPointY)
	{
		if (!allowPlaceMarker || !a_localMapMenu) {
			return;
		}
		allowPlaceMarker = false;

		auto* player = RE::PlayerCharacter::GetSingleton();
		if (!player) {
			allowPlaceMarker = true;
			return;
		}
		const RE::ObjectRefHandle playerMapMarker = player->GetInfoRuntimeData().playerMapMarker;
		SKSE::log::info("Local marker requested at ({:.1f}, {:.1f}), existing={:08X}",
			a_wndPointX, a_wndPointY, playerMapMarker.native_handle());

		if (playerMapMarker) {
			messageBox.callback->SetData(a_wndPointX, a_wndPointY);
			RE::UI__OpenMessageBox(messageBox.title, messageBox.callback, 0, 25, 4, messageBox.options);
		} else {
			PlaceMarkerImmediate(a_localMapMenu, a_wndPointX, a_wndPointY);
		}
	}
}
