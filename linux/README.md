# Linux: Rulemak workstation punctuation

English stays `us` (Colemak-DH positions are sent by the Corne firmware).
Russian letters retain the installed Rulemak-CDH mapping. This overlay
requires the `rulemak-cdh-xkb` base package and its `rulemak_cdh(basic)` symbols,
including the existing Ctrl-to-Latin behavior. It does not replace package files.

Install `xkb/symbols/rulemak_workstation` into
`~/.config/xkb/symbols/rulemak_workstation`, then use in niri:

```kdl
xkb {
    layout "us,rulemak_workstation"
    variant ",basic"
}
```

## Input

- Russian Shift+1…0: `! " № ; % : ? * ( )`.
- Plain comma/period stay comma/period; Shift gives semicolon/colon.
- Letters on SYMBOL remain: `ъ Ъ`, `ш Ш`, `щ Щ`, `э Э`, `ё Ё`.
- On SYMBOL, the left inner Alt is **sticky AltGr**. Hold it to access
  the US symbols on `= + [ ] { } \ |` and grave/tilde positions.
- For base-layer extras, tap that AltGr key, release SYMBOL, then press
  comma for `«`, period for `»`, or slash for `…`. Sticky AltGr is one-shot
  and defaults to a 1-second timeout; enter the next key promptly.
- AltGr+minus gives `—`; AltGr+Shift+minus gives `–`.
- AltGr+Shift+comma/period gives `< >`.
- AltGr+shifted digits retains the US symbols `! @ # $ % ^ & * ( )`.
- Base, home-row and thumb Alt remain ordinary left Alt. NUMBER, CONFIG
  and GAME bindings are unchanged. English input is unchanged by the host overlay.

The Corne AltGr change requires rebuilding/flashing the keymap. Installing
the Linux overlay alone does not add a new key to existing firmware.
If ZMK Studio has stored a custom keymap, it can override flashed defaults;
do not clear Bluetooth settings just to update a keymap.

No macOS files are modified. macOS would need a corresponding Option layer
before promising identical additional-symbol behavior there.

## Verification

```sh
python tests/test_linux_symbols.py
xkbcli compile-keymap --layout us,rulemak_workstation --variant ,basic --test
niri validate
```

`--baseline` tests the previously installed layout and demonstrates the
punctuation failures. These tests compile and evaluate real libxkbcommon
keymaps; they do not inject input into applications or verify flashed firmware.

## Revert the Linux overlay

Restore niri to `layout "us,ru"` and `variant ",rulemak_cdh"`.
The package's original map is left intact. The extra user symbol file can remain
unused. Firmware changes can be reverted independently.
