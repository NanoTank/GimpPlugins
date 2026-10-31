# NanoTank Glass P1.2 – Konturprofil und Materialkontrast

Stand: 31.10.2026, Branch `feature/nanotank-glass`.
P1.1 wurde vom Nutzer technisch in GIMP geprüft, ist visuell nicht abgenommen.
P1.2 wurde inzwischen visuell beurteilt: deutliche Verbesserung, teils metallisch/kunststoffartig.
Weiterhin keine Produktionsfreigabe. Folgestand: [GLASS-P1.3.md](GLASS-P1.3.md).

Verbindliche Produktgrenzen: [GLASS-MATERIAL-VISION.md](GLASS-MATERIAL-VISION.md).
Universeller 2D-/2,5D-Glasmaterialfilter, kein Button-Generator, kein 3D-Renderer.

## Ursache und gezielte Änderung

P1.1 leitete den inneren Dom aus den Koordinaten relativ zur gesamten Ebene ab.
Ein kleines oder versetztes Alpha-Objekt auf einer großen Ebene konnte dadurch
falsche, beinahe flache Normalen erhalten. Die Studioflächen wurden dann kaum
getroffen. Zusätzlich war die reflektierte dunkle Umgebung mit Belichtung 0,35
so schwach, dass steile Kanten auf hellem Hintergrund fast verschwanden.

P1.2 behält Abstandsfeld, Fresnel, analytische Studioflächen, premultiplizierte
Abtastung und beide bisherigen Ausgabemodi bei. Keine neue Benutzeroberfläche,
keine zusätzlichen Materialregler, keine neue Build-Infrastruktur.

- **Konturbezogene Oberfläche:** Volles Chamfer-Abstandsfeld der tatsächlichen
  Alpha-Kontur bzw. der Ebenenbegrenzung. Die generische Höhe folgt
  `h(d)=s × sqrt(q × (2-q))`, `q=clamp(d/s,0,1)`.
  `s` entspricht der halben kleineren Objektausdehnung. Eine separable
  Glättung mit Radius `ceil(0,30s)` entschärft innere Abstandsfeldrücken;
  Normalen stammen aus dem Höhenfeldgradienten. Keine auf die Form beschnittene
  rechteckige oder elliptische Innenwölbung mehr.
- **Randprofil:** Zusätzlich bleibt die schmale abgerundete Kappe für den Rand.
  Die effektive Breite sättigt weich:
  `b=L × (1-exp(-Dicke/L))`, `L=0,40 × kleinere Objektausdehnung`.
  Beim 200×120-Motiv: etwa 3,84 / 10,62 / 16,36 / 22,31 px für Dicke
  4 / 12 / 20 / 30. Das Randprofil überzieht kleine Objekte nicht unbeschränkt.
- **Reflexionskontrast:** Hauptstudiofläche etwas schmaler und quer breiter;
  Lichtgewichte 28 / 5 statt 14 / 2,5. Die hellen Flächen werden weiterhin in
  reflektierter Blickrichtung ausgewertet und folgen Normalen und Lichtrichtung.
- **Dunkle Umgebung:** Belichtung wächst an steilen Normalen mit
  `0,35 + 5 × (1-Nz)^0,65`. Damit erhalten geeignete Kanten Dunkelkontrast,
  ohne der klaren Mitte einen neuen weißen oder grauen Grundfilm zu geben.
- **Transparenz:** Tönungsfilm und Straight-/Premultiplied-Komposition unverändert.
  IOR 1 ohne Reflexion; Reflexion 0 und Tönung 0 bleiben unsichtbar.
  Der Auflagemodus erhält das Quellalpha exakt.

Die Alpha-Ausdehnung dient nur der Maßstabsermittlung. Der Höhenverlauf stammt
von der realen Kontur. Kreis und organische Form erzeugen andere Normalen als
ein Rechteck; es gibt keine getrennte Renderingengine je Objektart.

## Globale Geometrie und Vorschau

Ausdehnung und geglättetes Höhenprofil benötigen das vollständige Inputbild.
GEGL fordert und cached deshalb die ganze Ebene. Eine lokale Quelländerung
invalidiert den gesamten geometrisch abhängigen Bereich. Keine veränderliche
BBox im gemeinsam genutzten Thread-Zustand, operation-intern kein Thread-Splitting.

Das verhindert tileabhängige Geometrie und veraltete Vorschauen nach Maskenänderung.
Es erhöht aber Speicherbedarf und Aufwand pro Regleränderung. Grobe Spitzenabschätzung
für unsere Arbeitsarrays: 44 Byte pro Pixel plus Randpadding, zusätzlich zu GEGLs
Buffers/Caches. Bei 4K sind allein diese Arrays etwa 350 MiB. Große Ebenen sind
nicht praktisch auf Vorschauleistung abgenommen. Keine Performanceoptimierung vor
Bildabnahme; ein späterer Geometry-Cache muss Invalidierung/Persistenz korrekt lösen.

