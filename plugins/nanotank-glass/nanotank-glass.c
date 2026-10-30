/* SPDX-License-Identifier: MIT
 * NanoTank Glass: prototype GEGL area filter for GIMP 3.2.
 * Shape edges follow the layer rectangle or its alpha, NOT the GIMP selection.
 * Refraction samples this layer, not the merged projection underneath it.
 */
#ifdef GEGL_PROPERTIES

property_double (thickness, "Glasdicke / Kantenlicht (px)", 12.0)
  description ("Breite des hellen Glasrandes")
  value_range (1.0, 64.0)

property_double (rounding, "Eckenrundung (px)", 18.0)
  description ("Rundung der rechteckigen Glasfläche")
  value_range (0.0, 250.0)

property_double (opacity, "Glas-Deckkraft", 0.24)
  description ("Deckkraft der Glasauflage; der opake Untergrund bleibt erhalten")
  value_range (0.0, 1.0)

property_color (tint_color, "Tönungsfarbe", "rgb(150,190,230)")
  description ("Farbe der Glasauflage")

property_double (tint_strength, "Tönungsstärke", 0.22)
  description ("Einfluss der Tönungsfarbe auf die Glasauflage")
  value_range (0.0, 1.0)

property_double (reflection, "Reflexionen", 0.65)
  description ("Stärke von Fresnel-Kantenlicht und Glanzlichtern")
  value_range (0.0, 1.0)

property_double (refraction, "Brechung (px)", 4.0)
  description ("Verschiebt Bildpunkte der aktuellen Ebene")
  value_range (0.0, 32.0)

property_double (bulge, "Wölbung", 0.30)
  description ("Stärke der linsenartigen Wölbung")
  value_range (0.0, 1.0)

property_double (ior, "Brechungsindex", 1.50)
  description ("Optischer Brechungsindex, typisches Glas etwa 1,5")
  value_range (1.0, 2.5)

property_boolean (alpha_shape, "Form aus Ebenen-Alpha", FALSE)
  description ("Glasform folgt dem Alphakanal der Ebene statt deren rechteckiger Begrenzung")

#else

#define GEGL_OP_AREA_FILTER
#define GEGL_OP_NAME nanotank_glass
#define GEGL_OP_C_SOURCE nanotank-glass.c
#include "gegl-op.h"
#include "glass-core.h"

static inline GlassPixel
pixel_at (const gfloat *data, const GeglRectangle *r, gint x, gint y)
{
  gint ix = CLAMP(x - r->x, 0, r->width - 1);
  gint iy = CLAMP(y - r->y, 0, r->height - 1);
  const gfloat *v = data + (((gsize)iy * r->width + ix) * 4);
  return (GlassPixel){v[0],v[1],v[2],v[3]};
}

static inline GlassPixel
sample_at (const gfloat *data, const GeglRectangle *r, gfloat x, gfloat y)
{
  gint x0 = (gint)floorf(x), y0 = (gint)floorf(y);
  gfloat ax = x - x0, ay = y - y0;
  GlassPixel a = pixel_at(data,r,x0,y0), b = pixel_at(data,r,x0+1,y0);
  GlassPixel c = pixel_at(data,r,x0,y0+1), d = pixel_at(data,r,x0+1,y0+1);
  return (GlassPixel){
    (1-ay)*((1-ax)*a.r+ax*b.r)+ay*((1-ax)*c.r+ax*d.r),
    (1-ay)*((1-ax)*a.g+ax*b.g)+ay*((1-ax)*c.g+ax*d.g),
    (1-ay)*((1-ax)*a.b+ax*b.b)+ay*((1-ax)*c.b+ax*d.b),
    (1-ay)*((1-ax)*a.a+ax*b.a)+ay*((1-ax)*c.a+ax*d.a)
  };
}

static inline gfloat
rounded_sdf (gfloat x, gfloat y, const GeglRectangle *bounds, gfloat radius)
{
  gfloat w = bounds->width, h = bounds->height;
  gfloat cx = bounds->x + w*0.5f, cy = bounds->y + h*0.5f;
  radius = MIN(radius, MIN(w,h)*0.5f);
  gfloat qx = fabsf(x-cx) - (w*0.5f-radius);
  gfloat qy = fabsf(y-cy) - (h*0.5f-radius);
  return hypotf(MAX(qx,0.f),MAX(qy,0.f))+MIN(MAX(qx,qy),0.f)-radius;
}

static void
prepare (GeglOperation *operation)
{
  GeglProperties *p = GEGL_PROPERTIES(operation);
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER(operation);
  const Babl *space = gegl_operation_get_source_space(operation, "input");
  const Babl *format = babl_format_with_space("RGBA float", space);
  gint pad = (gint)ceil(MAX(p->thickness, 2.0*p->refraction + 2.0)) + 4;
  area->left = area->right = area->top = area->bottom = pad;
  gegl_operation_set_format(operation, "input", format);
  gegl_operation_set_format(operation, "output", format);
}

