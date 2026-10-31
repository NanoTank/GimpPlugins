# NanoTank Glass P1.4.1 – kleiner optischer Feinschliff

Branch `feature/nanotank-glass`, Entwickler NanoTank.
P1.4 wurde vom Nutzer **im Grundsatz visuell positiv abgenommen**, einschließlich
des Snapshot-Workflows als vertretbarem Zwischenstand. Das ist keine endgültige
Workflow-/Produktionsfreigabe. P1.4.1 wartet auf erneute visuelle Abnahme.

## Zwei gezielte Änderungen

1. **Abtastung bei lokaler Verkleinerung:** P1.4 verwendete überall einen einzelnen
   bilinear gefilterten Hintergrundpunkt. P1.4.1 schätzt den lokalen Abbildungsmaßstab
   aus benachbarten Snell-Abtastkoordinaten. Bei Verkleinerung gehen bis zu vier
   zusätzliche bilineare Proben in eine begrenzte Flächenmittelung ein. Die
   Umschaltung ist glatt gewichtet; zentral wird symmetrisch differenziert,
   an der Kontur nur innerhalb der Form. Der Sprung zum unverzerrten Außenbereich
   wird nicht als riesiger Filterbereich missverstanden. Die beiden Abtastachsen
   werden auf jeweils 2,5 Quellpixel begrenzt. Alle Proben und ihre Mischung
   werden premultipliziert verrechnet, einschließlich Alpha.
2. **Dezentere Starttönung:** Der Standard der Snapshot-Operation und des Helfers
   sinkt von 0,20 auf **0,12**. Keine neue Grunddeckkraft, kein weißer Film und
   keine pauschale Alphaanhebung. Explizit gesetzte Werte bleiben gültig; GIMP
   kann zuvor benutzte 0,20 merken. Zum Test neuer Startwerte ggf. 0,12 einstellen.

Das ist Abtastfilterung gegen Rasterartefakte, **keine Rauheit, Milchglas oder
Defokussierung**. Bei unverändertem oder vergrößertem Abtastbereich bleibt die
bisherige Einzelprobe aktiv. IOR=1, Brechung=0, deaktiviertes Pixelprofil und
opake Auflage verwenden weiterhin den bisherigen Abtastweg.

Reflexion **0,75**, Glanz **0,60**, IOR **1,5**, Brechung **1**, Licht **225°**
bleiben unverändert. Eine andere Energieverteilung der Studioflächen brachte
keinen klaren visuellen Vorteil und wurde nicht übernommen. Die akzeptierte
Geometrie und das Snell-Modell werden nicht verändert. Der eininputige normale
Glass-Filter einschließlich Altmodus und seiner Standards bleibt unangetastet.

## Visuelle Einschätzung und konkave Flächen

Die tatsächlichen Vergleichsbilder wurden geöffnet: Der Gewinn ist **behutsam**,
vor allem an feinen Linien und komprimierten Strukturen sichtbar. Die geringere
Starttönung lässt mehr ursprüngliche Hintergrundfarbe durch. Es wird keine
neue, deutlich stärkere Gesamtwirkung behauptet.

Die konkave Form H=−30 wurde gezielt über Text/Raster geprüft. Ihre verkleinernde
Abbildung profitiert von der lokalen Filterung, bleibt aber ein subtileres
Tiefensignal als der konvexe H=100-Kreis. Es gibt keine künstliche konkave
Zusatzschattierung und keine automatische Verstärkung negativer Höhen. Die
Lesbarkeit konkaver Formen ist damit nicht abschließend gelöst.

Beispielhafte **manuelle Startwerte**, keine neuen Presets oder Objektmodi:

| Motiv | Höhe px | Bereich % | Rand px |
| --- | --- | --- | --- |
| Linse / Kreis R=100 | +100 | 100 | 0 |
| Platte über Text | +10 | 15 | 4 |
| Organische Form | +40 | 100 | 4 |
| Konkave Form | −30 | 100 | 4 |

Dazu Tönung 0,12 und die obigen unveränderten Materialwerte. Der allgemeine
Dialogstandard H=50 bleibt bestehen; ein einziger Höhenstandard passt nicht
zugleich zu einer dünnen Platte und einem halbkugeligen Vorderseitenprofil.
Text sollte bei der Platte auch über den gekrümmten Rand reichen; die ebene Mitte
bleibt zu Recht unverzerrt. Die Werte wurden in den Motiven der Galerie verwendet.

## Reproduzierbare Galerie

```bash
bash tests/glass/compare-p141.sh
```

Das Skript baut über die gemeinsame SDK-Infrastruktur. Es verwendet P1.4 aus
Commit `9c77567` und den aktuellen Stand in getrennten `output`-Verzeichnissen,
keine Branchwechsel, keine zweite Compilerimplementierung, kein Deployment.
Pillow/DejaVuSans dienen nur zur Beschriftung und Anordnung echter Renderings.

