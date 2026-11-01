# Projektbereinigung – nur Glass P1.4.2

Entwickler: **NanoTank**. Datum: 2026-11-01.
Lokaler Branch: `cleanup/glass-only-p142`.
Basis: `c233520eb3d31957028c51fef92582802ae9060d` auf
`feature/nanotank-glass`, ausdrücklich vom Nutzer als verbindliche P1.4.2 bestätigt.

## Sicherheitsprüfung und Git

Vor Änderungen war das Arbeitsverzeichnis sauber. Alle lokalen Branches und
Remote-Tracking-Refs wurden geprüft; die GitHub-App bestätigte die vier aktuellen
Remote-Branches und den identischen Glass-Commit. Kein Netzwerkzugriff über gh
oder Git-Fetch, kein Push, Merge, Release, Reset oder Umschreiben der Historie.

Nur lokal lagen sechs nicht übertragene Experiment-Commits:
`f7c8d6d`, `7ba7351`, `87a02bf`, `67f2672` (FireLab),
`e9cb700`, `4ac526e` (BrushCut P0/P0b). FireLab war also bereits lokal in Git
versioniert. Der Vergleich mit Glass ergab ausschließlich Experiment-Dateien,
README-Ergänzungen und FireLab-Buildanbindung; keine Glass-Komponenten.
Die Bereinigung beginnt direkt bei P1.4.2, nicht bei einem älteren Glass-Stand.

Nach Prüfung lokal gelöscht:

- `feature/nanotank-brushcut` (`4ac526e`), einschließlich der geerbten FireLab-Historie.
- `feature/nanotank-firelab` (`67f2672`).
- `feature/plugin-foundation` (`2a712e4`), vollständig in Glass enthalten.

`main` und `feature/nanotank-glass` bleiben unverändert erhalten.
Remote-Branches bleiben alle bestehen: `main`, `feature/nanotank-glass`,
`feature/nanotank-glass-p0`, `feature/plugin-foundation`.
Alte Dateien können weiterhin in historischer Git-Historie bzw. bestehenden
Remote-Branches enthalten sein; es fand keine Historienbereinigung statt.
Autor und Committer werden vor dem lokalen Commit als **NanoTank** mit der
bereits konfigurierten GitHub-No-Reply-Adresse geprüft; keine globale Konfiguration.

## Entfernt und erhalten

Im aktiven Projektbaum entfallen sämtliche BrushCut-/FireLab-Quellen,
P0-/P0b-/Fluid-Prototypen, Tests, Spezifikationen, Messdaten, Galerien,
Build-Anbindungen und experimentellen Abhängigkeitseinträge. Sie wurden nicht
in den Bereinigungsbranch übernommen. Ihre unversionierten Ausgaben, Python-Caches,
GIMP-Plug-in-Builds und das FireLab-Galerielog wurden gezielt gelöscht.
Keine globalen Python-Pakete oder gemeinsam verwendeten Laufzeiten deinstalliert.

Entfernt wurden außerdem die Quellen und Vertragstests von Systemtest und
GEGL-Test sowie dessen lokales Modul. Bei geschlossenem GIMP wurde der verifizierte,
eigene installierte GEGL-Test-Symlink entfernt. Die alten FireLab-/Systemtest-
Prozedureinstellungen wurden gezielt gelöscht. Weitere Installationen dieser
Experimente wurden im GIMP-Flatpak-Benutzerverzeichnis nicht gefunden.
Keine Glass-Einstellungen, Nutzer-XCF oder fremden Plug-ins entfernt.

Vollständig bytegleich zur Referenz erhalten:

- `gegl/nanotank-glass/`: beide Operationen, Geometrie, Optik, Cache.
- `plugins/nanotank-glass-background/`: vollständiger Snapshot-Helfer.
- `tests/glass/`: sämtliche nativen und GIMP-/XCF-Tests.

Alle Glass-Entwicklungsdokumente bleiben erhalten. Lediglich die Forderung nach
erhaltenem Demonstrationsfilter wurde aus der historischen Spezifikation entfernt.
README, GEGL-Anleitung und Projektregeln beschreiben jetzt den Glass-only-Bestand.
Die gemeinsamen Skripte behalten SDK-Erkennung, Include-/Linkerflags, gezielte
Builds, isolierte Testziele und Eigentumsprüfungen. Modulauswahl und Standardziel
sind auf Glass begrenzt; Entfernung/Installation nativer Module schützt laufendes
GIMP. Der statische Glass-Test wurde an das neue Standardziel angepasst.

## Tatsächlich lokal geprüft

