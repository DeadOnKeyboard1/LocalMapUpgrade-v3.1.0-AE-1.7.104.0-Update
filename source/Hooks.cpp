#include "Hooks.h"
#include "LocalMapAccess.h"
#include "MarkerInput.h"
#include "Settings.h"
#include "ExtraMarkersManager.h"
#include "PlayerSetMarkerManager.h"
#include "ShaderManager.h"
#include "RE/B/BSShaderAccumulator.h"

bool FakeNotSmallWorld(RE::TESWorldSpace* a_worldSpace)
{
	if (a_worldSpace) {
		(void)hooks::TESWorldSpace::IsSmallWorld(a_worldSpace);
	}
	return false;
}

void AddExtraAndQuestMarkersToMap(RE::BSTArray<RE::MapMenuMarker>& a_mapMarkers,
	RE::BSTArray<RE::BGSInstancedQuestObjective>& a_objectives, std::uint32_t a_arg3)
{
	auto* localMapMenu = reinterpret_cast<RE::LocalMapMenu*>(
		reinterpret_cast<std::uintptr_t>(std::addressof(a_mapMarkers)) - offsetof(RE::LocalMapMenu, mapMarkers));
	if (auto* manager = LMU::ExtraMarkersManager::GetSingleton(); manager && localMapMenu) {
		manager->AddExtraMarkers(*localMapMenu);
	}
	hooks::AddQuestMarkersToMap(a_mapMarkers, a_objectives, a_arg3);
}

bool InvokeCreateAndPostProcessMarkers(RE::GFxValue::ObjectInterface* a_objIface, void* a_data,
	RE::GFxValue* a_result, const char* a_name, const RE::GFxValue* a_args, std::uint32_t a_numArgs, bool a_isDObj)
{
	const bool result = hooks::GFxValue::ObjectInterface::Invoke(
		a_objIface, a_data, a_result, a_name, a_args, a_numArgs, a_isDObj);
	if (result && a_objIface && a_data) {
		// Call through the original ObjectInterface instead of fabricating a GFxValue and
		// writing protected Scaleform internals.
		(void)hooks::GFxValue::ObjectInterface::Invoke(
			a_objIface, a_data, nullptr, "PostCreateMarkers", nullptr, 0, a_isDObj);
	}
	return result;
}

bool SetupWaterShaderTechnique(RE::BSWaterShader* a_shader, std::uint32_t a_technique)
{
	if (auto* accumulator = RE::BSShaderAccumulator::GetCurrentAccumulator()) {
		if (accumulator->GetRuntimeData().renderMode == RE::BSShaderAccumulator::RENDER_MODE::kLocalMap) {
			a_technique &= ~0x802;  // fix local-map water flicker
		}
	}
	return hooks::BSWaterShader::SetupTechnique(a_shader, a_technique);
}

bool CanProcess(RE::LocalMapMenu::InputHandler* a_handler, RE::InputEvent* a_event)
{
	const bool vanilla = hooks::LocalMapMenu::InputHandler::CanProcess(a_handler, a_event);
	if (!a_handler || !a_handler->localMapMenu || !a_event) {
		return vanilla;
	}

	auto& runtime = a_handler->localMapMenu->GetRuntimeData();
	if (!runtime.showingMap || !runtime.controlsReady) {
		return vanilla;
	}

	RE::GFxValue bottomBar;
	RE::GFxValue destinationButton;
	if (runtime.localMapMovie.GetMember("BottomBar", &bottomBar)) {
		if (bottomBar.GetMember("RightButton", &destinationButton)) {
			destinationButton.SetMember("visible", true);
		}
	} else if (runtime.localMapMovie.GetMember("_bottomBar", &bottomBar)) {
		RE::GFxValue buttonPanel;
		if (bottomBar.GetMember("buttonPanel", &buttonPanel) &&
			buttonPanel.GetMember("button5", &destinationButton)) {
			destinationButton.SetMember("_visible", true);
		}
	}

	return vanilla || a_event->GetEventType() == RE::INPUT_EVENT_TYPE::kButton;
}

