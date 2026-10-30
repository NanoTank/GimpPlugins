# SPDX-License-Identifier: MIT
"""Statische Kontrollen fuer den nativen GEGL-Test.

Echte C-Kompilierung und Flatpak-Menütest erfolgen auf dem Zielrechner.
"""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
SRC = (ROOT / "gegl/nanotank-gegl-test/nanotank-gegl-test.c").read_text(encoding="utf-8")
BUILD = (ROOT / "scripts/build-gegl.sh").read_text(encoding="utf-8")
DEPLOY = (ROOT / "scripts/deploy-gegl-flatpak.sh").read_text(encoding="utf-8")

class GeglContract(unittest.TestCase):
    def test_unique_operation(self):
        self.assertIn('"nanotank:gegl-test"', SRC)
        self.assertIn('"gimp:menu-path",  "<Image>/Filters/NanoTank/"', SRC)

    def test_ui_property(self):
        self.assertIn('property_double (brightness, "Helligkeit", 0.0)', SRC)
        self.assertIn('value_range (-1.0, 1.0)', SRC)

    def test_alpha_preserved(self):
        self.assertIn('dst[3] = src[3];', SRC)

    def test_build_in_gimp_sdk(self):
        self.assertIn('flatpak run --devel --command=sh org.gimp.GIMP', BUILD)
        self.assertIn('pkg-config --exists gegl-0.4', BUILD)

    def test_deploy_in_flatpak_data(self):
        self.assertIn('flatpak run --command=sh org.gimp.GIMP', DEPLOY)
        self.assertIn('gegl-0.4/plug-ins', DEPLOY)
        self.assertNotIn('GEGL_PATH=', DEPLOY)

if __name__ == "__main__":
    unittest.main()
