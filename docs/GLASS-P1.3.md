# NanoTank Glass P1.3 – universelle Pixelhöhengeometrie

Stand: 31.10.2026. Branch `feature/nanotank-glass`, Entwickleridentität NanoTank.
Vom Nutzer inzwischen als geometrischer Fortschritt positiv beurteilt; die
Materialwirkung bleibt noch nicht endgültig abgenommen. Folgestand mit Hintergrundinput:
[GLASS-P1.4.md](GLASS-P1.4.md).
2D-/2,5D-Materialfilter, keine 3D-Rekonstruktion oder vollständige Glaskugel.

## Bedienung und gespeicherte Effekte

Die bisherige Operation `nanotank:glass` und der Menüpfad bleiben bestehen.
**„Pixelhöhenprofil verwenden“ einschalten**, um die neue Geometrie zu testen.
Die Property `pixel-geometry` hat aus Kompatibilitätsgründen Standard **false**:
alte Effekte ohne diese Property müssen weiterhin wie P1.2 rechnen. Auch neu
angelegte Effekte beginnen deshalb zunächst im Altmodus. Kein stiller Wechsel.

| Neue Property / deutscher Regler | Standard | Bereich | Bedeutung |
| --- | --- | --- | --- |
| `pixel-geometry` / Pixelhöhenprofil verwenden | aus | ein/aus | Expliziter Wechsel auf die neue Geometrie |
| `surface-height` / Wölbungshöhe (Pixel) | +10 | −1000…+1000 | Signierte Höhe der Grundfläche |
| `surface-extent` / Wölbungsbereich (%) | 100 | 1…100 | Durchgehend gewölbt oder mit ebener Mitte |
| `edge-radius` / Kantenrundung (Pixel) | 4 | 0…64 | Unabhängiger Außenrand |

Im neuen Profil werden `curvature` und `roundness` **ignoriert**. Ihre IDs,
Wertebereiche und Standards bleiben für Altprojekte erhalten; die Beschriftung
nennt jetzt „Altmodus“. Keine Umdeutung von Faktoren in Pixel, keine zwei
wirksamen Regler für denselben Aspekt. `thickness` wirkt im neuen Profil als
positive Mindestdicke für den angenäherten optischen Weg; die Randbreite wird
hier nicht mehr durch Glasdicke bestimmt. Im Altmodus bleibt die alte Wirkung.

Die Geometrieumstellung ist ausdrücklich sichtbar und optional. Der Altmodus
wurde gegen das tatsächlich gebaute P1.2-Modul pixelidentisch geprüft. Alte
GEGL-XML-Effekte ohne neue Properties bleiben im Altmodus; neue signierte
Höhen und Bereichswerte überstehen einen GEGL-XML-Roundtrip. **Das ist noch
kein bestätigter XCF-Roundtrip im GIMP-Dialog.** Beim GEGL-XML-Farbformat wurde
eine Rundung der RGB-Tönungsfarbe auf drei Nachkommastellen beobachtet:
maximale Floatabweichung 0,0001602 im alten und 0,0000032 im neuen XML-Test.
Der direkte Modulvergleich ohne XML bleibt exakt gleich.

## Geometrie: ein glattes Konturfeld je getrenntem Objekt

Das neue Profil verwendet kein Chamfer-Abstandsfeld als Innenwölbung. Dessen
Medialachsen sowie die bisherige Boxglättung konnten in P1.2 große, harte und
dreieckige Glanzflächen begünstigen. Stattdessen wird je **4-zusammenhängender**
Maskenkomponente ein glattes skalares Poisson-Feld berechnet:

`−Δφ = 1` innerhalb der Form, `φ = 0` an ihrer Kontur.

Die Pixelberechnung verwendet einen Fünfpunktoperator mit halbzelligem
Dirichlet-Rand (ungerade Ghostwerte) und überrelaxierte Gauss-Seidel-Iteration.
Die Abbruchgrenze ist eine maximale relative Änderung von 1e−8 pro Sweep,
bei höchstens 3000 Sweeps. Ein nicht konvergiertes Feld wird nicht still gerendert;
Speicher-/Konvergenzfehler führen zu einem GEGL-Fehler und einer Diagnosemeldung.
Nur tatsächlich belegte Pixel werden iteriert; der vollständige Input bleibt
wie zuvor eine globale Cache-/Invalidierungsabhängigkeit.

Mit `t = φ / max(φ)` je Komponente ist das durchgehende Grundprofil:

`z_grund = H × sqrt(t)`.

Für einen kontinuierlichen Kreis gilt `φ=(R²−r²)/4`; damit ist dies genau
`H × sqrt(1−r²/R²)`. H=R ergibt die halbkugelige Vorderseite, H>R ein in Tiefe
gestrecktes Halbellipsoid. Die Rastermaske erzeugt erwartbare kleine Abweichungen.
Kein automatisch erkanntes Kugel-Preset und keine implizite Ellipse auf freien Formen.

Für einen begrenzten Bereich `p = Prozent/100` wird die glatte Konturkoordinate
`u=1−sqrt(1−t)` verwendet, `q=min(u/p,1)` und

