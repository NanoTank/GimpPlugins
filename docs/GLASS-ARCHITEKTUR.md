# NanoTank Glass – Architektur und Prototyp P0

Basis-Branch: `feature/plugin-foundation`, eigenständiger Branch: `feature/nanotank-glass-p0`.

## Entscheidung

Native **GEGL-Area-Operation** (`nanotank:glass`, C). Damit stehen der von GIMP
bereitgestellte Live-Vorschaudialog, nicht-destruktive Ebenenfilter, Undo/Redo
und XCF-Effektpersistenz prinzipiell zur Verfügung. Menüregistrierung über
`gimp:menu-path`, ohne Python-Wrapper, ohne GIMP-Fork.

- Quellcode: `plugins/nanotank-glass/` (kein Build-Artefakt im Git).
- Ausgabe: `output/gegl/nanotank-glass.so`.
- Installation: vorhandenes `scripts/install-flatpak.sh` verlinkt das Modul
  einzeln, ohne andere Installationen zu überschreiben.
- `scripts/build.sh` bleibt unverändert und behandelt weiterhin Python-Plug-ins.
- Native GEGL-Kompilierung separat: `scripts/build-glass.sh`.

## Algorithmus P0

- GEGL Area Filter mit gepaddetem Nachbarschaftszugriff auf **die aktive Ebene**.
- Lineares RGBA-float im Quellfarbraum; keine 8-Bit-Zwischenbilder.
- Abgerundete Rechteckform oder optional Alpha-Umriss der aktiven Ebene.
- Kantenlicht über ein Abstandsfeld, Fresnel-Schlick (IOR), gerichtete
  Glanzlichter, transparente farbige Glasauflage und einfache Wölbungsbrechung.
- Straight-alpha Over-Compositing; ein opakes Material behält Alpha 1.0.
- Auswahlmaske und weiche Auswahlkanten werden **durch GIMP** nachträglich
  auf die Filterwirkung angewendet.

## Grenzen (bewusst, nicht verschweigen)

1. Bei einer frei gezeichneten **Auswahl in einer vollständig opaken Ebene**
   kennt die GEGL-Operation die Auswahlsilhouette nicht. Die Glaswirkung wird
   auf die Auswahl begrenzt, aber der glänzende **Rand folgt derzeit dem
   Ebenenrechteck**, nicht der Auswahlkontur. Ein echter freier Glasrand
   benötigt eine explizite Maske/Alpha-Vorbereitung oder eine zweite GEGL-Pad.
2. Brechung verzerrt **nur die aktuelle Ebene**. Inhalte darunter werden nicht
   durch die Scheibe refraktiert. Für echte transparente Linsen ist eine
   Hintergrundprojektion/`aux`-Pad als nächste Phase vorgesehen.
3. Rauheit/Milchglas und Presets fehlen in P0.
4. Für einen echten Glaskörper sind später Lichtquelle/Normalen/Umgebungsbild
   nötig; P0 ist ein plausibler 2,5D-Look, kein Raytracer.
5. Die C-Quelle ist hier ohne die GIMP-Flatpak-Entwicklungsumgebung erstellt;
   **es gibt noch keinen erfolgreichen GIMP-Flatpak-Lade-/Bildtest**.

## Lokal bauen und testen

```bash
# Git-Klon, Branch feature/nanotank-glass-p0
bash scripts/build.sh
# WICHTIG: zuerst GEGL-SDK/ABI 0.4.72 im Build-Umfeld prüfen:
pkg-config --modversion gegl-0.4
bash scripts/build-glass.sh
bash scripts/install-flatpak.sh
# GIMP danach komplett neu starten
flatpak run org.gimp.GIMP
```

Build-Schutz: Standardmäßig wird nur GEGL **0.4.72** akzeptiert, wie in
`feature/plugin-foundation` für das aktuelle Flatpak dokumentiert.
Fremde GEGL-Versionen erfordern nach realer ABI-Prüfung ausdrücklich
`NANOTANK_ALLOW_OTHER_GEGL=1`; dies garantiert keine Kompatibilität.
Die auf dem Host installierten Header beweisen keine Flatpak-Kompatibilität.

## Manuelle Abnahme P0

1. 512x256 RGB-Ebene vollständig opak, rechteckig auswählen: Glass starten,
   Preview an/aus, Glanz und IOR ändern. Alpha muss überall opak bleiben.
2. Transparente neue 512x256 Ebene: Glass anwenden. Glasebene muss sichtbar,
   aber halbtransparent sein. Über kariertem Hintergrund prüfen.
3. Ebene mit runder transparenter Form: `Form aus Ebenen-Alpha` aktivieren;
   runde Form bleibt erhalten. Keine dunklen Halos an Alpha-Rändern.
4. Freie Auswahl mit weicher Kante: keine Änderung außerhalb der Auswahl;
   den noch **nicht** formfolgenden Rand dokumentieren.
5. Undo/Redo, Nichtdestruktivität via Filterliste, XCF speichern/neuladen,
   Export PNG, 8-Bit und 16/32-Bit, große Bilder testen.
6. GIMP-Diagnose/Plugin-Load (Flatpak) prüfen, bevor weitere Effekte ergänzt
   oder irgendetwas gemergt wird.
