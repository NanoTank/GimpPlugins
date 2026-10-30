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
- Native GEGL-`.so` werden künftig aus einem separaten, versionierten
  Build-Prozess nach `output/gegl/` ausgegeben, nicht in Git committet.
- Nach neuen oder aktualisierten Plug-ins GIMP komplett neu starten.
- `scripts/install-flatpak.sh` ist idempotent und überschreibt keine fremden Installationen.
- Fehler nicht durch Löschen der bestehenden GIMP-Konfiguration kaschieren.
- Kein automatischer Merge, Release oder Eingriff in andere Plug-ins ohne Abnahme.

## Definition of Done pro Filter

1. Lädt in GIMP Flatpak und ist unter Filter → NanoTank aufrufbar.
2. Tests für 8-bit, nach Möglichkeit 16/32-bit, Alpha, Auswahl und Größenränder.
3. Wirkung und Regler durch Nutzer visuell abgenommen.
4. Undo/Redo, XCF und Export nachvollziehbar getestet.
5. Dokumentation von Parametern, Installation und bekannten Grenzen.
