#!/usr/bin/env python3
"""Exercise the actual XKB map, without injecting keys into the desktop.

Requires libxkbcommon and the installed rulemak-cdh-xkb base package.
Run with --baseline to demonstrate the old punctuation failures.
"""
import ctypes as c
from pathlib import Path
import sys
import re

lib = c.CDLL('libxkbcommon.so.0')
def bind(name, restype, *args):
    fn = getattr(lib, name)
    fn.restype, fn.argtypes = restype, args
    return fn

ptr, u, s = c.c_void_p, c.c_uint32, c.c_char_p
class Names(c.Structure):
    _fields_ = [(k, s) for k in ('rules', 'model', 'layout', 'variant', 'options')]

ctx = bind('xkb_context_new', ptr, c.c_int)(0)
bind('xkb_context_include_path_clear', None, ptr)(ctx)
bind('xkb_context_include_path_append', c.c_int, ptr, s)(
    ctx, str(Path(__file__).resolve().parents[1] / 'linux/xkb').encode())
bind('xkb_context_include_path_append_default', c.c_int, ptr)(ctx)
baseline = '--baseline' in sys.argv
names = Names(b'evdev' if baseline else b'corne', b'pc105', b'us,ru' if baseline else b'us,rulemak_workstation',
              b',rulemak_cdh' if baseline else b',basic', b'' if baseline else b'corne:symbols')
keymap = bind('xkb_keymap_new_from_names', ptr, ptr, c.POINTER(Names), c.c_int)(ctx, c.byref(names), 0)
assert keymap, 'XKB compilation failed'
state = bind('xkb_state_new', ptr, ptr)(keymap)
key = bind('xkb_keymap_key_by_name', u, ptr, s)
mod = bind('xkb_keymap_mod_get_index', u, ptr, s)
update = bind('xkb_state_update_mask', u, ptr, u, u, u, u, u, u)
sym = bind('xkb_state_key_get_one_sym', u, ptr, u)
utf32 = bind('xkb_keysym_to_utf32', u, u)
failures, checks = [], 0

def check(name, expected, modifiers=(), group=1):
    global checks
    mask = 0
    for modifier in modifiers:
        idx = mod(keymap, modifier.encode())
        assert idx != 0xffffffff, modifier
        mask |= 1 << idx
    update(state, mask, 0, 0, 0, 0, group)
    actual = chr(utf32(sym(state, key(keymap, name.encode()))))
    checks += 1
    if actual != expected:
        failures.append(f'{name} {modifiers} group={group}: {actual!r} != {expected!r}')

for i, char in enumerate('!"№;%:?*()', 1):
    check(f'AE{i:02}', char, ('Shift',))
for i, char in enumerate('!@#$%^&*()', 1):
    check(f'AE{i:02}', char, ('Shift',), 0)
    check(f'AE{i:02}', char, ('Shift', 'Control'))
    check(f'AE{i:02}', char, ('Shift', 'Mod5'))
for name, lower, upper in [('AE12','ъ','Ъ'), ('AD11','ш','Ш'), ('AD12','щ','Щ'),
                           ('BKSL','э','Э'), ('TLDE','ё','Ё'), ('AC10','ю','Ю'), ('AC11','ь','Ь')]:
    check(name, lower)
    check(name, upper, ('Shift',))
for name, lower, upper in [('AE12','=','+'), ('AD11','[','{'), ('AD12',']','}'),
                           ('BKSL','\\','|'), ('TLDE','`','~'), ('AC11',"'",'"')]:
    check(name, lower, ('Mod5',))
    check(name, upper, ('Mod5','Shift'))
for name, low, high in [('AB08',',',';'), ('AB09','.',':'), ('AB10','/','?')]:
    check(name, low)
    check(name, high, ('Shift',))
for name, char, mods in [('AE11','—',('Mod5',)), ('AE11','–',('Mod5','Shift')),
                          ('AB08','«',('Mod5',)), ('AB09','»',('Mod5',)),
                          ('AB10','…',('Mod5',)), ('AB08','<',('Mod5','Shift')),
                          ('AB09','>',('Mod5','Shift'))]:
    check(name, char, mods)
for name, char in [('AC01','a'), ('AB03','c'), ('AB04','v'), ('AB01','z')]:
    check(name, char, ('Control',))
    check(name, char.upper(), ('Control','Shift'))
# Integration regression: consume the actual firmware SYMBOL bindings, not
# merely an idealized host key. Implicit Shift caused uppercase Russian letters.
firmware = (Path(__file__).resolve().parents[1] / 'config/corne_choc_pro.keymap').read_text()
symbol_block = firmware.split('raise_layer {', 1)[1].split('bindings = <', 1)[1].split('>;', 1)[0]
rows = [re.findall(r'&kp\s+(\w+)', line) for line in symbol_block.splitlines() if '&kp' in line]
legacy = {'PLUS': ('AE12', ('Shift',)), 'LBRC': ('AD11', ('Shift',)),
          'RBRC': ('AD12', ('Shift',)), 'PIPE': ('BKSL', ('Shift',)),
          'TILDE': ('TLDE', ('Shift',))}
for row, column, en, ru in [(0,0,'~','ё'), (2,7,'+','+'), (2,8,'{','«'),
                            (2,9,'}','»'), (2,10,'|','—'), (2,11,'~','…')]:
    code = rows[row][column]
    if code in legacy:
        name, implicit = legacy[code]
    else:
        assert re.fullmatch(r'F1[3-8]', code), f'Unsupported SYMBOL code {code}'
        name, implicit = 'FK' + code[1:], ()
    check(name, en, implicit, 0)
    check(name, ru, implicit, 1)
    check(name, ru.upper(), tuple(set(implicit) | {'Shift'}), 1)

print(f'{checks} checks; {len(failures)} failures')
print('\n'.join(failures))
sys.exit(bool(failures))
