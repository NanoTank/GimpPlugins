/* SPDX-License-Identifier: MIT — NanoTank
 * Per-operation cache. Validate actual binary contour, never input identity or
 * a hash alone. No pixels, colors or fractional alpha are cached. */
#ifndef NT_GLASS_GEOMETRY_CACHE_H
#define NT_GLASS_GEOMETRY_CACHE_H
#include <string.h>
typedef struct {
  GMutex mutex;
  NtGeoField field;
  float *mask,*height,*sx,*sy,*path;
  double h,extent,edge,thickness;
  gboolean profile_valid;
  guint64 field_builds,profile_builds;
} NtGeometryCache;
static void nt_geometry_cache_free(gpointer data)
{
  NtGeometryCache *c=data;
  nt_geometry_field_clear(&c->field);
  g_free(c->mask);g_free(c->height);g_free(c->sx);g_free(c->sy);g_free(c->path);
  g_mutex_clear(&c->mutex);g_free(c);
}
static NtGeometryCache *nt_geometry_cache(GeglOperation *operation)
{
  /* Creation only: computations on independent operations never share locks. */
  static GMutex creation;
  g_mutex_lock(&creation);
  NtGeometryCache *c=g_object_get_data(G_OBJECT(operation),"nanotank-geometry-cache");
  if(!c) {
    c=g_new0(NtGeometryCache,1);g_mutex_init(&c->mutex);
    g_object_set_data_full(G_OBJECT(operation),"nanotank-geometry-cache",c,nt_geometry_cache_free);
  }
  g_mutex_unlock(&creation);return c;
}
static gboolean nt_geometry_cached(GeglOperation *operation,const float *mask,
 int width,int rows,double h,double extent,double edge,double thickness,
 float *height,float *sx,float *sy,float *path)
{
  int x0=width,y0=rows,x1=0,y1=0;
  for(int y=0;y<rows;y++)for(int x=0;x<width;x++)if(mask[(gsize)y*width+x]>.001f){
    x0=MIN(x0,x);y0=MIN(y0,y);x1=MAX(x1,x+1);y1=MAX(y1,y+1);
  }
  int w=MAX(0,x1-x0),r=MAX(0,y1-y0);gsize n=(gsize)w*r;
  float *contour=g_try_new(float,MAX(n,1));if(!contour)return FALSE;
  for(int y=0;y<r;y++)for(int x=0;x<w;x++)
    contour[(gsize)y*w+x]=mask[(gsize)(y+y0)*width+x+x0]>.001f?1:0;
  NtGeometryCache *c=nt_geometry_cache(operation);gboolean ok=TRUE;
  g_mutex_lock(&c->mutex);
  if(n && (c->field.width!=w || c->field.rows!=r || !c->mask ||
            memcmp(c->mask,contour,n*sizeof(float)))) {
    /* Build into a temporary: failed allocation/convergence cannot validate
     * a stale field. Extent/position and fractional coverage remain live. */
    NtGeoField field={0};
    if(!nt_geometry_field_build(contour,w,r,&field)){ok=FALSE;goto done;}
    float *z=g_try_new(float,n),*a=g_try_new(float,n),*b=g_try_new(float,n),*p=g_try_new(float,n);
    if(!z||!a||!b||!p){g_free(z);g_free(a);g_free(b);g_free(p);nt_geometry_field_clear(&field);ok=FALSE;goto done;}
    nt_geometry_field_clear(&c->field);c->field=field;c->field_builds++;
    g_free(c->mask);c->mask=contour;contour=NULL;
    g_free(c->height);g_free(c->sx);g_free(c->sy);g_free(c->path);
    c->height=z;c->sx=a;c->sy=b;c->path=p;c->profile_valid=FALSE;
  }
  if(n && (!c->profile_valid || c->h!=h || c->extent!=extent || c->edge!=edge || c->thickness!=thickness)) {
    nt_geometry_field_profile(&c->field,h,extent,edge,thickness,c->height,c->sx,c->sy,c->path);
    c->profile_builds++;c->h=h;c->extent=extent;c->edge=edge;c->thickness=thickness;c->profile_valid=TRUE;
  }
  memset(height,0,(gsize)width*rows*sizeof(float));
  memset(sx,0,(gsize)width*rows*sizeof(float));memset(sy,0,(gsize)width*rows*sizeof(float));
  memset(path,0,(gsize)width*rows*sizeof(float));
  for(int y=0;y<r;y++) {
    gsize dest=(gsize)(y+y0)*width+x0,source=(gsize)y*w;
    memcpy(height+dest,c->height+source,w*sizeof(float));
    memcpy(sx+dest,c->sx+source,w*sizeof(float));memcpy(sy+dest,c->sy+source,w*sizeof(float));
    memcpy(path+dest,c->path+source,w*sizeof(float));
  }
done:
  g_mutex_unlock(&c->mutex);g_free(contour);return ok;
}
#endif
