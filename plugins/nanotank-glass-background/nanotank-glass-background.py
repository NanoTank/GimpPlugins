#!/usr/bin/env python3
# SPDX-License-Identifier: MIT – NanoTank
"""Thin GIMP input orchestration. Every material pixel is computed in native GEGL.

GIMP 3.2.6 refuses aux-input effects as persistent drawable filters. Therefore
this explicitly creates a new, labelled snapshot image; the source is untouched.
"""
import json
import sys
import gi

gi.require_version("Gimp", "3.0")
gi.require_version("Gegl", "0.4")
from gi.repository import Gimp, Gegl, GLib, GObject

Gegl.init(None)

PROCEDURE = "nanotank-glass-background"
MARKER = "nanotank-glass-snapshot"
DOUBLES = [
    ("surface-height", "Wölbungshöhe (Pixel)", -1000, 1000, 50),
    ("surface-extent", "Wölbungsbereich (%)", 1, 100, 100),
    ("edge-radius", "Kantenrundung (Pixel)", 0, 64, 4),
    ("thickness", "Glasdicke (Pixel)", 1, 64, 12),
    ("ior", "Brechungsindex", 1, 2.5, 1.5),
    ("refraction-strength", "Hintergrundbrechung", 0, 3, 1),
    ("reflection", "Reflexion", 0, 1, .75),
    ("highlights", "Glanzlichter", 0, 1, .6),
    ("light-angle", "Lichtrichtung (Grad)", 0, 360, 225),
    ("tint-strength", "Tönungsintensität", 0, 1, .2),
]


def make_snapshot(image, source, settings, show=True):
    """Project only layers below the top-level source in an isolated duplicate.

    No source visibility/selection/pixels are changed. The returned duplicate
    keeps original layers hidden below the explicit full-background replacement.
    No background/result is ever fed back into its own projection.
    """
    if not isinstance(source, Gimp.Layer):
        raise ValueError("Bitte eine einzelne Formebene wählen, keinen Kanal und keine Ebenenmaske.")
    if image.get_parasite(MARKER):
        raise ValueError("Dieses Bild ist bereits ein Glass-Snapshot. Bitte das Ausgangsbild wählen und neu berechnen.")
    if source.get_parent() is not None or source.is_group():
        raise ValueError("P1.4 benötigt eine einzelne Formebene außerhalb einer Gruppe. Gruppen im Hintergrund sind erlaubt.")
    if not source.get_visible() or source.get_mode() != Gimp.LayerMode.NORMAL:
        raise ValueError("Die Formebene muss sichtbar sein und den Ebenenmodus Normal verwenden.")
    position = image.get_item_position(source)
    offsets = source.get_offsets()
    ox, oy = offsets[-2:]
    if ox < 0 or oy < 0 or ox + source.get_width() > image.get_width() or oy + source.get_height() > image.get_height():
        raise ValueError("Die Formebene muss für diesen Snapshot-Test vollständig innerhalb des Bildes liegen.")
    result = image.duplicate()
    if result is None:
        raise RuntimeError("Das Testbild konnte nicht dupliziert werden.")
    try:
        layers = result.get_layers()
        # Read the original object's alpha, not the translucent output of an
        # already applied Glass coating. Change only the duplicated filter.
        for existing in layers[position].get_filters():
            if existing.get_operation_name() == "nanotank:glass":
                existing.set_visible(False)
        visibility = [layer.get_visible() for layer in layers]
        result.undo_disable()
        Gimp.Selection.none(result)  # only the duplicate; active selection is not a geometry input
        for index, layer in enumerate(layers):
            if index <= position:
                layer.set_visible(False)
        background = Gimp.Layer.new_from_visible(result, result, "Glass Hintergrundbrechung – Snapshot")
        for layer in layers:
            layer.set_visible(False)
        layers[position].set_visible(True)
        shape = Gimp.Layer.new_from_visible(result, result, "Glass Form – Snapshot-Referenz")
        for index, layer in enumerate(layers):
            layer.set_visible(visibility[index] if index < position else False)
        result.insert_layer(background, None, position)
        result.insert_layer(shape, None, position + 1)
        background.set_visible(True)
        shape.set_visible(False)
        effect = Gimp.DrawableFilter.new(background, "nanotank:glass-refraction", "Glass Hintergrundbrechung (Snapshot)")
        if effect is None:
            raise RuntimeError("Native Operation nanotank:glass-refraction fehlt. Bitte das Glass-Refraction-Modul bauen und installieren.")
        config = effect.get_config()
        for key, value in settings.items():
            config.set_property(key, value)
        effect.set_aux_input("aux", shape)
        effect.update()
        # Public merge API consumes the transient effect. Unlike append_filter,
        # this path supports aux inputs; originals belong to the other image.
        background.merge_filter(effect)
        if effect.is_valid():
            raise RuntimeError("Der native Effekt konnte nicht in den neuen Snapshot übernommen werden.")
        background.update(0, 0, background.get_width(), background.get_height())
        if not background.is_valid():
            raise RuntimeError("Die Snapshot-Ausgabe konnte nicht erzeugt werden.")
        metadata = {"version": "P1.4", "workflow": "snapshot-no-live-background",
                    "parameters": {key: value for key, value in settings.items() if key != "tint"}}
        metadata["tint-rgba"] = tuple(settings["tint"].get_rgba())
        result.attach_parasite(Gimp.Parasite.new(MARKER, 1, json.dumps(metadata).encode("utf-8")))
        result.set_selected_layers([background])
        result.undo_enable()
        if show:
            Gimp.Display.new(result)
            Gimp.displays_flush()
        return result
    except Exception:
        result.delete()
        raise


