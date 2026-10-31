# NanoTank Glass P1.1 – Glasoptik zur visuellen Abnahme

Stand: 31.10.2026. Branch: `feature/nanotank-glass`.
P1 wurde vom Nutzer in GIMP technisch geprüft, **visuell nicht abgenommen**.
Auch P1.1 benötigt die visuelle Nutzerabnahme. Keine Folgephase, kein Merge.

## Ursache des Schleiers

P1 verwendete als Grundfilm `1 - fill * (1 - tint)` – bei schwacher Tönung
nahezu Weiß. Dieser Film wurde mit der gesamten Glasdeckkraft über den
Hintergrund gelegt. Zusätzlich wurde jeder Fresnel-Anteil mit weißer Umgebung
und ein breites Glanzlicht mit einem zweiten Weißanteil verrechnet. Die
Straight-/Premultiplied-Rückrechnung war dabei mathematisch konsistent;
die gewählten Farb- und Lichtanteile erzeugten aber eine milchige Überlagerung.

Reproduzierbarer Referenzfall: leere 384×240-Ebene, Dicke 20, Kantenwölbung
0,65, Deckkraft 0,15, Tönung 0,2 mit Standardfarbe, IOR 1,5, Wölbung 0,
Reflexion und Glanz 1. Messung in der flachen Mitte, lineares RGBA:

| Version | Straight-RGB | Alpha | Hintergrundkontrast nach Over |
| --- | --- | --- | --- |
| P1 | 0,951827 / 0,974602 / 0,993227 | 0,214551 | 78,5449 % |
| P1.1 | 0,061281 / 0,244340 / 0,394044 | 0,022687 | 97,7313 % |

Der Farbwert bei niedrigem Alpha darf nicht isoliert bewertet werden: Die
sichtbare Farbenergie ist RGB × Alpha. Im P1.1-Referenzpixel beträgt sie etwa
0,00139 / 0,00554 / 0,00894. Die Kontrastangabe folgt aus tatsächlichen
Output-Pixeln und linearer Over-Komposition; keine Messung im GIMP-Display.
Sie gilt für diese flache Mitte, nicht pauschal für die hellen Reflexe.

## Gezielte Änderungen

Die native Area-Operation, Property-IDs, Menüeintrag, Parameterbereiche und
Float-Verarbeitung im Quellfarbraum bleiben bestehen.

- **Profil:** Abgerundete Kappe mit zum Inneren auslaufender Steigung statt
  linearer Fase. Dicke steuert die Breite dieses Profils sowie den optischen
  Weg bei interner Pixelverschiebung. Kantenwölbung verändert die Steilheit.
- **Normalen:** Geglätteter Abstandsfeldgradient (Sobel) für die Kontur plus
  deutlich wirksamerer elliptischer Oberflächendom. Wölbung verändert damit
  auch auf gleichmäßig grauem Material die Reflexionsverteilung.
- **Umgebung:** Zwei analytische Studioflächen werden in der reflektierten
  Blickrichtung ausgewertet. Ihre Position folgt der Lichtrichtung. Ein heller
  weicher Hauptreflex und eine schwächere Gegenfläche bewegen und krümmen sich
  mit den Normalen. Keine in Bildkoordinaten gezeichneten weißen Streifen.
- **Fresnel:** Schlick aus IOR und tatsächlichem Normalenwinkel, IOR=1 ohne
  Grenzflächenreflexion. Dunkle Umgebung kann steile Kanten abdunkeln; helle
  Lichtflächen geben begrenzte Glanzbereiche. „Reflexion“ skaliert den gesamten
  Umgebungsanteil, „Glanzlichter“ die hellen Studioflächen.
- **Komposition:** Reflektierte Farbe und Reflexionsalpha werden gemeinsam
  aus endlicher Umgebungshelligkeit berechnet. Körper und Reflexion werden
  premultipliziert kombiniert und genau einmal nach Straight-RGB zurückgerechnet.
- **Tönung:** Kein weißer Grundfilm mehr. Auflagen verwenden kanalabhängige
  exponentielle Absorption. Transparentes Glas verwendet einen schwachen
  farbigen Ersatzfilm, da RGBA ohne Hintergrundinput keine beliebige farbige
  Durchlässigkeit exakt darstellen kann.

### Bedeutung der vorhandenen Deckkraft-Property

