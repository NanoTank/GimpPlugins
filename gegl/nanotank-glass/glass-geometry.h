/* SPDX-License-Identifier: MIT
 * NanoTank: continuous, component-local 2.5D geometry. No GIMP dependency.
 */
#ifndef NT_GLASS_GEOMETRY_H
#define NT_GLASS_GEOMETRY_H
#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <stddef.h>

typedef struct {
  double maximum, minimum, omega;
  int x0, y0, x1, y1;
} NtGeoComponent;
typedef struct { int components, iterations; double residual; } NtGeoStats;

/* Profile derivative is with respect to normalized Poisson height t.
 * For a circle, t=1-r²/R², so a full profile is exactly H sqrt(t).
 * The restricted profile reaches an exactly flat plateau with zero slope. */
static inline double
nt_geo_profile(double t, double fraction, double *derivative)
{
  t=fmin(1.0,fmax(1e-12,t));
  if(fraction>=1.0-1e-10) {
    *derivative=0.5/sqrt(t); return sqrt(t);
  }
  double p=fmax(1e-6,fraction), u=1.0-sqrt(fmax(0.0,1.0-t));
  double q=u/p;
  if(q>=1.0) { *derivative=0; return 1; }
  double f=sqrt(fmax(1e-12,q*(2.0-q)));
  *derivative=(1.0-q)/(2.0*p*sqrt(fmax(1e-12,1.0-t))*f);
  return f;
}

/* Solve -Laplacian(phi)=1 with zero contour boundary, separately for each
 * 4-connected component. The half-cell boundary uses odd ghost values.
 * This smooth contour coordinate replaces medial-axis ridges of the SDF.
 * mask and optical_path may alias. All output arrays have width*rows entries.
 * thickness is the MINIMUM positive material path, never a signed height.
 */
/* Owned contour field: zero-initialize before first build; clear on release. */
typedef struct {
  int width, rows, count;
  int *label;
  double *phi;
  NtGeoComponent *comp;
  NtGeoStats stats;
} NtGeoField;

static void
nt_geometry_field_clear(NtGeoField *field)
{
  free(field->label);free(field->phi);free(field->comp);
  *field=(NtGeoField){0};
}

static int
nt_geometry_field_build(const float *mask,int width,int rows,NtGeoField *field)
{
  if(width<1 || rows<1 || (size_t)width*(size_t)rows>INT_MAX) return 0;
  size_t n=(size_t)width*rows;
  int *label=calloc(n,sizeof(int)), *queue=malloc(n*sizeof(int));
  double *phi=calloc(n,sizeof(double));
  NtGeoComponent *comp=NULL;
  int count=0, capacity=0, success=0;
  if(!label || !queue || !phi) goto done;
  for(size_t i=0;i<n;i++) label[i]=mask[i]>0.001f ? -1 : 0;
  for(int seed=0;seed<(int)n;seed++) if(label[seed]==-1) {
    if(count==capacity) {
      int next=capacity?capacity*2:16;
      NtGeoComponent *grown=realloc(comp,(size_t)next*sizeof(*comp));
      if(!grown) goto done;
      comp=grown;capacity=next;
    }
    NtGeoComponent *c=&comp[count++];
    *c=(NtGeoComponent){0,0,1,seed%width,seed/width,seed%width,seed/width};
    int head=0,tail=0; queue[tail++]=seed;label[seed]=count;
    while(head<tail) {
      int i=queue[head++], x=i%width, y=i/width;
      c->x0=x<c->x0?x:c->x0;c->x1=x>c->x1?x:c->x1;
      c->y0=y<c->y0?y:c->y0;c->y1=y>c->y1?y:c->y1;
      int near[4]={x?i-1:-1,x+1<width?i+1:-1,y?i-width:-1,y+1<rows?i+width:-1};
      for(int k=0;k<4;k++) if(near[k]>=0 && label[near[k]]==-1) {
        label[near[k]]=count;queue[tail++]=near[k];
      }
    }
    double size=fmin(fmax(c->x1-c->x0+2,c->y1-c->y0+2),2.0*sqrt(tail)+1.0);
    c->omega=fmin(1.995,2.0/(1.0+sin(3.141592653589793/size)));
  }
  int active=0;
  for(int i=0;i<(int)n;i++) if(label[i]) queue[active++]=i;
  double change=0; int iteration;
  for(iteration=0;iteration<3000;iteration++) {
    change=0;
    for(int j=0;j<active;j++) {
      int i=queue[j],x=i%width,y=i/width;
      int near[4]={x?i-1:-1,x+1<width?i+1:-1,y?i-width:-1,y+1<rows?i+width:-1};
      double sum=1;int outside=0;
      for(int k=0;k<4;k++) {
        if(near[k]>=0 && label[near[k]]==label[i]) sum+=phi[near[k]];
        else outside++;
      }
      double delta=comp[label[i]-1].omega*(sum/(4+outside)-phi[i]);
      phi[i]+=delta;
      change=fmax(change,fabs(delta)/fmax(1.0,fabs(phi[i])));
    }
    if(change<1e-8) break;
  }
  if(iteration==3000) goto done; /* Never quietly publish unconverged normals. */
  for(size_t i=0;i<n;i++) if(label[i]) {
    NtGeoComponent *c=&comp[label[i]-1];c->maximum=fmax(c->maximum,phi[i]);
  }
  nt_geometry_field_clear(field);
  *field=(NtGeoField){width,rows,count,label,phi,comp,{count,iteration+1,change}};
  label=NULL;phi=NULL;comp=NULL;
  success=1;
done:
  free(label);free(queue);free(phi);free(comp);return success;
}

