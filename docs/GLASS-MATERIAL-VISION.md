# NanoTank Glass – verbindliche Materialvision

Stand: 31.10.2026. Diese Produktanforderungen gelten über P1.2 hinaus;
sie sind kein Auftrag, alle genannten Möglichkeiten sofort zu implementieren.

## Produkt und Bedienung

NanoTank Glass ist ein universeller nativer **2D-/2,5D-Materialfilter für GIMP**,
kein Glasbutton-Generator und kein 3D-Renderer. Buttons sind übersichtliche
Testobjekte. Masken und Bildinhalte dürfen keiner bestimmten Objektkategorie
angehören. Glas auf Schrift, Logos, organischen Formen, Gesichtssilhouetten,
Schildern und beliebigen Auswahlen gehört zur langfristigen Produktvision.

Es wird keine anatomisch korrekte Tiefenrekonstruktion aus Fotos verlangt.
Eine Gesichtssilhouette mit einem plausiblen maskenbasierten Glasprofil genügt;
ein exakt rekonstruierter dreidimensionaler Glaskopf ist kein Entwicklungsziel.
Kein Blender-Ersatz, kein GIMP-Fork, keine zusätzliche 3D-Infrastruktur.

Bedienziel: **Ebene oder Auswahl wählen → Glass aufrufen → Material einstellen
→ Ergebnis erhalten.** Nötige Eingaben sollen später automatisch und
nicht-destruktiv bereitgestellt werden, ohne manuelle Hilfsebenenketten.

## Zwei verschiedene Materialkonzepte

**Glasauflage / Glass Coating:** Das vorhandene Material bleibt Bildträger;
sein Alpha, seine Farben und Struktur bleiben erhalten. Eine zusätzliche
Glasoberfläche liefert Absorption, Reflexionen und geometrisch begründete
optische Veränderungen. Das Material muss durch Glasdeckkraft nicht transparent
werden. Hierfür ist der bestehende Alpha-Erhalt-Modus der technische Ausgangspunkt.

**Materialumwandlung / Glass Material:** Das Objekt selbst wird als Glas
interpretiert. Seine Maske bestimmt den Körper; Farbe und innere Struktur des
Ausgangsbildes müssen bewusst als Tönung, Relief oder Detail interpretiert oder
ersetzt werden. Alpha absenken allein ist keine vollständige Materialumwandlung.
Der bisherige transparente Modus rendert einen klaren, maskenbasierten Glaskörper,
verwirft dabei die Quell-RGB-Struktur und bietet noch keine vollständige
bildinhaltabhängige Umwandlung. Diese Grenze muss in der Bedienung/Abnahme klar bleiben.

## Allgemeine mathematische Grundlage

- Tatsächliche Auswahlmaske oder Objekt-Alpha als Geometrieeingabe.
- Abstandsfeld zur realen Kontur, analytische maskenbasierte Höhenprofile.
- Normalen aus demselben Höhenprofil, das Reflexionen und Brechung steuert.
- Einstellbare Materialdicke, Randprofil und Wölbung, unabhängig von der Position
  innerhalb einer größeren transparenten Ebene.
- Langfristig kontinuierlich steuerbare Profile: flache Platte, leicht gewölbte Beschichtung,
  Kugel-/Halbkugelprofil für Kreise, tropfenartige Wölbung für geeignete freie Formen.
  Ein Kreis muss wahlweise Glasscheibe oder gewölbtes Objekt werden können.
- Optional zusätzliche **2D-Relief-/Höheninformation**, wenn der Anwender innere
  Formen beeinflussen möchte. Keine automatische 3D-Rekonstruktion erforderlich.
- IOR und Geometrie erfüllen verschiedene Aufgaben: Wasser etwa 1,33 und Glas
  etwa 1,5; IOR allein macht eine Platte nicht zum Tropfen.
- Später Rauheit/Matte möglich, keine Materialbibliothek oder neuen Regler dafür in P1.2.

Ein nur rechteckiges oder elliptisches Profil, das auf eine organische Maske
beschnitten wird, erfüllt diese Vision nicht. Maske und Profil liefern eine
plausible 2,5D-Näherung, keine eindeutige Rekonstruktion eines realen Volumens.
Mehrere getrennte Objekte und sehr dünne Schriftzüge brauchen später sorgfältig
gewählte lokale Profilskalen; ein globales Maskenrechteck ist dafür nicht ausreichend.

