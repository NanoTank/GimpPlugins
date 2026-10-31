/* SPDX-License-Identifier: MIT – NanoTank
 * Shared P1.3 studio reflection and a bounded-view Snell transmission model.
 */
#ifndef NT_GLASS_OPTICS_H
#define NT_GLASS_OPTICS_H
#include <math.h>
typedef struct { float alpha, color; } NtGlassReflection;
static inline float nt_opt_clamp(float x,float a,float b) {return fminf(b,fmaxf(a,x));}
static inline NtGlassReflection
nt_glass_reflection(float nx,float ny,float nz,float ior,float angle,
                    float reflection,float highlights)
{
  float lx=cosf(angle),ly=sinf(angle);
  float rx=2*nx*nz,ry=2*ny*nz,rz=2*nz*nz-1;
  float along=rx*lx+ry*ly,across=-rx*ly+ry*lx;
  float window=expf(-.5f*powf((along-.64f)/.22f,2)-.5f*powf(across/.65f,2));
  window*=nt_opt_clamp((rz+.15f)*4,0,1);
  float card=expf(-.5f*powf((along+.72f)/.20f,2)-.5f*powf(across/.70f,2));
  float lamps=highlights*(9*window+1.8f*card);
  float room=.35f+1.5f*powf(1-nz,.65f),radiance=room+lamps;
  float f0=powf((ior-1)/(ior+1),2);
  float fresnel=ior<=1?0:f0+(1-f0)*powf(1-nz,5);
  return (NtGlassReflection){1-expf(-reflection*fresnel*radiance),(.035f+lamps)/radiance};
}
/* View ray (0,0,-1), air->glass. Project the transmitted direction onto a
 * reference plane below the surface. This is not a second refracting surface
 * or raytraced sphere. Signed geometry changes N, not an arbitrary offset sign. */
static inline void
nt_glass_snell(float nx,float ny,float nz,float ior,float path,float strength,
               float *dx,float *dy)
{
  float eta=1/ior;
  float c=eta*nz-sqrtf(fmaxf(0,1-eta*eta*(1-nz*nz)));
  float tz=-eta+c*nz;
  float travel=fmaxf(0,path)*strength/fmaxf(1e-5f,-tz);
  *dx=c*nx*travel;*dy=c*ny*travel;
}
#endif