`z_grund = H × sqrt(q × (2−q))`.

Ab q=1 ist die Fläche exakt eben, die Ableitung am Anschluss null. Bei negativem
H ist dieses Plateau abgesenkt. Beim Kreis entspricht u genau der relativen
inneren Konturdistanz. Bei freien Formen ist u eine **Poisson-Konturkoordinate**,
kein exaktes euklidisches Prozent-Abstandsfeld; dafür besitzt sie keine medialen
Knicklinien. Diese Abweichung von der früher skizzierten reinen Distanzformel ist
bewusst, um glatte Normalen für allgemeine Formen zu erhalten.

Das Randprofil verwendet denselben glatten Koordinatenraum, aber eigene Breite:
`z_rand = e × (f(t,e/R_eff)−1)`, `R_eff=2sqrt(max φ)`.
Die effektive Rundung e wird für Kleinstformen auf 45 % dieses Maßstabs begrenzt.
Für eine unveränderte analytische Halbkugel **Kantenrundung auf 0** setzen;
ein zusätzlich gerundeter Rand verändert das kombinierte Profil absichtlich.

`z_gesamt = z_grund + z_rand`.

Beide Beiträge und ihre analytischen Ableitungen werden vor der Normalisierung
zusammengeführt. Die Normalen `N=normalize(−∂z/∂x,−∂z/∂y,1)` steuern sowohl
Reflexion als auch interne Pixelverschiebung. Die Ableitungsformel vermeidet die
scheinbare Singularität in der Mitte eines durchgehenden Kreisprofils. Am
Rasterrand entstehen endliche, pixelaufgelöste Grenzwerte. Silhouette und Ausgabe-
Alpha werden durch den Höhenaufbau nicht verformt.

Jede Komponente besitzt ihre eigene Normierung und ihren eigenen Maßstab.
Ein zusätzliches großes Objekt ändert das Profil eines getrennten kleinen
Objekts nicht. Dünne verbundene Bereiche teilen dagegen bewusst das gemeinsame
Konturfeld ihres Objekts. Nur diagonal berührende Pixel gelten als getrennt.
Für die Geometrie zählt Alpha >0,001 als belegt; die tatsächliche weiche
Pixeldeckung bleibt für die Ausgabe erhalten. Dies ist keine exakte Rekonstruktion
subpixeliger oder beliebig stark gefiederter Konturen.

## Materialdicke, Reflexion und Erweiterbarkeit

Der angenäherte optische Weg ist mindestens die positive Glasdicke und wächst
mit der relativen Höhe über dem niedrigsten Referenzniveau der Komponente.
Negative Grundhöhen können daher keine negative Materialdicke erzeugen.
Der Samplingweg bleibt begrenzt; es ist keine Strahlverfolgung durch Vorder-
und Rückseite. Brechung betrifft weiterhin nur die eigene Eingabeebene.

Im Pixelprofil ersetzen weich auslaufende Gaußflächen die scharf begrenzten
Superellipsen der alten Studiofläche. Die Lichtgewichte sinken von 28/5 auf
9/1,8; die dunkle Streifumgebung ist ebenfalls zurückgenommen. Keine pauschale
Verstärkung und kein neuer weißer Grundfilm. Fresnel folgt weiterhin IOR und
Normalenwinkel. Die alte Beleuchtung bleibt im Altmodus unverändert.

Grundgeometrie, Randprofil und **späteres lokales Oberflächenrelief** sind
getrennte Beiträge. Ein zukünftiges Relief ergänzt `z_gesamt` und seine
Ableitungen vor Normalisierung und optischer Wegberechnung. Deshalb kann später
eine global konvexe Fläche gleichzeitig lokale negative Mulden tragen. Die
Materialengine braucht dafür nicht ersetzt zu werden. P1.3 implementiert weder
Reliefpad noch Höhenkarten, Muster, Golfball-Generator oder Reliefeditor.

## Tatsächlich ausgeführte Tests

- Native Kompilierung gegen GIMP-Flatpak-SDK, GEGL **0.4.72**, Header/pc unter `/app`.
- Regulärer Build und gezieltes Deployment nur von Glass erfolgreich; eigener
  Installationssymlink im GIMP-Flatpak lesbar. Kein Austausch während einer
  registrierten GIMP-Sitzung, keine Änderung anderer Module oder Konfiguration.
- Portable Tests des **gleichen C-Geometriekerns**, den die Operation verwendet:
  Kreis R=100 mit H=0/10/50/100/150/−5/−20. Maximale Höhenabweichung bis r=90:
  0,573 px über alle getesteten Höhen; Winkelabweichung unter 0,012 rad.
  Bei H=100 maximale Höhenabweichung 0,382 px; Zentrum erreicht H.
- Exakt ebenes abgesenktes Plateau bei Bereich 15 %, unabhängige Glasdicke,
  unabhängige getrennte Komponenten, winzige und nur ein Pixel breite Formen,
  durchgehend endliche Höhen/Normalen sowie positiver Mindestweg.
