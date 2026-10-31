/* SPDX-License-Identifier: MIT – NanoTank
 * Native P1.4 background/compositing tests and reproducible GEGL images.
 */
#include <gegl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define W 384
#define H 280
#define N (W*H*4)
static float bg[N],shape[N],pixels[N],reference[N],text[N],on_pixels[N];
static const char *gallery="output/glass-p14";
static const GeglRectangle rect={0,0,W,H};
static GeglNode *root,*glass,*old,*over;
static GeglBuffer *background,*form,*output;
static void read_node(GeglNode *node) {
  gegl_node_blit_buffer(node,output,NULL,0,GEGL_ABYSS_NONE);
  gegl_buffer_get(output,&rect,1,babl_format("RGBA float"),pixels,GEGL_AUTO_ROWSTRIDE,GEGL_ABYSS_NONE);
  for(int i=0;i<N;i++) g_assert(isfinite(pixels[i]));
  for(int i=3;i<N;i+=4) g_assert(pixels[i]>=0 && pixels[i]<=1);
}
static void save(const char *name) {
  char path[512];snprintf(path,sizeof path,"%s/%s.png",gallery,name);
  GeglNode *source=gegl_node_new_child(root,"operation","gegl:buffer-source","buffer",output,NULL);
  GeglNode *sink=gegl_node_new_child(root,"operation","gegl:png-save","path",path,NULL);
  gegl_node_link(source,sink);gegl_node_process(sink);
}
static void fixture(int type,int partial) {
  for(int y=0;y<H;y++) for(int x=0;x<W;x++) {
    int i=(y*W+x)*4;double xx=x-192,yy=y-140,d=100-hypot(xx,yy);
    if(type==1 || type==4) d=fmin(125-fabs(xx),78-fabs(yy));
    if(type==2) d=90+14*sin(3*atan2(yy,xx))+9*cos(5*atan2(yy,xx))-hypot(xx,yy);
    if(type==5) d=fmax(42-hypot(x-90,y-140),73-hypot(x-270,y-140));
    shape[i]=.24f;shape[i+1]=.24f;shape[i+2]=.24f;shape[i+3]=CLAMP(d+.5,0,1);
    float a=((x/16+y/16)%2)?.65f:.035f;
    for(int k=0;k<3;k++) bg[i+k]=a;
    if(x%48<2 || y%48<2) {bg[i]=.38f;bg[i+1]=.09f;bg[i+2]=.025f;}
    if(type==1 || type==3) for(int k=0;k<3;k++) bg[i+k]=(((x/24+y/24)%2)?.62f:.32f)*(1-text[i+3])+.004f*text[i+3];
    bg[i+3]=partial ? .35f+.30f*(float)x/W : 1;
  }
  gegl_buffer_set(background,&rect,0,babl_format("RGBA float"),bg,GEGL_AUTO_ROWSTRIDE);
  gegl_buffer_set(form,&rect,0,babl_format("RGBA float"),shape,GEGL_AUTO_ROWSTRIDE);
}
int main(int argc,char **argv) {
  gegl_init(&argc,&argv);g_assert(gegl_has_operation("nanotank:glass-refraction"));
  if(getenv("NANOTANK_GLASS_GALLERY_ROOT")) gallery=getenv("NANOTANK_GLASS_GALLERY_ROOT");
  g_mkdir_with_parents(gallery,0755);
  root=gegl_node_new();background=gegl_buffer_new(&rect,babl_format("RGBA float"));
  form=gegl_buffer_new(&rect,babl_format("RGBA float"));output=gegl_buffer_new(&rect,babl_format("RGBA float"));
  GeglNode *t=gegl_node_new_child(root,"operation","gegl:text","string","GLAS 123 GLAS\nNanoTank GLASS",
    "font","Sans Bold","size",31.0,NULL);
  GeglNode *move=gegl_node_new_child(root,"operation","gegl:translate","x",20.0,"y",90.0,NULL);
  gegl_node_link(t,move);gegl_node_blit_buffer(move,output,&rect,0,GEGL_ABYSS_NONE);
  gegl_buffer_get(output,&rect,1,babl_format("RGBA float"),text,GEGL_AUTO_ROWSTRIDE,GEGL_ABYSS_NONE);
  GeglNode *b=gegl_node_new_child(root,"operation","gegl:buffer-source","buffer",background,NULL);
  GeglNode *s=gegl_node_new_child(root,"operation","gegl:buffer-source","buffer",form,NULL);
  glass=gegl_node_new_child(root,"operation","nanotank:glass-refraction",NULL);
  gegl_node_link(b,glass);gegl_node_connect(s,"output",glass,"aux");
  old=gegl_node_new_child(root,"operation","nanotank:glass","pixel-geometry",TRUE,
    "preserve-alpha",FALSE,"alpha-shape",TRUE,"glass-opacity",.15,NULL);
  gegl_node_link(s,old);
  over=gegl_node_new_child(root,"operation","gegl:over",NULL);
  gegl_node_link(b,over);gegl_node_connect(old,"output",over,"aux");
  const char *names[]={"circle","plate-text","organic","concave-text","coating","separate"};
  for(int scene=0;scene<6;scene++) {
    double height=scene==0?100:scene==3?-30:scene==4?35:scene==1?10:40;
    double extent=scene==1?15:100,edge=scene==0?0:4;
    fixture(scene,0);
    gegl_node_set(glass,"surface-height",height,"surface-extent",extent,"edge-radius",edge,
      "preserve-alpha",scene==4,"tint-strength",.2,NULL);
    gegl_node_set(old,"surface-height",height,"surface-extent",extent,"edge-radius",edge,
      "preserve-alpha",scene==4,"tint-strength",.2,NULL);
    char name[120];read_node(b);snprintf(name,sizeof name,"%s-original",names[scene]);save(name);
    read_node(over);snprintf(name,sizeof name,"%s-p13",names[scene]);save(name);
    gegl_node_set(glass,"refraction-strength",0.0,NULL);read_node(glass);
    memcpy(reference,pixels,sizeof pixels);snprintf(name,sizeof name,"%s-off",names[scene]);save(name);
    gegl_node_set(glass,"refraction-strength",1.0,NULL);read_node(glass);
    snprintf(name,sizeof name,"%s-on",names[scene]);save(name);
    /* Same geometric settings, but actual fresh operation defaults for the
     * material: shows the recommended tint change separately from sampling. */
    GeglNode *start=gegl_node_new_child(root,"operation","nanotank:glass-refraction",
      "surface-height",height,"surface-extent",extent,"edge-radius",edge,
      "preserve-alpha",scene==4,NULL);
    gegl_node_link(b,start);gegl_node_connect(s,"output",start,"aux");
    memcpy(on_pixels,pixels,sizeof pixels);read_node(start);
    snprintf(name,sizeof name,"%s-start",names[scene]);save(name);
    /* Preserve the original off/on comparison below. */
    memcpy(pixels,on_pixels,sizeof pixels);
    double delta=0;
    for(int i=0;i<N;i++) delta+=fabs(pixels[i]-reference[i]);
    printf("%s mean background-refraction delta %.6f\n",names[scene],delta/N);
    if(scene==4) g_assert(delta==0); else g_assert(delta/N>1e-4);
    for(int i=3;i<N;i+=4) g_assert(fabs(pixels[i]-1)<1e-6);
  }
  /* With zero reflection/tint, ior=1 or strength=0 is identity even for
   * semi-transparent backgrounds: no second unwarped image in the output. */
  fixture(0,1);gegl_node_set(glass,"preserve-alpha",FALSE,"surface-height",100.0,
    "edge-radius",0.0,"reflection",0.0,"tint-strength",0.0,"ior",1.0,NULL);read_node(glass);
  for(int i=0;i<N;i++) g_assert(fabsf(pixels[i]-bg[i])<3e-5);
  gegl_node_set(glass,"ior",1.5,"refraction-strength",0.0,NULL);read_node(glass);
  for(int i=0;i<N;i++) g_assert(fabsf(pixels[i]-bg[i])<3e-5);
  gegl_node_set(glass,"pixel-geometry",FALSE,"refraction-strength",3.0,NULL);read_node(glass);
  for(int i=0;i<N;i++) g_assert(fabsf(pixels[i]-bg[i])<3e-5);
  /* Constant HDR light remains constant under refraction; alpha is sampled,
   * not multiplied by the alpha of a second background copy. */
  for(int i=0;i<N;i+=4) {bg[i]=2.5f;bg[i+1]=1.7f;bg[i+2]=.4f;bg[i+3]=.37f;}
  gegl_buffer_set(background,&rect,0,babl_format("RGBA float"),bg,GEGL_AUTO_ROWSTRIDE);
  gegl_node_set(glass,"pixel-geometry",TRUE,"refraction-strength",1.0,NULL);read_node(glass);
  for(int i=0;i<N;i++) g_assert(fabsf(pixels[i]-bg[i])<3e-5);
  /* Tile requests agree with full rendering, including partial alpha. */
  fixture(2,1);read_node(glass);memcpy(reference,pixels,sizeof pixels);
  for(int y=0;y<H;y+=37) for(int x=0;x<W;x+=53) {
    GeglRectangle r={x,y,MIN(53,W-x),MIN(37,H-y)};
    gegl_node_blit(glass,1,&r,babl_format("RGBA float"),pixels+(y*W+x)*4,W*4*sizeof(float),GEGL_BLIT_DEFAULT);
  }
  for(int i=0;i<N;i++) g_assert(fabsf(pixels[i]-reference[i])<3e-5);
  /* Component isolation in the actual composer, not only in its core. */
  fixture(5,0);gegl_node_set(glass,"surface-height",40.0,NULL);read_node(glass);
  memcpy(reference,pixels,sizeof pixels);
  for(int y=0;y<H;y++) for(int x=W/2;x<W;x++) shape[(y*W+x)*4+3]=0;
  gegl_buffer_set(form,&rect,0,babl_format("RGBA float"),shape,GEGL_AUTO_ROWSTRIDE);read_node(glass);
  for(int y=90;y<190;y++) for(int x=40;x<140;x++) for(int k=0;k<4;k++) {
    int i=(y*W+x)*4+k;g_assert(fabsf(pixels[i]-reference[i])<3e-5);
  }
  /* Nonzero aux origin must align to the background coordinate system. A
   * linear ramp makes the exact expected translation independent of warp. */
  fixture(0,1);
  for(int y=0;y<H;y++) for(int x=0;x<W;x++) {
    int i=(y*W+x)*4;bg[i]=.1f+.002f*x;bg[i+1]=.2f+.003f*y;bg[i+2]=.4f;bg[i+3]=.37f;
  }
  gegl_buffer_set(background,&rect,0,babl_format("RGBA float"),bg,GEGL_AUTO_ROWSTRIDE);
  gegl_node_set(glass,"surface-height",100.0,NULL);read_node(glass);memcpy(reference,pixels,sizeof pixels);
  GeglNode *shift=gegl_node_new_child(root,"operation","gegl:translate","x",7.0,"y",11.0,NULL);
  gegl_node_link(s,shift);gegl_node_connect(shift,"output",glass,"aux");read_node(glass);
  for(int y=0;y<H-11;y++) for(int x=0;x<W-7;x++) {
    int a=(y*W+x)*4,b=((y+11)*W+x+7)*4;
    g_assert(fabsf(pixels[b]-reference[a]-.014f)<1e-4);
    g_assert(fabsf(pixels[b+1]-reference[a+1]-.033f)<1e-4);
    g_assert(fabsf(pixels[b+3]-reference[a+3])<1e-6);
  }
  puts("PASS P1.4: native inputs, Snell, sign, alpha/HDR, disabled geometry, coating bypass, tiles");
  g_object_unref(root);g_object_unref(background);g_object_unref(form);g_object_unref(output);gegl_exit();return 0;
}
