/* SPDX-License-Identifier: MIT */
/*
 * NanoTank Glass P1 - native GIMP/GEGL layer filter.
 *
 * 2D thin-surface approximation, NOT a 3D ray tracer.  "Brechung" samples
 * this operation's own input buffer.  GIMP does not hand the operation the
 * projection of the layers below or the active selection mask.
 */

#ifdef GEGL_PROPERTIES

property_double (thickness, "Glasdicke (Pixel)", 12.0)
  description ("Breite der Licht- und Schattenzone entlang der Glaskante")
  value_range (1.0, 64.0)
  ui_range    (1.0, 48.0)
  ui_steps    (1.0, 5.0)

property_double (roundness, "Kantenwölbung", 0.65)
  description ("Rundet das optische Kantenprofil; verändert nicht die Auswahlkontur")
  value_range (0.0, 1.0)
  ui_steps    (0.01, 0.10)

property_boolean (preserve_alpha, "Untergrund und Alpha erhalten", TRUE)
  description ("Ein: Opake Schilder bleiben opak, vorhandenes Alpha bleibt erhalten. Aus: neues Glas mit eigener Deckkraft.")

property_double (glass_opacity, "Glasdeckkraft (nur ohne Alpha-Erhalt)", 0.32)
  description ("Grunddeckkraft ohne Alpha-Erhalt; Reflexionen ergänzen eigenes Alpha")
  value_range (0.0, 1.0)
  ui_steps    (0.01, 0.10)

property_color (tint, "Tönungsfarbe", "#bcdff7")
  description ("Farbe des Glasmaterials")

property_double (tint_strength, "Tönungsintensität", 0.20)
  description ("Anteil der Tönungsfarbe an der Glasoberfläche")
  value_range (0.0, 1.0)
  ui_steps    (0.01, 0.10)

property_double (reflection, "Reflexion", 0.75)
  description ("Fresnel-basierte Spiegelung, an der Kante stärker")
  value_range (0.0, 1.0)
  ui_steps    (0.01, 0.10)

property_double (highlights, "Glanzlichter", 0.60)
  description ("Geometrieabhängiger Glanz eines gerichteten Flächenlichts")
  value_range (0.0, 1.0)
  ui_steps    (0.01, 0.10)

property_double (refraction, "Interne Lichtbrechung (Pixel)", 2.5)
  description ("Verschiebt nur Pixel der aktiven Ebene, niemals automatisch darunterliegende Ebenen")
  value_range (0.0, 10.0)
  ui_steps    (0.1, 1.0)

property_double (curvature, "Wölbung", 0.30)
  description ("Zusätzliche sanfte Verformung des eigenen Ebeneninhalts")
  value_range (-1.0, 1.0)
  ui_steps    (0.01, 0.10)

property_double (ior, "Brechungsindex", 1.50)
  description ("Beeinflusst Fresnel-Reflexion und interne Brechung; Glas etwa 1,5")
  value_range (1.0, 2.5)

property_double (light_angle, "Lichtrichtung (Grad)", 225.0)
  description ("Richtung des Flächenlichts in der Bildebene")
  value_range (0.0, 360.0)

property_boolean (alpha_shape, "Glasform aus Ebenen-Alpha", FALSE)
  description ("Für freigestellte Objekte; aus für Glas auf einer leeren Ebene. Auswahlkonturen werden nicht automatisch übertragen.")

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME     nanotank_glass
#define GEGL_OP_C_SOURCE nanotank-glass.c

#include "gegl-op.h"
#include <math.h>

static inline gfloat
nt_clamp (gfloat x, gfloat lo, gfloat hi)
{
  return fminf (hi, fmaxf (lo, x));
}

typedef struct
{
  const gfloat *pixels;
  GeglRectangle rect;
} NtTile;

/* Coordinates are global image coordinates.  The padded tile is read with
 * GEGL_ABYSS_NONE; out-of-bounds pixels are transparent, not clamped.
 */
