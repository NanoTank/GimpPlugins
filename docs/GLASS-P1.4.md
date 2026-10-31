# NanoTank Glass P1.4 – echte Hintergrundbrechung als Snapshot

Branch `feature/nanotank-glass`, Entwickler NanoTank.

**Nachträgliche Nutzerbeurteilung:** P1.4 ist im Grundsatz visuell positiv
abgenommen; Snapshot als Zwischenstand akzeptiert. Keine endgültige Workflow-
oder Produktionsfreigabe. Gezielter Folgestand: [GLASS-P1.4.1.md](GLASS-P1.4.1.md).

Die folgenden Angaben dokumentieren die ursprüngliche P1.4-Übergabe. P1.3 wurde vom Nutzer als
Geometriefortschritt positiv beurteilt; die endgültige Materialabnahme bleibt offen.
P1.4 ist eine erste nutzbare Hintergrundbrechung, **kein live aktualisierter
Hintergrundfilter und kein editierbarer XCF-Aux-Effekt**.

## Architekturentscheidung anhand des tatsächlichen GIMP-Verhaltens

GIMP 3.2.6 stellt `DrawableFilter.set_aux_input()` bereit. Ein Aux-Filter lässt sich
jedoch nicht als nichtdestruktiver Ebenenfilter anhängen: In einer isolierten
GIMP-Instanz wurde die Meldung
`effects with an 'aux' pad cannot be applied non-destructively` reproduziert;
die Effektliste blieb leer. Die Existenz der API allein beweist keine persistente
Unterstützung. Diese Grenze wird nicht durch fremde Pointer, externe temporäre
Bilddateien oder ein Umgehen der Core-Schutzprüfung kaschiert.

