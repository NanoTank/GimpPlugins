/* SPDX-License-Identifier: MIT */
#ifndef NANOTANK_GLASS_CORE_H
#define NANOTANK_GLASS_CORE_H

#include <math.h>

typedef struct { float r, g, b, a; } GlassPixel;
typedef struct {
  float opacity, tint_strength, reflection, ior;
  float tint[3];
} GlassSurface;

static inline float glass_clamp(float x, float a, float b)
{
  return fminf(b, fmaxf(a, x));
}

static inline float glass_fresnel(float cosine, float ior)
{
  float ratio = (1.0f - ior) / (1.0f + ior);
  float f0 = ratio * ratio;
  float t = 1.0f - glass_clamp(cosine, 0.0f, 1.0f);
  return f0 + (1.0f - f0) * t*t*t*t*t;
}

/* Straight (non-premultiplied) RGBA. Both RGB and alpha survive correctly. */
static inline GlassPixel glass_over(GlassPixel top, GlassPixel bottom)
{
  GlassPixel out;
  float ta = glass_clamp(top.a, 0.0f, 1.0f);
  float ba = glass_clamp(bottom.a, 0.0f, 1.0f);
  float a = ta + ba * (1.0f - ta);
  out.a = a;
  if (a < 1.0e-8f) {
    out.r = out.g = out.b = 0.0f;
  } else {
    out.r = (top.r*ta + bottom.r*ba*(1.0f-ta))/a;
    out.g = (top.g*ta + bottom.g*ba*(1.0f-ta))/a;
    out.b = (top.b*ta + bottom.b*ba*(1.0f-ta))/a;
  }
  return out;
}

/* Tint film plus angle-dependent reflection, physically motivated rather than ray traced. */
static inline GlassPixel glass_shade(GlassPixel material, float cover,
                                    float edge, float highlight,
                                    const GlassSurface *p)
{
  float f = glass_fresnel(1.0f - 0.80f*glass_clamp(edge, 0, 1), p->ior);
  float film_a = glass_clamp(p->opacity, 0, 1) * glass_clamp(cover, 0, 1);
  float tint = glass_clamp(p->tint_strength, 0, 1);
  float reflect_a = glass_clamp(p->reflection, 0, 1) *
      glass_clamp(cover, 0, 1) *
      glass_clamp(0.06f + 0.75f*f + 0.68f*highlight, 0, 1);
  GlassPixel film = {
      material.r*(1.0f-tint) + p->tint[0]*tint,
      material.g*(1.0f-tint) + p->tint[1]*tint,
      material.b*(1.0f-tint) + p->tint[2]*tint,
      film_a
  };
  GlassPixel white = { 1.0f, 1.0f, 1.0f, reflect_a };
  return glass_over(white, glass_over(film, material));
}

#endif
