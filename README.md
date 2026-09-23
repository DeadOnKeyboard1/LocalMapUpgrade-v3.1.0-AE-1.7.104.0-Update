# Local Map Upgrade v3.1.0 - Skyrim AE 1.7.104.0 Update

Compatibility, stability and MCM update of **Local Map Upgrade** for **Skyrim 1.7.104.0**.

This repository contains the complete source/build setup used for the 1.7.104.0 update. The update keeps the original mod's purpose and assets while adding the runtime-specific fixes, MCM integration, crash hardening and build/release tooling required by this version.

## Status

- Target runtime: **Skyrim 1.7.104.0 only**
- Native plugin version: **3.1.0**
- CommonLibSSE-NG: **9.0.1**, pinned to commit `736dc64094e59232abfbcdf796cd0a063e136ec6`
- Windows build: successful with the included build pipeline
- In-game smoke test: current build reported working on Skyrim 1.7.104.0 on 2026-09-23

The plugin intentionally rejects unsupported Skyrim runtimes rather than installing runtime-specific hooks against an unknown executable layout.

## Requirements

- Skyrim Special Edition / Anniversary Edition runtime **1.7.104.0**
- SKSE **2.3.1**
- Address Library for SKSE Plugins matching runtime 1.7.104.0
- Infinity UI
- SkyUI
- MCM Helper

## Main fixes in this update

- Safe shader compilation/error handling instead of dereferencing failed shader blobs.
- Defensive local-map ray picking during cell/load/world transitions.
- Marker message-box callbacks reacquire the current MapMenu instead of retaining a stale LocalMapMenu pointer.
- 1.7.104.0 hook sites are validated before patching.
- Missing UI/renderer/console-command/singleton state is handled defensively.
- Detect Life / Detect Dead / Aura Whisper marker ranges are derived from active player effects rather than fragile shader-effect hooks.
- Marker input/debouncing fixes for Skyrim 1.7.104.0.
- Settings files are loaded once per source file per reload instead of repeatedly per setting.
- Faster enemy lookup and cached undead keyword evaluation.
- Private local source paths are mapped out of release DLL/PDB output.

See `FIXES-2026-09-23.md` and `UPDATE-1.7.104-MCM.md` for details.

## Building

The recommended Windows build is:

```bat
Build-1.7.104.0.cmd clean
```

The batch:

1. finds/configures the MSVC toolchain,
2. ensures Git, CMake, Ninja and Node.js are available,
3. creates an isolated vcpkg checkout under `_toolchain\vcpkg`,
4. builds the pinned CommonLibSSE-NG dependency and `LocalMapUpgrade.dll`,
5. validates the MCM/ESP/VMAD/PEX/SEQ package structure, and
6. creates the Vortex/Nexus ZIP under `dist`.

Build logs are written to `build-logs`.

## GitHub repository replacement

`Upload-GitHub-Replace.cmd` is configured for:

`DeadOnKeyboard1/LocalMapUpgrade-v3.1.0-AE-1.7.104.0-Update`

It mirrors this source tree to the repository's `main` branch while excluding local build/toolchain/output directories. Before changing `main`, it creates a timestamped backup branch of the previous remote state. Git history is preserved; the script does **not** rewrite history with a force push.

Run:

```bat
Upload-GitHub-Replace.cmd
```

Use `Upload-GitHub-Replace.cmd --yes` to skip the final confirmation prompt.

## Credits

**Local Map Upgrade** was created by **alexsylex**.

Original project:
https://github.com/alexsylex/LocalMapUpgrade

Skyrim 1.7.104.0 compatibility, MCM integration and stability update:
**DeadOnKeyboard1**

## License

This update is distributed under **GPL-3.0-or-later** as stated in `LICENSE`, together with the applicable CommonLibSSE-NG exceptions in `licenses/CommonLibSSE-NG-EXCEPTIONS.md`.

The original Local Map Upgrade source and copyright notice remain preserved under their original MIT terms in `licenses/LocalMapUpgrade-MIT.txt`. Third-party components retain their respective licenses; see `THIRD_PARTY_NOTICES.md`.
