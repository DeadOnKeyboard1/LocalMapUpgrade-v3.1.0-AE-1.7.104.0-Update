# Local Map Upgrade: 1.7.104 marker fix and MCM

## Revision 2: MCM reading and accidental marker requests

- The user's saved MCM file has a UTF-8 BOM. The game INI collection and Windows
  INI API ignored its first section. SimpleIni now reads UTF-8 settings correctly.
  Native tests pass against the actual saved file without modifying it, as well as
  partial overrides, whitespace, invalid values and all eleven settings.
- Only a fresh press can place a marker. Release/held events and input that opens
  the map cannot initiate placement. Mouse input outside the map is excluded.
- Restore remapped event names before returning to avoid modifying shared input
  events for other handlers.
- ESP, registration script and saved user settings remain unchanged.
- Native regression tests pass; runtime gameplay confirmation is still required.

## Changes

- LocalMapMenu input vtable: ProcessButton is slot 7 on 1.7.104, not slot 5.
  Slot 5 is ProcessThumbstick; slot 6 is ProcessMouseMove. CanProcess remains slot 1.
- Player INFO_RUNTIME_DATA starts at 0x8E0 for this runtime. Native ID 40535
  reads/writes playerMapMarker at 0x934 and playerMarkerPath at 0x938.
- All eleven INI settings are available in a German MCM with three pages.
- Settings reload after closing the journal/MCM, opening the map, or loading a game.
  Color/fog shaders and immersive detection radii are updated when their settings change.
- Settings paths are resolved from SkyrimSE.exe, not the launcher's working directory.
- MCM values persist in Data/MCM/Settings/LocalMapUpgrade.ini. These override the
  plugin INI, which overrides the packaged MCM defaults.
- Original SWFs and the remaining original features are retained.

## Installation

Replace the previous LocalMapUpgrade update with the new ZIP in Vortex or MO2.
Enable LocalMapUpgrade.esp. It contains only an ESL-flagged MCM registration quest.
Keep SkyUI, MCM Helper, Infinity UI, SKSE and the matching Address Library installed.
No shared dependency DLLs or Address Library files are bundled.

## Verification

- Release DLL built using the existing Visual Studio/CommonLib configuration.
- LMU_MCM.pex compiled with Bethesda's compiler. The compile-only parent declaration
  in tests/papyrus-stubs is not distributed; the runtime parent comes from MCM Helper.
- tests/verify-runtime.py checks the actual executable addresses listed above.
- tools/build-package.cjs validates all eleven setting IDs/defaults and reads back
  the ESP/VMAD/player alias, SEQ and PEX header. ESP layout follows xEdit's
  Core/wbDefinitionsTES5.pas; VMAD strings are length-prefixed without trailing NUL.
- Not yet tested in the running game. Static/binary checks cannot verify gameplay.

## In-game checks

1. Place a marker in an interior and an exterior, then move/remove it.
2. Check WASD, right-button panning, mouse wheel, and controller movement.
3. Open Local Map Upgrade in MCM and toggle color/fog; close MCM and reopen the map.
4. Test NPC categories and immersive mode off/on/off, including detection spells.
5. Save/reload and restart to confirm MCM registration and persisted values.

LocalMapUpgrade.log records marker requests, successful positions and projection
failures at the default Information log level.
