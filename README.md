# NanoTank GIMP Plugins

Gemeinsame, offene Entwicklungsbasis für GIMP-3-Filter und Plug-ins.
Zielumgebung: **Kubuntu 26.04, GIMP 3.2.6 als Flatpak** (Stand Oktober 2026).
Lizenz: MIT (siehe [LICENSE](LICENSE)).

## Ziel

Alle Erweiterungen erscheinen möglichst unter **Filter → NanoTank**.
Ein einzelnes Plug-in wird unabhängig entwickelt und getestet.
Python/GIMP-3-API, Script-Fu oder native GEGL-Operationen werden abhängig von den Anforderungen eingesetzt.

## Verzeichnisstruktur

| Pfad | Zweck |
| --- | --- |
| `plugins/` | Python-3-/GIMP-3-Quellen, jeweils eigener Ordner |
| `scripts/` | Bauen, Symlink-Installation und Entfernen |
| `docs/` | Gemeinsame Konventionen und Filteranforderungen |
| `output/gimp-plug-ins/` | Erzeugte GIMP-Plug-in-Ordner (nicht versioniert) |
| `output/gegl/` | Zukünftige native GEGL-`.so`-Filter (nicht versioniert) |

## Erster Test

Im lokalen Git-Klon:

```bash
bash scripts/build.sh
bash scripts/install-flatpak.sh
flatpak run org.gimp.GIMP
```

In GIMP ein Bild öffnen, danach **Filter → NanoTank → Systemtest** auswählen.
Der Test zeigt nur eine Meldung an und verändert keine Pixel.

Bei späteren Änderungen: `bash scripts/build.sh` ausführen und **GIMP neu starten**.
Die einmal eingerichteten Symlinks zeigen automatisch auf den neuen Inhalt.
Auch Vorabversionen können so getestet werden; ein Merge auf `main` ist nicht nötig.

## Installationspfade für GIMP 3.2 Flatpak

- GIMP-Plug-in-Ordner: `~/.var/app/org.gimp.GIMP/config/GIMP/3.2/plug-ins/`
- GEGL-Operationen: `~/.var/app/org.gimp.GIMP/data/gegl-0.4/plug-ins/`

Die Installation legt **pro NanoTank-Plug-in** einen eigenen Symlink an;
existierende Dateien oder fremde Erweiterungen werden nicht überschrieben.
Niemals den gesamten GIMP-Plug-in-Ordner durch einen Symlink ersetzen.

Entfernen: `bash scripts/uninstall-flatpak.sh` (nur Links dieses Klons).
Die Quellen und `output/` bleiben dabei erhalten.

Wenn ein Symlink im Flatpak nicht erreichbar ist: prüfen, ob der Git-Klon
aus der Sandbox zugänglich ist (bei dieser GIMP-Installation wurde
`filesystem=host` gemeldet). Nicht ungeprüft globale Flatpak-Berechtigungen
erweitern; gegebenenfalls gezielten Dateisystemzugriff konfigurieren.

## Native GEGL-Filter

`output/gegl/` ist für spätere `nanotank-*.so`-Artefakte vorgesehen.
Das Installationsskript verlinkt vorhandene Dateien in den GEGL-Ordner.
**Es kompiliert noch keine nativen Filter.**
Diese müssen mit den Bibliotheken/der ABI von GIMPs Flatpak (aktuell
GEGL 0.4.72) gebaut und getestet werden. Die auf dem Kubuntu-Host
installierte GEGL-Version muss nicht kompatibel sein.

GEGL-Filter sollten den Menüpfad `<Image>/Filters/NanoTank/` verwenden.
Die erforderliche Registrierung wird im jeweiligen Filterprojekt getestet.

## Entwicklungsregeln

Siehe [docs/DEVELOPMENT.md](docs/DEVELOPMENT.md).
Glass-Anforderungen: [docs/GLASS-BRIEF.md](docs/GLASS-BRIEF.md).

**Status:** Grundgerüst und Python-Menütest vorhanden; Ausführung im echten
GIMP-Flatpak muss lokal abgenommen werden. Noch kein Bildfilter implementiert.
