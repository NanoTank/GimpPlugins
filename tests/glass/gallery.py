# SPDX-License-Identifier: MIT
"""Arrange actual GEGL renders at 1:1; magnify only the labelled detail crops."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

root = Path(__file__).resolve().parents[2] / "output/glass-p12"
font = ImageFont.truetype("DejaVuSans.ttf", 16)
rows = [("02-plate-pattern", "200 x 120: transparent / dunkle Struktur"),
        ("05-round-glass", "Durchmesser 150: Alpha-Kreis"),
        ("08-organic", "Organische Alpha-Silhouette / gleiche Materialparameter"),
        ("01-opaque-button", "Grauer Button mit Glasauflage"),
        ("07-text", "Glas über Text / klarer Hintergrund")]

def grid(path, scenes, columns):
    canvas = Image.new("RGB", (len(columns)*398+12, len(scenes)*274+38), "#f3f3f3")
    draw = ImageDraw.Draw(canvas)
    for col, (label, version, suffix) in enumerate(columns):
        draw.text((12+col*398,8), label, font=font, fill="black")
    for row, (name, label) in enumerate(scenes):
        y = 38+row*274
        draw.text((12,y), label, font=font, fill="black")
        for col, (_, version, suffix) in enumerate(columns):
            canvas.paste(Image.open(root/version/f"{name}{suffix}.png"), (12+col*398,y+26))
    canvas.save(root/path)

grid("comparison.png", rows, [("Ohne Glas", "p12", "-original"),
     ("P1.1 / identische Parameter", "p11", ""), ("P1.2 / identische Parameter", "p12", "")])
grid("materials.png", [("03-plate-dark", "Dasselbe Objekt: dunkler Grund"),
     ("04-plate-light", "Dasselbe Objekt: heller Grund"),
     ("01-opaque-button", "Opaker Untergrund bleibt erhalten"),
     ("07-text", "Text: Standard / stärkere Kombination")],
     [("P1.1", "p11", ""), ("P1.2 / Testwerte", "p12", ""),
      ("P1.2 / stärker: R 1, G 0,85, W 0,5", "p12", "-strong")])
# Details of actual saved renders; nearest sampling makes the 2x scale explicit.
canvas = Image.new("RGB", (1206, 2*286+38), "#f3f3f3")
draw = ImageDraw.Draw(canvas)
for col, (label, version, suffix) in enumerate([("P1.1", "p11", ""),
    ("P1.2", "p12", ""), ("P1.2 / stärker", "p12", "-strong")]):
    draw.text((12+col*398,8), label+" / Ausschnitt 2x", font=font, fill="black")
    for row, (name, box) in enumerate([("02-plate-pattern", (82,50,282,170)),
                                       ("05-round-glass", (105,38,305,158))]):
        im=Image.open(root/version/f"{name}{suffix}.png").crop(box)
        im=im.resize((400,240),Image.Resampling.NEAREST)
        canvas.paste(im,(6+col*400,40+row*286))
canvas.save(root/"details.png")
# Thickness shown at normal size, with the remaining controls below it.
canvas=Image.new("RGB",(1604,5*274+38),"#f3f3f3")
draw=ImageDraw.Draw(canvas)
for col, width in enumerate((4,12,20,30)):
    draw.text((12+col*398,8),f"Dicke {width} px",font=font,fill="black")
    canvas.paste(Image.open(root/"p12"/f"thickness-{width:02d}.png"),(12+col*398,38))
for row,(name,label) in enumerate([("curvature","Wölbung 0 / 0,75 (Auflage)"),
    ("reflection","Reflexion 0 / 1"),("highlights","Glanz 0 / 1"),("light","Licht 45 / 225 Grad")]):
    y=312+row*274
    draw.text((12,y),label,font=font,fill="black")
    for col,side in enumerate(("low","high")):
        canvas.paste(Image.open(root/"p12"/f"{name}-{side}.png"),(12+col*398,y+26))
canvas.save(root/"parameters.png")
# Browser gallery preserves native image sizes even on narrower displays.
(root/"index.html").write_text('''<!doctype html><html lang="de"><meta charset="utf-8">
<title>NanoTank Glass P1.2 – echte GEGL-Vergleiche</title>
<style>body{font:16px sans-serif;background:#eee;color:#222;margin:24px}img{display:block;max-width:none}section{overflow:auto;margin-bottom:32px}a{color:#136}</style>
<h1>NanoTank Glass P1.2</h1><p>Echte GEGL-Renderings, gleiche Inputs und Parameter.
Übersichten zeigen 1 Bildpixel pro Pixel; Details sind ausdrücklich 2× vergrößert.
Keine Hintergrundbrechung. Noch keine visuelle Nutzerabnahme.</p>
<p>Testwerte: Dicke 20, Wölbung 0,3, Reflexion 0,75, Glanz 0,6, Dichte 0,15,
Tönung 0,2, IOR 1,5, Licht 225°. Stärker: Reflexion 1, Glanz 0,85, Wölbung 0,5.</p>
''' + "".join(f'<h2>{label}</h2><section><a href="{file}"><img src="{file}" alt="{label}"></a></section>'
    for file,label in [("comparison.png","Ohne Glas / P1.1 / P1.2"),("materials.png","Material und Hintergrund"),
                       ("details.png","Kanten und Reflexe – 2×"),("parameters.png","Parameter – 1:1")]), encoding="utf-8")
for file in ("comparison.png","materials.png","details.png","parameters.png","index.html"):
    print(root/file)
