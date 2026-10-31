/* SPDX-License-Identifier: MIT – NanoTank
 * P1.4 complete-background compositor. input=background, aux=object RGBA.
 * The full composite replaces the background, never a translucent second copy.
 * Separate operation ID preserves GIMP's single-input NDE path of nanotank:glass.
 */
#ifdef GEGL_PROPERTIES
property_boolean (pixel_geometry,"Pixelhöhenprofil verwenden",TRUE)
property_boolean (preserve_alpha,"Glasauflage auf vorhandenem Material",FALSE)
property_double (surface_height,"Wölbungshöhe (Pixel)",50.0)
  value_range(-1000.0,1000.0)
property_double (surface_extent,"Wölbungsbereich (%)",100.0)
  value_range(1.0,100.0)
property_double (edge_radius,"Kantenrundung (Pixel)",4.0)
  value_range(0.0,64.0)
property_double (thickness,"Glasdicke (Pixel)",12.0)
  value_range(1.0,64.0)
property_double (ior,"Brechungsindex",1.5)
  value_range(1.0,2.5)
property_double (refraction_strength,"Hintergrundbrechung",1.0)
  value_range(0.0,3.0)
property_double (reflection,"Reflexion",0.75)
  value_range(0.0,1.0)
property_double (highlights,"Glanzlichter",0.6)
  value_range(0.0,1.0)
property_double (light_angle,"Lichtrichtung (Grad)",225.0)
  value_range(0.0,360.0)
property_color (tint,"Tönungsfarbe","#bcdff7")
property_double (tint_strength,"Tönungsintensität",0.2)
  value_range(0.0,1.0)
#else
#define GEGL_OP_COMPOSER
#define GEGL_OP_NAME nanotank_glass_refraction
#define GEGL_OP_C_SOURCE nanotank-glass-refraction.c
#include "gegl-op.h"
#include "glass-geometry.h"
#include "glass-optics.h"

