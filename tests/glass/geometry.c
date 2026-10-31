/* SPDX-License-Identifier: MIT – NanoTank */
#include "../../gegl/nanotank-glass/glass-geometry.h"
#include <assert.h>
#include <stdio.h>
#define W 256
#define N (W*W)
static float mask[N], z[N], sx[N], sy[N], path[N], reference[N];
static void circle(int cx,int cy,int r) {
  for(int y=0;y<W;y++) for(int x=0;x<W;x++)
    if(hypot(x-cx,y-cy)<=r) mask[y*W+x]=1;
}
static NtGeoStats build(double h,double range,double edge,double thick) {
  NtGeoStats stats;
  assert(nt_geometry_build(mask,W,W,h,range,edge,thick,z,sx,sy,path,&stats));
  for(int i=0;i<N;i++) {
    assert(isfinite(z[i])&&isfinite(sx[i])&&isfinite(sy[i])&&isfinite(path[i]));
    if(mask[i]) assert(path[i]>=thick-1e-4);
    else assert(z[i]==0&&sx[i]==0&&sy[i]==0&&path[i]==0);
  }
  return stats;
}
int main(void) {
  circle(128,128,100);
  const double heights[]={0,10,50,100,150,-5,-20};
  for(int h=0;h<7;h++) {
    NtGeoStats s=build(heights[h],100,0,12);
    assert(fabs(z[128*W+128]-heights[h])<1e-3);
    double maxerror=0;
    for(int r=0;r<=90;r+=5) {
      int i=128*W+128+r;
      double expected=heights[h]*sqrt(1-r*r/10000.0);
      maxerror=fmax(maxerror,fabs(z[i]-expected));
      assert(fabs(atan(sx[i])-atan(heights[h]*r/(10000.0*sqrt(1-r*r/10000.0))))<.012);
    }
    assert(maxerror<.65);
    printf("circle H=%g: max interior height error %.4f px (%d solver sweeps)\n",heights[h],maxerror,s.iterations);
  }
  build(-20,15,4,12);
  assert(fabs(z[128*W+128]+20)<1e-4);
  assert(fabs(sx[128*W+128+40])<1e-5); /* exact flat depressed plateau */
  for(int i=0;i<N;i++) reference[i]=z[i];
  build(-20,15,4,30);
  for(int i=0;i<N;i++) assert(z[i]==reference[i]); /* thickness independent */
  for(int i=0;i<N;i++) mask[i]=0;
  circle(50,60,24);build(10,100,2,12);
  for(int i=0;i<N;i++) reference[i]=z[i];
  circle(185,170,56);NtGeoStats c=build(10,100,2,12);assert(c.components==2);
  for(int y=25;y<95;y++) for(int x=15;x<85;x++)
    assert(fabs(z[y*W+x]-reference[y*W+x])<1e-4);
  for(int i=0;i<N;i++) mask[i]=0;
  mask[10*W+11]=1;mask[10*W+12]=1;mask[13*W+14]=1;
  assert(build(-100,1,64,1).components==2);
  puts("PASS: analytic circle, signed height, flat plateau, thickness, isolated components, subpixel-width limits");
  return 0;
}
