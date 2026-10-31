# NanoTank Glass P1 – Teststand und Übergabe

Stand: 31.10.2026. Entwicklung ausschließlich `feature/nanotank-glass`.
P1 wurde inzwischen vom Nutzer im GIMP getestet, visuell jedoch nicht abgenommen.
Aktueller Folgestand: [GLASS-P1.1.md](GLASS-P1.1.md). Die folgenden Angaben dokumentieren die P1-Übergabe.

## Vergleich und Integration

Der bisherige Glass-Branch bietet die bereits im GIMP-Menü bestätigte Operation,
RGBA-float im Quellfarbraum, begrenzte Tile-Verarbeitung und die gemeinsame
Flatpak-SDK-/Deployment-Infrastruktur. Seine vier Alpha-Abfragen erzeugten
breite, diskontinuierliche Kantennormalen. Der feste Fresnel-Wert entsprach nur
IOR 1,5, ein Reflexstreifen war in Bildkoordinaten aufgemalt. Transparentes
Glas erhielt überall konstantes Alpha, auch an Reflexionen.

Der alternative Branch `origin/feature/nanotank-glass-p0` bietet ein analytisches
abgerundetes Rechteck-Abstandsfeld, einen echten IOR-abhängigen Fresnel-Term
und Straight-alpha-Over-Komposition. Seine Alpha-Geometrie verwendet ebenfalls
nur vier Abfragen; die Normalen des Rechtecks folgen der Bildmitte statt dem
Abstandsfeldgradienten. Beide Prototypen interpolierten Straight-RGBA direkt,
was unsichtbare RGB-Werte in sichtbare Kanten mischen kann. Der alternative
Build konnte Host-GEGL verwenden und ersetzt die gemeinsame Infrastruktur;
dieser Teil wird nicht übernommen.

Gezielt übernommen wird die mathematische Idee des Abstandsfelds und des
IOR-basierten Fresnel-Terms (neu in die bestehende Operation integriert).
Keine pauschale Übernahme, kein zweites Modul, kein eigenes Preset-Rendering.
Das neue begrenzte Chamfer-Abstandsfeld verarbeitet Rechteck und Alpha-Kontur
mit derselben Kantenberechnung. Zwei Sweep-Durchläufe, diagonale Kosten √2;
keine exakte euklidische Distanz. Gepaddete Tiles begrenzen den Speicherbedarf.

Remote-Korrekturen bis d37c7df wurden per Fast-forward übernommen, `origin/main`
75fd3bd anschließend konfliktfrei mit Merge-Commit integriert. Beide Quellbranches
bleiben erhalten; keine Historie wurde überschrieben.

## Implementiert

- Native GEGL-Area-Operation und unveränderter Menüpfad Filter → NanoTank → Glass.
- Kontinuierliches Kantenprofil, Normalen aus Abstandsfeldgradienten.
- Schlick-Fresnel aus Brechungsindex (Standard 1,5).
- Flächenlicht-Glanz aus Oberflächennormale und einstellbarer Lichtrichtung.
- Dezente absorptionsartige Tönung auf vorhandenem Material.
- Auflagemodus: Quellalpha bleibt exakt erhalten, auch bei Halbtransparenz.
- Eigenständiges Glas: Grunddeckkraft plus eigenständiges Reflexionsalpha;
  reflektierte Energie wird vor der Ausgabe korrekt in Straight-RGB umgerechnet.
- Optional Alpha-Kontur auch im transparenten Modus; standardmäßig Rechteck,
  damit auf einer leeren Ebene überhaupt Glas erzeugt werden kann.
- Premultiplizierte bilineare Abtastung, danach Rückwandlung zu Straight-RGB.
- IOR wirkt auf interne Pixelverschiebung und Reflexion. Keine Hintergrundbrechung.

## Tatsächlich geprüft

- 16 statische Python-Vertragstests bestanden.
- Native Kompilierung mit GIMP-Flatpak-SDK, GEGL 0.4.72, Header unter /app.
- `tests/glass/run.sh`: echtes Laden und Rendern durch GEGL; Prüfung auf
  endliche Werte, opakes und partielles Alpha, Glas auf leerem Input,
  unsichtbares gesättigtes RGB außerhalb einer Alpha-Ellipse, IOR-Wirkung,
  Objekte mit 1/5/9 Pixeln und Gleichheit von Gesamtbild und Teilregionen.
- Gezieltes Deployment nur von Glass: vorhandener eigener Symlink ist im
  GIMP-Flatpak lesbar; Diagnose bestanden. Keine fremden Module verändert.
- Drei synthetische PNG-Vorschauen erzeugt und visuell geprüft:
  `output/glass-p1-0.png` (Auflage), `-1.png` (Alpha-Ellipse),
  `-2.png` (transparentes Glas über Schachbrett). Untracked, keine privaten Bilder.
