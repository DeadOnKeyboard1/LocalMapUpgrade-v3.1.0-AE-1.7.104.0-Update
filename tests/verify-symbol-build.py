"""Compare the working DLL with its symbol-enabled relink before packaging."""
from pathlib import Path
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

root = Path(__file__).resolve().parents[1]
old = pefile.PE(str(root / 'package/SKSE/Plugins/LocalMapUpgrade.dll'))
new = pefile.PE(str(root / 'build/Release/LocalMapUpgrade.dll'))
cs = Cs(CS_ARCH_X86, CS_MODE_64)
cs.skipdata = True
cs.detail = True
a = list(cs.disasm(old.sections[0].get_data(), old.sections[0].VirtualAddress))
b = list(cs.disasm(new.sections[0].get_data(), new.sections[0].VirtualAddress))
assert len(a) == len(b)
changes = 0
for x, y in zip(a, b):
    if x.bytes == y.bytes:
        continue
    assert x.address == y.address and x.size == y.size
    assert x.mnemonic == y.mnemonic == 'lea'
    assert x.bytes[:x.disp_offset] == y.bytes[:y.disp_offset]
    assert x.bytes[x.disp_offset + x.disp_size:] == y.bytes[y.disp_offset + y.disp_size:]
    assert y.disp - x.disp == 0xA0
    assert old.get_section_by_rva(x.address + x.size + x.disp).Name.startswith(b'.rdata')
    assert new.get_section_by_rva(y.address + y.size + y.disp).Name.startswith(b'.rdata')
    changes += 1
print(f'PASS: {len(a)} instructions compared; {changes} shifted data references only.')
