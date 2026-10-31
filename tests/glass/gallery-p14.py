# SPDX-License-Identifier: MIT – NanoTank
"""Arrange and label real native GEGL results. No generated wish images."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parents[2]/"output/glass-p14"
font=ImageFont.truetype("DejaVuSans.ttf",16)

def grid(name,rows,columns):
    out=Image.new("RGB",(len(columns)*398+12,len(rows)*318+44),"#f3f3f3")
    draw=ImageDraw.Draw(out)
    for col,(suffix,label) in enumerate(columns):
        draw.text((12+col*398,10),label,font=font,fill="black")
    for row,(scene,label) in enumerate(rows):
        y=44+row*318
        draw.text((12,y),label,font=font,fill="black")
        for col,(suffix,_) in enumerate(columns):
            out.paste(Image.open(root/f"{scene}-{suffix}.png"),(12+col*398,y+25))
    out.save(root/name)

grid("comparison.png",[("circle","Kreis R=100 / H=100 / Rand 0"),
    ("plate-text","Platte H=10 / Bereich 15 % / Rand 4"),
    ("organic","Organisch H=40 / Bereich 100 % / Rand 4")],
    [("original","Ohne Glas"),("p13","P1.3 ohne Hintergrundinput"),("on","P1.4 echte Hintergrundbrechung")])
grid("materials.png",[("circle","Kreis: gleicher Compositor, nur Brechung aus/an"),
    ("concave-text","Konkav H=−30 / Bereich 100 % / Rand 4"),
    ("separate","Getrennte Objekte / jeweils H=40"),
    ("coating","Auflage: Hintergrundbrechung absichtlich ohne Wirkung")],
    [("off","P1.4 Brechungsstärke 0"),("on","P1.4 Brechungsstärke 1")])
out=Image.new("RGB",(1206,670),"#f3f3f3");draw=ImageDraw.Draw(out)
for col,(name,suffix,label) in enumerate([("circle","p13","P1.3: Ausschnitt 2×"),
    ("circle","on","P1.4: derselbe Ausschnitt 2×"),("plate-text","on","P1.4: Rand und Text 2×")]):
    draw.text((12+col*398,10),label,font=font,fill="black")
    im=Image.open(root/f"{name}-{suffix}.png")
    out.paste(im.crop((90,50,290,200)).resize((400,300),Image.Resampling.NEAREST),(6+400*col,40))
    out.paste(im,(6+400*col,365))
out.save(root/"details.png")
(root/"index.html").write_text('''<!doctype html><html lang="de"><meta charset="utf-8"><title>NanoTank Glass P1.4</title>
<style>body{font:16px sans-serif;background:#eee;margin:24px}section{overflow:auto}img{max-width:none}</style>
<h1>NanoTank Glass P1.4 – echte Hintergrundbrechung</h1>
<p>Native GEGL-Renderings, kein Mockup. Wölbung/Normalen aus P1.3;
Snell-Transmission durch eine 2,5D-Referenzfläche, keine vollständige Kugeloptik.</p>
<p>Stärke 1, Dicke 12, IOR 1,5, Reflexion 0,75, Glanz 0,6, Licht 225°.
P1.3: tatsächliche Operation als transparente Auflage.
P1.4: separater Hintergrundinput und vollständiger Ersatzkomposit.
Die Materialkomposition unterscheidet sich deshalb auch bei Brechung 0.</p>
<p>GIMP-Workflow ist ein neues Snapshot-Bild, kein live aktualisierter Hintergrund.
Vergleiche 1:1, nur die Details ausdrücklich 2×.</p>
''' + ''.join(f'<h2>{name}</h2><section><img src="{name}.png" alt="{name}"></section>'
              for name in ("comparison","materials","details")),encoding="utf-8")
print(root/"comparison.png");print(root/"materials.png");print(root/"details.png")