static inline const gfloat *
nt_pixel (const NtTile *tile, gint x, gint y)
{
  if (x < tile->rect.x || x >= tile->rect.x + tile->rect.width ||
      y < tile->rect.y || y >= tile->rect.y + tile->rect.height)
    return NULL;
  return tile->pixels +
         (((gsize) (y - tile->rect.y) * tile->rect.width +
           (gsize) (x - tile->rect.x)) * 4);
}

static inline gfloat
nt_mask (const NtTile       *tile,
         const GeglRectangle *bounds,
         gint                x,
         gint                y,
         gboolean            alpha_shape)
{
  const gfloat *px;

  if (x < bounds->x || x >= bounds->x + bounds->width ||
      y < bounds->y || y >= bounds->y + bounds->height)
    return 0.0f;

  /* An explicit alpha-shape switch keeps geometry independent of material
   * mode. Empty layers need the rectangle; GIMP masks the result separately.
   */
  if (!alpha_shape)
    return 1.0f;

  px = nt_pixel (tile, x, y);
  return px ? nt_clamp (px[3], 0.0f, 1.0f) : 0.0f;
}

static void
nt_sample (const NtTile *tile, gfloat fx, gfloat fy, gfloat rgba[4])
{
  gint x = (gint) floorf (fx), y = (gint) floorf (fy);
  const gfloat *p[4];
  gfloat tx = fx - x, ty = fy - y;
  gfloat w[4] = {(1.0f - tx) * (1.0f - ty),
                  tx * (1.0f - ty),
                  (1.0f - tx) * ty,
                  tx * ty};
  gint i, k;

  p[0] = nt_pixel (tile, x, y);
  p[1] = nt_pixel (tile, x + 1, y);
  p[2] = nt_pixel (tile, x, y + 1);
  p[3] = nt_pixel (tile, x + 1, y + 1);

  rgba[3] = 0.0f;
  for (i = 0; i < 4; i++)
    if (p[i]) rgba[3] += w[i] * p[i][3];
  for (k = 0; k < 3; k++)
    {
      rgba[k] = 0.0f;
      for (i = 0; i < 4; i++)
        if (p[i]) rgba[k] += w[i] * p[i][k] * p[i][3];
      if (rgba[3] > 1e-8f) rgba[k] /= rgba[3];
    }
}

static GeglRectangle
get_bounding_box (GeglOperation *operation)
{
  const GeglRectangle *source =
    gegl_operation_source_get_bounding_box (operation, "input");
  GeglRectangle empty = {0, 0, 0, 0};
  /* We generate no pixels outside the layer's own bounds. */
  return source ? *source : empty;
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties         *o = GEGL_PROPERTIES (operation);
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  const Babl *space = gegl_operation_get_source_space (operation, "input");
  const Babl *format = babl_format_with_space ("RGBA float", space);
  gint pad = (gint) ceil (o->thickness + o->refraction + 10.0);

  gegl_operation_set_format (operation, "input", format);
  gegl_operation_set_format (operation, "output", format);
  area->left = area->right = area->top = area->bottom = pad;
}

