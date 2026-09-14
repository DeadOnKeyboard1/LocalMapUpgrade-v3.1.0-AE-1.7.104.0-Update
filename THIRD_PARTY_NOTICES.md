# Third-Party Notices

## CommonLibSSE-NG provenance

The source tree's vcpkg overlay pins:

- Repository lineage: `alandtse/CommonLibVR` / CommonLibSSE-NG
- Source commit: `e617713b2ae8a927bf925d1ad138cc48ab72e414`
- Historical port license: MIT
- OpenVR commit: `ebdea152f8aac77e9a6db29682b81d762159df7e`

The tested release DLL was built using an external vcpkg installation rather
than that checked-in overlay. Its DLL and PDB identify a CommonLibSSE-NG build
source directory beginning with `b2f24ebf25`; the remaining commit digits are
not embedded in the available artifacts.

Because that tested binary belongs to the newer CommonLibSSE-NG licensing
generation, the release includes the CommonLibSSE-NG GPL-3.0-or-later text and
its Modding and Linking Exceptions, plus the historical CommonLib MIT notice.

## Components

| Component | License | Notice file |
| --- | --- | --- |
| Local Map Upgrade update | GPL-3.0-or-later | `LICENSE` |
| Original Local Map Upgrade source | MIT | `licenses/LocalMapUpgrade-MIT.txt` |
| CommonLibSSE-NG (tested binary) | GPL-3.0-or-later with exceptions | `licenses/CommonLibSSE-NG-GPL-3.0-or-later.txt`, `licenses/CommonLibSSE-NG-EXCEPTIONS.md` |
| CommonLibSSE-NG (historical pinned source) | MIT | `licenses/CommonLibSSE-NG-MIT.txt` |
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
