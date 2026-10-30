# NanoTank Glass — Spezifikation P0

**Stand:** 30.10.2026. **Branch:** `feature/nanotank-glass`. **Status:** Quellprototyp, Flatpak-Build und GIMP-Abnahme noch offen.

## Zweck

- Menü: Filter → NanoTank → Glass.
- Anwendungsfälle: Glaslack auf opakem Schild, transparentes Glas auf separater Ebene, leicht getöntes Schild.
- Nicht-Ziele P0: Zugriff auf sichtbare Hintergrundkomposition, exakte Fase entlang freier Auswahl auf opaker Ebene, echte 3D-Raytracing-Physik, Milchglas/Rauheit, Lupe und Presets.

## Architektur

- Native GEGL-C-Operation: `nanotank:glass`; `GEGL_OP_AREA_FILTER`.
- Quelle: `gegl/nanotank-glass/nanotank-glass.c`; Modul: `output/gegl/nanotank-glass.so` (nicht versioniert).
- Gemeinsames GIMP-Flatpak-SDK: `bash scripts/build-gegl.sh nanotank-glass`.
- Gezieltes Deployment: `bash scripts/deploy-gegl-flatpak.sh nanotank-glass`; Diagnose: `bash scripts/diagnose-gegl-flatpak.sh nanotank-glass`; Entfernen: `bash scripts/undeploy-gegl-flatpak.sh nanotank-glass`.
- Bilddaten: RGBA float im Input-Farbraum, Kantenanalyse in lokal erweiterten Tiles, transparenter Abyss, Alpha separat von RGB behandelt.
- GIMP übernimmt Vorschau, Filterbegrenzung auf die aktive Auswahl und theoretisch nicht-destruktiven Ebenenfilter. Undo/XCF und tatsächliche GIMP-Registrierung müssen noch praktisch nachgewiesen werden.
- Keine Python-/Script-Fu-Bildberechnung.

## Bedienung

| Regler | Bereich | Standard | Wirkung |
| --- | --- | --- | --- |
| Glasdicke (Pixel) | 1–64 | 12 | Kantenbreite |
| Kantenwölbung | 0–1 | 0,65 | optisches Kantenprofil, keine geometrische Eckenänderung |
| Untergrund und Alpha erhalten | Ein/Aus | Ein | Ein: Quelle bleibt opak/Alpha unverändert |
| Glasdeckkraft | 0,02–1 | 0,32 | Ausgabe-Alpha, nur bei ausgeschaltetem Alpha-Erhalt |
| Tönungsfarbe | Farbwähler | #bcdff7 | Glasfarbe |
| Tönungsintensität | 0–1 | 0,20 | Farbanteil |
| Reflexion | 0–1 | 0,75 | Fresnel-Näherung nach Schlick (F0 = 0,04) |
| Glanzlichter | 0–1 | 0,60 | Richtungsglanz und weicher Reflexstreifen |
| Interne Lichtbrechung (Pixel) | 0–10 | 2,5 | verschiebt Pixel der eigenen Ebene |
| Wölbung | −1 bis 1 | 0,30 | begrenzte kissenförmige 2D-Brechung |

## Wichtige Grenzen

1. **Hintergrund:** Eine GEGL-Layeroperation erhält nicht automatisch die Projektion anderer Ebenen. Glas auf einer transparenten Ebene kann den sichtbaren Hintergrund nicht brechen; derzeit nur interne Pixelverschiebung.
2. **Auswahl:** GIMP maskiert die Filterausgabe auf eine beliebige (auch weiche) Auswahl, liefert die Auswahlkontur aber nicht als separates GEGL-Pad. Bei vollständig opaker aktiver Ebene wird das Kantenprofil aus der Ebenengrenze berechnet, nicht aus der inneren freien Auswahl. Bei freigestelltem Alpha-Objekt kann die Alpha-Silhouette für Kanten genutzt werden. Diese Einschränkung ist bei runden opaken Buttons besonders wichtig.
3. **Alpha-Vertrag:** Alpha-Erhalt an: jeder Output-Alphawert entspricht dem Quellpixel, opake Schilder bleiben opak. Alpha-Erhalt aus: Ausgabe-Alpha = Glasdeckkraft; auf einer **separaten leeren transparenten Ebene mit Auswahl** nutzen, nicht auf dem wertvollen Original.
4. **Physik:** Schlick-Fresnel und Kantenbeleuchtung sind 2D-Annäherungen, keine Messung echter Spiegelungen, kein vollständiges Materialmodell; keine automatischen abgerundeten Ecken.
5. **Qualität:** 16/32-bit, HDR, 4K-Leistung, GIMP-XCF und Alpha an weichen Auswahlrändern müssen noch praktisch geprüft werden.

