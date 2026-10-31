# SPDX-License-Identifier: MIT
"""Arrange actual GEGL test renders; no generated imagery or binary fixtures."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

root = Path(__file__).resolve().parents[2] / "output/glass-p11"
font = ImageFont.truetype("DejaVuSans.ttf", 16)
rows = [
    ("01-opaque-button", "Opaker grauer Button"),
    ("02-plate-pattern", "240 x 150 Glasplatte / Muster"),
    ("03-plate-dark", "Identische Platte / dunkel"),
    ("04-plate-light", "Identische Platte / hell"),
    ("05-round-glass", "Rundes Glas / Alpha-Kontur"),
]
canvas = Image.new("RGB", (796, len(rows) * 274 + 38), "#f3f3f3")
draw = ImageDraw.Draw(canvas)
draw.text((12, 8), "P1", font=font, fill="black")
draw.text((410, 8), "P1.1 – gleicher Input, gleiche Parameter", font=font, fill="black")
for row, (name, label) in enumerate(rows):
    y = 38 + row * 274
    draw.text((12, y), label, font=font, fill="black")
    for col, version in enumerate(("p1", "p11")):
        canvas.paste(Image.open(root / version / f"{name}.png"), (12 + col * 398, y + 26))
canvas.save(root / "comparison.png")
params = [("reflection", "Reflexion 0 / 1"), ("thickness", "Dicke 4 / 30 px"),
          ("light", "Licht 45 / 225 Grad"), ("opacity", "Dichte 0,02 / 0,8"),
          ("curvature", "Wölbung 0 / 0,75 (opake Auflage)"),
          ("highlights", "Glanz 0 / 1"), ("ior", "IOR 1 / 2")]
canvas = Image.new("RGB", (796, len(params) * 274), "#f3f3f3")
draw = ImageDraw.Draw(canvas)
for row, (name, label) in enumerate(params):
    y = row * 274
    draw.text((12, y), label, font=font, fill="black")
    for col, side in enumerate(("low", "high")):
        canvas.paste(Image.open(root / "p11" / f"{name}-{side}.png"), (12 + col * 398, y + 26))
canvas.save(root / "parameters.png")
print(root / "comparison.png")
print(root / "parameters.png")