static void
nt_geometry_field_profile(NtGeoField *field,double height_px,double range_percent,
                          double edge_px,double thickness,float *height,
                          float *slope_x,float *slope_y,float *optical_path)
{
  int width=field->width,rows=field->rows;
  size_t n=(size_t)width*rows;
  int *label=field->label;double *phi=field->phi;
  NtGeoComponent *comp=field->comp;
  for(int c=0;c<field->count;c++)comp[c].minimum=0;
  for(size_t i=0;i<n;i++) {
    if(!label[i]) {height[i]=slope_x[i]=slope_y[i]=optical_path[i]=0;continue;}
    NtGeoComponent *c=&comp[label[i]-1];
    double radius=2.0*sqrt(c->maximum);
    double t=phi[i]/c->maximum, dh, de=0;
    double f=nt_geo_profile(t,range_percent/100.0,&dh);
    double edge=fmin(fmax(0,edge_px),radius*0.45), z=height_px*f;
    double dz=height_px*dh;
    if(edge>1e-6) {
      double e=nt_geo_profile(t,edge/radius,&de);
      z+=edge*(e-1.0);dz+=edge*de;
    }
    height[i]=(float)z;c->minimum=fmin(c->minimum,z);
    int x=(int)i%width,y=(int)i/width;
    int near[4]={x?(int)i-1:-1,x+1<width?(int)i+1:-1,y?(int)i-width:-1,y+1<rows?(int)i+width:-1};
    double v[4];
    for(int k=0;k<4;k++) v[k]=near[k]>=0 && label[near[k]]==label[i] ? phi[near[k]] : -phi[i];
    /* Outward normal for a height graph z(x,y) is (-dz/dx,-dz/dy,1). */
    slope_x[i]=(float)(-dz*(v[1]-v[0])*0.5/c->maximum);
    slope_y[i]=(float)(-dz*(v[3]-v[2])*0.5/c->maximum);
  }
  for(size_t i=0;i<n;i++) if(label[i])
    optical_path[i]=(float)fmax(fmax(1e-6,thickness),
                              thickness+height[i]-comp[label[i]-1].minimum);
}

static inline int
nt_geometry_build(const float *mask,int width,int rows,double height_px,
                  double range_percent,double edge_px,double thickness,
                  float *height,float *slope_x,float *slope_y,float *optical_path,
                  NtGeoStats *stats)
{
  NtGeoField field={0};
  if(!nt_geometry_field_build(mask,width,rows,&field))return 0;
  nt_geometry_field_profile(&field,height_px,range_percent,edge_px,thickness,
                            height,slope_x,slope_y,optical_path);
  if(stats)*stats=field.stats;
  nt_geometry_field_clear(&field);return 1;
}
#endif
