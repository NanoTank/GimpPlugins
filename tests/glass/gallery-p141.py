# SPDX-License-Identifier: MIT – NanoTank
"""Direct P1.4/P1.4.1 comparison of real GEGL renders, at native scale."""
from pathlib import Path
from PIL import Image,ImageDraw,ImageFont,ImageChops
root=Path(__file__).resolve().parents[2]/"output/glass-p141"
font=ImageFont.truetype("DejaVuSans.ttf",16)
rows=[("circle","Kreis R=100 / H=100 / Rand 0"),
      ("plate-text","Platte H=10 / Bereich 15 % / Rand 4"),
      ("organic","Organisch H=40 / Bereich 100 % / Rand 4"),
      ("concave-text","Konkav H=−30 / Bereich 100 % / Rand 4"),
      ("separate","Getrennte Objekte / jeweils H=40")]

def grid(file,scenes,columns):
    out=Image.new("RGB",(len(columns)*398+12,len(scenes)*318+44),"#f3f3f3");d=ImageDraw.Draw(out)
    for col,(version,suffix,label) in enumerate(columns):
        d.text((12+col*398,10),label,font=font,fill="black")
    for row,(name,label) in enumerate(scenes):
        y=44+row*318;d.text((12,y),label,font=font,fill="black")
        for col,(version,suffix,_) in enumerate(columns):
            out.paste(Image.open(root/version/f"{name}-{suffix}.png"),(12+col*398,y+25))
    out.save(root/file)

grid("comparison.png",rows,[("p14","on","P1.4 / Tönung 0,20"),
    ("p141","on","P1.4.1 / gleiche Werte / Filterung")])
grid("materials.png",rows[:4],[("p14","on","P1.4 / Tönung 0,20"),
    ("p141","start","P1.4.1 / Starttönung 0,12")])
out=Image.new("RGB",(812,3*326+42),"#f3f3f3");d=ImageDraw.Draw(out)
for col,(version,label) in enumerate([("p14","P1.4 / Detail 2×"),("p141","P1.4.1 / gleiche Werte / 2×")]):
    d.text((10+col*402,10),label,font=font,fill="black")
    for row,(name,box) in enumerate([("circle",(90,40,290,190)),
        ("concave-text",(90,70,290,220)),("plate-text",(65,55,265,205))]):
        im=Image.open(root/version/f"{name}-on.png").crop(box).resize((400,300),Image.Resampling.NEAREST)
        out.paste(im,(6+col*402,40+row*326))
out.save(root/"details.png")
for name in [r[0] for r in rows]+["coating"]:
    a=Image.open(root/"p14"/f"{name}-p13.png");b=Image.open(root/"p141"/f"{name}-p13.png")
    assert ImageChops.difference(a,b).getbbox() is None, name
(root/"index.html").write_text('''<!doctype html><html lang="de"><meta charset="utf-8"><title>NanoTank Glass P1.4.1</title>
<style>body{font:16px sans-serif;background:#eee;margin:24px}section{overflow:auto}img{max-width:none}</style>
<h1>NanoTank Glass P1.4.1 – gezielter Feinschliff</h1><p>Echte GEGL-Renderings.
Vergleich: gleiche Parameter, nur lokale Hintergrundfilterung bei Verkleinerung.
Materialansicht: zusätzlich neuer Tönungsstandard 0,12 statt 0,20.
Reflexion 0,75, Glanz 0,6, IOR 1,5, Brechung 1, Dicke 12, Licht 225° unverändert.
Übersichten 1:1; Details ausdrücklich 2×. Kleine Änderungen, keine neue Geometrie und kein Live-Workflow.</p>
''' + ''.join(f'<h2>{name}</h2><section><img src="{name}.png" alt="{name}"></section>'
              for name in ("comparison","materials","details")),encoding="utf-8")
print(root/"comparison.png");print(root/"materials.png");print(root/"details.png")
print("PASS: P1.3 images remain identical in all six scenes")