static gboolean
process (GeglOperation *operation, GeglBuffer *input, GeglBuffer *output,
         const GeglRectangle *roi, gint level)
{
  GeglProperties *p = GEGL_PROPERTIES(operation);
  GeglOperationAreaFilter *area = GEGL_OPERATION_AREA_FILTER(operation);
  GeglRectangle src = {roi->x-area->left,roi->y-area->top,
                      roi->width+area->left+area->right,
                      roi->height+area->top+area->bottom};
  const GeglRectangle *bounds = gegl_buffer_get_extent(input);
  const Babl *format = gegl_operation_get_format(operation, "input");
  GlassSurface gs;
  gfloat rgba[4] = {0.6f,0.75f,0.9f,1.0f};
  gfloat *source, *dest;
  gint x,y;
  (void)level;
  if (!bounds || bounds->width <= 0 || bounds->height <= 0 ||
      roi->width <= 0 || roi->height <= 0)
    return TRUE;
  /* Guard multiplication on extremely large or malformed regions. */
  if ((gsize)src.width > G_MAXSIZE / ((gsize)src.height * 4 * sizeof(gfloat)) ||
      (gsize)roi->width > G_MAXSIZE / ((gsize)roi->height * 4 * sizeof(gfloat)))
    return FALSE;
  source = g_try_new(gfloat,(gsize)src.width*src.height*4);
  dest = g_try_new(gfloat,(gsize)roi->width*roi->height*4);
  if (!source || !dest) {g_free(source);g_free(dest);return FALSE;}
  gegl_buffer_get(input,&src,1.0,format,source,GEGL_AUTO_ROWSTRIDE,GEGL_ABYSS_CLAMP);
  gegl_color_get_pixel(p->tint_color,format,rgba);
  gs = (GlassSurface){p->opacity,p->tint_strength,p->reflection,p->ior,
                      {rgba[0],rgba[1],rgba[2]}};
  for (y=0;y<roi->height;y++) for (x=0;x<roi->width;x++) {
    gfloat px=roi->x+x+0.5f, py=roi->y+y+0.5f;
    GlassPixel base=pixel_at(source,&src,roi->x+x,roi->y+y);
    gfloat cover, edge, nx=0, ny=0;
    if (p->alpha_shape) {
      gint rad=MAX(1,(gint)ceil(p->thickness));
      gfloat left=pixel_at(source,&src,roi->x+x-rad,roi->y+y).a;
      gfloat right=pixel_at(source,&src,roi->x+x+rad,roi->y+y).a;
      gfloat up=pixel_at(source,&src,roi->x+x,roi->y+y-rad).a;
      gfloat down=pixel_at(source,&src,roi->x+x,roi->y+y+rad).a;
      cover=glass_clamp(base.a,0,1);
      edge=glass_clamp(1.0f-MIN(MIN(left,right),MIN(up,down)),0,1);
      nx=left-right; ny=up-down;
    } else {
      gfloat sdf=rounded_sdf(px,py,bounds,(gfloat)p->rounding);
      cover=glass_clamp(0.5f-sdf,0,1);
      edge=glass_clamp(1.0f+sdf/(gfloat)MAX(1.0,p->thickness),0,1);
      nx=(px-(bounds->x+bounds->width*0.5f))/(bounds->width*0.5f);
      ny=(py-(bounds->y+bounds->height*0.5f))/(bounds->height*0.5f);
    }
    /* Soft diagonal reflection, plus smaller upper-left glint. */
    gfloat ux=(px-bounds->x)/MAX(1.f,(gfloat)bounds->width);
    gfloat uy=(py-bounds->y)/MAX(1.f,(gfloat)bounds->height);
    gfloat band=(ux*0.80f+uy*0.50f-0.32f);
    gfloat highlight=0.85f*expf(-(band*band)/0.009f)*
                       (0.45f+0.55f*(1.0f-uy));
    highlight+=0.22f*expf(-((ux-0.15f)*(ux-0.15f)/0.02f+
                            (uy-0.12f)*(uy-0.12f)/0.02f));
    gfloat dx=(gfloat)p->refraction *
              (0.5f*nx*edge + 0.75f*(gfloat)p->bulge*(0.5f-ux));
    gfloat dy=(gfloat)p->refraction *
              (0.5f*ny*edge + 0.75f*(gfloat)p->bulge*(0.5f-uy));
    GlassPixel shifted=sample_at(source,&src,roi->x+x+dx,roi->y+y+dy);
    /* Preserve material alpha: an opaque button cannot become transparent. */
    shifted.a=base.a;
    GlassPixel shaded=glass_shade(shifted,cover,edge,highlight,&gs);
    /* Out-of-shape pixels stay strictly identical to the input. */
    if (cover<=0.0f) shaded=base;
    gsize off=((gsize)y*roi->width+x)*4;
    dest[off]=shaded.r;dest[off+1]=shaded.g;
    dest[off+2]=shaded.b;dest[off+3]=shaded.a;
  }
  gegl_buffer_set(output,roi,0,format,dest,GEGL_AUTO_ROWSTRIDE);
  g_free(source);g_free(dest);
  return TRUE;
}

static void
gegl_op_class_init (GeglOpClass *klass)
{
  GeglOperationClass *operation=GEGL_OPERATION_CLASS(klass);
  GeglOperationFilterClass *filter=GEGL_OPERATION_FILTER_CLASS(klass);
  operation->prepare=prepare;
  filter->process=process;
  gegl_operation_class_set_keys(operation,
    "name", "nanotank:glass",
    "title", "NanoTank Glass (Prototyp)",
    "categories", "artistic",
    "description", "Glasauflage mit Fresnel-Reflexion, Tönung und Ebenenbrechung",
    "gimp:menu-path", "<Image>/Filters/NanoTank/",
    "gimp:menu-label", "Glass",
    NULL);
}
#endif
