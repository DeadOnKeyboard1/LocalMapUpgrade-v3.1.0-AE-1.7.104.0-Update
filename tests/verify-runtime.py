"""Read-only checks against the user's 1.7.104 executable and Address Library map."""
from pathlib import Path
import struct
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

workspace = Path(__file__).resolve().parents[3]
exe = Path(r'D:\SteamLibrary\steamapps\common\Skyrim Special Edition\SkyrimSE.exe')
offsets = {}
for line in (workspace / 'adresslibrary/offsets-1-7-104-0.txt').read_text().splitlines():
    parts = line.split()
    if len(parts) == 2:
        offsets[int(parts[0])] = int(parts[1], 16)
pe = pefile.PE(str(exe), fast_load=True)
base = pe.OPTIONAL_HEADER.ImageBase
cs = Cs(CS_ARCH_X86, CS_MODE_64)

def data(address, count):
    return pe.get_data(address - base, count)

vtable = offsets[216412]
slots = struct.unpack('<8Q', data(vtable, 64))
assert slots[5] == base + 0x9949D0, 'Old slot must remain the thumbstick handler'
assert slots[7] == base + 0x994A30, 'Button handler no longer matches this runtime'
assert data(slots[5], 9).hex() == '4883ec28f30f104a28'
button_ops = list(cs.disasm(data(slots[7], 64), slots[7]))
assert any(i.mnemonic == 'call' and i.op_str == 'qword ptr [rax + 0x10]' for i in button_ops)
marker_ops = list(cs.disasm(data(offsets[40535], 300), offsets[40535]))
assert any(i.mnemonic == 'add' and i.op_str == 'rcx, 0x934' for i in marker_ops)
assert any(i.mnemonic == 'mov' and i.op_str == 'r8, qword ptr [rbp + 0x938]' for i in marker_ops)
assert 0x8E0 + 0x54 == 0x934 and 0x8E0 + 0x58 == 0x938
world_ops = list(cs.disasm(data(offsets[53108], 64), offsets[53108]))
assert any(i.mnemonic == 'lea' and i.op_str == 'r15, [rcx + 0x30470]' for i in world_ops)
for identifier, offset, expected in [(52971, 0x7A6, 0x999330),
                                     (52966, 0xD9, 0xCF6160),
                                     (16335, 0x105, 0x30DB60)]:
    address = offsets[identifier] + offset
    code = data(address, 5)
    assert code[0] == 0xE8
    assert address + 5 + struct.unpack('<i', code[1:])[0] == base + expected
print('PASS: button vtable slot 7, player marker/path offsets, world marker array, and 3 direct hook targets.')
