"""Read-only snapshot of LocalMapUpgrade settings and rendering state. No injection."""
import ctypes as c
from ctypes import wintypes as w
from pathlib import Path
import re
import struct
import sys
import pefile

root = Path(__file__).resolve().parents[1]
k32 = c.WinDLL('kernel32', use_last_error=True)
psapi = c.WinDLL('psapi', use_last_error=True)
k32.OpenProcess.argtypes = [w.DWORD, w.BOOL, w.DWORD]
k32.OpenProcess.restype = w.HANDLE
k32.ReadProcessMemory.argtypes = [w.HANDLE, c.c_void_p, c.c_void_p, c.c_size_t, c.POINTER(c.c_size_t)]
k32.ReadProcessMemory.restype = w.BOOL
k32.CloseHandle.argtypes = [w.HANDLE]
psapi.EnumProcessModulesEx.argtypes = [w.HANDLE, c.POINTER(c.c_void_p), w.DWORD, c.POINTER(w.DWORD), w.DWORD]
psapi.GetModuleFileNameExW.argtypes = [w.HANDLE, c.c_void_p, w.LPWSTR, w.DWORD]
handle = k32.OpenProcess(0x0400 | 0x0010, False, int(sys.argv[1]))
if not handle:
    raise c.WinError(c.get_last_error())

def read(address, count):
    buffer = c.create_string_buffer(count)
    size = c.c_size_t()
    if not k32.ReadProcessMemory(handle, address, buffer, count, c.byref(size)) or size.value != count:
        raise OSError(f'Cannot read {count} bytes at {address:X}')
    return buffer.raw

def unpack(address, fmt='Q'):
    return struct.unpack('<' + fmt, read(address, struct.calcsize('<' + fmt)))[0]

try:
    modules = (c.c_void_p * 1024)()
    needed = w.DWORD()
    if not psapi.EnumProcessModulesEx(handle, modules, c.sizeof(modules), c.byref(needed), 3):
        raise c.WinError(c.get_last_error())
    found = {}
    for address in modules[:needed.value // c.sizeof(c.c_void_p)]:
        name = c.create_unicode_buffer(32768)
        psapi.GetModuleFileNameExW(handle, address, name, len(name))
        path = Path(name.value)
        if path.name.lower() in ('localmapupgrade.dll', 'skyrimse.exe'):
            found[path.name.lower()] = (address, path)
    dll_base, dll_path = found['localmapupgrade.dll']
    game_base, _ = found['skyrimse.exe']
    print('DLL:', dll_path, hex(dll_base))
    disk = pefile.PE(str(dll_path), fast_load=True)
    symbols_dll = pefile.PE(str(root / 'build/Release/LocalMapUpgrade.dll'), fast_load=True)
    # Refuse to interpret symbols unless the settings and marker consumers match.
    for rva, count in ((0x3750, 2000), (0x26030, 3500)):
        assert disk.get_data(rva, count) == symbols_dll.get_data(rva, count), 'Symbol map mismatch'
        assert read(dll_base + rva, count) == disk.get_data(rva, count), 'Live code differs'
    assert disk.get_data(0x97000, 0x600) == symbols_dll.get_data(0x97000, 0x600), 'Data layout mismatch'
    symbols = {}
    for line in (root / 'build/Release/LocalMapUpgrade.map').read_text().splitlines():
        match = re.match(r'\s+\w+:\w+\s+(\S+)\s+([0-9A-Fa-f]{16})\s', line)
        if match:
            symbols[match[1]] = int(match[2], 16) - symbols_dll.OPTIONAL_HEADER.ImageBase + dll_base
    for name, address in symbols.items():
        if name.startswith('?localMap') and '@mapmenu@settings' in name:
            print(name.split('@')[0][1:], unpack(address, 'f' if 'PanSpeed' in name else '?'))
    def symbol(prefix):
        return next(address for name, address in symbols.items() if name.startswith(prefix))
    shader = unpack(symbol('?singleton@ShaderManager'))
    markers = unpack(symbol('?singleton@ExtraMarkersManager'))
    print('Shader singleton:', hex(shader), 'shape/style:', read(shader, 8).hex() if shader else 'NULL')
    print('Marker singleton:', hex(markers), 'radii:', struct.unpack('<3I', read(markers, 12)) if markers else 'NULL')
    pixel = unpack(symbol('?localMapPixelShader@'))
    print('Patched pixel wrapper:', hex(pixel), 'id:', unpack(pixel, 'I') if pixel else None)
    print('Bound shader in wrapper:', hex(unpack(pixel + 8)) if pixel else None)
    if pixel:
        program = unpack(pixel + 8)
        print('Shader object first 32 bytes:', read(program, 32).hex())
        vtable = unpack(program)
        print('Shader COM vtable:', hex(vtable), 'QueryInterface:', hex(unpack(vtable)))
    for group in ('?roundShaders@', '?squaredShaders@'):
        print(group, [hex(v) for v in struct.unpack('<4Q', read(symbol(group), 32))])
    print('Native fog:', unpack(game_base + 0x2079F48, '?'))
    manager = unpack(game_base + 0x33D40A0)
    effects = unpack(manager + 0x28)
    effect = unpack(effects + 98 * 8)
    print('ImageSpace effect 98:', hex(effect), 'vtable:', hex(unpack(effect) - game_base))
    bs_shader = effect - 0x90
    print('BSShader vtable:', hex(unpack(bs_shader) - game_base))
    table = bs_shader + 0x58
    capacity = unpack(table + 0xC, 'I')
    entries = unpack(table + 0x28)
    print('Pixel shader table capacity:', capacity, 'entries:', hex(entries))
    if 0 < capacity < 1024:
        for i in range(capacity):
            pointer, next_entry = struct.unpack('<QQ', read(entries + i * 16, 16))
            if next_entry and pointer:
                print('Pixel shader entry:', i, hex(pointer), 'id:', unpack(pointer, 'I'), 'program:', hex(unpack(pointer + 8)))
    for rva in (0x994026, 0x993339, 0x2486D5):
        print('Hook bytes', hex(rva), read(game_base + rva, 5).hex())
finally:
    k32.CloseHandle(handle)
