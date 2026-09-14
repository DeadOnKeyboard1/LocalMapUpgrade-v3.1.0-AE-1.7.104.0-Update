from pathlib import Path
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

workspace = Path(__file__).resolve().parents[3]
offsets = {}
for line in (workspace / 'adresslibrary/offsets-1-7-104-0.txt').read_text().splitlines():
    parts = line.split()
    if len(parts) == 2:
        offsets[int(parts[0])] = int(parts[1], 16)
pe = pefile.PE(r'D:\SteamLibrary\steamapps\common\Skyrim Special Edition\SkyrimSE.exe', fast_load=True)
base = pe.OPTIONAL_HEADER.ImageBase
cs = Cs(CS_ARCH_X86, CS_MODE_64)
