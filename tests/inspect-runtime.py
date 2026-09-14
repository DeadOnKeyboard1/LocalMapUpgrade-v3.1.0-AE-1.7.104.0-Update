"""Disassemble selected runtime functions for read-only compatibility analysis."""
import sys
from verify_runtime_support import pe, offsets, cs, base

for spec in sys.argv[1:]:
    key, _, count = spec.partition(':')
    address = base + int(key, 16) if key.startswith('0x') else offsets[int(key)]
    print(f'\n{spec}: {address:X}')
    for instruction in cs.disasm(pe.get_data(address - base, int(count, 0) if count else 1024), address):
        print(f'{instruction.address - base:08X}  {instruction.mnemonic:8} {instruction.op_str}')
