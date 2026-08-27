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
