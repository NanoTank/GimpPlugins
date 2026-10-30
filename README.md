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
| `scripts/` | Bauen, Sandbox-Check, Installation (Symlink oder verwaltete Kopie) und Entfernen |
| `docs/` | Gemeinsame Konventionen und Filteranforderungen |
| `output/gimp-plug-ins/` | Erzeugte GIMP-Plug-in-Ordner (nicht versioniert) |
| `output/gegl/` | Zukünftige native GEGL-`.so`-Filter (nicht versioniert) |

## Empfohlener Entwicklungsmodus (GIMP 3.2 Flatpak)

**Direkt aus `output/` laden – ohne Symlink oder Kopie.**

1. Einmalig in GIMP: **Bearbeiten → Einstellungen → Ordner → Plug-ins**.
   Dort den folgenden Ordner als **zusätzlichen** Suchpfad hinzufügen
   (bestehende Pfade beibehalten):

   `/home/mic/Projekte/GimpPlugins/output/gimp-plug-ins`

   Alternativ kann derselbe Pfad passend zum lokalen Git-Klon angegeben werden.
   In der GIMP-Flatpak-Sandbox auf der Zielmaschine ist dieser Projektpfad
   nachweislich lesbar (Diagnose 2026-10-30). Ob GIMP den Systemtest erfolgreich
   registriert, muss nach der Änderung in GIMP noch geprüft werden.

2. Im lokalen Git-Klon:

   ```bash
   bash scripts/build.sh
   ```

3. GIMP vollständig beenden und neu starten, ein Bild öffnen und
   **Filter → NanoTank → Systemtest** aufrufen.

Bei späteren Python-Plugin-Änderungen reicht `bash scripts/build.sh` und ein
Neustart von GIMP. Die GIMP-Menüintegration kann so auch Feature-Branches und
Vorabversionen testen, ohne diese nach `main` zu mergen.

## Alternative Installation und Diagnose

Es gibt weiterhin `scripts/install-flatpak.sh` und
`scripts/uninstall-flatpak.sh`, um einzelne NanoTank-Plug-ins in einem
GIMP-Benutzerverzeichnis zu verlinken oder eine eindeutig markierte Kopie
anzulegen/zu entfernen. **Auf der Zielmaschine zeigte die Flatpak-Diagnose aber,
dass sogar eine installierte Kopie im Benutzerverzeichnis aus der Sandbox nicht
erreichbar ist.** Diese Methode deshalb vorerst **nicht** als funktionierend
betrachten und lieber den direkten Suchpfad verwenden.

Die GIMP-3.2-Benutzerkonfiguration liegt normalerweise unter
`$XDG_CONFIG_HOME/GIMP/3.2/` (im Flatpak-Kontext), die GIMP-Benutzer-Plug-ins
unter `plug-ins/`. Welchen Pfad die laufende GIMP-Instanz tatsächlich scannt,
zeigt **Bearbeiten → Einstellungen → Ordner → Plug-ins**.

Für eine genaue Erreichbarkeitsprüfung:

```bash
bash scripts/diagnose-flatpak.sh
```

Die Diagnose darf nie als Beleg dafür ausgegeben werden, dass GIMP das Plug-in
bereits registriert hat. Der entscheidende Akzeptanztest ist ein sichtbarer und
aufrufbarer **Systemtest** im GIMP-Menü. Der Eintrag verändert keine Bildpixel.

Das `output/gegl/`-Verzeichnis ist separat. Native GEGL-Operationen verwenden
einen anderen Ladevorgang; ihre Entwicklung/Installation ist noch nicht fertig
implementiert und muss separat in der Flatpak-Sandbox getestet werden.

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

**Status (30.10.2026):** Python-Menüintegration und tatsächlicher Aufruf des
Systemtests in GIMP 3.2.6 Flatpak auf Kubuntu vom Nutzer erfolgreich abgenommen.
Der Screenshot zeigt die GIMP-Meldung „NanoTank-Systemtest erfolgreich: Plug-in geladen.“
Native GEGL-Integration und tatsächliche Bildfilter bleiben separat zu prüfen.
