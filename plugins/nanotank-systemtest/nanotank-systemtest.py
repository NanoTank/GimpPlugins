#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
"""Harmloser GIMP-3-Systemtest für Filter > NanoTank."""

import sys

import gi

gi.require_version("Gimp", "3.0")
from gi.repository import Gimp, GLib


PROCEDURE = "nanotank-systemtest"


class NanoTankSystemTest(Gimp.PlugIn):
    def do_query_procedures(self):
        return [PROCEDURE]

    def do_set_i18n(self, procedure_name):
        return False

    def do_create_procedure(self, name):
        if name != PROCEDURE:
            return None

        procedure = Gimp.ImageProcedure.new(
            self, name, Gimp.PDBProcType.PLUGIN, self.run, None
        )
        procedure.set_image_types("*")
        procedure.set_menu_label("Systemtest")
        procedure.add_menu_path("<Image>/Filters/NanoTank/")
        procedure.set_documentation(
            "Prüft die NanoTank-Menüintegration",
            "Zeigt eine Meldung an und verändert das Bild nicht.",
            name,
        )
        procedure.set_attribution("NanoTank", "NanoTank", "2026")
        return procedure

    def run(self, procedure, run_mode, image, drawables, config, run_data):
        Gimp.message("NanoTank-Systemtest erfolgreich: Plug-in geladen.")
        return procedure.new_return_values(
            Gimp.PDBStatusType.SUCCESS, GLib.Error()
        )


Gimp.main(NanoTankSystemTest.__gtype__, sys.argv)