## Weitere Stufen nach P1.2, separat zu beauftragen

1. Auswahlmaske als persistente Geometrieeingabe verfügbar machen. Falls ein
   dünner GIMP-Helfer nötig ist: nur Eingaben/Filteraufbau, keine Python-Bildengine.
2. Gemeinsame Höhen-/Bereichsparameter für kontinuierliche Oberflächenprofile
   einführen; keine Objektmodi, gemeinsame native Materialengine.
3. Optional Relief-/Detailinput mit definierter Skalierung und Normalenberechnung.
4. Hintergrundprojektion unterhalb des Objekts sicher bereitstellen und korrekt
   zusammensetzen: ohne Rückkopplung, mit Ebenenversatz, Masken und XCF-Persistenz.
5. Regeln für Quellstruktur bei Materialumwandlung festlegen, statt stilles
   RGB-Verwerfen als fertige Umwandlung auszugeben.

Diese Schritte erweitern die vorhandene GEGL-Architektur schrittweise. Sie sind
keine Voraussetzung für den begrenzten P1.2-Teststand und werden nicht ungefragt begonnen.

## Verbindliche Geometriesemantik: Wölbungshöhe in Pixeln

Langfristig ist die Wölbung eine geometrische **vorzeichenbehaftete Höhe H in Pixeln**, unabhängig
von Material-/Randdicke, kein nur abstrakter Faktor 0…1. Ein Kreis mit Radius R
bekommt das kontinuierliche ellipsoide Vorderseitenprofil

`z(r) = H × sqrt(1 - (r/R)²)`, für `0 ≤ r ≤ R`.

Bei R=100: H=0 flach, H=10 leicht gewölbt, H=50 Linsenform, H=100 Halbkugel,
H=150 axial gestrecktes Halbellipsoid. Keine getrennten Button-/Kugel-/Ei-Presets
nötig. Die Ausgangssilhouette bleibt erhalten. Normalen, Reflexion und Brechung
müssen aus dieser tatsächlichen Höhe folgen; die Höhe muss auch bei anderen
Objektgrößen geometrisch dieselbe Bedeutung behalten.

Die Ableitung wird am idealen Ellipsoidrand unbeschränkt. Die Implementierung
muss dort endliche, stetig zum Tangentialwinkel übergehende Normalen liefern,
z.B. über eine analytisch normierte Ellipsoidnormale und kontrollierte
Pixelabdeckung. Nicht einfach einen unendlichen Gradienten in RGB/Alpha tragen.
H=0 ist gesondert als flache Fläche zu behandeln. Medialachsen und sehr dünne
Formen dürfen ebenfalls keine Normalensprünge oder Kachelnähte erzeugen.

Für Ellipsen und organische Masken wird ein entsprechendes verallgemeinertes
Höhenprofil benötigt. Abstandsfeld und optionales 2D-Relief bleiben geeignete
Grundlagen, sind aber kein Beweis, dass jede komplexe Maske eine exakte Kugel
oder ein eindeutiges räumliches Objekt beschreibt.

### Abgrenzung und kompatible Folgephase nach P1.2

P1.2 verwendet weiterhin die vorhandene dimensionslose GEGL-Property `curvature`.
Die geglättete Konturkappe plus Randprofil erfüllt **nicht** die exakte Aussage
„H=R ergibt eine Halbkugel“. Sie darf daher nicht mit Pixelhöhen beschriftet
oder als bereits fertige Umsetzung dieser Anforderung präsentiert werden.

Die Folgephase muss ein explizites kompatibles Parameter-/Versionskonzept
vorlegen: bestehende gespeicherte `curvature`-Werte nicht still als Pixel lesen,
Legacy-Einstellungen weiterhin interpretieren und eine bewusste Umstellung auf
eine neue Höhenproperty ermöglichen. Ein erneutes Öffnen alter XCF-Effekte darf
keine unbemerkte Einheitenmigration auslösen. Die genaue GIMP-Serialisierung
und die Trennung von alten und neuen Werten sind praktisch zu prüfen.

