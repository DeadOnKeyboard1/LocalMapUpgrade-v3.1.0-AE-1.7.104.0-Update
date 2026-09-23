#pragma once

#include "utils/Trampoline.h"

namespace RE
{
	class BSWaterShader;
}

bool FakeNotSmallWorld(RE::TESWorldSpace* a_worldSpace);
void AddExtraAndQuestMarkersToMap(RE::BSTArray<RE::MapMenuMarker>& a_mapMarkers,
	RE::BSTArray<RE::BGSInstancedQuestObjective>& a_objectives, std::uint32_t a_arg3);
bool InvokeCreateAndPostProcessMarkers(RE::GFxValue::ObjectInterface* a_objIface, void* a_data,
	RE::GFxValue* a_result, const char* a_name, const RE::GFxValue* a_args, std::uint32_t a_numArgs, bool a_isDObj);
bool SetupWaterShaderTechnique(RE::BSWaterShader* a_shader, std::uint32_t a_technique);
bool CanProcess(RE::LocalMapMenu::InputHandler* a_localMapInputHandler, RE::InputEvent* a_event);
bool ProcessButton(RE::LocalMapMenu::InputHandler* a_localMapInputHandler, RE::ButtonEvent* a_event);
bool ToggleFogOfWar(const RE::SCRIPT_PARAMETER* a_paramInfo, RE::SCRIPT_FUNCTION::ScriptData* a_scriptData,
	RE::TESObjectREFR* a_thisObj, RE::TESObjectREFR* a_containingObj, RE::Script* a_scriptObj,
	RE::ScriptLocals* a_locals, double& a_result, std::uint32_t& a_opcodeOffsetPtr);

namespace hooks
{
	class LocalMapMenu
	{
		static constexpr REL::VariantID AdvanceId{ 52078, 52966, 0x90ED80 };
		static constexpr REL::VariantID PopulateDataId{ 52081, 52971, 0x90F3C0 };

	public:
		class InputHandler
		{
		public:
			static inline REL::Relocation<std::uintptr_t> vTable{ RE::VTABLE_LocalMapMenu__InputHandler[0] };
			static inline REL::Relocation<bool (RE::LocalMapMenu::InputHandler::*)(RE::InputEvent*)> CanProcess;
			static inline REL::Relocation<bool (RE::LocalMapMenu::InputHandler::*)(RE::ButtonEvent*)> ProcessButton;
		};

		class LocalMapCullingProcess
		{
			static constexpr REL::VariantID RenderOffScreenId{ 16094, 16335, 0x206C90 };

		public:
			static inline REL::Relocation<void (RE::LocalMapMenu::LocalMapCullingProcess::*)()> RenderOffScreen{ RenderOffScreenId };
		};

		static inline REL::Relocation<void (RE::LocalMapMenu::*)()> Advance{ AdvanceId };
		static inline REL::Relocation<void (RE::LocalMapMenu::*)()> PopulateData{ PopulateDataId };
	};

	class BSWaterShader
	{
	public:
		static inline REL::Relocation<std::uintptr_t> vTable{ RE::VTABLE_BSWaterShader[0] };
		static inline REL::Relocation<bool (RE::BSWaterShader::*)(std::uint32_t)> SetupTechnique;
	};

	class TESWorldSpace
	{
	public:
		static inline REL::Relocation<bool (RE::TESWorldSpace::*)()> IsSmallWorld;
	};

	class GFxValue
	{
	public:
		class ObjectInterface
		{
		public:
			static inline REL::Relocation<bool (RE::GFxValue::ObjectInterface::*)(void*, RE::GFxValue*, const char*,
				const RE::GFxValue*, std::uint32_t, bool)> Invoke;
		};
	};

	inline REL::Relocation<void (*)(RE::BSTArray<RE::MapMenuMarker>&,
		RE::BSTArray<RE::BGSInstancedQuestObjective>&, std::uint32_t)> AddQuestMarkersToMap;

	[[nodiscard]] inline bool ValidateCallSite(std::uintptr_t a_address, std::string_view a_name)
	{
		if (!a_address) {
			SKSE::log::critical("Hook validation failed for {}: null address", a_name);
			return false;
		}

		MEMORY_BASIC_INFORMATION memory{};
		if (!VirtualQuery(reinterpret_cast<const void*>(a_address), &memory, sizeof(memory)) ||
			memory.State != MEM_COMMIT || (memory.Protect & (PAGE_NOACCESS | PAGE_GUARD))) {
			SKSE::log::critical("Hook validation failed for {} at 0x{:X}: address is not readable", a_name, a_address);
			return false;
		}
		if (*reinterpret_cast<const std::uint8_t*>(a_address) != 0xE8) {
			SKSE::log::critical("Hook validation failed for {} at 0x{:X}: expected CALL rel32 (E8)", a_name, a_address);
			return false;
		}
		return true;
	}