class GlassBackground(Gimp.PlugIn):
    def do_query_procedures(self):
        return [PROCEDURE]

    def do_set_i18n(self, name):
        return False, None, None

    def do_create_procedure(self, name):
        if name != PROCEDURE:
            return None
        proc = Gimp.ImageProcedure.new(self, name, Gimp.PDBProcType.PLUGIN, self.run, None)
        proc.set_image_types("RGB*")
        proc.set_menu_label("Glass – Hintergrundbrechung (Snapshot)…")
        proc.add_menu_path("<Image>/Filters/NanoTank/")
        proc.set_documentation("Glas mit gebrochenem Hintergrund als neues Snapshot-Bild",
            "Ausgangsbild bleibt unverändert. Hintergrund wird nicht live aktualisiert; keine aktive Auswahlgeometrie.", name)
        proc.set_attribution("NanoTank", "NanoTank", "2026")
        help_text = {
            "surface-height": "Geometrische Höhe: positiv konvex, negativ konkav",
            "surface-extent": "100 bis zur Mitte; kleinere Werte lassen eine ebene Mitte",
            "edge-radius": "Unabhängiger gerundeter Außenrand; 0 für ein reines Linsenprofil",
            "thickness": "Positive Mindestdicke des angenäherten optischen Wegs",
            "ior": "Glas etwa 1,5; Wasser etwa 1,33; 1 ohne Brechung",
            "refraction-strength": "0 ohne Hintergrundverzerrung, 1 für den Modellmaßstab",
            "reflection": "Stärke der geometrieabhängigen Umgebungsreflexion",
            "highlights": "Helligkeit der reflektierten Studioflächen",
            "light-angle": "Dreht die reflektierte Beleuchtung",
            "tint-strength": "Absorption des übertragenen Hintergrundlichts",
        }
        for key, label, low, high, default in DOUBLES:
            proc.add_double_argument(key, label, help_text[key], low, high, default, GObject.ParamFlags.READWRITE)
        proc.add_boolean_argument("preserve-alpha", "Glasauflage auf vorhandenem Material",
            "Ein: Material erhalten, ohne Brechung darunterliegender Ebenen", False, GObject.ParamFlags.READWRITE)
        proc.add_color_argument("tint", "Tönungsfarbe", "Absorption im Glas", False,
            Gegl.Color.new("#bcdff7"), GObject.ParamFlags.READWRITE)
        return proc

    def run(self, procedure, run_mode, image, drawables, config, data):
        if len(drawables) != 1:
            return procedure.new_return_values(Gimp.PDBStatusType.CALLING_ERROR,
                GLib.Error("Bitte genau eine sichtbare Formebene wählen."))
        if run_mode == Gimp.RunMode.INTERACTIVE:
            gi.require_version("GimpUi", "3.0")
            from gi.repository import GimpUi
            GimpUi.init(PROCEDURE)
            dialog = GimpUi.ProcedureDialog.new(procedure, config, "Glass – neues Snapshot-Bild")
            dialog.fill(None)
            accepted = dialog.run()
            dialog.destroy()
            if not accepted:
                return procedure.new_return_values(Gimp.PDBStatusType.CANCEL, GLib.Error())
        try:
            keys = [row[0] for row in DOUBLES] + ["preserve-alpha", "tint"]
            make_snapshot(image, drawables[0], {key: config.get_property(key) for key in keys},
                show=run_mode == Gimp.RunMode.INTERACTIVE)
        except Exception as exc:
            return procedure.new_return_values(Gimp.PDBStatusType.EXECUTION_ERROR, GLib.Error(str(exc)))
        return procedure.new_return_values(Gimp.PDBStatusType.SUCCESS, GLib.Error())


if __name__ == "__main__":
    Gimp.main(GlassBackground.__gtype__, sys.argv)