typedef struct { GeglRectangle r; gfloat *p; } NtBackground;
static const float *at(const NtBackground *b,int x,int y) {
  if(x<b->r.x || y<b->r.y || x>=b->r.x+b->r.width || y>=b->r.y+b->r.height) return NULL;
  return b->p+((gsize)(y-b->r.y)*b->r.width+x-b->r.x)*4;
}
static void sample(const NtBackground *b,float x,float y,float rgba[4]) {
  x=nt_opt_clamp(x,b->r.x,b->r.x+b->r.width-1);
  y=nt_opt_clamp(y,b->r.y,b->r.y+b->r.height-1);
  int ix=(int)floorf(x),iy=(int)floorf(y);
  float tx=x-ix,ty=y-iy;
  const float *p[4]={at(b,ix,iy),at(b,MIN(ix+1,b->r.x+b->r.width-1),iy),
    at(b,ix,MIN(iy+1,b->r.y+b->r.height-1)),
    at(b,MIN(ix+1,b->r.x+b->r.width-1),MIN(iy+1,b->r.y+b->r.height-1))};
  float w[4]={(1-tx)*(1-ty),tx*(1-ty),(1-tx)*ty,tx*ty};
  rgba[0]=rgba[1]=rgba[2]=rgba[3]=0;
  for(int j=0;j<4;j++) if(p[j]) {
    rgba[3]+=p[j][3]*w[j];
    for(int k=0;k<3;k++) rgba[k]+=p[j][k]*p[j][3]*w[j];
  }
  if(rgba[3]>1e-8f) for(int k=0;k<3;k++) rgba[k]/=rgba[3];
}
static GeglRectangle bounds(GeglOperation *op) {
  const GeglRectangle *r=gegl_operation_source_get_bounding_box(op,"input");
  return r?*r:(GeglRectangle){0,0,0,0};
}
static GeglRectangle required(GeglOperation *op,const gchar *pad,const GeglRectangle *roi) {
  (void)roi;const GeglRectangle *r=gegl_operation_source_get_bounding_box(op,pad);
  return r?*r:(GeglRectangle){0,0,0,0};
}
static GeglRectangle cached(GeglOperation *op,const GeglRectangle *roi) {(void)roi;return bounds(op);}
static GeglRectangle invalidated(GeglOperation *op,const gchar *pad,const GeglRectangle *roi) {
  (void)pad;GeglRectangle r=bounds(op);gegl_rectangle_bounding_box(&r,&r,roi);return r;
}
static void prepare(GeglOperation *op) {
  const Babl *space=gegl_operation_get_source_space(op,"input");
  const Babl *format=babl_format_with_space("RGBA float",space);
  gegl_operation_set_format(op,"input",format);gegl_operation_set_format(op,"aux",format);
  gegl_operation_set_format(op,"output",format);
}
static gboolean process(GeglOperation *op,GeglBuffer *input,GeglBuffer *aux,GeglBuffer *output,
                         const GeglRectangle *roi,gint level) {
  (void)level;
  GeglProperties *o=GEGL_PROPERTIES(op);
  const Babl *fmt=gegl_operation_get_format(op,"output");
  GeglRectangle br=bounds(op);
  const GeglRectangle *ar=gegl_operation_source_get_bounding_box(op,"aux");
  if(!input || !aux || !ar || br.width<1 || br.height<1 || ar->width<1 || ar->height<1) {
    if(input) gegl_buffer_copy(input,roi,GEGL_ABYSS_NONE,output,roi);
    return TRUE;
  }
  gsize bn=(gsize)br.width*br.height,an=(gsize)ar->width*ar->height,rn=(gsize)roi->width*roi->height;
  if(an>INT_MAX || bn>G_MAXSIZE/(4*sizeof(float)) || rn>G_MAXSIZE/(4*sizeof(float))) return FALSE;
  float *bp=g_try_new(float,bn*4),*ap=g_try_new(float,an*4),*dst=g_try_new(float,rn*4);
  float *mask=g_try_new(float,an),*height=g_try_new(float,an),*sx=g_try_new(float,an),*sy=g_try_new(float,an),*path=g_try_new(float,an);
  gboolean ok=FALSE;
  if(!bp || !ap || !dst || !mask || !height || !sx || !sy || !path) goto done;
  gegl_buffer_get(input,&br,1,fmt,bp,GEGL_AUTO_ROWSTRIDE,GEGL_ABYSS_NONE);
  gegl_buffer_get(aux,ar,1,fmt,ap,GEGL_AUTO_ROWSTRIDE,GEGL_ABYSS_NONE);
  for(gsize i=0;i<an;i++) mask[i]=nt_opt_clamp(ap[i*4+3],0,1);
  if(o->pixel_geometry && !nt_geometry_build(mask,ar->width,ar->height,
       o->surface_height,o->surface_extent,o->edge_radius,o->thickness,height,sx,sy,path,NULL)) goto done;
  float tint[4];gegl_color_get_pixel(o->tint,fmt,tint);
  NtBackground bg={br,bp};
  for(int y=roi->y;y<roi->y+roi->height;y++) for(int x=roi->x;x<roi->x+roi->width;x++) {
    float *out=dst+((gsize)(y-roi->y)*roi->width+x-roi->x)*4;
    const float *original=at(&bg,x,y);
    if(!original) {for(int k=0;k<4;k++) out[k]=0;continue;}
    if(x<ar->x || x>=ar->x+ar->width || y<ar->y || y>=ar->y+ar->height) {
      for(int k=0;k<4;k++) out[k]=original[k];
      continue;
    }
    gsize i=(gsize)(y-ar->y)*ar->width+x-ar->x;
    float cover=mask[i];
    if(cover<=0) {for(int k=0;k<4;k++) out[k]=original[k];
      continue;}
    float nz=o->pixel_geometry?1/sqrtf(1+sx[i]*sx[i]+sy[i]*sy[i]):1;
    float nx=o->pixel_geometry?sx[i]*nz:0,ny=o->pixel_geometry?sy[i]*nz:0;
    float dx=0,dy=0,transmitted[4];
    if(o->pixel_geometry && !o->preserve_alpha)
      nt_glass_snell(nx,ny,nz,o->ior,path[i],o->refraction_strength,&dx,&dy);
    sample(&bg,x+dx,y+dy,transmitted);
    NtGlassReflection refl=nt_glass_reflection(nx,ny,nz,o->ior,o->light_angle*G_PI/180,
                                             o->reflection,o->highlights);
    float layer_alpha;
    if(o->preserve_alpha) layer_alpha=1;
    else layer_alpha=refl.alpha+(1-refl.alpha)*transmitted[3];
    out[3]=cover*layer_alpha+(1-cover)*original[3];
    for(int k=0;k<3;k++) {
      float absorption=expf(-o->tint_strength*.6f*fmaxf(0,1-tint[k]));
      float value=o->preserve_alpha?ap[i*4+k]:transmitted[k];
      float alpha=o->preserve_alpha?1:transmitted[3];
      float premul=refl.alpha*refl.color+(1-refl.alpha)*value*absorption*alpha;
      out[k]=(cover*premul+(1-cover)*original[k]*original[3])/fmaxf(1e-8f,out[3]);
    }
  }
  gegl_buffer_set(output,roi,0,fmt,dst,GEGL_AUTO_ROWSTRIDE);ok=TRUE;
done:
  g_free(bp);g_free(ap);g_free(dst);g_free(mask);g_free(height);g_free(sx);g_free(sy);g_free(path);
  return ok;
}
static void gegl_op_class_init(GeglOpClass *klass) {
  GeglOperationClass *op=GEGL_OPERATION_CLASS(klass);
  op->prepare=prepare;op->get_bounding_box=bounds;op->get_required_for_output=required;
  op->get_cached_region=cached;op->get_invalidated_by_change=invalidated;op->threaded=FALSE;
  GEGL_OPERATION_COMPOSER_CLASS(klass)->process=process;
  gegl_operation_class_set_keys(op,"name","nanotank:glass-refraction","title","NanoTank Glass Hintergrundbrechung",
    "categories","Artistic","description","P1.4: vollständiger Hintergrundkomposit mit separater Alphaform",NULL);
}
#endif