Öffentliche API-Referenzen:
[Zusatzinput setzen](https://developer.gimp.org/api/3.0/libgimp/method.DrawableFilter.set_aux_input.html),
[Effekt übernehmen](https://developer.gimp.org/api/3.0/libgimp/method.Drawable.merge_filter.html),
[neue Ebene aus sichtbarer Projektion](https://developer.gimp.org/api/3.0/libgimp/ctor.Layer.new_from_visible.html).

Daher zwei ausdrücklich unterschiedliche Wege:

- **Filter → NanoTank → Glass** bleibt die bisherige eininputige Operation
  `nanotank:glass`, mit P1.3-Geometrie, Altmodus und normalem Ebeneneffekt.
  Sie liest weiterhin keine anderen Ebenen.
- **Filter → NanoTank → Glass – Hintergrundbrechung (Snapshot)…** ist ein dünner
  GIMP-Helfer. Er erzeugt ein neues Ergebnisbild mit nativer GEGL-Brechung.
  Dieses Bild aktualisiert sich nicht, wenn später der Hintergrund geändert wird.

Die zweite native Operation heißt `nanotank:glass-refraction`. Ihre Quellen liegen
im selben Glass-Verzeichnis. Kompilierung und Deployment verwenden ausschließlich
die bestehenden gemeinsamen Skripte mit einem weiteren gezielten Modulnamen.
Eine Änderung der alten Operation auf einen Aux-Typ hätte deren NDE-Kompatibilität
gefährdet und wurde ausdrücklich vermieden.

## Was der Helfer tut – keine Python-Bildberechnung

1. Prüft genau eine sichtbare Formebene im Modus Normal auf oberster Ebene der
   Ebenenhierarchie. Gruppen unterhalb der Form sind als Hintergrund erlaubt.
2. Dupliziert das Bild im Speicher. **Das Ausgangsbild, seine Auswahl, Sichtbarkeit
   und Pixel bleiben unverändert.**
3. Erstellt in der Kopie eine Projektion ausschließlich der sichtbaren Ebenen
   unterhalb des Formobjekts. Das Objekt selbst und alle Vordergrundebenen sind
   ausgeschlossen. Gruppen-/Masken-/Filterdarstellung der unteren Ebenen kommt
   aus GIMPs tatsächlicher Projektion, nicht aus einem selbstgebauten Python-Mixer.
4. Erstellt einen separaten Form-Snapshot mit Bildkoordinaten/Versatz, Objektalpha,
   Maske und Ebenendeckung. Bereits vorhandene `nanotank:glass`-Effekte werden
   nur in der Kopie deaktiviert, damit ihre transparente Ausgabe nicht nochmals
   zur Formmaske und Doppelbeschichtung wird. Andere Effekte werden mitprojiziert.
5. Übergibt Hintergrund als `input` und Form als `aux` an native GEGL.
6. Übernimmt den transienten Effekt ausschließlich in die neu erzeugte
   Snapshot-Ebene. Im neuen Bild sind die ursprünglichen unteren Ebenen und die
   Quelle verborgen erhalten; Vordergrundebenen darüber bleiben sichtbar.
7. Kennzeichnet das Bild mit einem persistenten Snapshot-Parasiten und den
   geometrischen Parametern. Eine erneute Verarbeitung dieses Ergebnisbilds wird
   verweigert; zum Neuberechnen das unveränderte Ausgangsbild wählen.

Keine temporären PNGs, keine externen Hintergrunddateien und keine eingebrannten
Änderungen an der ursprünglichen Ebene. Der Snapshot selbst ist bewusst berechnet
und gerastert. Die behaltenen Quell-/Referenzebenen sind keine Live-Verknüpfung.
Zum Ändern der Materialparameter muss aus dem Ausgangsbild neu gerechnet werden.

## Warum ein vollständiger Hintergrundkomposit nötig ist

Ein verzerrter Hintergrund mit Glasalpha über seinem unverzerrten Original würde
doppelte Konturen und falsche Teiltransparenz erzeugen. Für beliebige RGBA-Werte
kann eine normale transparente Auflage den ursprünglichen Hintergrund nicht
allgemein korrekt ersetzen.

Die native Operation liefert deshalb einen **vollständigen Ersatzkomposit**:
außerhalb der Glasform unverändert, innerhalb das gebrochene Bild mit Material.
Der Helfer lässt darunter nicht nochmals denselben unverzerrten Hintergrund
sichtbar. Auch der Originalhintergrund bleibt in dessen eigenem Bild unverändert.

Bei Maskendeckung m werden der ursprüngliche und der Glas-kompositierte Hintergrund
premultipliziert interpoliert. Die Glasalpha entsteht aus gesampeltem Hintergrund-
alpha und Reflexion, nicht aus der Multiplikation mit einer zweiten Hintergrundkopie.
Auf opakem Hintergrund bleibt Alpha 1. Teiltransparentes Alpha wird tatsächlich
mitgebrochen. Bilineare Abtastung erfolgt premultipliziert; RGB wird am Ende einmal
in Straight-RGBA zurückgerechnet. Scene-linear RGB/HDR wird nicht auf 0…1 begrenzt.

Mit bekanntem Hintergrund lässt sich die Glasfarbe jetzt als kanalabhängige
Absorption des übertragenen Bildes anwenden, statt eines farbigen Ersatzfilms.
Die alte Glasdeckkraft/Tönungsdichte bleibt in der alten Operation unverändert;
der neue Snapshot verwendet Tönungsfarbe und Tönungsintensität zur Transmission.
Es gibt bewusst keinen zweiten weißen oder grauen Grundfilm.

## Brechung aus der P1.3-Geometrie

`glass-geometry.h` liefert unverändert signierte Höhe, Normalen und positiven
angenäherten optischen Weg je zusammenhängender Komponente. `glass-optics.h`
teilt das P1.3-Studioreflexionsmodell zwischen beiden nativen Operationen.
Keine zusätzliche Renderengine je Form und keine neuen Reliefsysteme.

Für Blickrichtung I=(0,0,−1), Oberflächennormale N und η=1/IOR:

`T = η I + (η Nz − sqrt(1−η²(1−Nz²))) N`.

Die XY-Verschiebung ist `Txy / (−Tz) × optischer Weg × Brechungsstärke`.
Höhe, Bereich, Kantenrundung, Glasdicke und IOR wirken dadurch auf die tatsächlich
abgetasteten Hintergrundkoordinaten. Konvexe und konkave Flächen verändern die
Abtastrichtung über ihre Normalen. Getrennte Komponenten behalten unabhängige
Profile. IOR 1, Stärke 0 oder deaktiviertes Pixelprofil erzeugen keine Verschiebung.
Die neue Operation ist ein neuer Compositor, kein Altmodus-Ersatz; das Abschalten
des Pixelprofils ergibt dort eine flache Oberfläche, nicht die alte Faktorwölbung.

Das ist **Snell-Transmission an einer Vorderfläche zu einer Referenzebene**.
Es ist kein Raytracing durch Vorder- und Rückseite einer massiven Kugel. Der
positive P1.3-Weg ist eine 2,5D-Näherung. Hintergrund außerhalb der Bildgrenze ist
unbekannt; der Sampler wiederholt dort Randpixel (Clamp), statt schwarze Halos
aus transparentem Abyss hinzuzufügen. Große Verschiebungen können deshalb
sichtbare Randwiederholungen erzeugen.

Im Auflagemodus wird **keine Hintergrundbrechung** angewendet: Eine opake
Materialoberfläche soll nicht plötzlich Inhalte von darunterliegenden Ebenen
zeigen. Die Quellalpha-/Materialbedeutung bleibt erhalten; Reflexion und Absorption
werden auf die Form-RGB angewendet und regulär mit dem Hintergrund zusammengesetzt.

## Tatsächlich erzeugte visuelle Vergleiche

`tests/glass/refraction.c` rendert über echte GEGL-Operationen:

- Kreis R=100/H=100 über Raster, Rand 0.
- Rechteck H=10/Bereich 15 % über Text und Raster, Rand 4.
- Organische Form H=40 über strukturiertem Hintergrund.
- Konkave Form H=−30 über Text und Raster.
- Zwei getrennte Objekte, jeweils H=40.
- Opake Auflage H=35, ohne/mit Brechungsregler: absichtlich gleich.

Reflexion 0,75, Glanz 0,6, IOR 1,5, Licht 225°, Dicke 12, Tönung 0,2.
Vergleich zu P1.3 nutzt die tatsächliche alte Operation über demselben Hintergrund.
Zusätzlich gibt es P1.4-Stärke 0/1 mit ansonsten identischem Compositor; damit lässt
sich die reine Brechungsänderung von der verbesserten Materialkomposition trennen.

Galerie unter `output/glass-p14/`, ohne Binärdateien in Git:

- `comparison.png`: ohne Glas / P1.3 / P1.4.
- `materials.png`: P1.4-Stärke 0/1, konkav, getrennt und Auflage.
- `details.png`: ausdrücklich 2× vergrößerte Ausschnitte und normale Größe.
- `index.html`: Bilder im Originalmaßstab.

Die Bilder wurden tatsächlich geöffnet: Rasterlinien wölben sich unter dem Kreis
sichtbar; organische Konturen führen zu eigenen Verzerrungsverläufen. Die ebene
Plattenmitte verzerrt korrekt nicht, am gerundeten Rand werden Raster/Text versetzt.
Der Materialgewinn ist deutlich, ersetzt aber keine visuelle Nutzerabnahme oder
einen Nachweis exakt gemessener realer Glasoptik.

## Durchgeführte Prüfungen und Grenzen der Aussage

- SDK-Kompilierung beider nativer Module gegen GEGL 0.4.72, Header/pc unter `/app`.
- Native Hintergrund-/Form-Inputs, endliche RGBA-Werte und Alpha 0…1.
- IOR=1, Stärke=0 und Pixelprofil aus: Identität bei ausgeschalteter Reflexion/Tönung,
  auch auf teiltransparentem Hintergrund. Konstantes HDR-RGBA bleibt unter Brechung
  konstant; keine doppelte Alpha-Überlagerung.
- Unabhängige getrennte Komponenten in der tatsächlichen Compositor-Ausgabe,
  nicht nur im Geometriekern. Nichtnull-Versatz des Forminputs korrekt ausgerichtet.
- Gesamtbild und angeforderte Teilregionen identisch, auch bei Teiltransparenz.
- Auflagemodus mit/ohne Hintergrundbrechungsregler identisch.
- Bestehende P1.3-Geometrie-, Alpha-, XML- und Kompatibilitätsprüfungen gegen die
  jetzt gemeinsame Optik bestanden. Alte eininputige Operation weiterhin vorhanden.
- 16 statische Vertragstests und Shell-/Python-Syntaxprüfungen bestanden.
- Produktiver Build beider Module erfolgreich; beide eigenen GEGL-Symlinks im
  Flatpak sichtbar. Helfer am tatsächlich von GIMP verwendeten Ort installiert;
  keine fremden Module, Suchpfade oder Einstellungen verändert.
- **Isolierter echter GIMP-3.2.6-Batchtest:** Menü-Prozedur registriert und über die
  PDB aufgerufen; native Pixeländerungen nachgewiesen, Ausgangsebenen/-auswahl
  unverändert, Vordergrund ausgeschlossen, vorhandener P1.3-Filter im Original
  weiterhin sichtbar, eigener Ergebnis-Snapshot als erneuter Input verweigert.
- Synthetischer GIMP-Snapshot als XCF gespeichert und neu geladen, persistente
  Kennzeichnung vorhanden und geprüfte Pixel identisch. Das ist ein **gerasterter
  Snapshot-Roundtrip**, kein editierbarer Aux-Effekt-Roundtrip.

Nicht praktisch abgenommen: interaktiver Standarddialog, Live-Vorschau,
GUI-Undo/Redo und sämtliche 16-/32-Bit-GIMP-Workflows. Der Helfer hat in P1.4
**keine Live-Vorschau und keinen live aktualisierten Hintergrund**. Die bestehende
P1.3-Ebenenfilteroberfläche bleibt dafür unverändert verfügbar.

Aktuelle Eingabegrenzen: RGB-Bild und einzelne normale, sichtbare Formebene auf oberster
Hierarchieebene, vollständig im Bildbereich. Eine verschachtelte Formgruppe oder
Form im anderen Mischmodus wird ausdrücklich abgelehnt. Hintergrundgruppen sind
erlaubt und werden durch GIMP projiziert; komplexe Gruppenfälle sind nicht als
eigene vollständige Abnahmematrix getestet. Aktive Auswahl dient nicht als
Geometrieinput; sie bleibt im Original erhalten und wird nur in der Kopie geleert.
Keine Auswahlmaskenbrücke in dieser Phase.

Große Bilder benötigen mehrere vollständige Speicherprojektionen und den bekannten
Poisson-Solver. 4K-Interaktivität ist nicht zugesichert. Die Lösung enthält keine
versteckten automatischen Aktualisierungen. Für eine spätere wirklich persistente
Live-Anbindung muss die GIMP-Aux-NDE-/Serialisierungsgrenze separat gelöst werden.

## Bauen, installieren und ausprobieren

**GIMP vollständig schließen.** Dann gezielt nur Glass und seinen Helfer:

```bash
git switch feature/nanotank-glass
git pull --ff-only
bash scripts/build-gegl.sh nanotank-glass && \
bash scripts/build-gegl.sh nanotank-glass-refraction && \
bash scripts/deploy-gegl-flatpak.sh nanotank-glass && \
bash scripts/deploy-gegl-flatpak.sh nanotank-glass-refraction && \
bash scripts/build.sh nanotank-glass-background && \
bash scripts/install-flatpak.sh nanotank-glass-background
```

Der Python-Build/-Installer kann jetzt gezielt genau diesen Helfer behandeln;
der gezielte Installer fasst keine nativen oder anderen Python-Module an.
Die produktive Installation dieses Helfers läuft im Flatpak-Kontext und ermittelt
den Plug-in-Ordner mit `Gimp.directory()`. Der früher angenommene Host-XDG-Pfad
war hier innerhalb der Sandbox nicht sichtbar. Keine zusätzlichen Suchpfade oder
GIMP-Konfiguration wurden dafür geändert. Explizite Testprofil-Overrides bleiben
möglich. Die eigene markierte Fehlkopie des ersten Installationsversuchs wurde
gezielt zurückgenommen; keine anderen Installationen wurden bereinigt.
Native Build-/Deploy-Schutzprüfungen tolerieren kurz auslaufende SDK-Sitzungen,
verweigern aber den Austausch bei weiterhin registriertem GIMP.

GIMP starten, eine Alphaform über Text oder Muster auswählen. Unter
**Filter → NanoTank → Glass – Hintergrundbrechung (Snapshot)…** zunächst H=100,
Bereich 100, Rand 0, IOR 1,5, Stärke 1 für einen Kreis R=100 testen. Für die Platte:
H=10, Bereich 15, Rand 4; Text auch über den Rand führen. Negativtest H=−30.
Der normale Prozedurdialog öffnet nach Bestätigung ein **neues** Ergebnisbild.
Zum Variieren zum ursprünglichen Bild wechseln und den Helfer erneut aufrufen.
P1.3-Glaseffekte im Original müssen nicht gelöscht werden; sie werden nur in der
Projektionskopie deaktiviert. Für eine Materialauflage den entsprechenden Schalter
verwenden und keinen Durchblick durch den opaken Untergrund erwarten.

## Reproduzierbare lokale Tests ohne produktives Deployment

```bash
NANOTANK_GEGL_BUILD_DIR="$PWD/output/glass-p14/gegl" \
  bash scripts/build-gegl.sh nanotank-glass
NANOTANK_GEGL_BUILD_DIR="$PWD/output/glass-p14/gegl" \
  bash scripts/build-gegl.sh nanotank-glass-refraction
bash tests/glass/run-p14.sh
python3 tests/glass/gallery-p14.py
# Registrierung des Helfers nur im isolierten GIMP-Testprofil:
bash scripts/build.sh nanotank-glass-background
NANOTANK_GIMP_PLUGINS_DIR="$PWD/output/glass-p14/probe-profile/plug-ins" \
NANOTANK_GEGL_PLUGINS_DIR="$PWD/output/glass-p14/probe-gegl" \
  bash scripts/install-flatpak.sh nanotank-glass-background
bash tests/glass/run-gimp-p14.sh
```

Testbilder/XCF, Prozedurcache und Logs bleiben unter `output/`. Test-GIMP verwendet
ein eigenes Profil und keine offenen Nutzerbilder. Keine CI, externen Dienste,
Releases, History-Rewrites oder automatische Folgephase.
