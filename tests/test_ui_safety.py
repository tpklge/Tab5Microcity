import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class UiSafetyTests(unittest.TestCase):
    def test_never_install_null_lvgl_transition_descriptor(self):
        """LVGL dereferences an installed transition when a widget is pressed."""
        source = (ROOT / "main/ui/application.cpp").read_text()
        compact = "".join(source.split())
        self.assertNotIn("lv_obj_set_style_transition(b,nullptr", compact)

    def test_reserved_multicell_tiles_do_not_erase_anchor_sprite(self):
        source = (ROOT / "main/ui/application.cpp").read_text()
        compact = "".join(source.split())
        self.assertIn("footprintWidth>1&&c.stage>1&&!constructionView", compact)
        self.assertIn("if(!coveredByAnchor)add(id", compact)


if __name__ == "__main__":
    unittest.main()
