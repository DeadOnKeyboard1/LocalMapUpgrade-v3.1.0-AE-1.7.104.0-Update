#include "Hooks.h"
#include "Settings.h"
#include "utils/Logger.h"

extern const SKSE::LoadInterface* skse;
void SKSEMessageListener(SKSE::MessagingInterface::Message* a_msg);

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	if (!a_skse) {
		return false;
	}
	skse = a_skse;

	const auto* plugin = SKSE::PluginDeclaration::GetSingleton();
	if (!plugin || !logger::init(plugin->GetName())) {
		return false;
	}

	const auto runtime = REL::Module::get().version();
	if (runtime != REL::Version{ 1, 7, 104, 0 }) {
		logger::critical("Unsupported Skyrim runtime {}. This build targets 1.7.104.0 only.", runtime.string("."));
		return false;
	}

	logger::info("Loading {} {} for Skyrim {}", plugin->GetName(), plugin->GetVersion(), runtime.string("."));
	SKSE::Init(a_skse);

	settings::Init(std::string(plugin->GetName()) + ".ini");
	logger::set_level(settings::debug::logLevel, settings::debug::logLevel);

	auto* messaging = SKSE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener("SKSE", SKSEMessageListener)) {
		logger::critical("Failed to register the SKSE message listener");
		return false;
	}
	if (!hooks::Install()) {
		logger::critical("Hook installation failed; refusing to load an unsafe partial plugin");
		return false;
	}

	logger::info("Successfully loaded Local Map Upgrade for Skyrim 1.7.104.0");
	return true;
}
