# Local Map Upgrade: Skyrim 1.7.104.0 marker fix and MCM

## MCM reading and marker input

- Saved MCM files with a UTF-8 BOM are read through SimpleIni so the first section is not lost.
- Only a fresh marker press can request placement. Release/held events and the input event that opens the map cannot place a marker.
- Mouse input outside the usable local-map bounds is excluded.
- Remapped event names are restored before returning so shared input events are not left modified for other handlers.
- All eleven native settings are exposed through the MCM.

## Runtime-specific changes

- LocalMapMenu input vtable handling is adapted for Skyrim 1.7.104.0.
- Player/map runtime data used by marker placement is handled for the 1.7.104.0 layout.
- Settings reload after closing the journal/MCM, opening the map, or loading a game.
- Color/fog shaders and immersive detection ranges react to the current settings.
- Settings paths are resolved relative to SkyrimSE.exe rather than the launcher's working directory.
- MCM values persist in `Data/MCM/Settings/LocalMapUpgrade.ini`; saved MCM values override the plugin INI, which overrides packaged defaults.
- Original SWFs and the remaining original Local Map Upgrade features are retained.

## Stability changes

The 1.7.104.0 update also includes defensive native-code changes beyond the MCM itself. Shader compilation failures, marker raycasts during load/cell transitions, marker message-box lifetime, hook validation and active detection effects are hardened. See `FIXES-2026-09-23.md` for the complete list.

## Installation

Replace the previous Local Map Upgrade update with the newly built ZIP in Vortex or MO2. Enable `LocalMapUpgrade.esp`; it contains the ESL-flagged MCM registration quest. Keep SkyUI, MCM Helper, Infinity UI, SKSE and the matching Address Library installed.

No shared dependency DLLs or Address Library files are bundled.

## Build and package verification

- The native plugin builds through `Build-1.7.104.0.cmd` using the pinned CommonLibSSE-NG/vcpkg configuration.
- `LMU_MCM.pex` is included as a tracked source asset; the runtime parent comes from MCM Helper.
- `tools/build-package.cjs` validates all eleven setting IDs/defaults and reads back the ESP/VMAD/player alias, SEQ and PEX header.
- Localization generation validates the packaged translation tables.
- The current final build was reported loading and functioning in Skyrim 1.7.104.0 on 2026-09-23. This was a gameplay smoke test, not a claim that every possible mod combination has been exhaustively tested.

## Recommended in-game regression checks

1. Place, move and remove a marker in both interior and exterior local maps.
2. Check keyboard/controller movement, right-button panning and mouse-wheel behavior.
3. Toggle color/fog settings in MCM, close MCM and reopen the local map.
4. Test NPC marker categories and immersive detection mode with relevant detection effects.
5. Save/reload and restart to confirm MCM registration and persisted values.
