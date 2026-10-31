/* SPDX-License-Identifier: MIT — NanoTank
 * Actual GEGL evaluation, including full float-buffer readback. */
#include <gegl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef NT_GEOMETRY_HEADER
#define NT_GEOMETRY_HEADER "../../gegl/nanotank-glass/glass-geometry.h"
#endif
#include NT_GEOMETRY_HEADER
static double now(void){return g_get_monotonic_time()/1e6;}
int main(int argc,char **argv){
 gegl_init(&argc,&argv);g_assert(gegl_has_operation("nanotank:glass"));
 const char *tag=argc>1?argv[1]:"current";
 const int sizes[][3]={{1024,768,0},{512,512,1},{1024,768,2},{1920,1080,0},{3840,2160,0}};
 puts("case,phase,seconds,geometry_seconds,iterations");
 for(int c=0;c<5;c++){
  int w=sizes[c][0],h=sizes[c][1],kind=sizes[c][2];size_t n=(size_t)w*h;
  float *src=g_new0(float,n*4),*dst=g_new(float,n*4),*mask=g_new(float,n);
  for(int y=0;y<h;y++)for(int x=0;x<w;x++){
   double dx=x-w/2,dy=y-h/2;float a=kind==1?CLAMP(100.5-hypot(dx,dy),0,1):
     kind==2?(fabs(dx+160)<70&&fabs(dy)<55)||(hypot(dx-120,dy)<90):fabs(dx)<160&&fabs(dy)<90;
   size_t i=(size_t)y*w+x;src[4*i]=.35;src[4*i+1]=.08;src[4*i+2]=.03;src[4*i+3]=a;mask[i]=a;
  }
  float *z=g_new(float,n),*sx=g_new(float,n),*sy=g_new(float,n),*path=g_new(float,n);NtGeoStats st;
  double t=now();g_assert(nt_geometry_build(mask,w,h,10,100,4,12,z,sx,sy,path,&st));double solve=now()-t;
  g_free(z);g_free(sx);g_free(sy);g_free(path);g_free(mask);
  GeglRectangle r={0,0,w,h};GeglBuffer *b=gegl_buffer_new(&r,babl_format("RGBA float"));
  gegl_buffer_set(b,&r,0,babl_format("RGBA float"),src,GEGL_AUTO_ROWSTRIDE);
  GeglNode *root=gegl_node_new(),*s=gegl_node_new_child(root,"operation","gegl:buffer-source","buffer",b,NULL);
  GeglNode *g=gegl_node_new_child(root,"operation","nanotank:glass","alpha-shape",TRUE,"pixel-geometry",TRUE,"preserve-alpha",FALSE,NULL);gegl_node_link(s,g);
  for(int k=0;k<7;k++){
   if(k==1)gegl_node_set(g,"surface-height",-20.0,NULL);
   if(k==2)gegl_node_set(g,"reflection",.6,NULL);
   if(k>=3)gegl_node_set(g,"light-angle",45.0+k,NULL);
   t=now();gegl_node_blit(g,1,&r,babl_format("RGBA float"),dst,GEGL_AUTO_ROWSTRIDE,GEGL_BLIT_DEFAULT);double elapsed=now()-t;
   for(size_t i=0;i<n*4;i++)g_assert(isfinite(dst[i]));
   printf("%dx%d-%d,%s,%.6f,%.6f,%d\n",w,h,kind,k==0?"cold":k==1?"geometry":k==2?"material":"repeat",elapsed,solve,st.iterations);fflush(stdout);
   char name[256];snprintf(name,sizeof name,"output/glass-p142/%s-%d-%d.bin",tag,c,k);FILE *f=fopen(name,"wb");g_assert(f);g_assert(fwrite(dst,sizeof(float),n*4,f)==n*4);fclose(f);
  }
  g_object_unref(root);g_object_unref(b);g_free(src);g_free(dst);
 }
 gegl_exit();return 0;
}