- `git diff c233520 -- gegl/nanotank-glass plugins/nanotank-glass-background tests/glass`:
  leer. Damit bleiben auch Operation-IDs, Parameterdefinitionen, Standardwerte,
  Wertebereiche und vorhandene Preset-/XCF-Semantik unverändert.
- Beide Module erfolgreich im GIMP-Flatpak-SDK gebaut: GEGL **0.4.72**,
  `pkg-config` und `gegl-op.h` aus `/app`; isolierte Ausgabe
  `output/glass-cleanup/gegl/`. Installierte Glass-Module wurden nicht ersetzt.
- Python-Helfer gebaut, 8 verbliebene statische Glass-Vertragstests bestanden.
- Syntax aller gemeinsamen Shellskripte und `git diff --check` bestanden.
- Isolierter Installations-, Wiederholungs- und Entfernungstest mit temporären
  Zielverzeichnissen bestanden: nur Glass installiert, fremde Dateien erhalten.
  Flatpak-Sichtbarkeit wurde für diesen Skripttest simuliert, keine Neuinstallation
  des produktiven Glass vorgenommen.
- `bash tests/glass/run-p142.sh`: Cachetreffer, Profilwechsel, Alphaänderung und
  Restore, leere/kleine Komponenten und konkurrierender Zugriff bestanden;
  Ausgabe bitidentisch zum ungecachten vollständigen Feld.
- `run-p13.sh`: analytischer Kreis, positive/negative Höhe, Plateau,
  Dickenunabhängigkeit, Komponenten, Alpha, Tiles und XML bestanden.
  Bestehende XML-Farbserialisierungsabweichungen unverändert separat gemeldet:
  Altmodus maximal 0,0001602, Pixelprofil 0,0000032.
- `run-p14.sh`: native Eingänge, Snell, Vorzeichen, Alpha/HDR, abgeschaltete
  Geometrie, opake Auflage und Tiles bestanden.
- `run-gimp-p14.sh`: echter GIMP-3.2.6-Batchtest, Helferregistrierung und Aufruf,
  63 geänderte native Pixel, Quellschutz, Rückkopplungsverweigerung und
  Snapshot-XCF-Neuladen bestanden.
- Vorhandenes `gimp-performance.py` mit neu gebauten Modulen in isoliertem Profil:
  fünf synthetische Fälle bis 4K, Material-/Geometrieänderungen am echten
  Ebenenfilter, `NORMAL_FILTER_XCF_RELOAD_OK` und `GIMP_PERFORMANCE_OK` bestätigt.
  Eine anschließend ergänzte separate Operationsabfrage benötigte eine eigene
  GEGL-Initialisierung; mit `Gegl.init(None)` separat erfolgreich wiederholt.
- GIMP-Laufzeitabfrage: ausschließlich `nanotank:glass` und
  `nanotank:glass-refraction` registriert. Alle **17** normalen Glass-Parameter
  verfügbar. Keine Systemtest-/FireLab-Prozedur im isolierten Testprofil.
- Abschließende Dateinamenssuche: keine BrushCut-, FireLab-, Systemtest- oder
  GEGL-Test-Dateien mehr im aktiven Projekt einschließlich `output/` und im
  geprüften GIMP-Flatpak-Benutzerverzeichnis. Historische Git-Objekte ausgenommen.

Lokale Logs/Builds liegen unversioniert unter `output/`. Kein umfangreicher neuer
Benchmarkvergleich und keine Änderung der Glass-Bildberechnung.

## Verbleibende Nutzerabnahme

Die Tests belegen technische Ladbarkeit in GIMP und synthetische XCF-Persistenz.
Eine neue interaktive Prüfung von Menü, Vorschau, Undo/Redo und vorhandenen
Nutzer-XCF wurde hier nicht durchgeführt. Keine vollständige praktische Abnahme
behauptet. Die zuvor bestätigte P1.4.2 bleibt der unveränderte Referenzstand.

Minimaler Test: GIMP neu starten, vorhandene Glass-XCF auf einer Kopie öffnen,
**Filter → NanoTank → Glass** bzw. den vorhandenen Ebenenfilter bearbeiten,
Material- und Geometrieregler ändern, Undo/Redo prüfen, Kopie speichern/neuladen.
Den vorhandenen Snapshot-Helfer einmal auf einem synthetischen Bild aufrufen.
Zusätzliche eingestellte NanoTank-Filter sollen nicht mehr erscheinen.
Keine Neuinstallation von Glass erforderlich. Nach lokalem Commit stoppen und
Nutzerabnahme abwarten; kein Push oder Merge.
