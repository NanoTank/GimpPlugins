# NanoTank Glass – Anforderungen für einen separaten Entwicklungs-Chat

Ein **echter GIMP-3-Filter** unter `Filter → NanoTank → Glass`, nicht
ein eigenständiges externes Bildprogramm.

## Ziel

Aus einer beliebigen GIMP-Auswahl eine überzeugende Glasoberfläche machen.
Gleicher Filter für **opake Schilder/Buttons** (z. B. Metall, Papier, Grafik
unter einer gläsernen Auflage) und **halb- bis vollständig transparente**
Glaselemente. Darunterliegende Inhalte und Alphakanal müssen bewusst und
getrennt behandelt werden; ein Glas-Transparenzregler darf nicht unbeabsichtigt
einen opaken Untergrund transparent machen.

## Wichtige Parameter

- Glasdicke, Kantenrundung/Fase, Kantenbreite
- Reflexionsstärke, Richtung/Position und Glanzlichter
- Transparenz/Deckkraft der Glasauflage; Tönungsfarbe und Tönungsintensität
- Lichtbrechung, Wölbung, optionaler Lupeneffekt
- Oberflächenrauheit/Milchglas als optionale spätere Erweiterung
- Sinnvolle Presets, z. B. Glas auf Schild, klarer Glasbutton, getöntes Glas

## Qualitätskriterien

- Freie Auswahl, weiche Auswahlkanten, runde oder eckige Formen und Alpha.
- Live-Vorschau, verständliche deutsche Beschriftungen.
- Möglichst nicht-destruktive Bearbeitung, Undo/Redo, XCF-Projekt erhalten.
- Physikalisch plausible Glasoptik (z. B. Fresnel, Brechung); keine Behauptung
  einer vollständigen 3D-Lichtsimulation aus einem einzigen 2D-Bild.
- Geeignete Integration für GIMP 3.2.6 Flatpak, mit gemeinsamer NanoTank-Basis.
- Zuerst Architektur und realistische Machbarkeit des Hintergrundzugriffs
  prüfen, dann minimalen Prototyp für Nutzerabnahme erstellen.

**Hier nur die Basis; Umsetzung in einem eigenen Feature-Branch/Chat.**