- **Echte GEGL-Renderings:** Kreisprofile, Rechteck 15/100 %, konkave Fläche mit
  gerundetem Rand, organische Form, Ellipse, getrennte Kreise, dünne Glyphenformen,
  opake Auflage H=35. Keine KI-Bilder oder nachgemalten Wunschresultate.
- Endliche RGBA-Werte, Alpha 0…1, unverändertes opakes und partielles Quellalpha,
  keine Ausgabe außerhalb der Alphaform, HDR-Erhalt ohne optische Farbanteile,
  konsistente Ausgabe angeforderter Teilregionen, verschobene GEGL-Eingabe.
- Neues GEGL-XML mit negativen Pixelhöhen/Bereich und alte XML ohne neue Properties.
- Direkter P1.2-/Altmodusvergleich mit echten Modulen, Pixel für Pixel identisch.
- Bestehende Render-/Materialregressionen bestanden, einschließlich Cache-Edit,
  sRGB/Rec2020, RGBA-double, winziger Formen, unsichtbarem RGB und klarer Glasmitte
  im Altmodus. Diese Altmodustests ersetzen nicht automatisch dieselben Fälle im
  neuen Profil; dort wurden die oben separat genannten Fälle geprüft.

Noch offen: P1.3 im tatsächlichen GIMP-Dialog, Live-Regler und Leistung auf großen
Ebenen, Undo/Redo, XCF-Roundtrip und Export. Kein visueller Nutzerentscheid vorweggenommen.
Die Vergleichsbilder wurden lokal tatsächlich geöffnet: Geometrieunterschiede
sind sichtbar und die Reflexe weicher. Flache Profile und die graue Auflage bleiben
bei geringer Höhe sehr zurückhaltend; das ist keine garantierte visuelle Verbesserung
auf jedem Hintergrund. Fehlende Hintergrundbrechung bleibt ein wesentlicher
Materialhinweis, der in dieser Phase noch nicht simuliert wird.

## Grenzen

- Keine aktive GIMP-Auswahlmaske als Geometrieinput, nur Alpha oder Ebenenrechteck.
- Kein Hintergrundinput, keine vollständige Glaskugel mit Rückseite.
- Freie Formen erhalten ein glattes mathematisches Profil, keine eindeutige
  reale Volumenrekonstruktion. Löcher, dünne Stege und feine Schrift sind rasterabhängig.
- Volle Ebene wird gecached; zusätzliche Solverarrays erhöhen den Speicherbedarf.
  Große 4K-Ebenen sind nicht auf interaktive Leistung abgenommen; im ungünstigen
  Fall ist auch die endliche Konvergenzgrenze eine Einschränkung.
- Höhere Geometrie bedeutet nicht automatisch überzeugendes Glas: feste Studioumgebung
  und fehlende Hintergrundbrechung bleiben stilistische/technische Grenzen.

## Reproduzieren und testen

Isoliert ohne Austausch des installierten Moduls:

```bash
NANOTANK_GEGL_BUILD_DIR="$PWD/output/glass-p13/gegl" \
  bash scripts/build-gegl.sh nanotank-glass
bash tests/glass/run-p13.sh --compare-p12
python3 tests/glass/gallery-p13.py
```

`--compare-p12` extrahiert nur Glass-Quellen und das gemeinsame SDK-Buildskript aus
P1.2-Commit `eba9090` in `output/`; kein Branchwechsel, kein zweiter Compilerpfad.
Galerie benötigt Pillow/DejaVuSans. Alle Bilder, XMLs, Testprogramme und Module
bleiben untracked unter `output/glass-p13/`:

- `comparison.png`: signierte Kreisprofile und Rechteck-Bereiche.
- `materials.png`: organisch, getrennt, dünn, Ellipse, Auflage und Altvergleich.
- `details.png`: ausdrücklich 2× vergrößerte Ausschnitte plus normale Größe.
- `index.html`: Browsergalerie in ursprünglichem Maßstab.

Installation nach vollständigem Schließen von GIMP:

```bash
git switch feature/nanotank-glass
git pull --ff-only
bash scripts/build-gegl.sh nanotank-glass && \
bash scripts/deploy-gegl-flatpak.sh nanotank-glass && \
bash scripts/diagnose-gegl-flatpak.sh nanotank-glass
```

GIMP neu starten, Testkopie öffnen, **Filter → NanoTank → Glass**.
**Pixelhöhenprofil verwenden einschalten**. Für eine Alphaform: Glasform aus
Ebenen-Alpha an; für transparentes Material Alpha-Erhalt aus, für eine Auflage an.
Beginnen mit H=+10/+50/−10, Bereich 15/100, Rand 0/4/6. Kreis R=100 und H=100
mit Rand 0 zeigt das Halbkugelexperiment. Dichte 0,15, Reflexion 0,75, Glanz 0,6,
IOR 1,5, Licht 225°. Bei einer Auflage ist H=35 im Galeriebeispiel deutlicher als H=10.
Nach Prüfung erneut bearbeiten, Undo/Redo und XCF testen. Altprojekte zunächst
mit ausgeschaltetem Pixelprofil kontrollieren; Wechsel nur bewusst vornehmen.

Danach keine weitere Phase und kein Merge ohne visuelle Rückmeldung.