static gboolean
process (GeglOperation       *operation,
         GeglBuffer          *input,
         GeglBuffer          *output,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *o = GEGL_PROPERTIES (operation);
  const GeglRectangle *bounds =
    gegl_operation_source_get_bounding_box (operation, "input");
  const Babl *format = gegl_operation_get_format (operation, "output");
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER (operation);
  GeglRectangle read_rect;
  NtTile tile;
  gfloat *src, *dst, *distance;
  gfloat tint[4] = {0.74f, 0.87f, 0.97f, 1.0f};
  gsize in_pixels, out_pixels;
  gint x, y, pad;
  const gfloat angle = o->light_angle * G_PI / 180.0;
  const gfloat light_x = cosf (angle), light_y = sinf (angle);

  (void) level;

  if (!input || !output || !bounds || roi->width <= 0 || roi->height <= 0)
    return TRUE;

  pad = area->left;
  read_rect.x = roi->x - pad;
  read_rect.y = roi->y - pad;
  read_rect.width = roi->width + pad * 2;
  read_rect.height = roi->height + pad * 2;

  /* Avoid integer and allocation overflow for pathological ROIs. */
  if (read_rect.width <= 0 || read_rect.height <= 0 ||
      (gsize) read_rect.width > G_MAXSIZE / (gsize) read_rect.height / 4 / sizeof (gfloat) ||
      (gsize) roi->width > G_MAXSIZE / (gsize) roi->height / 4 / sizeof (gfloat))
    return FALSE;

  in_pixels = (gsize) read_rect.width * read_rect.height;
  out_pixels = (gsize) roi->width * roi->height;
  src = g_try_new (gfloat, in_pixels * 4);
  dst = g_try_new (gfloat, out_pixels * 4);
  distance = g_try_new (gfloat, in_pixels);
  if (!src || !dst || !distance)
    {
      g_free (src);
      g_free (dst);
      g_free (distance);
      return FALSE;
    }

  gegl_buffer_get (input, &read_rect, 1.0, format, src,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  tile.pixels = src;
  tile.rect = read_rect;

  /* Bounded chamfer distance to the silhouette. Padding exceeds the bevel
   * support, so independently rendered tiles produce the same local field.
   * Fractional alpha seeds preserve the position of antialiased contours. */
  for (y = 0; y < read_rect.height; y++)
    for (x = 0; x < read_rect.width; x++)
      {
        gfloat mask = nt_mask (&tile, bounds, read_rect.x + x,
                               read_rect.y + y, o->alpha_shape);
        distance[(gsize)y * read_rect.width + x] =
          mask < 0.999f ? mask : o->thickness + 2.0f;
      }
  for (gint pass = 0; pass < 2; pass++)
    for (gint yy = 0; yy < read_rect.height; yy++)
      for (gint xx = 0; xx < read_rect.width; xx++)
        {
          gint step = pass ? -1 : 1;
          x = pass ? read_rect.width - 1 - xx : xx;
          y = pass ? read_rect.height - 1 - yy : yy;
          gsize idx = (gsize)y * read_rect.width + x;
          for (gint dy = -1; dy <= 0; dy++)
            for (gint dx = -1; dx <= 1; dx++)
              {
                if (dy == 0 && dx != -1) continue;
                gint sx = x + dx * step, sy = y + dy * step;
                if (sx >= 0 && sy >= 0 && sx < read_rect.width && sy < read_rect.height)
                  distance[idx] = fminf (distance[idx],
                    distance[(gsize)sy * read_rect.width + sx] +
                    (dx && dy ? 1.41421356f : 1.0f));
              }
        }

  if (o->tint)
    gegl_color_get_pixel (o->tint, format, tint);

  for (y = roi->y; y < roi->y + roi->height; y++)
    for (x = roi->x; x < roi->x + roi->width; x++)
      {
        const gfloat *center = nt_pixel (&tile, x, y);
        gfloat *out = dst + ((gsize) (y - roi->y) * roi->width + (x - roi->x)) * 4;
        gsize di = (gsize)(y - read_rect.y) * read_rect.width + x - read_rect.x;
        gfloat d = distance[di];
        gfloat gx = (distance[di + 1] - distance[di - 1]) * 0.5f;
        gfloat gy = (distance[di + read_rect.width] - distance[di - read_rect.width]) * 0.5f;
        gfloat edge = nt_clamp (1.0f - d / o->thickness, 0.0f, 1.0f);
        gfloat slope = edge * (1.5f + 5.0f * o->roundness);
        gfloat magnitude = hypotf (gx, gy);
        gfloat nx = magnitude > 1e-6f ? -gx / magnitude : 0.0f;
        gfloat ny = magnitude > 1e-6f ? -gy / magnitude : 0.0f;
        gfloat u = (x - bounds->x + 0.5f) / MAX (1, bounds->width);
        gfloat v = (y - bounds->y + 0.5f) / MAX (1, bounds->height);
        gfloat refracted[4], f0, fresnel, mirror, shine, shadow;
        gfloat dx, dy, fill;
        gint k;

        if (!center)
          continue;



        /* Bounded refraction inside this layer; the curvature term acts
         * over the full layer, including its center.
         */
        dx = -nx * (gfloat) o->refraction * edge +
             (u - 0.5f) * 12.0f * (gfloat) o->curvature;
        dy = -ny * (gfloat) o->refraction * edge +
             (v - 0.5f) * 12.0f * (gfloat) o->curvature;
        dx *= (1.0f - 1.0f / o->ior);
        dy *= (1.0f - 1.0f / o->ior);
        nt_sample (&tile, x + dx, y + dy, refracted);
        if (refracted[3] < 0.01f && center[3] > 0.01f)
          for (k = 0; k < 3; k++)
            refracted[k] = center[k];

        /* Schlick Fresnel; slope follows the bounded silhouette distance.
         * A 2.5D normal, not a reconstructed 3D object.
         */
        f0 = powf ((o->ior - 1.0f) / (o->ior + 1.0f), 2.0f);
        /* A bevel normal plus a shallow dome. An area light is evaluated
         * against the half vector: no painted stripe in image coordinates. */
        gfloat sx = nx * slope + (u - 0.5f) * o->curvature;
        gfloat sy = ny * slope + (v - 0.5f) * o->curvature;
        gfloat inv = 1.0f / sqrtf (1.0f + sx*sx + sy*sy);
        fresnel = f0 + (1.0f - f0) * powf (1.0f - inv, 5.0f);
        mirror = nt_clamp (o->reflection * fresnel, 0.0f, 0.95f);
        gfloat half_dot = (sx * light_x * 0.45f + sy * light_y * 0.45f + 0.893f) * inv;
        shine = o->highlights * o->reflection * (f0 * 9.0f) *
                powf (nt_clamp (half_dot, 0.0f, 1.0f), 20.0f);
        shine = nt_clamp (shine, 0.0f, 0.95f);
        shadow = (gfloat) o->roundness * edge *
                 fmaxf (0.0f, -(nx * light_x + ny * light_y)) * 0.23f *
                 nt_clamp (f0 / 0.04f, 0.0f, 1.0f);
        fill = nt_clamp ((gfloat) o->tint_strength *
                         (0.75f + 0.25f * edge), 0.0f, 1.0f);

        for (k = 0; k < 3; k++)
          {
            /* Beer-Lambert-style absorption on material; transparent glass
             * carries tint rather than black from an empty source buffer. */
            gfloat color = o->preserve_alpha
              ? refracted[k] * expf (-fill * (1.0f - tint[k]) * 2.0f)
              : tint[k] * fill;
            color *= (1.0f - shadow);
            color = color * (1.0f - mirror) + mirror;
            color = color * (1.0f - shine) + shine;
            /* No clamp: keep legal scene-linear HDR channels. */
            out[k] = color;
          }
        out[3] = o->preserve_alpha
               ? center[3]
               : (gfloat) o->glass_opacity;
        if (!o->preserve_alpha)
          {
            gfloat body = o->glass_opacity;
            gfloat surface = 1.0f - (1.0f - mirror) * (1.0f - shine);
            out[3] = body + surface * (1.0f - body);
            if (o->alpha_shape) out[3] *= center[3];
            /* Convert the premultiplied film/reflection sum to straight RGB. */
            for (k = 0; k < 3; k++)
              out[k] = ((1.0f - fill * (1.0f - tint[k])) * body * (1.0f - surface) + surface) /
                       fmaxf (body + surface * (1.0f - body), 1e-8f);
          }
      }

  gegl_buffer_set (output, roi, 0, format, dst, GEGL_AUTO_ROWSTRIDE);
  g_free (src);
  g_free (dst);
  g_free (distance);
  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation_class = GEGL_OPERATION_CLASS (klass);
  GeglOperationFilterClass *filter_class =
    GEGL_OPERATION_FILTER_CLASS (klass);

  operation_class->prepare = prepare;
  operation_class->get_bounding_box = get_bounding_box;
  filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
                                 "name",            "nanotank:glass",
                                 "title",           "NanoTank Glass",
                                 "categories",      "Artistic",
                                 "description",     "Glasauflage mit Fresnel-Kanten, Glanz und interner Lichtbrechung",
                                 "gimp:menu-path",  "<Image>/Filters/NanoTank/",
                                 "gimp:menu-label", "Glass",
                                 NULL);
}
#endif
