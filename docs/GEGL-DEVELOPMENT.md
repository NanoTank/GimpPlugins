# NanoTank Glass – native Entwicklung

Zielumgebung: GIMP 3.2.6 Flatpak, GEGL 0.4.72, passendes GIMP-Flatpak-SDK
(zur Referenzabnahme `org.gnome.Sdk//51`). Entwickler: **NanoTank**.

## Bauen

GIMP vor dem Ersetzen geladener Module vollständig schließen:

```bash
bash scripts/build-gegl.sh nanotank-glass &&
bash scripts/build-gegl.sh nanotank-glass-refraction
```

Ohne Argument wird `nanotank-glass` gebaut. Quellen beider Module befinden
sich in `gegl/nanotank-glass/`, Ausgaben in `output/gegl/`.
Isolierte Testbuilds über `NANOTANK_GEGL_BUILD_DIR` bleiben möglich.

Das Skript verwendet `flatpak run --devel --command=sh org.gimp.GIMP` und
prüft `pkg-config`, GIMPs `/app/lib/pkgconfig/gegl-0.4.pc` sowie
`/app/include/gegl-0.4/gegl-op.h`. Der Quellordner steht explizit im
Include-Suchpfad, weil `gegl-op.h` die Operationsquelle erneut einbindet.
Kein Rückfall auf Host-GEGL. Bei SDK-Problemen:

```bash
bash scripts/diagnose-gegl-sdk.sh
```

## Installation und Diagnose

Nur nach erfolgreichem Build, bei geschlossenem GIMP:

```bash
bash scripts/deploy-gegl-flatpak.sh nanotank-glass &&
bash scripts/deploy-gegl-flatpak.sh nanotank-glass-refraction &&
bash scripts/diagnose-gegl-flatpak.sh nanotank-glass
```

Das Ziel wird innerhalb der Flatpak-Umgebung ermittelt:
`$XDG_DATA_HOME/gegl-0.4/plug-ins/`. Eigene Symlinks werden bevorzugt,
andernfalls eindeutig markierte Kopien. Fremde Dateien werden nicht ersetzt;
kein globales `GEGL_PATH`. Eine auslaufende SDK-Instanz wird kurz abgewartet,
eine weiterhin laufende GIMP-Instanz verhindert das Deployment.

Gezielt entfernen: `bash scripts/undeploy-gegl-flatpak.sh nanotank-glass`
oder `nanotank-glass-refraction`. Ohne Argument werden beide Glass-Module
entfernt. Dies ist keine erforderliche Maßnahme für die normale Nutzung.

Der Snapshot-Helfer wird separat mit `scripts/build.sh nanotank-glass-background`
gebaut und mit `scripts/install-flatpak.sh nanotank-glass-background` installiert.
Eine lesbare Datei beweist noch keine GEGL-Registrierung oder Menüfunktion.

## Technische und praktische Prüfung

`tests/glass/` enthält die nativen Render-, Material-, Geometrie-, Cache-,
Hintergrundbrechungs- und GIMP-/XCF-Prüfungen. Details und Aufrufparameter
stehen in [GLASS-P1.4.2.md](GLASS-P1.4.2.md) und den dort referenzierten
historischen Glass-Dokumenten. Testbuilds und Testprofile bleiben unter
`output/`; vorhandene Nutzerbilder und GIMP-Einstellungen bleiben erhalten.

Praktisch auf einer Bildkopie prüfen: Glass-Menü, deutsche Parameter,
Vorschau, Alpha, Auswahl, Ebenenfilter erneut öffnen, Undo/Redo sowie
XCF speichern und neu laden. Frühere technische Prüfungen und die bestätigte
P1.4.2-Referenz ersetzen keine erneute interaktive Abnahme nach der Bereinigung.