bool ProcessButton(RE::LocalMapMenu::InputHandler* a_handler, RE::ButtonEvent* a_event)
{
	if (!a_handler || !a_handler->localMapMenu || !a_event) {
		return false;
	}

	auto* menu = a_handler->localMapMenu;
	auto& runtime = menu->GetRuntimeData();
	auto* userEvents = RE::UserEvents::GetSingleton();
	if (!userEvents) {
		return hooks::LocalMapMenu::InputHandler::ProcessButton(a_handler, a_event);
	}

	struct RestoreEventName
	{
		RE::ButtonEvent* event;
		RE::BSFixedString name;
		~RestoreEventName() { if (event) event->userEvent = name; }
	} restore{ a_event, a_event->userEvent };

	bool isLocationFinderShown = false;
	RE::GFxValue locationFinder;
	if (runtime.localMapMovie.GetMember("_locationFinder", &locationFinder) && locationFinder.IsObject()) {
		RE::GFxValue shown;
		if (locationFinder.GetMember("_bShown", &shown)) {
			isLocationFinderShown = shown.GetBool();
		}
	}

	const bool readyBefore = runtime.showingMap && runtime.controlsReady && !isLocationFinderShown;
	const auto device = a_event->GetDevice();
	if (readyBefore && device == RE::INPUT_DEVICE::kMouse) {
		if (auto* cursor = RE::MenuCursor::GetSingleton()) {
			const auto& topLeft = LMU::GetLocalMapTopLeft(*menu);
			const auto& bottomRight = LMU::GetLocalMapBottomRight(*menu);
			const bool inBounds = cursor->cursorPosX > topLeft.x && cursor->cursorPosX < bottomRight.x &&
				cursor->cursorPosY > topLeft.y && cursor->cursorPosY < bottomRight.y;
			if (inBounds) {
				if (a_event->userEvent == userEvents->mapLookMode) {
					a_event->userEvent = userEvents->localMapMoveMode;
				} else if (a_event->userEvent == userEvents->localMapMoveMode) {
					a_event->userEvent = userEvents->click;
				}
			}
		}
	}

	const bool retval = hooks::LocalMapMenu::InputHandler::ProcessButton(a_handler, a_event);
	const bool readyAfter = runtime.showingMap && runtime.controlsReady && !isLocationFinderShown;
	if (!readyAfter) {
		return retval;
	}

	const bool markerAction = a_event->userEvent == userEvents->click ||
		a_event->userEvent == userEvents->placePlayerMarker;
	if (markerAction) {
		auto* markerManager = LMU::PlayerSetMarkerManager::GetSingleton();
		if (!markerManager) {
			return retval;
		}

		const auto& topLeft = LMU::GetLocalMapTopLeft(*menu);
		const auto& bottomRight = LMU::GetLocalMapBottomRight(*menu);
		float wndPointX = (topLeft.x + bottomRight.x) * 0.5F;
		float wndPointY = (topLeft.y + bottomRight.y) * 0.5F;
		bool inBounds = true;
		if (auto* cursor = RE::MenuCursor::GetSingleton()) {
			wndPointX = cursor->cursorPosX;
			wndPointY = cursor->cursorPosY;
			if (device == RE::INPUT_DEVICE::kMouse) {
				inBounds = wndPointX >= topLeft.x && wndPointX <= bottomRight.x &&
					wndPointY >= topLeft.y && wndPointY <= bottomRight.y;
			}
		} else if (device == RE::INPUT_DEVICE::kMouse) {
			inBounds = false;
		}

		if (LMU::ShouldPlaceMarker(readyBefore, readyAfter, markerManager->CanPlaceMarker(), inBounds,
			a_event->Value(), a_event->HeldDuration())) {
			markerManager->PlaceMarker(menu, wndPointX, wndPointY);
		} else if (a_event->Value() == 0.0F) {
			markerManager->AllowPlaceMarker();
		}
		return true;
	}

	if (auto* localMapCamera = menu->localCullingProcess.GetLocalMapCamera()) {
		if (a_event->userEvent == userEvents->up) {
			localMapCamera->translationInput.y += settings::mapmenu::localMapKeyboardPanSpeed;
		} else if (a_event->userEvent == userEvents->down) {
			localMapCamera->translationInput.y -= settings::mapmenu::localMapKeyboardPanSpeed;
		} else if (a_event->userEvent == userEvents->left) {
			localMapCamera->translationInput.x -= settings::mapmenu::localMapKeyboardPanSpeed;
		} else if (a_event->userEvent == userEvents->right) {
			localMapCamera->translationInput.x += settings::mapmenu::localMapKeyboardPanSpeed;
		}
	}
	return retval;
}

bool ToggleFogOfWar(const RE::SCRIPT_PARAMETER*, RE::SCRIPT_FUNCTION::ScriptData*, RE::TESObjectREFR*,
	RE::TESObjectREFR*, RE::Script*, RE::ScriptLocals*, double&, std::uint32_t&)
{
	if (auto* shaderManager = LMU::ShaderManager::GetSingleton()) {
		shaderManager->ToggleFogOfWarLocalMapShader();
	}
	return true;
}