## Minimaler Test auf dem Zielrechner

GIMP **vollständig schließen**, dann:

    cd ~/Projekte/GimpPlugins
    git fetch origin
    git switch feature/nanotank-glass
    git pull --ff-only
    python3 -m unittest discover -s tests -p 'test_*contract.py'
    bash scripts/build-gegl.sh nanotank-glass && \
    bash scripts/deploy-gegl-flatpak.sh nanotank-glass && \
    bash scripts/diagnose-gegl-flatpak.sh nanotank-glass

Wenn der Branch lokal noch nicht vorhanden ist, einmalig zuvor:
    git switch -c feature/nanotank-glass --track origin/feature/nanotank-glass

Hinweis zum automatischen Deployment: Der Flatpak-SDK-Build startet eine
kurzlebige Sandbox mit der App-ID `org.gimp.GIMP`. `flatpak ps` kann sie noch
kurz als aktiv melden, auch wenn kein GIMP-Fenster geöffnet ist. Das
Deployment-Skript wartet bei einer gemeldeten Instanz daher höchstens rund
3 Sekunden und prüft erneut. Bei dauerhaft aktiver Instanz wird weiterhin
**nicht** installiert; die Instanzen werden zur Diagnose ausgegeben.
Ein späterer manueller Aufruf des Deploy-Skripts ist dann möglich.

GIMP neu starten und auf einer **Testkopie** Filter → NanoTank → Glass ausführen. Native C-Module können GIMP bei Fehlern beenden.

## Manuelle Abnahme-Checkliste

- [ ] GIMP-SDK/GEGL-Version geprüft, Native-Modul erfolgreich gebaut, keine Host-GEGL-Abhängigkeit.
- [ ] Menüeintrag Glass, deutsche Regler und Live-Vorschau.
- [ ] Opaker Metallbutton mit Alpha-Erhalt: optischer Effekt, Alpha bleibt erhalten.
- [ ] Transparente leere Ebene mit aktiver Auswahl, Alpha-Erhalt aus: Glasdeckkraft 0,25 und 0,75 sichtbar; darunterliegendes Bild sichtbar, aber **nicht** gebrochen.
- [ ] Alpha-freigestellter runder Button: Glanz entlang der Alphakante.
- [ ] Auswahlrechteck, freie Auswahl und weiche Auswahl: GIMP begrenzt das Ergebnis auf diese Maske.
- [ ] 8-bit sowie 16/32-bit (falls möglich); Bild- und Ebenenrandfälle.
- [ ] Undo/Redo, Ebenenfilter erneut öffnen, XCF speichern/öffnen und exportieren.
- [ ] Der bisherige NanoTank GEGL-Test bleibt unberührt.
- [ ] Nutzer hat tatsächliche Glaswirkung visuell abgenommen.

## Nächste Ausbaustufe

Nach visuellem Feedback: Kontur-Maskenbrücke (optional dünner GIMP-Helfer), echtes Hintergrund-Pad für Brechung/Lupe, Presets, Rauheit/Milchglas. Keine automatische Freigabe oder Merge.

**Keine CI-Workflows, kein automatischer Merge, keine Änderungen an fremden GIMP-Modulen.**
