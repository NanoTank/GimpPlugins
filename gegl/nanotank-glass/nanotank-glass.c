/* SPDX-License-Identifier: MIT */
/*
 * NanoTank Glass P1.3 - native GIMP/GEGL layer filter.
 *
 * 2D thin-surface approximation, NOT a 3D ray tracer.  "Brechung" samples
 * this operation's own input buffer.  GIMP does not hand the operation the
 * projection of the layers below or the active selection mask.
 */

#ifdef GEGL_PROPERTIES

property_double (thickness, "Glasdicke (Pixel)", 12.0)
  description ("Altmodus: Randbreite. Pixelhöhenprofil: positive Mindestdicke für die interne Brechung.")
  value_range (1.0, 64.0)
  ui_range    (1.0, 48.0)
  ui_steps    (1.0, 5.0)

property_double (roundness, "Kantenwölbung (Altmodus)", 0.65)
  description ("Nur Altmodus: optisches Randprofil; im Pixelprofil stattdessen Kantenrundung verwenden")
  value_range (0.0, 1.0)
  ui_steps    (0.01, 0.10)

property_boolean (preserve_alpha, "Untergrund und Alpha erhalten", TRUE)
  description ("Ein: Opake Schilder bleiben opak, vorhandenes Alpha bleibt erhalten. Aus: neues Glas mit eigener Deckkraft.")

property_double (glass_opacity, "Glasdeckkraft (Tönungsdichte)", 0.32)
  description ("Optische Dichte der Tönung ohne Alpha-Erhalt; klares Glas bleibt durchsichtig, Reflexionen ergänzen Alpha")
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

property_double (curvature, "Wölbung (Altmodus)", 0.30)
  description ("Krümmung der Oberfläche: verändert Normalen, Reflexionen und interne Brechung")
  value_range (-1.0, 1.0)
  ui_steps    (0.01, 0.10)

property_boolean (pixel_geometry, "Pixelhöhenprofil verwenden", FALSE)
  description ("Ein: geometrische Höhe, Bereich und Kantenrundung. Aus: bisherige Einstellungen unverändert berechnen.")

property_double (surface_height, "Wölbungshöhe (Pixel)", 10.0)
  description ("Nur Pixelhöhenprofil: positiv konvex, negativ konkav; unabhängig von Glasdicke")
  value_range (-1000.0, 1000.0)
  ui_range (-100.0, 150.0)
  ui_steps (1.0, 10.0)

property_double (surface_extent, "Wölbungsbereich (%)", 100.0)
  description ("Nur Pixelhöhenprofil: 100 bis zur Mitte, kleinere Werte mit ebener Mitte")
  value_range (1.0, 100.0)
  ui_steps (1.0, 10.0)

property_double (edge_radius, "Kantenrundung (Pixel)", 4.0)
  description ("Nur Pixelhöhenprofil: unabhängiger Außenrand; 0 für reine Linse/Halbkugel")
  value_range (0.0, 64.0)
  ui_steps (0.5, 5.0)

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
#include "glass-geometry.h"

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

/* Contour extent and the smoothed distance surface are global geometry
 * dependencies. Cache the full layer; no mutable geometry cache in process. */
static GeglRectangle
get_required_for_output (GeglOperation *operation, const gchar *pad,
                         const GeglRectangle *roi)
{
  (void) pad; (void) roi;
  return get_bounding_box (operation);
}

static GeglRectangle
get_cached_region (GeglOperation *operation, const GeglRectangle *roi)
{
  (void) roi;
  return get_bounding_box (operation);
}

static GeglRectangle
get_invalidated_by_change (GeglOperation *operation, const gchar *pad,
                            const GeglRectangle *changed)
{
  GeglRectangle full = get_bounding_box (operation);
  (void) pad;
  /* A tiny contour edit can alter the extent and normals throughout the
   * object. Include old bounds if an upstream extent is shrinking. */
  gegl_rectangle_bounding_box (&full, &full, changed);
  return full;
}

/* Separable box smoothing of the distance-derived height. Outside the full
 * input extent the distance is zero; the denominator stays constant there. */
