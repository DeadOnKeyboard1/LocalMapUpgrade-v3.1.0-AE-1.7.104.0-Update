#include "Settings.h"
#include "ShaderManager.h"
#include "ExtraMarkersManager.h"
#include "FullAPI.h"
#include "IUI/API.h"

const SKSE::LoadInterface* skse = nullptr;

namespace LMU
{
	bool isIconDisplayExtensionPatched = false;
}

void InfinityUIMessageListener(SKSE::MessagingInterface::Message* a_msg);

class SettingsMenuObserver : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
public:
	RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
		RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
	{
		if (a_event && ((!a_event->opening && a_event->menuName == RE::JournalMenu::MENU_NAME) ||
			(a_event->opening && a_event->menuName == RE::MapMenu::MENU_NAME))) {
			if (auto* taskInterface = SKSE::GetTaskInterface()) {
				taskInterface->AddTask([] { settings::Reload(); });
			}
		}
		return RE::BSEventNotifyControl::kContinue;
	}
};

void SKSEMessageListener(SKSE::MessagingInterface::Message* a_msg)
{
	if (!a_msg) {
		return;
	}

	if (a_msg->type == SKSE::MessagingInterface::kPostLoad) {
		if (auto* messaging = SKSE::GetMessagingInterface();
			messaging && messaging->RegisterListener("InfinityUI", InfinityUIMessageListener)) {
			logger::info("Registered for Infinity UI messages");
		} else {
			// Missing Infinity UI should not take Skyrim down. Core map rendering remains usable;
			// extra-marker graphics simply stay disabled.
			logger::error("Infinity UI was not detected; extra local-map marker graphics are disabled");
		}
	} else if (a_msg->type == SKSE::MessagingInterface::kDataLoaded) {
		LMU::ShaderManager::InitSingleton();
		if (LMU::ShaderManager::GetSingleton() && SKSE::GetMessagingInterface()) {
			LMU::API::PixelShaderPropertiesHookMessage pixelShaderPropertiesHook;
			pixelShaderPropertiesHook.SetPixelShaderProperties = &LMU::ShaderManager::SetPixelShaderProperties;
			pixelShaderPropertiesHook.GetPixelShaderProperties = &LMU::ShaderManager::GetPixelShaderProperties;
			DispatchMessage(pixelShaderPropertiesHook);
		}

		LMU::ExtraMarkersManager::InitSingleton();
		if (auto* ui = RE::UI::GetSingleton()) {
			static SettingsMenuObserver settingsObserver;
			ui->AddEventSink<RE::MenuOpenCloseEvent>(&settingsObserver);
		} else {
			logger::warn("UI singleton unavailable at DataLoaded; settings observer not installed");
		}
	} else if (a_msg->type == SKSE::MessagingInterface::kPostLoadGame ||
		a_msg->type == SKSE::MessagingInterface::kNewGame) {
		settings::Reload();
		if (auto* markers = LMU::ExtraMarkersManager::GetSingleton()) {
			markers->ResetDisplayRadii();
		}
	}
}

void InfinityUIMessageListener(SKSE::MessagingInterface::Message* a_msg)
{
	using namespace IUI;
	if (!a_msg || !a_msg->sender || std::string_view(a_msg->sender) != "InfinityUI") {
		return;
	}

	const auto* message = API::TranslateAs<API::Message>(a_msg);
	if (!message || !message->movie) {
		return;
	}
	auto* movieDef = message->movie->GetMovieDef();
	const char* fileURL = movieDef ? movieDef->GetFileURL() : nullptr;
	if (!fileURL || std::string_view(fileURL).find("Map") == std::string_view::npos) {
		return;
	}

	switch (a_msg->type) {
	case API::Message::Type::kStartLoadInstances:
		LMU::isIconDisplayExtensionPatched = false;
		logger::debug("Infinity UI started loading map patches");
		break;

	case API::Message::Type::kPostPatchInstance:
		if (const auto* msg = API::TranslateAs<API::PostPatchInstanceMessage>(a_msg)) {
			RE::GFxValue expected;
			if (message->movie->GetVariable(&expected, LMU::ExtraMarkersManager::extensionPath.data()) &&
				msg->newInstance == expected) {
				LMU::isIconDisplayExtensionPatched = true;
			}
		}
		break;

	case API::Message::Type::kFinishLoadInstances:
		if (!LMU::isIconDisplayExtensionPatched) {
			logger::error("Infinity UI IconDisplayExtension.swf was not patched; extra marker graphics are disabled");
		} else {
			logger::debug("Infinity UI local-map extension loaded successfully");
		}
		break;

	case API::Message::Type::kPostInitExtensions:
		logger::debug("Infinity UI extension initialization finished");
		break;

	default:
		break;
	}
}