	inline bool Install()
	{
		if (REL::Module::get().version() != REL::Version{ 1, 7, 104, 0 }) {
			SKSE::log::critical("Unsupported Skyrim runtime {}. Local Map Upgrade requires 1.7.104.0.",
				REL::Module::get().version().string("."));
			return false;
		}

		struct IsSmallWorldHook : Hook<5>
		{
			static std::uintptr_t Address()
			{
				return LocalMapMenu::LocalMapCullingProcess::RenderOffScreen.address() + 0x105;
			}
			explicit IsSmallWorldHook(std::uintptr_t a_address) :
				Hook{ a_address, reinterpret_cast<std::uintptr_t>(&FakeNotSmallWorld) }
			{}
		};

		struct AddQuestMarkersToMapHook : Hook<5>
		{
			static std::uintptr_t Address()
			{
				return LocalMapMenu::PopulateData.address() + 0x7A6;
			}
			explicit AddQuestMarkersToMapHook(std::uintptr_t a_address) :
				Hook{ a_address, reinterpret_cast<std::uintptr_t>(&AddExtraAndQuestMarkersToMap) }
			{}
		};

		struct InvokeCreateMarkersHook : Hook<5>
		{
			static std::uintptr_t Address()
			{
				return LocalMapMenu::Advance.address() + 0xD9;
			}
			explicit InvokeCreateMarkersHook(std::uintptr_t a_address) :
				Hook{ a_address, reinterpret_cast<std::uintptr_t>(&InvokeCreateAndPostProcessMarkers) }
			{}
		};

		const auto isSmallWorldAddress = IsSmallWorldHook::Address();
		const auto addQuestMarkersAddress = AddQuestMarkersToMapHook::Address();
		const auto invokeCreateMarkersAddress = InvokeCreateMarkersHook::Address();
		if (!ValidateCallSite(isSmallWorldAddress, "RenderOffScreen/IsSmallWorld") ||
			!ValidateCallSite(addQuestMarkersAddress, "PopulateData/AddQuestMarkersToMap") ||
			!ValidateCallSite(invokeCreateMarkersAddress, "Advance/CreateMarkers")) {
			return false;
		}

		IsSmallWorldHook isSmallWorldHook{ isSmallWorldAddress };
		AddQuestMarkersToMapHook addQuestMarkersToMapHook{ addQuestMarkersAddress };
		InvokeCreateMarkersHook invokeCreateMarkersHook{ invokeCreateMarkersAddress };
		static DefaultTrampoline defaultTrampoline{
			isSmallWorldHook.getSize() + addQuestMarkersToMapHook.getSize() + invokeCreateMarkersHook.getSize()
		};

		TESWorldSpace::IsSmallWorld = defaultTrampoline.write_call(isSmallWorldHook);
		AddQuestMarkersToMap = defaultTrampoline.write_call(addQuestMarkersToMapHook);
		GFxValue::ObjectInterface::Invoke = defaultTrampoline.write_call(invokeCreateMarkersHook);

		LocalMapMenu::InputHandler::CanProcess = LocalMapMenu::InputHandler::vTable.write_vfunc(1, CanProcess);
		constexpr std::size_t buttonSlot = 7;  // verified on Skyrim 1.7.104.0
		LocalMapMenu::InputHandler::ProcessButton = LocalMapMenu::InputHandler::vTable.write_vfunc(buttonSlot, ProcessButton);
		BSWaterShader::SetupTechnique = BSWaterShader::vTable.write_vfunc(2, &SetupWaterShaderTechnique);

		if (auto* tfow = RE::SCRIPT_FUNCTION::LocateConsoleCommand("ToggleFogOfWar")) {
			tfow->executeFunction = &ToggleFogOfWar;
		} else {
			SKSE::log::warn("ToggleFogOfWar was not found; fog synchronization hook skipped");
		}

		SKSE::log::info("1.7.104.0 local-map hooks installed successfully");
		return true;
	}
}
