# NanoTank Glass P1.4.2

Entwicklungsbasis für genau eine Bilderweiterung: **NanoTank Glass** unter
**Filter → NanoTank → Glass**. Ziel: Kubuntu, GIMP 3.2.6 Flatpak und dessen
GEGL 0.4.72. Entwickler: **NanoTank**. Lizenz: [MIT](LICENSE).

Verbindliche, vom Nutzer bestätigte Referenz: `feature/nanotank-glass`,
Commit `c233520eb3d31957028c51fef92582802ae9060d` (P1.4.2).
Die Projektbereinigung verändert keine Glass-Bildberechnung, Parameter oder IDs.

## Bestand

| Pfad | Zweck |
| --- | --- |
| `gegl/nanotank-glass/` | Glass, Hintergrundbrechung, Geometrie, Optik und Cache |
| `plugins/nanotank-glass-background/` | Dünner GIMP-Helfer für Hintergrund-Snapshots |
| `tests/glass/` | Native Render-, Geometrie-, Cache- und GIMP-/XCF-Prüfungen |
| `scripts/` | Glass-Build, Installation, Entfernung und Diagnose |
| `docs/GLASS-*.md` | Glass-Anforderungen, Entwicklung und bekannte Grenzen |
| `output/` | Unversionierte lokale Builds und Glass-Testausgaben |

Glass besteht aus zwei nativen Modulen und einem Snapshot-Helfer. Das Modul
`nanotank:glass-refraction` gehört zum Snapshot-Workflow und ist kein zusätzlicher
produktiver Filter. Der normale Ebenenfilter bekommt nicht automatisch den
sichtbaren Hintergrund anderer Ebenen; dafür dient der Snapshot-Helfer.
Snapshots aktualisieren sich nicht automatisch bei späteren Hintergrundänderungen.

## Bauen und installieren

GIMP vor Änderungen an geladenen Modulen **vollständig schließen**.

```bash
bash scripts/build-gegl.sh nanotank-glass &&
bash scripts/build-gegl.sh nanotank-glass-refraction &&
bash scripts/build.sh nanotank-glass-background
```

Die nativen Builds verwenden ausschließlich das passende GIMP-Flatpak-SDK,
keine Host-GEGL-Bibliotheken. Ohne Argument baut `build-gegl.sh` Glass.

Nach erfolgreichem Build:

```bash
bash scripts/deploy-gegl-flatpak.sh nanotank-glass &&
bash scripts/deploy-gegl-flatpak.sh nanotank-glass-refraction &&
bash scripts/install-flatpak.sh nanotank-glass-background
```

Anschließend GIMP neu starten. Die Skripte verwenden eigene Symlinks oder
markierte Kopien und überschreiben keine fremden Installationen.
Für den Python-Helfer kann alternativ `output/gimp-plug-ins` dieses Klons als
zusätzlicher GIMP-Plug-in-Suchpfad eingetragen bleiben.

## Prüfen und bedienen

```bash
python3 -m unittest discover -s tests -p 'test_*contract.py'
bash tests/glass/run-p142.sh
bash scripts/diagnose-gegl-flatpak.sh nanotank-glass
bash scripts/diagnose-flatpak.sh
```

Weitere gezielte Render-/GIMP-Tests und bekannte optische Grenzen:
[Glass P1.4](docs/GLASS-P1.4.md), [P1.4.1](docs/GLASS-P1.4.1.md),
[P1.4.2](docs/GLASS-P1.4.2.md). Historische Prüfstände bleiben als Dokumentation
erhalten; die aktuelle Referenz ist die oben genannte P1.4.2.

Vorhandene Glass-XCF auf einer Kopie öffnen, Effekt erneut bearbeiten,
Material- und Geometrieregler prüfen, Undo/Redo sowie Speichern/Neuladen testen.
Technische Batchtests ersetzen keine neue interaktive Nutzerabnahme.

Build-Diagnose: [GEGL-Entwicklung](docs/GEGL-DEVELOPMENT.md).
Projektregeln: [AGENTS.md](AGENTS.md), [Entwicklung](docs/DEVELOPMENT.md).
Bereinigungsnachweis: [Glass-only](docs/GLASS-ONLY-CLEANUP.md).
Kein automatischer Push, Merge, Release oder CI-Build.
