/* SPDX-License-Identifier: MIT */
/*
 * Native GEGL-Systemtest fuer GIMP 3.2 Flatpak.
 * Der Regler verändert nur RGB; Alpha bleibt bytegenau im float-Pfad.
 * Keine Schrift-/Grafik-Bibliotheken außerhalb des GIMP-SDK benötigt.
 */

#ifdef GEGL_PROPERTIES

property_double (brightness, "Helligkeit", 0.0)
  description ("Linearer RGB-Helligkeitsversatz; 0 bewahrt das Original")
  value_range (-1.0, 1.0)
  ui_range    (-1.0, 1.0)
  ui_steps    (0.01, 0.10)

#else

#define GEGL_OP_POINT_FILTER
#define GEGL_OP_NAME     nanotank_gegl_test
#define GEGL_OP_C_SOURCE nanotank-gegl-test.c

#include "gegl-op.h"

static gboolean
process (GeglOperation       *operation,
         void                *in_buf,
         void                *out_buf,
         glong                n_pixels,
         const GeglRectangle *roi,
         gint                 level)
{
  GeglProperties *properties = GEGL_PROPERTIES (operation);
  const gfloat   *src = in_buf;
  gfloat         *dst = out_buf;
  const gfloat    brightness = (gfloat) properties->brightness;
  glong           i;

  (void) roi;
  (void) level;

  for (i = 0; i < n_pixels; i++)
    {
      /* Linear RGB-Werte koennen bei HDR ausserhalb 0..1 liegen:
       * fuer den Test nicht abschneiden. Alpha bleibt unveraendert.
       */
      dst[0] = src[0] + brightness;
      dst[1] = src[1] + brightness;
      dst[2] = src[2] + brightness;
      dst[3] = src[3];
      src += 4;
      dst += 4;
    }

  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass            *operation_class =
    GEGL_OPERATION_CLASS (klass);
  GeglOperationPointFilterClass *point_filter_class =
    GEGL_OPERATION_POINT_FILTER_CLASS (klass);

  point_filter_class->process = process;

  gegl_operation_class_set_keys (operation_class,
                                 "name",            "nanotank:gegl-test",
                                 "title",           "NanoTank GEGL-Test",
                                 "categories",      "Artistic",
                                 "description",     "Einfacher GEGL-Test mit Helligkeitsregler",
                                 "gimp:menu-path",  "<Image>/Filters/NanoTank/",
                                 "gimp:menu-label", "GEGL-Test",
                                 NULL);
}

#endif
