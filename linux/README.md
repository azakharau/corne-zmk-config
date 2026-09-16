# Linux: Rulemak workstation punctuation

English stays `us` (Colemak-DH positions are sent by the Corne firmware).
Russian letters retain the installed Rulemak-CDH mapping. This overlay
requires the `rulemak-cdh-xkb` base package and its `rulemak_cdh(basic)` symbols,
including the existing Ctrl-to-Latin behavior. It does not replace package files.

Install the files from `xkb/symbols/` into `~/.config/xkb/symbols/`,
and `xkb/rules/corne` into `~/.config/xkb/rules/corne`, then use in niri:

```kdl
xkb {
    rules "corne"
    layout "us,rulemak_workstation"
    variant ",basic"
    options "corne:symbols"
}
```

## Input

- Russian Shift+1…0: `! " № ; % : ? * ( )`.
- Plain comma/period stay comma/period; Shift gives semicolon/colon.
- Unshifted letter keys on SYMBOL remain `ъ ш щ э ё`; explicit Shift
  produces capitals. No SYMBOL position produces a capital just by itself.
- Six SYMBOL positions use unshifted F13..F18 transport codes. They are
  translated by the host, avoiding the ambiguity between firmware-injected
  Shift and a Shift physically held by the user:

  | Position | English | Russian |
  |---|---|---|
  | Top-left (F18) | `~` | `ё` (Shift: `Ё`) |
  | Former PLUS (F13) | `+` | `+` |
  | Former LBRC (F14) | `{` | `«` |
  | Former RBRC (F15) | `}` | `»` |
  | Former PIPE (F16) | `\|` | `—` |
  | Bottom-right (F17) | `~` | `…` |

  F1..F12 are unchanged. F13..F18 are reserved by this host configuration
  and no longer act as function keys on other keyboards either. This option
  requires English in group 1 and Russian in group 2, exactly as shown above.
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

No macOS files are modified. **Do not expect this firmware's six new symbol
positions to work on an unconfigured Mac or other computer.** It needs an
equivalent F13..F18 adapter for both input languages on that host. This is a
Linux-targeted firmware change, not yet a cross-platform-complete solution.

## Verification

```sh
python tests/test_linux_symbols.py
xkbcli compile-keymap --rules corne --layout us,rulemak_workstation --variant ,basic --options corne:symbols --test
niri validate
```

`--baseline` tests the previously installed layout and demonstrates the
punctuation failures. These tests compile and evaluate real libxkbcommon
keymaps; they do not inject input into applications or verify flashed firmware.

## Revert the Linux overlay

Remove the `rules "corne"` and `options "corne:symbols"` entries and
restore niri to `layout "us,ru"` and `variant ",rulemak_cdh"`.
The package's original map is left intact. The extra user symbol file can remain
unused. Firmware F13..F18 bindings must also be reverted for their old behavior.

## References

- [ZMK modifier functions and implicit modifiers](https://zmk.dev/docs/keymaps/modifiers)
- [libxkbcommon custom rules and options](https://xkbcommon.org/doc/current/custom-configuration.html)