static void
nt_smooth_height (const gfloat *distance, gfloat *scratch, gfloat *height,
                  gint width, gint rows, gint radius, gfloat scale)
{
  for (gsize i = 0; i < (gsize) width * rows; i++)
    {
      gfloat q = nt_clamp (distance[i] / scale, 0.0f, 1.0f);
      height[i] = scale * sqrtf (q * (2.0f - q));
    }
  for (gint y = 0; y < rows; y++)
    {
      gfloat sum = 0.0f;
      for (gint x = 0; x <= MIN (radius, width - 1); x++) sum += height[(gsize)y * width + x];
      for (gint x = 0; x < width; x++)
        {
          scratch[(gsize)y * width + x] = sum / (2 * radius + 1);
          if (x - radius >= 0) sum -= height[(gsize)y * width + x - radius];
          if (x + radius + 1 < width) sum += height[(gsize)y * width + x + radius + 1];
        }
    }
  for (gint x = 0; x < width; x++)
    {
      gfloat sum = 0.0f;
      for (gint y = 0; y <= MIN (radius, rows - 1); y++) sum += scratch[(gsize)y * width + x];
      for (gint y = 0; y < rows; y++)
        {
          height[(gsize)y * width + x] = sum / (2 * radius + 1);
          if (y - radius >= 0) sum -= scratch[(gsize)(y - radius) * width + x];
          if (y + radius + 1 < rows) sum += scratch[(gsize)(y + radius + 1) * width + x];
        }
    }
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
  GeglRectangle read_rect, shape;
  NtTile tile;
  gfloat *src, *dst, *distance, *height, *scratch, *geo_x = NULL, *geo_y = NULL;
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
  height = g_try_new (gfloat, in_pixels);
  scratch = o->pixel_geometry ? NULL : g_try_new (gfloat, in_pixels);
  if (!src || !dst || !distance || !height || (!o->pixel_geometry && !scratch))
    {
      g_free (src);
      g_free (dst);
      g_free (distance);
      g_free (height);
      g_free (scratch);
      return FALSE;
    }

  gegl_buffer_get (input, &read_rect, 1.0, format, src,
                   GEGL_AUTO_ROWSTRIDE, GEGL_ABYSS_NONE);
  tile.pixels = src;
  tile.rect = read_rect;

  gfloat bevel = 1.0f;
  if (!o->pixel_geometry)
    {
      shape = *bounds;
      if (o->alpha_shape)
        {
          gint x0 = bounds->x + bounds->width, y0 = bounds->y + bounds->height;
          gint x1 = bounds->x, y1 = bounds->y;
          for (y = bounds->y; y < bounds->y + bounds->height; y++)
            for (x = bounds->x; x < bounds->x + bounds->width; x++)
              {
                const gfloat *px = nt_pixel (&tile, x, y);
                if (px && px[3] > 0.01f)
                  {
                    x0 = MIN (x0, x); y0 = MIN (y0, y);
                    x1 = MAX (x1, x + 1); y1 = MAX (y1, y + 1);
                  }
              }
          if (x1 > x0 && y1 > y0)
            shape = (GeglRectangle) { x0, y0, x1 - x0, y1 - y0 };
        }
      /* Soft saturation leaves a clear centre even for small objects, while
       * retaining visible differences between 4, 12, 20 and 30 px. */
      gfloat limit = MAX (1.0f, MIN (shape.width, shape.height) * 0.40f);
      bevel = limit * (1.0f - expf (-o->thickness / limit));

      /* Full chamfer distance to the silhouette. Fractional alpha seeds locate
       * antialiased contours. Global height smoothing also uses the interior. */
      for (y = 0; y < read_rect.height; y++)
        for (x = 0; x < read_rect.width; x++)
          {
            gfloat mask = nt_mask (&tile, bounds, read_rect.x + x,
                                   read_rect.y + y, o->alpha_shape);
            distance[(gsize)y * read_rect.width + x] =
              mask < 0.999f ? mask : MAX (shape.width, shape.height) + 2.0f;
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

      /* A generic contour-derived cap, softened at medial ridges. The same
       * scalar surface is used for rectangles, circles and organic silhouettes. */
      gfloat scale = MAX (1.0f, MIN (shape.width, shape.height) * 0.5f);
      gint smoothing = MAX (1, (gint) ceil (scale * 0.30f));
      nt_smooth_height (distance, scratch, height, read_rect.width,
                        read_rect.height, smoothing, scale);

    }
  if (o->pixel_geometry)
    {
      geo_x = g_try_new (gfloat, in_pixels);
      geo_y = g_try_new (gfloat, in_pixels);
      for (y = 0; y < read_rect.height; y++)
        for (x = 0; x < read_rect.width; x++)
          distance[(gsize)y * read_rect.width + x] =
            nt_mask (&tile, bounds, read_rect.x+x, read_rect.y+y, o->alpha_shape);
      if (!geo_x || !geo_y || !nt_geometry_build (distance, read_rect.width,
           read_rect.height, o->surface_height, o->surface_extent, o->edge_radius,
           o->thickness, height, geo_x, geo_y, distance, NULL))
        {
          g_warning ("NanoTank Glass: Pixelgeometrie konnte nicht berechnet werden (Speicher oder Konvergenz).");
          g_free(src); g_free(dst); g_free(distance); g_free(height);
          g_free(scratch); g_free(geo_x); g_free(geo_y);
          return FALSE;
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
        gfloat refracted[4], f0, fresnel, surface, fill;
        gfloat dx, dy;
        gint k;

        if (!center)
          continue;
        if (o->alpha_shape && center[3] <= 0.0f)
          {
            for (k = 0; k < 4; k++) out[k] = o->preserve_alpha ? center[k] : 0.0f;
            continue;
          }

        gfloat sx, sy, edge=0.0f;
        if (o->pixel_geometry)
          { sx=geo_x[di]; sy=geo_y[di]; }
        else
          {
            gfloat d = distance[di];
            gint stride = read_rect.width;
            /* Sobel derivative suppresses lattice-direction noise on circles. */
            gfloat gx = (2.0f * (distance[di + 1] - distance[di - 1]) +
                         distance[di + stride + 1] - distance[di + stride - 1] +
                         distance[di - stride + 1] - distance[di - stride - 1]) * 0.125f;
            gfloat gy = (2.0f * (distance[di + stride] - distance[di - stride]) +
                         distance[di + stride + 1] - distance[di - stride + 1] +
                         distance[di + stride - 1] - distance[di - stride - 1]) * 0.125f;
            edge = nt_clamp (1.0f - d / bevel, 0.0f, 1.0f);
            gfloat slope;
            gfloat magnitude = hypotf (gx, gy);
            gfloat nx = magnitude > 1e-6f ? -gx / magnitude : 0.0f;
            gfloat ny = magnitude > 1e-6f ? -gy / magnitude : 0.0f;
            /* A rounded cap rises from the contour into a shallow ellipsoidal
             * dome. Its derivative tends smoothly to zero at the bevel join.
             * Both cap and dome follow the actual contour, not a bounding ellipse. */
            gfloat t = nt_clamp (d / bevel, 0.025f, 1.0f);
            gfloat cap = (1.0f - t) / sqrtf (fmaxf (0.02f, t * (2.0f - t)));
            slope = (0.4f + 1.8f * o->roundness) * cap;
            gfloat dome = 2.8f * o->curvature;
            sx = nx * slope - (height[di + 1] - height[di - 1]) * 0.5f * dome;
            sy = ny * slope - (height[di + stride] - height[di - stride]) * 0.5f * dome;
          }
        gfloat nz = 1.0f / sqrtf (1.0f + sx*sx + sy*sy);
        gfloat normal_x = sx * nz, normal_y = sy * nz;

        /* Bounded thin-lens approximation, sampling this layer only. The
         * same normals drive lighting and displacement; thickness matters. */
        gfloat optical = (1.0f - 1.0f / o->ior);
        gfloat travel = o->refraction * (0.35f + 0.65f * o->thickness / 12.0f);
        if (o->pixel_geometry)
          travel = o->refraction * (0.35f + 0.65f * distance[di] / 12.0f);
        travel = fminf (travel, o->refraction + o->thickness * 0.25f);
        dx = -normal_x * optical * travel;
        dy = -normal_y * optical * travel;
        nt_sample (&tile, x + dx, y + dy, refracted);
        if (refracted[3] < 0.01f && center[3] > 0.01f)
          for (k = 0; k < 3; k++) refracted[k] = center[k];

        f0 = powf ((o->ior - 1.0f) / (o->ior + 1.0f), 2.0f);
        fresnel = o->ior <= 1.0 ? 0.0f :
          f0 + (1.0f - f0) * powf (1.0f - nz, 5.0f);

        /* Reflect view (0,0,1) about N. Evaluate a fixed studio environment
         * in reflected-direction space, never a stripe in image space.
         * A broad softbox and a weaker opposite card reveal curvature;
         * the dim room contributes very little at normal incidence. */
        gfloat rx = 2.0f * normal_x * nz;
        gfloat ry = 2.0f * normal_y * nz;
        gfloat rz = 2.0f * nz * nz - 1.0f;
        gfloat along = rx * light_x + ry * light_y;
        gfloat across = -rx * light_y + ry * light_x;
        gfloat box_u = (along - 0.64f) / 0.22f;
        gfloat box_v = across / 0.82f;
        gfloat window = expf (-powf (box_u, 4.0f) - powf (box_v, 4.0f));
        window *= nt_clamp ((rz + 0.15f) * 4.0f, 0.0f, 1.0f);
        gfloat card = expf (-powf ((along + 0.72f) / 0.18f, 2.0f) -
                             powf (across / 0.85f, 4.0f));
        /* Finite radiance compressed to a display-referred reflection
         * coverage. This keeps RGB/alpha energy coupled without clipping. */
        if (o->pixel_geometry)
          {
            /* Gaussian angular softboxes avoid the sharp superellipse
             * shoulders of the legacy environment. No image-space paint. */
            window = expf (-0.5f * powf ((along-0.64f)/0.22f, 2.0f) -
                           0.5f * powf (across/0.65f, 2.0f));
            window *= nt_clamp ((rz+0.15f)*4.0f, 0.0f, 1.0f);
            card = expf (-0.5f * powf ((along+0.72f)/0.20f, 2.0f) -
                         0.5f * powf (across/0.70f, 2.0f));
          }
        gfloat lamps = o->highlights * (o->pixel_geometry
                      ? (9.0f * window + 1.8f * card)
                      : (28.0f * window + 5.0f * card));
        /* The low room exposure at the centre is unchanged. Steep
         * surfaces reflect a darker room with greater coverage, yielding
         * a readable opposite edge on bright backgrounds. */
        gfloat grazing = powf (1.0f - nz, 0.65f);
        gfloat room = 0.35f + (o->pixel_geometry ? 1.5f : 5.0f) * grazing;
        gfloat radiance = room + lamps;
        gfloat environment = (0.035f + lamps) / radiance;
        surface = 1.0f - expf (-o->reflection * fresnel * radiance);

        /* Absorption, rather than a white opaque film. At tint=0 clear
         * glass has no body alpha: RGBA cannot encode coloured transmission
         * exactly without the background, so use a weak coloured film. */
        fill = o->tint_strength * (0.6f + 0.4f * edge);
        gfloat absorption[3], density = 0.0f;
        for (k = 0; k < 3; k++)
          {
            absorption[k] = fmaxf (0.0f, 1.0f - tint[k]);
            density = fmaxf (density, absorption[k]);
          }
        gfloat body = 1.0f - expf (-o->glass_opacity * fill * density);
        gfloat alpha = body + surface * (1.0f - body);
        for (k = 0; k < 3; k++)
          {
            if (o->preserve_alpha)
              {
                gfloat transmitted = refracted[k] * expf (-fill * absorption[k]);
                out[k] = transmitted * (1.0f - surface) + surface * environment;
              }
            else
              {
                gfloat film = density > 1e-8f ? 1.0f - absorption[k] / density : 0.0f;
                out[k] = (film * body * (1.0f - surface) + surface * environment) /
                         fmaxf (alpha, 1e-8f);
              }
          }
        out[3] = o->preserve_alpha ? center[3] : alpha;
        if (!o->preserve_alpha && o->alpha_shape) out[3] *= center[3];
      }

  gegl_buffer_set (output, roi, 0, format, dst, GEGL_AUTO_ROWSTRIDE);
  g_free (src);
  g_free (dst);
  g_free (distance);
  g_free (height);
  g_free (scratch);
  g_free (geo_x);
  g_free (geo_y);
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
  operation_class->get_required_for_output = get_required_for_output;
  operation_class->get_cached_region = get_cached_region;
  operation_class->get_invalidated_by_change = get_invalidated_by_change;
  operation_class->threaded = FALSE;
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