Abnahmetests für diese Folgephase: R=100 mit H=0/10/50/100/150, weitere Radien,
Normals-/Höhenproben gegen das analytische Profil, stetige endliche Randnormalen,
Alpha unverändert, unabhängige Dicke, Größen-/Translationsinvarianz und
Alt-XCF-Roundtrip. Keine 3D-Infrastruktur und keine Materialbibliothek erforderlich.

## Verbindliche Erweiterung: Wölbungsbereich in Prozent

Neben Höhe H in Pixeln steuert künftig **Wölbungsbereich p in Prozent**, wie weit
die Wölbung von der tatsächlichen Kontur ins Objekt reicht. Bei 100 % erreicht
sie die maximale innere Distanz. Bei 15 % bleibt ein großer geometrisch ebener
Innenbereich. Beide Regler, nicht Objektmodi für Button/Linse/Kugel/Tropfen,
sollen die Übergänge kontinuierlich beschreiben. Die spätere Umsetzung ersetzt
damit die zunächst erwogene Auswahl geometrischer Objekt-Presets.

Ein möglicher gemeinsamer Profilansatz zur Prüfung in der Folgephase:

- `D` = maximale innere Konturdistanz, `w = (p/100) × D`.
- `q = clamp(d/w,0,1)`, `z(d) = H × sqrt(q × (2-q))`.
- Für `d >= w` ist die Höhe konstant H, der Gradient an diesem Übergang null.
- Beim Kreis und p=100 ist D=R; damit entspricht das Profil exakt dem oben
  beschriebenen halbellipsoiden Vorderseitenprofil. H=R ist eine Halbkugel.

Beispiel R=100: H=10/p=100 flache durchgehende Linse, H=10/p=15 ebene Mitte
mit gerundetem Außenbereich, H=50/p=100 starke Linse, H=100/p=100 Halbkugel,
H=150/p=100 gestrecktes Halbellipsoid. Ein Profil für alle fünf Fälle.

Dies ist eine mathematische Grundlage, noch keine implementierte Spezifikation
für alle Maskentopologien: Medialachsen, dünne Äste und getrennte Teilobjekte
benötigen stabile lokale Behandlung. Nullbreite, Subpixelbereiche und H=0 sind
explizit ohne Division durch null/Normalensprünge zu behandeln. Der Anschluss an
die ebene Mitte muss glatt bleiben; der äußere Ellipsoidrand braucht weiterhin
eine endliche analytisch normierte Grenzbehandlung.

Glasdicke/optische Weglänge, H und p sind getrennte Größen. Normalen, Fresnel,
Glanz und Brechung stammen aus **demselben** resultierenden Höhenfeld.
Bei H=R entsteht zunächst nur die sichtbare halbkugelige Vorderfläche; eine
vollständige Glaskugel benötigt später zusätzlich eine definierte Rückseite
bzw. optische Weglänge, keine behauptete vollständige Kugelsimulation.

Vor neuer UI „Kantenwölbung“ und die derzeitige Dicken-/Randkopplung prüfen:
keine redundanten Regler einführen. Die bestehenden Properties nicht still
umdeuten. Höhen- und Bereichsparameter gemeinsam mit nachvollziehbarer Legacy-
Interpretation und XCF-Kompatibilität in einer Folgephase umstellen. Zusätzliche
Abnahme: Kreis/Rechteck/organisch, p=15/100, verschiedene Größen, dünne Formen,
Höhen-/Normalenstetigkeit am Plateau, unveränderte Silhouette und Alt-XCFs.

**P1.2 implementiert H/p nicht.** Sein geglättetes Konturprofil und die bisherigen
Faktor-/Randregler dürfen nicht als fertige Umsetzung dieser Geometrieparameter
dargestellt werden. Keine Umfangsausweitung der laufenden Phase.


## Verbindliche Erweiterung: konkave Oberflächen und unabhängiger Rand

Die geometrische Wölbungshöhe H ist **signiert**: positive Werte wölben die
Oberfläche nach außen, negative nach innen. Beispiele: +10 px leicht konvex,
0 px flach, −3 px leicht konkav, −10 px deutliche Mulde. Diese Höhen sind relativ
zur Referenzfläche des inneren Oberflächenprofils zu verstehen, nicht als
negative absolute Materialdicke. Der Wölbungsbereich p bleibt unabhängig davon.

Mindestens drei geometrische Aspekte müssen unterscheidbar bleiben:

1. Außenkantenprofil: Rundung oder Fase.
2. Innere Oberflächenwölbung: signierte geometrische Höhe H.
3. Wölbungsbereich p: relative Ausdehnung dieser Oberflächenkrümmung.

Eine konkave Tastenkappenform kann einen gerundeten Außenrand und gleichzeitig
eine eingesenkte Mitte besitzen. Ein negatives H darf daher nicht einfach das
gesamte Randprofil invertieren. Konzeptuell kann ein gemeinsames Höhenfeld aus
einem unabhängigen Randprofil und dem signierten inneren Profil entstehen;
der Anschluss muss höhen- und normalenkontinuierlich ausgelegt werden. Eine
bloße Addition beliebiger Profile garantiert diese Anschlussbedingungen nicht.

Im oben beschriebenen inneren Ansatz kann H negativ eingesetzt werden:
`z_innen(d) = H × sqrt(q × (2-q))`. Damit kehrt sich der innere Höhenverlauf um,
nicht die Alphasilhouette. Dies ist eine Grundlage zur späteren Prüfung, noch
keine fertige Spezifikation für die Kombination mit einem gerundeten Außenrand.
Der Wölbungsbereich begrenzt die Krümmung auch bei negativem H; ein Plateau
jenseits dieses Bereichs liegt dann auf einem niedrigeren Referenzniveau.

Die Normalen müssen aus dem gemeinsamen tatsächlichen Höhenfeld folgen und
für beide Vorzeichen eine konsistente, zum Betrachter gerichtete Orientierung
behalten. Fresnel reagiert auf den Normalenwinkel, Reflexionsrichtung und
Glanzverteilung auf die geänderte Neigung; spätere Brechung muss dieselbe
Geometrie verwenden. Reflexe nicht lediglich vertauschen oder per Vorzeichen
von H aufmalen. H=0 sowie sehr kleine Objekte, schmale Bereiche, Medialachsen
und der äußere Profilrand benötigen weiterhin endliche, stetige Normalen.
Bei späterer optischer Weglänge/Rückseite zusätzlich sicherstellen, dass eine
Mulde keine unbeabsichtigt negative Materialdicke erzeugt.

Die Folgephase prüft positive und negative H bei Kreis, Rechteck und organischer
Maske, insbesondere gerundeten Rand mit konkaver Mitte, p=15/100, den stetigen
Durchgang durch H=0, unabhängige Außenrundung und unverändertes Alpha. Das
Legacy-/XCF-Konzept gilt unverändert: gespeicherte Faktorwerte nicht still zu
signierten Pixelhöhen umdeuten.

**Einordnung:** konzeptionelle Produktanforderung für die spätere Geometriephase.
Die vorhandene P1.2-Property `curvature` erlaubt bereits negative Faktorwerte;
das ist kein Nachweis der hier beschriebenen Pixelhöhe oder eines vollständig
kontrollierten konkaven Profils. P1.2-Berechnung, Regler und Installation werden
für diese Dokumentationsergänzung nicht verändert.

## P1.3-Stand und späteres lokales Relief

P1.3 setzt eine ausdrücklich aktivierbare Pixelhöhengeometrie um; Details,
mathematische Abweichungen von der früheren Distanzskizze und Kompatibilität:
[GLASS-P1.3.md](GLASS-P1.3.md). Der alte Faktorpfad bleibt erhalten. Die
Vorarbeiten und früheren Aussagen „noch nicht implementiert“ oben beziehen
sich auf P1.2 und ersetzen nicht den aktuellen P1.3-Teststand.

Verbindlich getrennte Beiträge bleiben **Grundgeometrie**, **Kantenprofil** und
**optionales lokales Oberflächenrelief**. Beispielsweise kann eine konvexe
Glasoberfläche gleichzeitig viele lokale konkave Vertiefungen tragen. Die
signierte Grundhöhe ist kein exklusiver globaler Schalter gegen lokale Mulden.
Ein späterer Graustufen-/Masken-/Musterinput ergänzt das gemeinsame Höhenfeld;
Reflexionen und Brechung verwenden dessen gemeinsame endgültige Normalen.
Optische Weglänge und unveränderte Silhouette sind dabei erneut abzusichern.
Kein Golfball-Generator, Reliefeditor, Reliefpad oder 3D-System in P1.3.
