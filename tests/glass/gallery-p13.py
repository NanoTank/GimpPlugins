# SPDX-License-Identifier: MIT – NanoTank
"""Labels and arranges actual GEGL renderings. No invented image content."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
root=Path(__file__).resolve().parents[2]/"output/glass-p13"
font=ImageFont.truetype("DejaVuSans.ttf",17)

def grid(filename,rows):
    columns=max(map(len,rows));out=Image.new("RGB",(columns*398+12,len(rows)*318+12),"#f3f3f3")
    d=ImageDraw.Draw(out)
    for row,items in enumerate(rows):
        for col,(file,label) in enumerate(items):
            x,y=12+col*398,12+row*318
            d.text((x,y),label,font=font,fill="black")
            out.paste(Image.open(root/f"{file}.png"),(x,y+26))
    out.save(root/filename)

grid("comparison.png",[
 [("circle-+000","R=100 / H=0 px"),("circle-+010","R=100 / H=+10 px"),("circle-+050","R=100 / H=+50 px")],
 [("circle-+100","R=100 / H=+100 px (Vorderseite)"),("circle-+150","R=100 / H=+150 px"),("circle--020","R=100 / H=−20 px")],
 [("rectangle-15","H=+10 / Bereich 15 % / Rand 4"),("rectangle-100","H=+10 / Bereich 100 % / Rand 4"),("concave-key","H=−10 / Bereich 100 % / Rand 6")]])
grid("materials.png",[
 [("organic","Organisch / H=20 / Rand 2"),("separate","Getrennte Formen / gleiche Höhe"),("strokes","Schmale Formen / Konturen")],
 [("ellipse","Ellipse / gleicher Profilansatz"),("coating","Opake Auflage / H=35 / Alpha erhalten"),("circle--005","Kreis / H=−5 px")],
 [("p12-reference","P1.2 / bisherige Parameter"),("legacy-reference","P1.3 Altmodus / pixelidentisch"),("rectangle-100","P1.3 Pixelprofil / eigene Parameter")]])
# A more legible small overview and 2x details of actual source pixels.
out=Image.new("RGB",(1206,650),"#f3f3f3");d=ImageDraw.Draw(out)
for col,(name,label) in enumerate([("rectangle-100","Rechteck / Ausschnitt 2×"),
                                  ("organic","Organisch / Ausschnitt 2×"),
                                  ("circle-+100","Kreis H=100 / Ausschnitt 2×")]):
    d.text((12+col*398,12),label,font=font,fill="black")
    im=Image.open(root/f"{name}.png").crop((80,40,280,190)).resize((400,300),Image.Resampling.NEAREST)
    out.paste(im,(col*400+6,40))
    im=Image.open(root/f"{name}.png")
    out.paste(im,(col*400+6,360))
out.save(root/"details.png")
(root/"index.html").write_text('''<!doctype html><html lang="de"><meta charset="utf-8"><title>NanoTank Glass P1.3</title>
<style>body{font:16px sans-serif;background:#eee;margin:24px}section{overflow:auto}img{max-width:none}</style>
<h1>NanoTank Glass P1.3 – echte GEGL-Renderings</h1>
<p>Neue Pixelgeometrie ausdrücklich aktiviert. Rand 0 für reine Kreisprofile.
Bereich 100 %, soweit nicht anders beschriftet. Höhe und Rand in Pixeln.
Kein Hintergrundinput, keine vollständige Kugel, keine visuelle Abnahme.</p>
<p>Reflexion 0,75; Glanz 0,6; Tönungsdichte 0,15; Tönung 0,2; IOR 1,5; Licht 225°.
Übersichten 1:1, Details ausdrücklich 2×. P1.2/Altmodusvergleich zeigt dieselben alten Parameter;
Pixelprofil verwendet neue geometrische Größen und ist kein numerisch gleichgesetzter Faktor.</p>
''' + ''.join(f'<h2>{name}</h2><section><img src="{name}.png" alt="{name}"></section>'
              for name in ("comparison","materials","details")),encoding="utf-8")
print(root/"index.html")