Die Property `glass-opacity` bleibt erhalten, heißt jetzt im Dialog
**„Glasdeckkraft (Tönungsdichte)“**. Sie bestimmt die Dichte des getönten
Materialanteils, nicht direkt das fertige Output-Alpha. Dieses setzt sich aus
Tönungsdichte, Tönungsintensität, Farbe, Profil und Reflexion zusammen.

`Körperalpha = 1 - exp(-Deckkraft × Tönungsintensität × Profilfaktor × Farbdichte)`.
Farbdichte ist die größte Absorption eines RGB-Kanals; Profilfaktor 0,6 bis 1.
Mit Tönungsintensität 0 ist der Grundkörper vollkommen klar. Bei Reflexion 0
und Tönung 0 entsteht exakt Alpha 0. Deckkraft 1 ist daher **kein Schalter für
ein vollständig opakes weißes Objekt**. Das ist eine bewusste semantische
Korrektur; gespeicherte P1-Effekte behalten ihre Zahlen, sehen aber anders aus.

Im Auflagemodus bleibt Output-Alpha exakt das Alpha des Quellpixels;
Deckkraft/Tönungsdichte ist dort wie bisher ohne Wirkung. Vorhandenes Material
bleibt sichtbar, es gibt keinen zusätzlichen pauschalen Aufhellungsfilm.

## Reproduzierbare Bildtests

`tests/glass/material.c` erzeugt echte GEGL-Renderings, keine nachgemalten
Erwartungsbilder:

1. Freigestellter mittelgrauer rechteckiger Button (240×150, Alpha 1).
2. Transparente Alpha-Glasplatte (240×150) mittig auf 384×240 über kontrastreichem
   Muster. Die Formmaske hat Alpha 1, der Filter erzeugt die Glastransparenz.
3. Dieselbe Platte auf dunklem bzw. hellem gleichmäßigem Hintergrund.
4. Kreisförmiges Glas mit Radius 75 und geglätteter Alphakontur.
5. Einzelvergleiche für Reflexion 0/1, Dicke 4/30, Licht 45/225°,
   Deckkraft 0,02/0,8, Wölbung 0/0,75, Glanz 0/1 und IOR 1/2.

Vergleichsbilder verwenden identische Inputs und Parameter für P1 und P1.1.
Im Standardvergleich: Dicke 20, Deckkraft 0,15, Reflexion 0,75, Glanz 0,6,
Wölbung 0,3, Licht 225°, übrige Werte wie im Dialog. Die runden und eckigen
Objekte sind Alpha-Objekte; der Test behauptet keine Auswahlmasken-Anbindung.

Nach erfolgreichem Build, GIMP für das reguläre Buildziel geschlossen lassen:

```bash
bash tests/glass/run.sh
bash tests/glass/compare-p1.sh
python3 tests/glass/gallery.py
```

Die Galerie benötigt Pillow und DejaVuSans. Alle PNGs, Module und Testprogramme
liegen untracked unter `output/`. Der P1-Vergleich extrahiert nur Glass-Quelle
und gemeinsames Buildskript aus Commit `320707d` in einen isolierten Ausgabeordner.
Er verändert weder Branch noch Installation. Keine zweite Compilerimplementierung.

Für Bildtests bei laufendem GIMP kann das gemeinsame Skript isoliert bauen:

```bash
NANOTANK_GEGL_BUILD_DIR="$PWD/output/glass-p11/gegl" \
  bash scripts/build-gegl.sh nanotank-glass
NANOTANK_GLASS_TEST_MODULES="$PWD/output/glass-p11/gegl" \
  bash tests/glass/run.sh
```

Diese Tests setzen GEGL_PATH nur für ihren eigenen Prozess. Keine globale
Konfiguration. Der reguläre Glass-Build verweigert inzwischen bei registrierter
GIMP-Instanz den Austausch seines üblichen Ausgabeziels, denn auch ein bestehender
Installationssymlink würde bereits beim Build auf das neue Modul zeigen.

## Durchgeführte Prüfungen

- 16 statische Python-Vertragstests bestanden, `git diff --check` sauber.
- Native Kompilierung im passenden GIMP-Flatpak-SDK mit GEGL 0.4.72 und
  GEGL-Header/pc-Datei unter `/app`; keine Host-GEGL-Bibliotheken.
- Tatsächliche GEGL-Registrierung und Rendering im SDK-Prozess bestanden.
- Endliche Floatwerte, Alpha 0…1, exakter Quellalpha-Erhalt einschließlich
  partieller Alpha-Werte, HDR-Werterhalt und Materialstruktur ohne optische
  Beiträge, keine Farbsäume durch gesättigtes unsichtbares RGB bestanden.
