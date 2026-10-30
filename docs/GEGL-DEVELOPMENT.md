# NanoTank GEGL – nativer Funktionstest für GIMP 3.2 Flatpak

Status: **Quellcode und Build-/Deploy-Skripte bereit, noch nicht in der
Nutzerinstallation kompiliert oder in GIMP aufgerufen**.

## Voraussetzungen

- GIMP 3.2.6 als Flatpak, GEGL 0.4.72 (Versionen gemäß Nutzer-Debugausgabe).
- Passendes `org.gnome.Sdk//51`, vom Nutzer installiert.
- Der GIMP-Flatpak-Build muss die GEGL-Entwicklungsdateien enthalten.
- Git-Klon unter `/home/mic/Projekte/GimpPlugins` – die Sandbox kann den
  Projektordner laut vorheriger Diagnose lesen.

## Bauen

GIMP zunächst **vollständig schließen**, dann:

```bash
cd ~/Projekte/GimpPlugins
git switch feature/plugin-foundation
git pull --ff-only
bash scripts/build-gegl.sh
```

Das Skript verwendet `flatpak run --devel --command=sh org.gimp.GIMP`,
damit Compiler und `pkg-config` aus dem passenden GNOME-SDK und die
GEGL-Entwicklungsdateien aus `/app` stammen. Wenn eines davon fehlt,
bricht es ab – **kein stiller Rückfall auf Host-GEGL**.

Ausgabe: `output/gegl/nanotank-gegl-test.so`.

## In GIMP integrieren

```bash
bash scripts/deploy-gegl-flatpak.sh
bash scripts/diagnose-gegl-flatpak.sh
```

Das Deploy-Skript legt die GEGL-Module im **über Flatpak ermittelten**
`$XDG_DATA_HOME/gegl-0.4/plug-ins/` ab; zuerst ein Symlink
auf den Projekt-`output`-Ordner, ersatzweise eine eindeutig markierte
Kopie. Es ändert **nicht** `GEGL_PATH`, überschreibt keine fremden
Dateien und verweigert die Aktualisierung, solange GIMP läuft.

Bei Symlink-Installation reicht nach einem Build ein GIMP-Neustart.
Bei Kopie zusätzlich das Deploy-Skript ausführen.

Anschließend GIMP starten, Bild öffnen und
**Filter → NanoTank → GEGL-Test** suchen.

## Funktionsprüfung

1. Regler **Helligkeit** erscheint (-1 bis +1).
2. Bei 0: Bild unverändert.
3. Bei positivem Wert: Bild heller; bei negativem Wert: dunkler.
4. Auswahlbereich testen; außerhalb der Auswahl darf nichts verändert werden.
5. Transparente Ebene testen; Alpha soll unverändert bleiben.
6. Effekt bestätigen, Ebenenfilter wieder öffnen und Wert verändern.
7. Undo/Redo sowie Speichern/erneutes Öffnen einer XCF testen.

Das ist **nur ein Funktionstest**, keine produktive Helligkeitskorrektur.
Auf einer Kopie eines Testbilds arbeiten, denn ein fehlerhaftes natives
C-Modul kann GIMP abstürzen lassen.

## Fehlerdiagnose

Falls der Build fehlschlägt, die **erste Fehlermeldung** des Skripts
sichern. Falls der Build funktioniert, GIMP aber den Filter nicht zeigt:

```bash
bash scripts/diagnose-gegl-flatpak.sh
flatpak run --verbose org.gimp.GIMP 2>&1 | tee /tmp/nanotank-gegl-gimp.log
```

Dieser Sichtbarkeitstest beweist noch keine GEGL-Registrierung.
Abnahme erst mit sichtbarem Menüpunkt und tatsächlich funktionierendem Regler.

Referenzen:
- https://developer.gimp.org/resource/writing-a-filter/point-filter-operation/
- https://developer.gimp.org/resource/writing-a-filter/gimp-integration/
- https://developer.gimp.org/resource/writing-a-filter/architecture-install/