- `output/glass-p141/comparison.png`: P1.4/P1.4.1, **identische Parameter** mit
  Tönung 0,20; dadurch ist die reine Abtaständerung beurteilbar.
- `output/glass-p141/materials.png`: zusätzlich neuer Tönungsstandard 0,12;
  dieselben geometrischen Parameter pro Motiv.
- `output/glass-p141/details.png`: ausdrücklich 2× vergrößerte Ausschnitte.
- `output/glass-p141/index.html`: Originalmaßstab ohne automatische Verkleinerung.

Kreis H=100, Platte über Text, organische Form, konkave Form und getrennte Objekte
sind enthalten. Die Ausgabe der normalen P1.3-Operation wurde ebenfalls verglichen:
in allen sechs Motiven einschließlich Auflage **pixelidentische Referenz-PNGs**.

## Durchgeführte Prüfungen

- Native Kompilierung gegen GIMP-Flatpak-SDK / GEGL 0.4.72, Header/pc unter `/app`.
- Gezielte Bereitstellung nur des Hintergrundmoduls und Helfers erfolgreich;
  eigene Symlinks im Flatpak lesbar. Die normale Glass-Installation bleibt unverändert.
- Tatsächliche GEGL-Renderings beider Versionen, gleiche Eingaben und Parameter.
- Bestehende gezielte Hintergrundtests bestanden: endliche Werte, Alpha 0…1,
  IOR=1/Brechung=0/Pixelprofil aus, teiltransparenter Hintergrund, konstantes HDR,
  keine doppelte Hintergrundalpha, Auflage ohne Brechung, getrennte Komponenten,
  Versatz und übereinstimmende Teilregionen/Gesamtbilder.
- Isolierter GIMP-Batchtest mit P1.4.1-Modul und Helfer: Registrierung und echter
  Prozeduraufruf, neuer Default 0,12, sichtbare native Pixeländerungen,
  Original-/Auswahlerhalt, Vordergrundausschluss, Deaktivierung des vorhandenen
  Glass-Effekts nur in der duplizierten Formprojektion, Rückkopplungsschutz und Snapshot-XCF-Speichern/Neuladen.
- Statische Vertragsprüfungen und gezielte Shell-/Python-Syntaxprüfungen.

Eine neue manuelle GIMP-Dialog-/GUI-Undo-Abnahme wird nicht behauptet. Der
XCF-Test betrifft weiterhin das gerasterte Snapshot-Ergebnis, keinen editierbaren
Aux-Effekt. Die erneute visuelle Nutzerabnahme bleibt offen.

## Verbleibende Grenzen

Unverändert gelten die Grenzen aus [GLASS-P1.4.md](GLASS-P1.4.md): Vorderfläche zur
Referenzebene statt massiver Körper mit Rückseite, feste Studio-Umgebung,
Snapshot statt Live-Aktualisierung, RGB-Formebene im normalen Modus auf oberster
Hierarchieebene, kein aktiver Auswahlmaskeninput. Die begrenzte Mehrfachabtastung
ist kein vollständiger anisotroper Rekonstruktionsfilter und löst extreme
Verkleinerung, Fokusfalten oder Randpixelwiederholung nicht grundsätzlich.
Eine wirklich ruhigere physikalische Verzeichnung würde eine weitere Modellphase
benötigen; diese wird hier nicht begonnen.

## Aktualisieren und testen

**GIMP vollständig schließen.** Es genügt die Aktualisierung des Hintergrundmoduls
und Helfers; die normale eininputige Glass-Operation wurde nicht verändert:

```bash
git switch feature/nanotank-glass
git pull --ff-only
bash scripts/build-gegl.sh nanotank-glass-refraction && \
bash scripts/deploy-gegl-flatpak.sh nanotank-glass-refraction && \
bash scripts/build.sh nanotank-glass-background && \
bash scripts/install-flatpak.sh nanotank-glass-background
```

Danach GIMP starten, Ausgangsbild mit Alphaform über Text/Muster wählen und
**Filter → NanoTank → Glass – Hintergrundbrechung (Snapshot)…** aufrufen.
Tönung 0,12 gegen 0,20 vergleichen; für die konkave Form H=−30 testen. Zur Variation
weiterhin zum Ausgangsbild zurückkehren. Alte Snapshot-Bilder behalten ihre
Pixel unverändert; neue Ergebnisse sind als P1.4.1 gekennzeichnet.

Keine neue Workflow-Architektur, Regler, Reliefs, Auswahlhelfer, Rauheit, CI oder
Folgephase. Nach Push auf visuelle Rückmeldung warten.
