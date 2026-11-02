"""Execute the supplied app's real colour conversion locally, without Bluetooth.

Requires capstone/unicorn and the user's decrypted, exact 3.0.2 executable.
No app bytes, Home documents or keys belong in this source repository.
"""
import argparse, hashlib, json, struct
from pathlib import Path
from unicorn import Uc, UC_ARCH_ARM64, UC_MODE_ARM, UC_HOOK_CODE
from unicorn.arm64_const import *

EXPECTED = '6c74a0bf6609ccf3ce5a132318eaf31f411743d8a78ce2b760ed15c813788eb2'

def verify(binary):
    data = Path(binary).read_bytes()
    assert hashlib.sha256(data).hexdigest() == EXPECTED, 'Exact supported original executable required'
    uc = Uc(UC_ARCH_ARM64, UC_MODE_ARM)
    at = 32
    for _ in range(struct.unpack_from('<I', data, 16)[0]):
        command, size = struct.unpack_from('<II', data, at)
        if command == 0x19:
            address, length, offset, file_length = struct.unpack_from('<QQQQ', data, at + 24)
            if file_length:
                uc.mem_map(address, (length + 4095) & ~4095)
                uc.mem_write(address, data[offset:offset + file_length])
        at += size
    stack, stop = 0x70000000, 0x71000000
    uc.mem_map(stack, 0x10000)
    uc.mem_map(stop, 4096)
    def reg(n): return uc.reg_read(UC_ARM64_REG_X0 + n)
    def cstring(address):
        result = bytearray()
        while True:
            b = uc.mem_read(address, 1)
            if b == b'\0': return result.decode()
            result += b
            address += 1
    captures = []
    def hook(uc, address, size, _):
        if address == stop:
            uc.emu_stop(); return
        if address == 0x100480e2c:  # objc_storeStrong
            uc.mem_write(reg(0), struct.pack('<Q', reg(1)))
        elif address in (0x100480df0, 0x100480dcc): pass  # retain return/value
        elif address in (0x100480dc0, 0x100480e5c): pass  # release
        elif address == 0x100480d9c:
            selector = cstring(reg(1))
            if selector.startswith('lightHSLSetWithDestination:'):
                captures.append(dict(target=reg(2), lightness=reg(3), hue=reg(4), saturation=reg(5)))
            elif selector not in ('setRetryCount:', 'transitionTime', 'delay', 'retryCount'):
                raise AssertionError('Unexpected native call: ' + selector)
            uc.reg_write(UC_ARM64_REG_X0, 0)
        else: return
        uc.reg_write(UC_ARM64_REG_PC, uc.reg_read(UC_ARM64_REG_LR))
    uc.hook_add(UC_HOOK_CODE, hook)
    for hue, saturation, lightness in [(240, 80, 60), (120, 95, 25), (300, 72, 44)]:
        uc.reg_write(UC_ARM64_REG_SP, stack + 0x8000)
        uc.reg_write(UC_ARM64_REG_LR, stop)
        uc.reg_write(UC_ARM64_REG_X0, stack + 0x100)
        for n, value in [(1, 0), (2, 0x234), (3, 0), (4, 0), (5, 0), (6, 0), (7, 0)]:
            uc.reg_write(UC_ARM64_REG_X0 + n, value)
        for register, value in [(UC_ARM64_REG_S0, hue), (UC_ARM64_REG_S1, saturation), (UC_ARM64_REG_S2, lightness)]:
            uc.reg_write(register, struct.unpack('<I', struct.pack('<f', value))[0])
        uc.emu_start(0x1002e4070, stop, count=2000)
        assert captures[-1] == dict(target=0x234, lightness=lightness, hue=hue, saturation=saturation)
    # Execute the original scene enum's raw-value initializer and getter.
    # Its fifth case (HSL colour) is persisted as 8, not its ordinal 4.
    raw, tag, output = stack+0x200, stack+0x208, stack+0x210
    uc.mem_write(raw, struct.pack('<q', 8))
    uc.reg_write(UC_ARM64_REG_X0, raw);uc.reg_write(UC_ARM64_REG_X8, tag)
    uc.reg_write(UC_ARM64_REG_LR, stop);uc.emu_start(0x10012d448, stop, count=100)
    assert uc.mem_read(tag,1)==b'\x04'
    uc.reg_write(UC_ARM64_REG_X20, tag);uc.reg_write(UC_ARM64_REG_X8, output)
    uc.reg_write(UC_ARM64_REG_LR, stop);uc.emu_start(0x10012d478, stop, count=100)
    assert struct.unpack('<q',uc.mem_read(output,8))[0]==8
    print(json.dumps({'native_app_color_contract': 'degrees / percent', 'actual_native_calls': captures, 'native_scene_hsl_device_type':8, 'passed': True}))

if __name__ == '__main__':
    parser = argparse.ArgumentParser();parser.add_argument('executable');verify(parser.parse_args().executable)
