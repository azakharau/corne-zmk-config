# Corne Choc Pro ZMK configuration

Personal ZMK firmware configuration for a wireless Corne Choc Pro split
keyboard built around the nRF52840. The repository contains the board
definition, keymap, power and Bluetooth settings, and a GitHub Actions build.

## Highlights

- separate left and right firmware targets;
- Nice!View support;
- ZMK Studio support on the USB-connected left half;
- reset images for recovering stored settings;
- tuned sleep, Bluetooth, and split-connection behavior;
- a Colemak-DH-oriented keymap with host-side Rulemak support.

## Build artifacts

Every push and pull request runs the upstream ZMK user-config workflow. A
successful run produces firmware for:

- left half with Nice!View and ZMK Studio;
- right half with Nice!View;
- left and right settings-reset images.

To build a fork, enable GitHub Actions and push to its default branch. The
workflow and west manifest pin the same ZMK revision for reproducibility.

## Flashing

1. Download the artifact from a successful GitHub Actions run.
2. Put one keyboard half into its UF2 bootloader.
3. Copy the matching left or right `.uf2` file to the mounted drive.
4. Repeat for the other half.

Use the settings-reset images only when intentionally clearing stored ZMK
settings, then flash the normal images again.

## CONFIG layer

Hold both thumb layer keys (LOWER + RAISE). Positions below refer to the
base Colemak-DH letters, independent of the host's English/Russian layout.

| Base key | CONFIG action |
| --- | --- |
| X / C / D | Bluetooth profiles 1 / 2 / 3 (ZMK indices 0 / 1 / 2) |
| R / S | Bluetooth profiles 4 / 5 (ZMK indices 3 / 4) |
| Q, held for 1 second | Clear pairing for the currently selected profile only |
| W / F | Prefer USB / Bluetooth output |
| B | Unlock ZMK Studio (also retained on NUMBER + B) |
| G | Toggle GAME |
| J | Toggle RGB underglow |
| L / U | Decrease / increase RGB brightness |
| M | Next RGB effect |
| N / E | Decrease / increase hue |

The profile keys occupy the NUMBER layer's physical 1–5 positions. A short
Q tap does nothing. Unassigned CONFIG keys are blocked, apart from the
transparent layer thumbs. GAME remains below NUMBER, SYMBOL and CONFIG,
so these controls (including CONFIG + G to leave GAME) remain accessible.

Firmware layer order is CLMKDH, GAME, NUMBER, SYMBOL, CONFIG. If a saved
ZMK Studio keymap overrides the firmware defaults, restore the stock keymap
in Studio after flashing; do not use settings-reset images just to update
the layout, since they also erase stored settings and pairings.

## Host layout

The keymap is designed to work with a host-side Rulemak layout adapted for
Colemak-DH scan codes. The repository links to the upstream
[`Rulemak_MacOS`](https://github.com/nakrein/Rulemak_MacOS) project but does not
redistribute its files because that project does not declare a license. Install
and adapt a host layout separately if you need Russian input.

## Provenance

The custom Corne Choc Pro board definition was originally developed by Alexei
Torgashov and carries SPDX MIT headers. This configuration retains that
attribution while adding project-specific keymap and stability changes. See
[`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md).

## License

Repository content is available under the MIT License unless a file states a
different license. See [`LICENSE`](LICENSE).
