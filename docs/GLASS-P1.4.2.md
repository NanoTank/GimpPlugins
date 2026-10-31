# NanoTank Glass P1.4.2 – Vorschau und Bedienbarkeit

Branch `feature/nanotank-glass`, Entwickler **NanoTank**.
P1.4.1 bleibt der bildbasiert positiv beurteilte Materialstand. Der Nutzer hat
inzwischen die Alphaform, Altmodus, Reflexion, Glanz, Tönung und transparentes
Material im normalen GIMP-Ebenenfilter bestätigt. **Kein bestätigter Alphafehler.**
P1.4.2 verändert keine Materialformeln und wartet auf die praktische Dialogabnahme.

## Ursache und gezielte Änderungen

Der normale Filter baute bei jeder Auswertung die gesamte Pixelgeometrie neu auf.
Damit liefen Komponentenlabelung und Poisson-SOR auch bei reinen Materialänderungen
erneut. Beim 319 × 179 px großen Rechteck benötigte die vollständige Geometrie
rund 440 ms, der gesamte bisherige GEGL-Render rund 470 ms. Der Solver brauchte
920 Durchläufe; beim Kreis 665, bei getrennten Formen 589. Das erklärt den
Engpass, reproduziert auf diesem Rechner aber keine mehrsekündige Dialogblockade.

- `glass-geometry.h` trennt das konturabhängige Feld vom daraus abgeleiteten
  Höhenprofil. Der bisherige Aufruf bleibt als kompatibler Wrapper erhalten.
- Der normale Filter hält Feld und Profil **pro Operationsinstanz**. Reflexion,
  Glanz, Tönung, Lichtwinkel und IOR benötigen keinen neuen Solverlauf.
  Höhe, Bereich, Rundung und Dicke leiten nur das Profil erneut ab.
- Jede Auswertung liest die aktuelle Alphadeckung. Der Cache vergleicht die
  vollständige binäre Kontur oberhalb der bisherigen Schwelle 0,001 exakt,
  einschließlich ihrer Abmessungen. Kein Vertrauen auf Bufferadresse oder Hash.
  Geänderte Löcher, Komponenten oder Konturen lösen eine Neuberechnung aus.
  Unveränderte Kontur bei anderen RGB-Werten oder anteiligem Alpha benötigt
  keinen Solver; Farbe und tatsächliches Alpha werden dennoch aktuell verarbeitet.
- Solver und Cache arbeiten auf der eng begrenzten Konturfläche. Aktuelle Position
  und Ebenengrenzen bestimmen die Zuordnung; eine reine Verschiebung verändert
  das geometrische Profil nicht. Getrennte Komponenten bleiben unabhängig.
- Bei Alphaformen werden Farbpixel nur um die belegte Fläche einschließlich
  Abtastreserve gelesen und gerendert. Außerhalb wird im Auflagemodus die Quelle
  übernommen, im transparenten Modus RGBA null geschrieben. Auch sehr schwache
  Alphaanteile unterhalb der Geometrieschwelle bleiben bei der Ausgabe berücksichtigt.
- Ein Mutex schützt Aufbau, Vergleich und Kopieren eines Instanzcaches.
  Keine geteilten Geometriefelder zwischen Filtern. Der Cache wird mit der
  Operation freigegeben und nicht in XCF/XML gespeichert. GEGLs bisherige globale
  Konturabhängigkeit und vollständige Invalidierung bleiben erhalten.

**Keine Änderung** an SOR-Reihenfolge, Toleranz 1e-8, Grenze 3000, Höhenprofil,
Normalen, Licht-, Brechungs-, Alpha- oder Farbrechnung. Eine Vorberechnung der
Solver-Nachbarn brachte keinen belastbaren Zusatzgewinn und wurde verworfen.
Keine verringerte Bildqualität zur Beschleunigung.

## Messungen

Lokal: AMD Ryzen 9 7950X, GIMP 3.2.6 Flatpak, GEGL 0.4.72; Build im passenden
GIMP-SDK. Referenz **bc5ad2c** (unverändertes P1.4.1). Beide Versionen wurden
sequentiell mit identischen Eingaben gemessen. Alle Fälle verwenden Alphakontur,
Pixelprofil und transparentes Material. Das Rechteck ist 319 × 179 px groß;
1080p und 4K bezeichnen die **Ebene mit diesem kleinen Objekt**, keine vollständig
gefüllte 4K-Geometrie. Kreis: Durchmesser 200 px; getrennte Formen: Rechteck und Kreis.

Je Fall eine Erstberechnung, Höhe +10 → −20, Reflexion 0,75 → 0,60 und vier
aufeinanderfolgende Lichtwinkeländerungen. Die letzte Spalte ist deren Median.
Zeiten in **Millisekunden**, keine garantierten Reaktionszeiten anderer Rechner.

