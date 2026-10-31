/* SPDX-License-Identifier: MIT
 * Reproducible material/parameter comparisons, using real GEGL pixels.
 * --baseline renders the same scenes with P1 and skips P1.1-only assertions.
 */
#include <gegl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define W 384
#define H 240
#define N (W*H*4)
static float src_pixels[N], pixels[N], saved[N], composite[N];
static GeglNode *root, *glass;
static GeglBuffer *src, *dst;
static const GeglRectangle extent = {0,0,W,H};
static void render(void) {
  gegl_node_blit_buffer(glass,dst,NULL,0,GEGL_ABYSS_NONE);
  gegl_buffer_get(dst,&extent,1,babl_format("RGBA float"),pixels,GEGL_AUTO_ROWSTRIDE,GEGL_ABYSS_NONE);
  for(int i=0;i<N;i++) g_assert(isfinite(pixels[i]));
  for(int i=3;i<N;i+=4) g_assert(pixels[i]>=0 && pixels[i]<=1);
}
static void input_shape(int shape, int material) {
  for(int y=0;y<H;y++) for(int x=0;x<W;x++) {
    int i=(y*W+x)*4;
    float d=shape==1 ? 75-hypotf(x+.5f-W*.5f,y+.5f-H*.5f) :
      fminf(fminf(x-71.5f,311.5f-x),fminf(y-44.5f,194.5f-y));
    float mask=CLAMP(d,0,1);
    for(int k=0;k<3;k++) src_pixels[i+k]=material ? .24f : 0;
    src_pixels[i+3]=shape==2?0:mask;
    /* Poison hidden RGB: premultiplied sampling must not expose it. */
    if(mask==0 && material) {src_pixels[i]=30;src_pixels[i+1]=0;src_pixels[i+2]=20;}
  }
  gegl_buffer_set(src,&extent,0,babl_format("RGBA float"),src_pixels,GEGL_AUTO_ROWSTRIDE);
  gegl_node_set(glass,"preserve-alpha",material,"alpha-shape",shape!=2,NULL);
}
static void defaults(void) {
  gegl_node_set(glass,"thickness",20.0,"roundness",.65,"glass-opacity",.15,
    "tint-strength",.2,"reflection",.75,"highlights",.6,
    "refraction",2.5,"curvature",.3,"ior",1.5,"light-angle",225.0,NULL);
}
static void save(const char *dir,const char *name,int bg) {
  for(int y=0;y<H;y++) for(int x=0;x<W;x++) {
    int i=(y*W+x)*4;
    float base=bg==0?.025f:bg==1?.82f:((x/16+y/16)%2?.62f:.04f);
    for(int k=0;k<3;k++) {
      float b=base;
      if(bg==2 && (x%48<3 || y%48<3)) b=k==0?.55f:k==1?.12f:.035f;
      composite[i+k]=pixels[i+k]*pixels[i+3]+b*(1-pixels[i+3]);
    }
    composite[i+3]=1;
  }
  char path[512];snprintf(path,sizeof path,"%s/%s.png",dir,name);
  GeglBuffer *b=gegl_buffer_new(&extent,babl_format("RGBA float"));
  gegl_buffer_set(b,&extent,0,babl_format("RGBA float"),composite,GEGL_AUTO_ROWSTRIDE);
  GeglNode *s=gegl_node_new_child(root,"operation","gegl:buffer-source","buffer",b,NULL);
  GeglNode *p=gegl_node_new_child(root,"operation","gegl:png-save","path",path,NULL);
  gegl_node_link(s,p);gegl_node_process(p);g_object_unref(b);
}
static double difference(void) {
  double sum=0;for(int i=0;i<N;i++) sum+=fabs(pixels[i]-saved[i]);return sum/N;
}
int main(int argc,char **argv) {
  int baseline=argc>1 && strcmp(argv[1],"--baseline")==0;
  const char *dir=baseline?"output/glass-p11/p1":"output/glass-p11/p11";
  g_mkdir_with_parents(dir,0755);
  gegl_init(&argc,&argv);g_assert(gegl_has_operation("nanotank:glass"));
  root=gegl_node_new();src=gegl_buffer_new(&extent,babl_format("RGBA float"));
  dst=gegl_buffer_new(&extent,babl_format("RGBA float"));
  GeglNode *s=gegl_node_new_child(root,"operation","gegl:buffer-source","buffer",src,NULL);
  glass=gegl_node_new_child(root,"operation","nanotank:glass",NULL);gegl_node_link(s,glass);
  defaults();input_shape(0,TRUE);render();save(dir,"01-opaque-button",0);
  for(int i=0;i<W*H;i++) {
    g_assert(pixels[i*4+3]==src_pixels[i*4+3]);
    if(src_pixels[i*4+3]>.1f) g_assert(pixels[i*4]<1.05f);
  }
  defaults();input_shape(0,FALSE);render();
  save(dir,"02-plate-pattern",2);save(dir,"03-plate-dark",0);save(dir,"04-plate-light",1);
  defaults();input_shape(1,FALSE);render();save(dir,"05-round-glass",2);
  /* Parameter comparisons: each case starts at exactly the same defaults. */
  const char *props[]={"reflection","thickness","light-angle","glass-opacity","curvature","highlights","ior"};
  const double low[]={0,4,45,.02,0,0,1};
  const double high[]={1,30,225,.8,.75,1,2};
  const char *names[]={"reflection","thickness","light","opacity","curvature","highlights","ior"};
  for(int p=0;p<7;p++) {
    defaults();input_shape(0,p==4);gegl_node_set(glass,props[p],low[p],NULL);render();
    memcpy(saved,pixels,sizeof pixels);char name[128];snprintf(name,sizeof name,"%s-low",names[p]);save(dir,name,2);
    gegl_node_set(glass,props[p],high[p],NULL);render();
    snprintf(name,sizeof name,"%s-high",names[p]);save(dir,name,2);
    double delta=difference();printf("%s mean RGBA delta %.7f\n",names[p],delta);
    if(!baseline) g_assert(delta>1e-5);
  }
  /* Exact flat-centre measurements, matching the user's 0.15 density case. */
  defaults();input_shape(2,FALSE);
  gegl_node_set(glass,"curvature",0.0,"reflection",1.0,"highlights",1.0,NULL);render();
  int c=(H/2*W+W/2)*4;float alpha=pixels[c+3];
  printf("flat centre: RGB %.6f %.6f %.6f alpha %.6f; contrast retained %.6f\n",
         pixels[c],pixels[c+1],pixels[c+2],alpha,1-alpha);
  if(!baseline) g_assert(alpha<.03f);
  float premul=pixels[c]*alpha;
  if(!baseline) {
    for(int k=0;k<3;k++) {
      float black=pixels[c+k]*alpha;
      float white=black+1-alpha;
      g_assert(white-black>.97f);
      g_assert(black<.012f);
    }
  }
  if(!baseline) g_assert(premul<.01f);
  /* Neutral, unlit glass is exactly invisible; density never adds white. */
  gegl_node_set(glass,"reflection",0.0,"tint-strength",0.0,NULL);render();
  if(!baseline) for(int i=3;i<N;i+=4) g_assert(pixels[i]==0);
  gegl_node_set(glass,"reflection",1.0,"ior",1.0,NULL);render();
  if(!baseline) for(int i=3;i<N;i+=4) g_assert(pixels[i]==0);
  /* Both compositing extremes, HDR preservation, hidden RGB, feather alpha. */
  defaults();input_shape(0,TRUE);
  for(int i=0;i<W*H;i++) {
    src_pixels[i*4+3]*=.37f;
    if(src_pixels[i*4+3]>0) src_pixels[i*4]=1.8f;
  }
  gegl_buffer_set(src,&extent,0,babl_format("RGBA float"),src_pixels,GEGL_AUTO_ROWSTRIDE);
  gegl_node_set(glass,"reflection",0.0,"tint-strength",0.0,"refraction",0.0,NULL);render();
  for(int i=0;i<W*H;i++) {
    g_assert(pixels[i*4+3]==src_pixels[i*4+3]);
    if(!baseline && src_pixels[i*4+3]>0) g_assert(fabs(pixels[i*4]-1.8f)<1e-6f);
  }
  /* Material structure survives: without optical contributions this is an
   * identity even for HDR and partial alpha, not a white replacement film. */
  for(int i=0;i<W*H;i++) {
    if(src_pixels[i*4+3]>0) {
      src_pixels[i*4]=(i%2)?.1f:.5f;
      src_pixels[i*4+1]=(i%2)?.2f:.7f;
    }
  }
  gegl_buffer_set(src,&extent,0,babl_format("RGBA float"),src_pixels,GEGL_AUTO_ROWSTRIDE);
  render();
  if(!baseline) for(int i=0;i<N;i++) if(src_pixels[(i/4)*4+3]>0)
    g_assert(fabsf(pixels[i]-src_pixels[i])<1e-6f);
  /* Every pixel outside the geometry remains transparent, including feathers. */
  defaults();input_shape(1,FALSE);render();
  for(int i=0;i<W*H;i++) if(src_pixels[i*4+3]==0) g_assert(pixels[i*4+3]==0);
  memcpy(saved,pixels,sizeof pixels);
  for(int y=0;y<H;y+=31) for(int x=0;x<W;x+=47) {
    GeglRectangle r={x,y,MIN(47,W-x),MIN(31,H-y)};
    gegl_node_blit(glass,1,&r,babl_format("RGBA float"),pixels+(y*W+x)*4,W*4*sizeof(float),GEGL_BLIT_DEFAULT);
  }
  for(int i=0;i<N;i++) g_assert(fabsf(pixels[i]-saved[i])<2e-5f);
  printf("PASS %s: real GEGL material render, alpha, parameters, tile consistency\n",baseline?"P1 comparison":"P1.1");
  g_object_unref(root);g_object_unref(src);g_object_unref(dst);gegl_exit();return 0;
}