## Tatsächliche Bildvergleiche

Alle Bilder stammen aus `nanotank:glass`, nicht aus KI-generierten Wunschbildern.
Hintergründe sind deterministische Testmuster; Text stammt aus `gegl:text`.
Ungefiltertes transparentes Glas bedeutet Hintergrund ohne Glasebene; beim
opaken Motiv zeigt das Original den tatsächlichen grauen Quellbutton.

- 200×120 transparente Platte auf dunkler kontrastreicher Struktur.
- Kreis mit 150 px Durchmesser.
- Unregelmäßige organische Alpha-Form aus einer deterministischen Polarkurve.
- Mittelgraues opakes Objekt mit Glasauflage.
- Transparente Platte über Text und größeren kontrastreichen Strukturen.
- Identisches Objekt über hellem und dunklem gleichmäßigem Grund.
- Zusätzliche Ausgabe mit tatsächlichen Regler-Standards, nur Materialmodus und
  Alphaform für den transparenten Fall umgeschaltet (`06-factory-defaults.png`).

Vergleichswerte: Dicke 20, Kantenwölbung 0,65, Dichte 0,15, Tönung 0,2,
Reflexion 0,75, Glanz 0,6, Wölbung 0,3, IOR 1,5, Licht 225°.
Stärkerer Vergleich: Reflexion 1, Glanz 0,85, Wölbung 0,5; sonst gleiche Werte.
P1.1 aus festem Commit `451f592` wird mit genau denselben Eingaben gerendert.

Galerie (untracked):

- `output/glass-p12/index.html` – Browseransicht ohne automatische Bildverkleinerung.
- `output/glass-p12/comparison.png` – ohne Glas / P1.1 / P1.2, gleiche Größe.
- `output/glass-p12/materials.png` – Hintergründe und stärkere Kombination.
- `output/glass-p12/details.png` – echte Ausschnitte, ausdrücklich 2×.
- `output/glass-p12/parameters.png` – Dicke 4/12/20/30, Wölbung, Reflexion,
  Glanz und Lichtrichtung bei normaler Bildgröße.

## Visuelle Einschätzung des Entwicklungsstands

Die gerenderten Einzelbilder und Galerien wurden tatsächlich geöffnet. Die
Kanten und gerichteten Reflexe sind bei normaler Bildgröße besser sichtbar;
die runde und organische Form erhalten deutlich unterschiedliche Glanzverläufe.
Auf hellem Grund ist das Objekt durch dunkle Kanten deutlich besser lesbar.
Die Mitte enthält weiterhin gut erkennbare Strukturen, keinen zurückgekehrten
milchigen Grundfilm. Dicke und Wölbung verändern die sichtbare Reflexionsverteilung.

**Das ist keine pauschal bestandene gestalterische Abnahme.** Besonders die
rechteckige Auflage zeigt weiterhin einen stilisierten Fasencharakter. Stärkere
Parameter können schnell nach poliertem Kunststoff oder einem geschliffenen
Objekt aussehen. Die feste Studio-Umgebung passt nicht automatisch zum Hintergrund.
Ohne Hintergrundbrechung fehlt ein wichtiges Signal für Glas. P1.2 verbessert
Sichtbarkeit und Formbezug, erreicht aber nicht durchgehend den gewünschten
hochwertigen Materialeindruck auf allen Motiven. Dies wird nicht durch weitere
pauschale Aufhellung kaschiert.

Sinnvolle spätere Schritte bleiben kontinuierliche maskenbasierte Höhenprofile,
optional ein 2D-Reliefinput und korrekt integrierte Hintergrundbrechung. Keine
3D-Rekonstruktion erforderlich. Diese Schritte sind **nicht** Teil von P1.2.

## Technische Tests – tatsächlich durchgeführt

- SDK-Build gegen GEGL 0.4.72 mit Headern/pc-Datei unter `/app`.
- Tatsächliche GEGL-Registrierung und Rendering in der passenden SDK-Sandbox.
- Bestehende Tests: endliche Werte, Alpha 0…1, opakes/partielles Alpha exakt
  erhalten, HDR/Materialstruktur ohne optische Beiträge, unsichtbare gesättigte
  Quell-RGB-Werte ohne Farbhalo, 1/5/9-Pixel-Objekte, IOR und Render-Teilregionen.
- Alle sieben Parametervergleiche ergeben unterschiedliche Renderings; Bildgalerie
  ergänzt die rein numerische Prüfung.
- Regression der flachen Glasmitte: Alpha **0,022687**, unverändert zu P1.1;
  Hintergrundkontrast dort **97,7313 %**. Dieselbe Farbenergieprüfung wie P1.1.
- Alpha-Objekt identisch nach Verschieben und Vergrößern der transparenten Ebene.
- Cache nach lokalem Alpha-Edit liefert dasselbe Ergebnis wie eine frische Operation.
- Identität ohne optische Beiträge bei sRGB-/Rec2020-Quellfarbräumen und
  RGBA-double-Input, nach Babl-Konvertierung auf gemeinsamen Zielfarbraum.
