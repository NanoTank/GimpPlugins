# SPDX-License-Identifier: MIT
"""Static contracts only. Passing these does not establish successful C build or GIMP load."""
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
SRC = (ROOT / "gegl/nanotank-glass/nanotank-glass.c").read_text(encoding="utf-8")
BUILD = (ROOT / "scripts/build-gegl.sh").read_text(encoding="utf-8")
DEPLOY = (ROOT / "scripts/deploy-gegl-flatpak.sh").read_text(encoding="utf-8")
DIAGNOSE = (ROOT / "scripts/diagnose-gegl-flatpak.sh").read_text(encoding="utf-8")
UNDEPLOY = (ROOT / "scripts/undeploy-gegl-flatpak.sh").read_text(encoding="utf-8")
SPEC = (ROOT / "docs/GLASS-SPEC.md").read_text(encoding="utf-8")


class GlassContract(unittest.TestCase):
    def test_native_gimp_operation(self):
        self.assertIn("#define GEGL_OP_AREA_FILTER", SRC)
        self.assertIn('#define GEGL_OP_C_SOURCE nanotank-glass.c', SRC)
        self.assertIn('"nanotank:glass"', SRC)
        self.assertIn('"gimp:menu-path",  "<Image>/Filters/NanoTank/"', SRC)
        self.assertIn('"gimp:menu-label", "Glass"', SRC)

    def test_german_ui(self):
        for label in ("Glasdicke", "Kantenwölbung",
                      "Untergrund und Alpha erhalten", "Glasdeckkraft",
                      "Tönungsfarbe", "Tönungsintensität", "Reflexion",
                      "Glanzlichter", "Interne Lichtbrechung", "Wölbung"):
            with self.subTest(label=label):
                self.assertIn(label, SRC)

    def test_alpha_preserve_contract(self):
        self.assertIn("out[3] = o->preserve_alpha", SRC)
        self.assertIn("? center[3]", SRC)
        self.assertIn(": (gfloat) o->glass_opacity;", SRC)

    def test_local_area_processing(self):
        for symbol in ("GEGL_ABYSS_NONE", "RGBA float",
                       "gegl_buffer_get (input", "gegl_buffer_set (output",
                       "get_bounding_box", "gegl_operation_set_format"):
            self.assertIn(symbol, SRC)

    def test_shared_build_keeps_systemtest_default(self):
        self.assertIn('MODULE="${1:-nanotank-gegl-test}"', BUILD)
        self.assertIn("nanotank-gegl-test|nanotank-glass", BUILD)
        self.assertIn("flatpak run --devel --command=sh org.gimp.GIMP", BUILD)
        self.assertIn("pkg-config --exists gegl-0.4", BUILD)
        self.assertIn("-lm", BUILD)

    def test_single_module_operations(self):
        self.assertIn('files=("$OUT/$1.so")', DEPLOY)
        self.assertIn("nanotank-glass", DIAGNOSE)
        self.assertIn("nanotank-glass", UNDEPLOY)
        self.assertIn('[ "$wanted" = all ] || [ "$name" = "$wanted.so" ] || continue', UNDEPLOY)
        self.assertNotIn("GEGL_PATH=", DEPLOY)

    def test_deploy_waits_for_transient_sdk_sandbox_without_disabling_guard(self):
        self.assertIn("gimp_instance_running()", DEPLOY)
        self.assertIn("for attempt in 1 2 3 4 5 6 7 8 9 10", DEPLOY)
        self.assertIn("if gimp_instance_running; then", DEPLOY)
        self.assertIn("Zum Schutz vor Austausch geladener GEGL-Module", DEPLOY)
        self.assertIn("flatpak ps --columns=instance,application,pid,child-pid", DEPLOY)

    def test_limits_and_acceptance_documented(self):
        for keyword in ("Hintergrund", "Auswahlkontur", "Undo/Redo", "Flatpak"):
            self.assertIn(keyword, SPEC)


if __name__ == "__main__":
    unittest.main()