- Float-Rendering getestet; GIMP-Konvertierung nach 8/16/32 Bit bleibt offen.

Nicht hier nachgewiesen: neuer GIMP-Dialogtest, Live-Regler, Undo/Redo,
XCF-Persistenz, Export aus GIMP, große Bilder und abschließende Nutzerabnahme.
Die bereits bestätigte P0-Menüregistrierung ersetzt diese Prüfungen nicht.

## Bekannte Grenzen und nächste Architektur

Die Auswahlmaske wird von GIMP nach der Operation angewendet. Auf einer opaken
Ebene folgt die Geometrie weiterhin der Ebenengrenze, nicht einer inneren freien
Auswahl. Für den ersten verlässlichen Formtest ein freigestelltes Alpha-Objekt
verwenden und „Glasform aus Ebenen-Alpha“ einschalten. Stark halbtransparente
Flächen können als verbreiterte Randzone erscheinen; das Alpha wird nicht als
separate geometrische Maske geliefert. Keine exakte Geometrie aus Farbe ableiten.

Sauberer späterer Weg: GEGL-Compositor mit separatem Maskenpad und optionalem
Hintergrundpad. Ein dünner GIMP-Helfer liest die tatsächliche Auswahl und erzeugt
einen persistenten Masken-Snapshot mit korrektem Ebenenversatz. Die native
Operation berechnet daraus Geometrie; GIMP maskiert das Ergebnis weiterhin.
Vor Umsetzung sind DrawableFilter-Zusatzinputs und ihre XCF-Persistenz mit der
installierten libgimp-Version real zu prüfen. Keine externen Buffer-Pointer in
persistenten Parametern speichern. Ein Helfer braucht eine eindeutige Strategie
für erneutes Bearbeiten und aktualisierte Auswahlen; nicht still jedes Mal die
aktuelle Auswahl anstelle der gespeicherten Geometrie verwenden.

Hintergrundbrechung braucht eine Projektion ausschließlich der sichtbaren Ebenen
unterhalb des Zielobjekts, mit Ebenenversatz, Masken und Gruppen berücksichtigt.
Ein Snapshot ist ohne weitere Integration nicht live: Änderungen darunter müssen
explizit aktualisiert werden. Eigene Ausgabe und darüberliegende Ebenen dürfen
nicht in diesen Input gelangen. Transparente gebrochene Hintergründe lassen sich
nicht einfach über das unverzerrte Original legen, ohne doppelte Bildanteile;
dafür ist eine definierte Compositing-Integration erforderlich. Diese Phase
wird zurückgestellt, statt falsche Hintergrundinformationen zu verzerren.

## Minimaler Test auf dem Zielsystem

GIMP vor **Build und Deployment** vollständig schließen: ein installierter
Symlink kann bereits beim Build auf das neu gebaute Modul zeigen.

```bash
git switch feature/nanotank-glass
git pull --ff-only
bash scripts/build-gegl.sh nanotank-glass && \
bash scripts/deploy-gegl-flatpak.sh nanotank-glass && \
bash scripts/diagnose-gegl-flatpak.sh nanotank-glass
# Optionaler automatisierter Bildtest, ohne Änderung installierter Module:
bash tests/glass/run.sh
```

GIMP neu starten und eine Testkopie verwenden:

1. Opaken Button als eigene Ebene öffnen, „Untergrund und Alpha erhalten“ an.
   Bei freigestelltem Button „Glasform aus Ebenen-Alpha“ an. Licht auf 45/225°,
   Dicke auf 4/12/24, IOR auf 1/1,5/2 und Reflexion variieren. Alpha bleibt gleich.
2. Für transparentes Glas eine leere Ebene über Text/Schachbrett anlegen.
   „Untergrund und Alpha erhalten“ aus, „Glasform aus Ebenen-Alpha“ aus.
   Zunächst volle rechteckige Ebene verwenden: Deckkraft 0,08/0,32/0,7,
   Tönung und Licht variieren. Der Hintergrund bleibt unverzerrt sichtbar.
3. Freie und weiche Auswahl prüfen: Wirkung außerhalb unverändert; der neue
   Glasrand folgt hier noch nicht automatisch der Auswahlkontur.
4. Vorschau umschalten, mehrfach Regler ändern, bestätigen, Effekt wieder öffnen,
   Undo/Redo; XCF speichern und neu öffnen, anschließend PNG exportieren.
5. Auf hellem/dunklem Hintergrund sowie 8-, 16- und 32-Bit testen.

Nach dieser Abnahme erst weitere Funktionen entwickeln; kein Merge nach main.
