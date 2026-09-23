# Third-Party Notices

## CommonLibSSE-NG provenance

This source tree builds against the pinned CommonLibSSE-NG 9.0.1 source used by
the included vcpkg overlay:

- Repository: `alandtse/CommonLibSSE-NG`
- Source commit: `736dc64094e59232abfbcdf796cd0a063e136ec6`
- Version: 9.0.1
- License: GPL-3.0-or-later with the upstream Modding and Linking Exceptions
- OpenVR source commit: `60eb187801956ad277f1cae6680e3a410ee0873b`

The overlay also patches CommonLibSSE-NG's generated CMake package metadata so
its public DirectXTK dependency is discovered by consumers. No gameplay code is
changed by that packaging patch.

## Components

| Component | License | Notice file |
| --- | --- | --- |
| Local Map Upgrade update | GPL-3.0-or-later | `LICENSE` |
| Original Local Map Upgrade source | MIT | `licenses/LocalMapUpgrade-MIT.txt` |
| CommonLibSSE-NG 9.0.1 | GPL-3.0-or-later with exceptions | `licenses/CommonLibSSE-NG-GPL-3.0-or-later.txt`, `licenses/CommonLibSSE-NG-EXCEPTIONS.md` |
| SimpleIni | MIT | `licenses/SimpleIni-MIT.txt` |
| OpenVR | BSD-3-Clause | `licenses/OpenVR-BSD-3-Clause.txt` |
| DirectXMath | MIT | `licenses/DirectXMath.txt` |
| DirectX Tool Kit | MIT | `licenses/DirectXTK.txt` |
| fmt | MIT | `licenses/fmt.txt` |
| rapidcsv | BSD-3-Clause | `licenses/rapidcsv.txt` |
| spdlog | MIT | `licenses/spdlog.txt` |
| Xbyak | BSD-3-Clause | `licenses/Xbyak-BSD-3-Clause.txt` |

External runtime requirements such as SKSE, Address Library, Infinity UI,
SkyUI, MCM Helper, Skyrim, and Windows are not redistributed by this source
repository.