- Kleinstformen (1/5/9 Pixel), Alpha-Ellipse, Alpha-Kreis, Rechteck und identische
  Teilregionen/Gesamtbild bestanden. Außerhalb der Glas-Alphakontur Alpha 0.
- Alle sieben Parametervergleiche ergeben messbar unterschiedliche Renderings.
- Flache Glasmitte Alpha <0,03 und geringe premultiplizierte Farbenergie;
  ohne Tönung/Reflexion exakt transparent, bei IOR 1 ohne Reflexionsalpha.
- P1-Baseline aus festem Commit kompiliert und dieselben Szenen gerendert.
- Vergleichs- und Parametergalerie tatsächlich geöffnet und visuell geprüft:
  deutlich weniger Schleier, gebündelte Reflexe, sichtbare Profilunterschiede.
- Finaler regulärer SDK-Build und gezieltes Deployment nur von Glass erfolgreich,
  nachdem keine GIMP-Instanz mehr registriert war. Eigener Symlink im Flatpak lesbar.

**Nicht durchgeführt:** neuer GIMP-Dialogtest für P1.1, Undo/Redo, Wiederbearbeiten,
XCF-Roundtrip, GIMP-Export und 8/16/32-Bit-GIMP-Konvertierungen. Keine bestandene
visuelle Nutzerabnahme behauptet. Keine privaten Bilder oder Binärdateien committed.

## Weiterhin bestehende Grenzen

- Kein Zugriff auf untere Ebenen: deren Inhalte bleiben unverzerrt. Interne
  Brechung betrifft ausschließlich das Inputbild der bearbeiteten Ebene.
- Eine freie Auswahl auf opaker Ebene begrenzt nur die Wirkung; ihre Kontur ist
  der Geometrie nicht bekannt. Keine neue Masken- oder Compositing-Infrastruktur.
- Die Wölbung orientiert sich an Ebenenkoordinaten, der Rand an der Alphakontur.
  Sehr asymmetrische oder versetzte Alpha-Objekte haben deshalb keinen exakt
  konturangepassten Dom. Stark halbtransparente Masken bleiben geometrisch begrenzt.
- Analytische Studioflächen ersetzen keine reale Umgebungsaufnahme; keine
  vollständige 3D-Physik. Chamfer-Distanzen sind euklidisch angenähert.
- Auf sehr hellem unstrukturiertem Grund ist klares Glas zurückhaltender sichtbar.
  Reflexe und dunklere Kanten helfen, ersetzen aber keine Hintergrundbrechung.
- Ohne Hintergrundinput bleibt die farbige Transmission ein RGBA-Ersatzfilm;
  keine exakte Beer-Lambert-Filterung fremder Ebenen.

## Aktualisieren und in GIMP prüfen

GIMP vollständig schließen, dann:

```bash
git switch feature/nanotank-glass
git pull --ff-only
bash scripts/build-gegl.sh nanotank-glass && \
bash scripts/deploy-gegl-flatpak.sh nanotank-glass && \
bash scripts/diagnose-gegl-flatpak.sh nanotank-glass
```

GIMP starten, **Filter → NanoTank → Glass**, zunächst neue Testeffekte:

- **Opaker Button:** grauen Button auf transparenter Ebene anlegen. Beide
  Schalter „Untergrund und Alpha erhalten“ und „Glasform aus Ebenen-Alpha“ an.
  Dicke 12–20, Wölbung 0,3–0,6, Reflexion 0,75, Glanz 0,6, IOR 1,5.
- **Transparentes Glas:** auf separater Ebene mittig ein weißes Rechteck von
  etwa 240×150 oder einen Kreis auf transparentem Außenbereich anlegen.
  Alpha-Erhalt **aus**, Glasform aus Ebenen-Alpha **an**. Quellfarbe ist hier
  unerheblich; Alpha definiert die Form. Deckkraft 0,15, Tönung 0,2,
  Reflexion 0,75–1, Glanz 0,6–1, Wölbung 0,3. Über Text/Muster, hell und dunkel testen.
- Für völlig leere Ebenen muss „Glasform aus Ebenen-Alpha“ weiterhin **aus** sein;
  der Glasrand liegt dann an der Ebenengrenze.
- Licht 45/225°, Dicke 4/30 und Wölbung 0/0,75 direkt vergleichen. Zum Schluss
  Effekt bestätigen, erneut öffnen, Undo/Redo, XCF speichern/neuladen und exportieren.

Erst nach visueller Rückmeldung weitere Entwicklung.
