# Gemeinsame Entwicklungsregeln

- Repository: https://github.com/NanoTank/GimpPlugins
- GitHub-Arbeitsbereich ist ausschließlich der persönliche Account `NanoTank` und dessen Repositories. Keine Lese- oder Schreibaktionen auf Repositories fremder Organisationen, in denen der Nutzer Mitglied ist.
- GIMP 3.2.6 Flatpak als primäre Zielplattform.
- Alle GIMP-Prozeduren unter `<Image>/Filters/NanoTank/`, wenn die API dies zulässt.
- Eindeutige Prozedur-IDs und Binärdateinamen mit Präfix `nanotank-`.
- Ein Filter pro Verzeichnis / Feature-Branch. `main` enthält nur abgenommene Versionen.
- Visuell prüfbare Vorschau und sinnvolle Parameter statt fest einkodierter Werte.
- Auswahlmaske, Alphakanal, Ebenen, Undo/Redo und XCF-Speichern/Laden prüfen.
- Bei nativen GEGL-Filtern ABI und Bibliotheksversion **innerhalb** der Flatpak-Laufzeit verifizieren.
- Python-GIMP-Plug-ins werden in `plugins/nanotank-name/nanotank-name.py` geführt.
  `scripts/build.sh` kopiert sie nach `output/gimp-plug-ins/`.
- Native GEGL-Quellen liegen in `gegl/`; Build mit GIMPs passendem Flatpak-SDK
  über `scripts/build-gegl.sh`, Ausgabe nach `output/gegl/` (nicht in Git).
- GEGL-Ladung über Flatpak-Benutzerverzeichnis mittels
  `scripts/deploy-gegl-flatpak.sh` (eigenen Symlink bevorzugen, sonst verwaltete
  Kopie); kein globales `GEGL_PATH` setzen oder fremde Filter überschreiben.
- Vor Update/Entfernen nativer GEGL-Dateien GIMP vollständig schließen.
- Es gelten zwei getrennte Abnahmen: Python-Menütest bestanden,
  nativer GEGL-Menü-/Reglertest noch nicht auf Zielsystem ausgeführt.
- Nach neuen oder aktualisierten Plug-ins GIMP komplett neu starten.
- `scripts/install-flatpak.sh` ist idempotent und überschreibt keine fremden Installationen.
- Fehler nicht durch Löschen der bestehenden GIMP-Konfiguration kaschieren.
- Kein automatischer Merge, Release oder Eingriff in andere Plug-ins ohne Abnahme.
- GitHub ist primär Austausch- und Versionsspeicher für Nutzer, Codex und ChatGPT.
- **Keine automatischen GitHub-Actions-Pipelines** bei Push, Pull Requests oder kleinen Änderungen. Builds und Regressionen lokal auf dem Entwicklerrechner bzw. in Codex, bei Bedarf gezielt manuell.
- CI-/Workflow-Dateien in `.github/workflows/` nur nach vorheriger ausdrücklicher Freigabe des Nutzers; keine automatischen Releases oder Artefakt-Uploads.

## Definition of Done pro Filter

1. Lädt in GIMP Flatpak und ist unter Filter → NanoTank aufrufbar.
2. Tests für 8-bit, nach Möglichkeit 16/32-bit, Alpha, Auswahl und Größenränder.
3. Wirkung und Regler durch Nutzer visuell abgenommen.
4. Undo/Redo, XCF und Export nachvollziehbar getestet.
5. Dokumentation von Parametern, Installation und bekannten Grenzen.