- 16 statische Python-Vertragstests und `git diff --check` bestanden.
- Regulärer SDK-Build und gezieltes Glass-Deployment erfolgreich; bestehender
  eigener Symlink ist im GIMP-Flatpak lesbar. Die erste Deploy-Prüfung blockierte
  korrekt wegen einer noch registrierten SDK-Instanz; nach deren Ende erfolgreich.

Noch nicht für P1.2 nachgewiesen: tatsächlicher GIMP-Dialogtest, Live-Regler,
Undo/Redo, XCF-Roundtrip, Export und GIMP-Präzisionswechsel. SDK-Rendering und
Babl-Farbtests ersetzen diese Tests nicht. Nutzerabnahme bleibt offen.

## Weitere Grenzen

Die Operation kennt noch keine aktive Auswahlmaske, sondern nur Ebenen-Alpha
bzw. Ebenenrechteck. Untere Ebenen werden nicht gebrochen. Das transparente
Rendering verwirft Quell-RGB und ist keine vollständige Materialumwandlung von
Bilddetails. Glasdeckkraft bleibt Tönungsdichte, kein direktes Ausgabe-Alpha.
Eine Maske allein definiert kein eindeutiges Volumen. Das eine generische Profil
ersetzt noch keine kontinuierliche Steuerung durch Pixelhöhe und Wölbungsbereich.
Zusammenhanglose Objekte teilen momentan einen globalen Profilmaßstab; sehr dünne
Schrift und starke Halbtransparenz sind noch eingeschränkt. Keine zusätzliche
Auswahl-/Tiefen-/Hintergrund-Infrastruktur in dieser Phase.

## Reproduzieren und in GIMP testen

Isoliertes Bauen/Rendern ohne Austausch eines installierten Moduls:

```bash
NANOTANK_GEGL_BUILD_DIR="$PWD/output/glass-p12/gegl" \
  bash scripts/build-gegl.sh nanotank-glass
NANOTANK_GLASS_TEST_MODULES="$PWD/output/glass-p12/gegl" \
  bash tests/glass/run.sh
bash tests/glass/compare-p1.sh --p11
python3 tests/glass/gallery.py
```

Pillow und DejaVuSans werden nur für Beschriftung/Anordnung der echten Renderings
benötigt. Keine Binärdateien ins Git, keine externen Dienste.

Für GIMP **vollständig schließen**, dann:

```bash
git switch feature/nanotank-glass
git pull --ff-only
bash scripts/build-gegl.sh nanotank-glass && \
bash scripts/deploy-gegl-flatpak.sh nanotank-glass && \
bash scripts/diagnose-gegl-flatpak.sh nanotank-glass
```

Zunächst neue Effekte auf Testkopien erzeugen. Transparente Formen: auf separater
Ebene eine opake Form mit transparentem Außenbereich, Alpha-Erhalt **aus**,
Glasform aus Ebenen-Alpha **an**. Dichte 0,15, Tönung 0,2, Dicke 12–20,
Wölbung 0,3, Reflexion 0,75, Glanz 0,6, IOR 1,5, Licht 225°.
Für Auflagen beide Schalter **an**. Kreis und organische Form ebenfalls prüfen.
Wölbung 0/0,75, Licht 45/225°, Dicke 4/12/30 vergleichen. Falls gewünscht die
stärkere Vergleichskombination testen, ohne sie als obligatorischen Standard
zu verstehen. Danach Effekt wieder öffnen, Undo/Redo und XCF testen.

Nach Push keine weitere Phase und kein Merge ohne visuelle Rückmeldung.

### Nachträgliche Präzisierung der Wölbungseinheit

Der Nutzer hat eine geometrische Wölbungshöhe in Pixeln als langfristig verbindlich
festgelegt. Siehe [Materialvision](GLASS-MATERIAL-VISION.md#verbindliche-geometriesemantik-wölbungshöhe-in-pixeln).
P1.2 deutet die gespeicherte Property `curvature` ausdrücklich **nicht** in Pixel um.
Das aktuelle geglättete Profil ist keine exakte Umsetzung von H=R als Halbkugel.
Die neue Höhenproperty samt expliziter Alt-XCF-Kompatibilität bleibt eine gezielte
Folgephase; P1.2 führt dafür keine neuen Regler oder Geometrieversionen ein.

Der zusätzlich verbindliche **Wölbungsbereich (%)** wird gemeinsam mit der
Pixelhöhe zurückgestellt. Er soll später die Breite vom gewölbten Rand bis zur
ebenen Mitte relativ zur Konturdistanz steuern; 100 % ergibt eine durchgehende
Wölbung. Keine Objektmodi oder neuen redundanten Regler in P1.2. Das gemeinsame
Höhenfeld, die Unabhängigkeit von Glasdicke sowie die Prüfung von „Kantenwölbung“
und der Legacy-XCF-Semantik sind in der Materialvision festgehalten.
