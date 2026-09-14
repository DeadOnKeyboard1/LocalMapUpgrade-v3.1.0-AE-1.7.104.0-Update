# Local Map Upgrade v3.1.0 - Skyrim 1.7.104.0 Update

Source for the Skyrim 1.7.104.0 compatibility update of Local Map Upgrade,
including its MCM integration and marker fixes.

## Requirements

- Skyrim Special Edition / Anniversary Edition runtime 1.7.104.0
- SKSE 2.3.1
- Address Library for SKSE Plugins for runtime 1.7.104.0
- Infinity UI
- SkyUI and MCM Helper for the included MCM

See `UPDATE-1.7.104-MCM.md` for the runtime-specific changes and verification
notes.

## Credits

Local Map Upgrade was created by alexsylex. The original project is available
at https://github.com/alexsylex/LocalMapUpgrade.

The Skyrim 1.7.104.0 compatibility and MCM update was prepared by
DeadOnKeyboard1.

## Building

The native plugin uses CMake, Visual Studio 2022, vcpkg, and the custom
`commonlibsse-ng` overlay in `cmake/ports`. Set `VCPKG_ROOT` to a working vcpkg
checkout and configure with an appropriate preset from `CMakePresets.json`.

The checked-in overlay pins the historical CommonLibVR/CommonLibSSE-NG commit
`e617713b2ae8a927bf925d1ad138cc48ab72e414`. The tested release binary was
produced earlier with an external vcpkg package whose debug paths identify a
different CommonLibSSE-NG revision by the prefix `b2f24ebf25`. Its complete
commit hash is not embedded in the available artifacts. See
`THIRD_PARTY_NOTICES.md` for the provenance and license record.

## License

This update is distributed under GPL-3.0-or-later as stated in `LICENSE`, with
the applicable CommonLibSSE-NG exceptions in
`licenses/CommonLibSSE-NG-EXCEPTIONS.md`. The original Local Map Upgrade source
and copyright notice remain available under their original MIT terms in
`licenses/LocalMapUpgrade-MIT.txt`. Other components retain their respective
licenses; see `THIRD_PARTY_NOTICES.md`.
