# Live verification, 2026-09-11

Runtime: Skyrim Steam 1.7.104.0. Installed package: fix2.
Read-only process inspection and Skyrim window captures; no injection, memory
writes, game input automation or changes to the installed DLL. The user changed
settings through MCM and reopened the local map.

## Observations

1. Initial snapshot: color/fog/immersive enabled, keyboard pan speed 50.
   All NPC categories enabled. Detection radii zero, as expected in immersive mode.
2. User disabled color/fog/immersive and selected keyboard pan speed 150.
   Live DLL values matched. Native fog flag became false. Detection radii became
   UINT_MAX. The pixel shader switched to square/monochrome/no-fog.
   Window capture showed a monochrome map with corpse icons.
3. User enabled color and disabled corpse icons, leaving immersive disabled.
   Live DLL values matched. The pixel shader switched to square/color/no-fog.
   Window capture showed a colored map without the previous corpse icons.

Only one pixel shader entry exists for the native local-map effect (index 98).
Its program pointer matched the selected LocalMapUpgrade shader in each snapshot.
Symbol addresses were checked against the installed DLL's settings and marker
consumer code before interpreting live values.

## Limits

The reported failure to apply all settings was not reproduced in this session.
No additional gameplay fix is justified by these measurements. The cause of the
earlier observation remains unknown. Color and corpse visibility were visually
verified in both tested states; keyboard panning speed, fog and immersive radii
were verified internally, not by a complete behavioral test. Other individual
NPC category toggles, detection spells and log-level changes were not exercised.

The installed DLL and existing fix2 ZIP were not replaced. User-selected test
values remain in the user's MCM settings file.