### Tatsächliche GEGL-Operation einschließlich Float-Auslesung

Je Tabellenzelle **P1.4.1 → P1.4.2**.

| Motiv / Ebene | Erstberechnung | Höhe ändern | Material ändern | Wiederholungen |
|---|---:|---:|---:|---:|
| Button / 1024 × 768 | 472.7 → 442.3 | 468.2 → 12.9 | 468.3 → 10.5 | 468.2 → 10.6 |
| Kreis / 512 × 512 | 183.3 → 177.7 | 184.4 → 5.7 | 186.0 → 5.0 | 184.5 → 5.1 |
| Getrennte Objekte / 1024 × 768 | 224.9 → 206.9 | 214.9 → 10.3 | 214.6 → 9.1 | 213.9 → 9.1 |
| Button / 1920 × 1080 | 509.1 → 453.9 | 494.2 → 20.1 | 494.4 → 16.5 | 493.7 → 16.4 |
| Button / 3840 × 2160 | 735.3 → 503.6 | 674.8 → 47.2 | 676.8 → 45.8 | 675.8 → 42.1 |


### Tatsächliche GIMP-Ebenenfilter

Isoliertes GIMP-Testprofil, echter nichtdestruktiver `DrawableFilter`, Änderungen
am vorhandenen Filter und erzwungene sichtbare Bildprojektion nach jeder Änderung.
Die Zeit enthält die vollständige Projektion und deren Kopie in eine temporäre
Testebene, nicht nur den GEGL-Kern. GIMP-Programmstart ist nicht enthalten.
**Batchbetrieb, keine Messung von Maus-/Reglereingaben im interaktiven Dialog.**

| Motiv / Ebene | Erstberechnung | Höhe ändern | Material ändern | Wiederholungen |
|---|---:|---:|---:|---:|
| Button / 1024 × 768 | 667.1 → 643.9 | 475.4 → 26.4 | 475.5 → 24.9 | 474.0 → 25.4 |
| Kreis / 512 × 512 | 192.5 → 182.7 | 184.9 → 11.7 | 188.5 → 11.0 | 185.0 → 10.1 |
| Getrennte Objekte / 1024 × 768 | 246.1 → 223.7 | 228.9 → 24.4 | 224.6 → 22.2 | 238.5 → 22.3 |
| Button / 1920 × 1080 | 530.4 → 488.0 | 506.5 → 53.3 | 485.9 → 51.4 | 487.5 → 49.2 |
| Button / 3840 × 2160 | 764.2 → 631.2 | 741.1 → 191.6 | 709.2 → 175.7 | 710.8 → 171.8 |

Die Wiederverwendung bleibt also auch innerhalb des tatsächlichen GIMP-Filters
wirksam. Beim 4K-Fall enthält die GIMP-Zeit erhebliche Kosten der kompletten
Bildprojektion. Das ist kein Nachweis einer bestimmten Bildrate des Dialogs.
Eine interaktive GIMP-Dialogprüfung mit echten Nutzerbildern bleibt offen;
hier stand keine Desktop-Bedienautomation zur Verfügung. Es wird ausdrücklich
keine bestandene praktische Nutzerabnahme behauptet.

## Bedienung und Kompatibilität

Property-IDs, Typen, Wertebereiche und Standardwerte bleiben unverändert.
Gespeicherte Effekte behalten ihre Bedeutung, `pixel-geometry` bleibt standardmäßig
ausgeschaltet. Kein stiller Wechsel alter Projekte.

- **Glasauflage: Untergrund und Alpha erhalten** erklärt im Tooltip beide
  Materialmodi: ursprüngliches Material erhalten oder durch transparentes
  Glasmaterial ersetzen.
- **Tönungsdichte (Glasdeckkraft)** erklärt, dass sie nicht die gesamte
  Ausgabe-Deckkraft steuert; Reflexionen und Glanz ergänzen Alpha unabhängig.
- Alte Regler bleiben mit **Altmodus**, die drei neuen Regler explizit mit
  **Pixelprofil** gekennzeichnet. Die Tooltips erläutern die Zuständigkeit.
  Die normale GIMP-Oberfläche bleibt bestehen; Regler werden weiterhin angezeigt
  und nicht dynamisch deaktiviert. Kein neuer Dialog.

## Bestandene Prüfungen

- SDK-Build und tatsächliche GEGL-Registrierung beider Glass-Module.
- **35 tatsächliche GEGL-Renderings bitidentisch** zu bc5ad2c, einschließlich
  negativer Höhe, Material- und wiederholter Lichtänderungen in allen fünf Fällen.
