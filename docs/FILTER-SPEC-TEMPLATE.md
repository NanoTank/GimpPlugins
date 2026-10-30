# NanoTank [Filtername] — Spezifikation

> Vorlage kopieren nach `docs/[FILTERNAME]-SPEC.md` und passend zum Auftrag
> ausfüllen. Offene Punkte markieren, nicht mit erfundenen Annahmen füllen.

## Zweck und Anwendernutzen

- **Name / Menü:** `Filter → NanoTank → [Filtername]`
- **Ausgangssituation:** Welche Ebene, Auswahl oder welches Bild liegt vor?
- **Erwünschtes Ergebnis:** Was muss sichtbar anders sein?
- **Anwendungsfälle:** Mindestens zwei realistische Beispiele.
- **Nicht-Ziele:** Was gehört ausdrücklich nicht zur ersten Version?

## Technische Architektur

- **GEGL-Operation:** `nanotank:[name]` (GEGL in C als Standard)
- **Quellordner:** `gegl/nanotank-[name]/`
- **Build-Ausgabe:** `output/gegl/nanotank-[name].so`
- **GIMP-/GEGL-Zielversion:** GIMP 3.2.6 Flatpak; GEGL ABI in Runtime prüfen
- **Eingabedaten:** Aktive Ebene, Alpha, Auswahlmaske, Grenzen, benötigter
  Hintergrund/andere Ebenen; prüfen, ob GEGL die Daten tatsächlich erhält.
- **Optionaler Helfer:** Python/Script-Fu nur, falls mit GEGL allein nicht
  möglich; begründen und die Bildberechnung in GEGL belassen.
- **Bearbeitungsmodell:** Live-Vorschau, nicht-destruktiver Ebenenfilter,
  Speichern/Laden von XCF, Undo/Redo.

## Bedienung und Parameter

| Parameter (deutsch) | Bedeutung | Bereich/Einheit | Standard | Wirkung/Grenzfall |
| --- | --- | --- | --- | --- |
| [Parameter 1] | [Beschreibung] | [Bereich] | [Wert] | [Null, Minimum, Maximum] |
| [Parameter 2] | [Beschreibung] | [Bereich] | [Wert] | [Grenzfall] |

- **Presets:** Namen, konkrete Parameterwerte, Custom-Modus nach Änderung.
- **Vorschau:** Was muss beim Verstellen unmittelbar sichtbar sein?
- **Fehlerfälle:** Was passiert bei leerer Auswahl, winziger Form, fehlendem
  Alpha oder fehlendem Hintergrund?

## Bildqualität und Grenzen

- Arbiträre Formen, harte/weiche Auswahlkanten und saubere Alpha-Komposition.
- 8/16/32-bit beziehungsweise float, Farbmanagement/HDR soweit relevant.
- Kanten, Ebenengrenzen, große Bilder und Transparenz prüfen.
- Physikalische/algorithmische Annahmen und ihre Grenzen offen benennen.
- Bei teuren Effekten: Qualität erhalten; Optimierung erst nach korrekter
  Funktion und visueller Abnahme.

## Umsetzung in kleinen Schritten

1. Architektur-/Schnittstellenprüfung und minimaler GEGL-Prototyp.
2. Technischer lokaler Build gegen das passende Flatpak-SDK.
3. Laden in **bestehendem** GIMP, Menü/Regler/Live-Vorschau bestätigen.
4. Visuelle Abnahme durch Nutzer; nur relevante Punkte gezielt verbessern.
5. Lokale Regressionen und Dokumentation vor Freigabe.

## Akzeptanzkriterien

- [ ] Eigener Menüeintrag unter `Filter → NanoTank`.
- [ ] Tatsächliche Bildwirkung entspricht den Beispielen.
- [ ] Auswahl und Alpha bleiben korrekt (auch weiche Kanten).
- [ ] Regler/Presets funktionieren und haben sinnvolle Grenzwerte.
- [ ] Vorschau, Undo/Redo, XCF-Speichern/Neuladen, erneutes Bearbeiten geprüft.
- [ ] Andere NanoTank-Filter und die GIMP-Installation sind unverändert.
- [ ] Einfache Reproduktionsschritte und Grenzen dokumentiert.
- [ ] Nutzer hat das visuelle Ergebnis ausdrücklich abgenommen.

## Arbeitsweise und Übergabe

- Nur eigenen Feature-Branch verwenden; `main` nicht selbständig mergen.
- Builds, Tests und Testbilder lokal; **keine automatischen GitHub-Actions**.
- Ausgabe nach jeder Phase: Commit, Änderungen, tatsächlich geprüfte Tests,
  visuelle Grenzen, knappe Anleitung für den nächsten Nutzertest.
