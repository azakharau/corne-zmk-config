"""Validate physical-key routing through the real keymap's layer stack.

These are keymap contract checks, not a firmware/hardware timing simulator.
Run: python -m unittest discover -s tests -p test_config_layout.py -v
"""
from pathlib import Path
import re
import unittest

SOURCE = (Path(__file__).resolve().parents[1] / "config/corne_choc_pro.keymap").read_text()
SOURCE = re.sub(r"/\*.*?\*/|//[^\n]*", "", SOURCE, flags=re.S)
LAYERS = []
for name, body in re.findall(r"(\w+_layer)\s*\{(.*?)\};", SOURCE, re.S):
    bindings = re.search(r"bindings\s*=\s*<(.*?)>;", body, re.S).group(1)
    LAYERS.append((name, [tuple(item.split()) for item in re.findall(r"&[^&]+", bindings)]))
INDEX = {name: i for i, (name, _) in enumerate(LAYERS)}


def route(position, *active):
    for index in sorted({0, *(INDEX[name] for name in active)}, reverse=True):
        binding = LAYERS[index][1][position]
        if binding != ("&trans",):
            return binding
    raise AssertionError(f"No binding at position {position}")


class ConfigLayout(unittest.TestCase):
    def test_matrix_and_named_layer_indices_match(self):
        for name, bindings in LAYERS:
            self.assertEqual(len(bindings), 46, name)
        for symbol, name in [("LOWER", "lower_layer"), ("RAISE", "raise_layer"),
                             ("CONFIG", "config_layer"), ("GAME", "game_layer")]:
            value = int(re.search(rf"#define\s+{symbol}\s+(\d+)", SOURCE).group(1))
            self.assertEqual(value, INDEX[name], symbol)

    def test_profile_keys_follow_number_positions_even_in_game(self):
        for extra in [(), ("game_layer",)]:
            for position, digit, profile in [(30, "N1", "0"), (31, "N2", "1"),
                                             (32, "N3", "2"), (16, "N4", "3"),
                                             (17, "N5", "4")]:
                with self.subTest(extra=extra, position=position):
                    self.assertEqual(route(position, "lower_layer", *extra), ("&kp", digit))
                    self.assertEqual(route(position, "lower_layer", "raise_layer", "config_layer", *extra),
                                     ("&bt", "BT_SEL", profile))

    def test_config_actions_and_inert_unused_keys(self):
        wanted = {1: ("&bt_clear_hold", "BT_CLR", "0"), 2: ("&out", "OUT_USB"),
                  3: ("&out", "OUT_BLE"), 5: ("&studio_unlock",),
                  8: ("&rgb_ug", "RGB_TOG"), 9: ("&rgb_ug", "RGB_BRD"),
                  10: ("&rgb_ug", "RGB_BRI"), 16: ("&bt", "BT_SEL", "3"),
                  17: ("&bt", "BT_SEL", "4"), 19: ("&tog", "GAME"),
                  22: ("&rgb_ug", "RGB_EFF"), 23: ("&rgb_ug", "RGB_HUD"),
                  24: ("&rgb_ug", "RGB_HUI"), 30: ("&bt", "BT_SEL", "0"),
                  31: ("&bt", "BT_SEL", "1"), 32: ("&bt", "BT_SEL", "2")}
        for extra in [(), ("game_layer",)]:
            for position in range(46):
                if position in (41, 44):  # Held layer keys must retain release behavior.
                    continue
                with self.subTest(extra=extra, position=position):
                    self.assertEqual(route(position, "lower_layer", "raise_layer", "config_layer", *extra),
                                     wanted.get(position, ("&none",)))

    def test_layer_thumbs_remain_reachable_in_game(self):
        for active in [("game_layer",), ("lower_layer", "game_layer"),
                       ("raise_layer", "game_layer"),
                       ("lower_layer", "raise_layer", "config_layer", "game_layer")]:
            self.assertEqual(route(41, *active), ("&lt", "LOWER", "ESCAPE"))
            self.assertEqual(route(44, *active), ("&lt", "RAISE", "SEMICOLON"))


if __name__ == "__main__":
    unittest.main()