- Cachetest: echte Treffer ohne erneuten Feld-/Profilaufbau, Profiländerung ohne
  Solver, Änderung/Zurücksetzen eines inneren Alphalochs bei unveränderter Bounding
  Box, Teilalpha, Schwellenwechsel, leere und winzige getrennte Formen sowie vier
  konkurrierende Aufrufer derselben Instanz. Alle Ausgabearrays bitidentisch zum
  ungekürzten, ungecachten Feld.
- P1.3-Regression: analytischer Kreis, positive/negative Höhen, Plateau,
  Dickenunabhängigkeit, Komponenten, Altmodus-XML, Pixelprofil-XML, Alpha/HDR,
  Teilregionen und verschobene Kontur. Bekannte XML-Farbserialisierungsabweichung
  bleibt separat: Altmodus maximal 0,0001602; Pixelprofil 0,0000032.
- P1.4-Regression: Snell, positive/negative Geometrie, IOR=1, Brechung=0,
  deaktiviertes Pixelprofil, opake Auflage, getrennte Komponenten, Alpha/HDR, Tiles.
- Tatsächlicher GIMP-Batchtest des normalen Ebenenfilters in fünf Fällen;
  synthetische XCF mit gespeichertem negativem Pixelprofil erneut geladen.
- Tatsächlicher GIMP-Snapshot-Helfertest: registrierter Aufruf, veränderte native
  Pixel, Quellschutz, Rückkopplungsverweigerung und Snapshot-XCF-Neuladen.
- 16 bestehende statische Vertragstests bestanden. Diese ersetzen keine Bildtests.

Gezielte Prüfungen: `bash tests/glass/run-p142.sh`.
Vollständige lokale Reproduktion der Messungen:
`bash tests/glass/run-p142.sh --benchmark`.
Der Benchmark nutzt ausschließlich die gemeinsame Build-Infrastruktur. Rohzeiten,
Logs, synthetische XCF und Float-Referenzen liegen unversioniert unter
`output/glass-p142/`. Die vollständigen Float-Vergleiche benötigen mehrere GB
lokalen Speicher. Keine neuen Vergleichsgalerien oder Binärdateien in Git.

## Grenzen und praktische Abnahme

Der erste Solverlauf und echte Konturänderungen bleiben teuer. Bei sehr großen,
vollständig gefüllten oder komplizierten Formen kann dieser unveränderte Solver
weiterhin lange rechnen oder die bestehende Konvergenzgrenze erreichen. Die
Messung kleiner Objekte auf einer 4K-Ebene belegt keine schnelle vollständig
gefüllte 4K-Linse. Der Instanzcache hält ungefähr 32 Byte pro Pixel seiner
Kontur-Bounding-Box zuzüglich Komponentenmetadaten; bei großen, weit auseinanderliegenden Formen entsprechend mehr.
Die aktuelle Alphakontur wird weiterhin bei jeder Auswertung gelesen.

Undo/Redo durch die GIMP-Oberfläche, interaktives erneutes Öffnen des Dialogs,
Reglerflüssigkeit und Export bleiben praktisch zu prüfen. Der Cache kennt keine
Undo-Historie; zurückgesetzte Eingabepixel werden anhand ihrer tatsächlichen
Kontur neu geprüft. Kontur-Restore ist automatisiert abgedeckt. XCF-Neuladen
baut eine neue Instanz auf. Snapshot-Workflow, Hintergrundinput und bekannte
optische Grenzen von P1.4.1 bleiben unverändert.

## Aktualisieren

**GIMP vollständig schließen**, dann im Repository:

```bash
git switch feature/nanotank-glass
git pull --ff-only
bash scripts/build-gegl.sh nanotank-glass && \
bash scripts/deploy-gegl-flatpak.sh nanotank-glass
```

Nur das normale Modul muss aktualisiert werden. Das Hintergrundmodul wurde wegen
des gemeinsamen Headers mitgebaut und geprüft, benötigt aber keine Neuinstallation:
sein kompatibler Wrapper liefert weiterhin dieselben Ergebnisse. Der Python-Helfer
ist unverändert. Die produktive GIMP-Installation wurde hier nicht verändert.

Danach am vorhandenen roten Button **Filter → NanoTank → Glass** testen:
Alphakontur und Pixelhöhenprofil einschalten; Reflexion, Glanz, Tönung und Licht
mehrfach ändern, danach Höhe/Bereich/Rundung. Anschließend Kontur bearbeiten,
Undo/Redo, Effekt erneut öffnen und synthetische XCF speichern/neuladen.
Beide Materialmodi prüfen. Eine Verzögerung beim ersten Aktivieren ist weiterhin
möglich; spätere Materialänderungen sollen keine neue Poisson-Berechnung auslösen.

Keine weitere Entwicklungsphase vor praktischer Nutzerabnahme. Kein Merge nach main.
