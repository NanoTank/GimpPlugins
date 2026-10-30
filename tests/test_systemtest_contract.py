# SPDX-License-Identifier: MIT
"""Statische Vertragsprüfungen – kein Ersatz für einen echten GIMP-Start."""

import ast
import pathlib
import unittest


ROOT = pathlib.Path(__file__).resolve().parents[1]
PATH = ROOT / "plugins" / "nanotank-systemtest" / "nanotank-systemtest.py"


class SystemTestContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.source = PATH.read_text(encoding="utf-8")
        cls.tree = ast.parse(cls.source)
        cls.plugin = next(
            node
            for node in cls.tree.body
            if isinstance(node, ast.ClassDef) and node.name == "NanoTankSystemTest"
        )

    def test_i18n_return_matches_gimp_python_gi_contract(self):
        method = next(
            node
            for node in self.plugin.body
            if isinstance(node, ast.FunctionDef) and node.name == "do_set_i18n"
        )
        returned = [node for node in ast.walk(method) if isinstance(node, ast.Return)]
        self.assertEqual(len(returned), 1)
        self.assertEqual(ast.literal_eval(returned[0].value), (False, None, None))

    def test_menu_location(self):
        self.assertIn('"<Image>/Filters/NanoTank/"', self.source)

    def test_procedure_name(self):
        self.assertIn('PROCEDURE = "nanotank-systemtest"', self.source)


if __name__ == "__main__":
    unittest.main()
