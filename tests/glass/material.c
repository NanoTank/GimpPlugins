/* SPDX-License-Identifier: MIT
 * Reproducible material/parameter comparisons, using real GEGL pixels.
 * --baseline renders the same scenes with P1 and skips P1.1-only assertions.
 */
#include <gegl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define W 384
#define H 240
#define N (W*H*4)
static float src_pixels[N], pixels[N], saved[N], composite[N], text_pixels[N];
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
      fminf(fminf(x-91.5f,291.5f-x),fminf(y-59.5f,179.5f-y));
    if(shape==3) {
      float xx=x+.5f-W*.5f, yy=y+.5f-H*.5f;
      float angle=atan2f(yy,xx);
      float radius=67.0f+13.0f*sinf(3.0f*angle)+7.0f*cosf(5.0f*angle);
      d=radius-hypotf(xx,yy);
    }
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
    float base=bg==0?.018f:bg==1?.82f:((x/16+y/16)%2?.095f:.008f);
    for(int k=0;k<3;k++) {
      float b=base;
      if(bg==2 && (x%48<3 || y%48<3)) b=k==0?.16f:k==1?.06f:.025f;
      if(bg==3) {
        b=((x/24+y/24)%2)?.55f:.36f;
        b=b*(1-text_pixels[i+3])+.006f*text_pixels[i+3];
      }
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
/* Four equivalent views: source without glass, default filtered output,
 * stronger filtered output. Transparent originals show the background alone;
 * the opaque original shows the source material. */
static void scene(const char *dir,const char *name,int shape,int material,int bg) {
  char file[100];
  defaults();input_shape(shape,material);render();
  snprintf(file,sizeof file,"%s",name);save(dir,file,bg);
  for(int i=0;i<N;i++) pixels[i]=material?src_pixels[i]:0;
  snprintf(file,sizeof file,"%s-original",name);save(dir,file,bg);
  gegl_node_set(glass,"reflection",1.0,"highlights",.85,"curvature",.5,NULL);render();
  snprintf(file,sizeof file,"%s-strong",name);save(dir,file,bg);
}
static double difference(void) {
  double sum=0;for(int i=0;i<N;i++) sum+=fabs(pixels[i]-saved[i]);return sum/N;
}
int main(int argc,char **argv) {
  int baseline=argc>1 && strcmp(argv[1],"--baseline")==0;
  const char *gallery = getenv ("NANOTANK_GLASS_GALLERY_ROOT");
  const char *label = getenv ("NANOTANK_GLASS_BASELINE_LABEL");
  char dir[512];
  snprintf (dir, sizeof dir, "%s/%s", gallery ? gallery : "output/glass-p12",
            baseline ? (label ? label : "p11") : "p12");
  g_mkdir_with_parents(dir,0755);
  gegl_init(&argc,&argv);g_assert(gegl_has_operation("nanotank:glass"));
  root=gegl_node_new();src=gegl_buffer_new(&extent,babl_format("RGBA float"));
  dst=gegl_buffer_new(&extent,babl_format("RGBA float"));
  GeglNode *s=gegl_node_new_child(root,"operation","gegl:buffer-source","buffer",src,NULL);
  glass=gegl_node_new_child(root,"operation","nanotank:glass",NULL);gegl_node_link(s,glass);
  GeglNode *text=gegl_node_new_child(root,"operation","gegl:text",
    "string","GLAS 123\nNanoTank","font","Sans Bold","size",22.0,NULL);
  GeglNode *move=gegl_node_new_child(root,"operation","gegl:translate","x",108.0,"y",83.0,NULL);
  gegl_node_link(text,move);gegl_node_blit_buffer(move,dst,&extent,0,GEGL_ABYSS_NONE);
  gegl_buffer_get(dst,&extent,1,babl_format("RGBA float"),text_pixels,GEGL_AUTO_ROWSTRIDE,GEGL_ABYSS_NONE);
  scene(dir,"01-opaque-button",0,TRUE,0);
  scene(dir,"02-plate-pattern",0,FALSE,2);
  scene(dir,"03-plate-dark",0,FALSE,0);
  scene(dir,"04-plate-light",0,FALSE,1);
  scene(dir,"05-round-glass",1,FALSE,2);
  scene(dir,"07-text",0,FALSE,3);
  scene(dir,"08-organic",3,FALSE,2);
  input_shape(0,FALSE);
  defaults();input_shape(0,TRUE);render();save(dir,"01-opaque-button",0);
  for(int i=0;i<W*H;i++) {
    g_assert(pixels[i*4+3]==src_pixels[i*4+3]);
    if(src_pixels[i*4+3]>.1f) g_assert(pixels[i*4]<1.05f);
  }
  defaults();input_shape(0,FALSE);render();
  save(dir,"02-plate-pattern",2);save(dir,"03-plate-dark",0);save(dir,"04-plate-light",1);
  defaults();input_shape(1,FALSE);render();save(dir,"05-round-glass",2);
  /* Include the actual operation defaults, not only the user's test values. */
  GeglNode *factory = gegl_node_new_child(root,"operation","nanotank:glass",
                                         "preserve-alpha",FALSE,"alpha-shape",TRUE,NULL);
  gegl_node_link(s,factory);
  gegl_node_blit_buffer(factory,dst,NULL,0,GEGL_ABYSS_NONE);
  gegl_buffer_get(dst,&extent,1,babl_format("RGBA float"),pixels,GEGL_AUTO_ROWSTRIDE,GEGL_ABYSS_NONE);
  save(dir,"06-factory-defaults",2);
  defaults();input_shape(0,FALSE);
  for(int thickness=4;thickness<=30;thickness=thickness==4?12:thickness==12?20:thickness==20?30:31) {
    gegl_node_set(glass,"thickness",(double)thickness,NULL);render();
    char name[80];snprintf(name,sizeof name,"thickness-%02d",thickness);save(dir,name,2);
  }
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
  if(!baseline) {
    defaults();input_shape(0,FALSE);render();memcpy(saved,pixels,sizeof pixels);
    GeglRectangle large={-37,-29,W+400,H+300}, shifted={101,59,W,H};
    GeglBuffer *big=gegl_buffer_new(&large,babl_format("RGBA float"));
    gegl_buffer_set(big,&shifted,0,babl_format("RGBA float"),src_pixels,GEGL_AUTO_ROWSTRIDE);
    gegl_node_set(s,"buffer",big,NULL);
    gegl_buffer_set_extent(dst,&shifted);
    gegl_node_blit_buffer(glass,dst,&shifted,0,GEGL_ABYSS_NONE);
    gegl_buffer_get(dst,&shifted,1,babl_format("RGBA float"),pixels,GEGL_AUTO_ROWSTRIDE,GEGL_ABYSS_NONE);
    for(int i=0;i<N;i++) g_assert(fabsf(pixels[i]-saved[i])<3e-5f);
    gegl_node_set(s,"buffer",src,NULL);gegl_buffer_set_extent(dst,&extent);g_object_unref(big);
    puts("PASS: alpha object invariant under translation and transparent canvas padding");
  }
  if(!baseline) {
    /* A small remote alpha edit changes global geometry. Cached rendering
     * must agree with a freshly created operation after that edit. */
    defaults();input_shape(0,FALSE);render();
    float dot[4]={0,0,0,1};GeglRectangle patch={8,9,1,1};
    gegl_buffer_set(src,&patch,0,babl_format("RGBA float"),dot,GEGL_AUTO_ROWSTRIDE);
    render();memcpy(saved,pixels,sizeof pixels);
    GeglNode *old=glass;
    glass=gegl_node_new_child(root,"operation","nanotank:glass",NULL);
    gegl_node_link(s,glass);defaults();
    gegl_node_set(glass,"preserve-alpha",FALSE,"alpha-shape",TRUE,NULL);render();
    for(int i=0;i<N;i++) g_assert(fabsf(pixels[i]-saved[i])<3e-5f);
    glass=old;
    puts("PASS: global geometry cache invalidates after a local alpha edit");
    /* Identity in two source spaces, and a double-precision source buffer.
     * Compare after Babl's conversion to the same destination format. */
    const char *spaces[]={"sRGB","Rec2020"};
    for(int sp=0;sp<2;sp++) {
      const Babl *fmt=babl_format_with_space("RGBA double",babl_space(spaces[sp]));
      GeglBuffer *converted=gegl_buffer_new(&extent,fmt);
      for(int i=0;i<W*H;i++) {
        src_pixels[i*4]=.31f;src_pixels[i*4+1]=.27f;
        src_pixels[i*4+2]=.19f;src_pixels[i*4+3]=.37f;
      }
      gegl_buffer_set(converted,&extent,0,babl_format("RGBA float"),src_pixels,GEGL_AUTO_ROWSTRIDE);
      gegl_buffer_get(converted,&extent,1,babl_format("RGBA float"),saved,GEGL_AUTO_ROWSTRIDE,GEGL_ABYSS_NONE);
      gegl_node_set(s,"buffer",converted,NULL);defaults();
      gegl_node_set(glass,"preserve-alpha",TRUE,"alpha-shape",TRUE,
        "reflection",0.0,"tint-strength",0.0,"refraction",0.0,NULL);render();
      for(int i=0;i<N;i++) g_assert(fabsf(pixels[i]-saved[i])<3e-5f);
      gegl_node_set(s,"buffer",src,NULL);g_object_unref(converted);
    }
    puts("PASS: identity across sRGB/Rec2020 source spaces and RGBA-double input");
  }
  printf("PASS %s: real GEGL material render, alpha, parameters, tile consistency\n",baseline?"baseline comparison":"P1.2");
  g_object_unref(root);g_object_unref(src);g_object_unref(dst);gegl_exit();return 0;
}
