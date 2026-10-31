/* SPDX-License-Identifier: MIT – NanoTank
 * Actual GEGL P1.3 renders and integration assertions; no synthetic normals.
 */
#include <gegl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define W 384
#define H 280
#define N (W*H*4)
static float source[N], pixels[N], reference[N], composed[N];
static GeglNode *root,*glass,*input_node;
static GeglBuffer *input,*output;
static const GeglRectangle extent={0,0,W,H};
static void shape(int type) {
  for(int y=0;y<H;y++) for(int x=0;x<W;x++) {
    int i=(y*W+x)*4;
    double xx=x-192,yy=y-140,d;
    if(type==0) d=100-hypot(xx,yy);
    else if(type==1) d=fmin(120-fabs(xx),75-fabs(yy));
    else if(type==2) d=85+12*sin(3*atan2(yy,xx))+7*cos(5*atan2(yy,xx))-hypot(xx,yy);
    else if(type==3) d=fmax(42-hypot(x-90,y-140),73-hypot(x-270,y-140));
    else if(type==4) { /* thin strokes, counters and separate glyph-like parts */
      double a=fmin(13-abs(x-125),80-abs(y-140));
      double b=fmin(55-abs(x-220),12-abs(y-85));
      double c=fmin(12-abs(x-175),60-abs(y-140));
      double e=fmin(12-abs(x-265),60-abs(y-140));
      d=fmax(fmax(a,b),fmax(c,e));
    } else d=100-hypot(xx/1.35,yy/.8);
    source[i]=.24f;source[i+1]=.24f;source[i+2]=.24f;
    source[i+3]=CLAMP(d+.5,0,1);
    if(source[i+3]==0) {source[i]=30;source[i+1]=0;source[i+2]=20;}
  }
  gegl_buffer_set(input,&extent,0,babl_format("RGBA float"),source,GEGL_AUTO_ROWSTRIDE);
}
static void render(void) {
  gegl_node_blit_buffer(glass,output,NULL,0,GEGL_ABYSS_NONE);
  gegl_buffer_get(output,&extent,1,babl_format("RGBA float"),pixels,GEGL_AUTO_ROWSTRIDE,GEGL_ABYSS_NONE);
  for(int i=0;i<N;i++) g_assert(isfinite(pixels[i]));
  for(int i=3;i<N;i+=4) {
    g_assert(pixels[i]>=0 && pixels[i]<=1);
    if(source[i]==0) g_assert(pixels[i]==0);
  }
}
static void save(const char *name) {
  for(int y=0;y<H;y++) for(int x=0;x<W;x++) {
    int i=(y*W+x)*4;float bg=((x/20+y/20)%2)?.14f:.018f;
    for(int k=0;k<3;k++) composed[i+k]=pixels[i+k]*pixels[i+3]+bg*(1-pixels[i+3]);
    composed[i+3]=1;
  }
  char path[300];snprintf(path,sizeof path,"output/glass-p13/%s.png",name);
  GeglBuffer *b=gegl_buffer_new(&extent,babl_format("RGBA float"));
  gegl_buffer_set(b,&extent,0,babl_format("RGBA float"),composed,GEGL_AUTO_ROWSTRIDE);
  GeglNode *s=gegl_node_new_child(root,"operation","gegl:buffer-source","buffer",b,NULL);
  GeglNode *sink=gegl_node_new_child(root,"operation","gegl:png-save","path",path,NULL);
  gegl_node_link(s,sink);gegl_node_process(sink);g_object_unref(b);
}
static GeglNode *find_glass(GeglNode *graph) {
  GSList *children=gegl_node_get_children(graph);GeglNode *found=NULL;
  for(GSList *i=children;i;i=i->next)
    if(!g_strcmp0(gegl_node_get_operation(i->data),"nanotank:glass")) found=i->data;
  g_slist_free(children);return found;
}
int main(int argc,char **argv) {
  int baseline=argc>1 && !strcmp(argv[1],"--baseline");
  gegl_init(&argc,&argv);g_assert(gegl_has_operation("nanotank:glass"));
  g_mkdir_with_parents("output/glass-p13",0755);
  root=gegl_node_new();input=gegl_buffer_new(&extent,babl_format("RGBA float"));
  output=gegl_buffer_new(&extent,babl_format("RGBA float"));
  input_node=gegl_node_new_child(root,"operation","gegl:buffer-source","buffer",input,NULL);
  glass=gegl_node_new_child(root,"operation","nanotank:glass","preserve-alpha",FALSE,
    "alpha-shape",TRUE,"glass-opacity",.15,"thickness",12.0,NULL);
  gegl_node_link(input_node,glass);shape(1);render();
  FILE *f=fopen(baseline?"output/glass-p13/baseline.bin":"output/glass-p13/legacy.bin","wb");
  g_assert(f);g_assert(fwrite(pixels,sizeof pixels,1,f)==1);fclose(f);
  save(baseline?"p12-reference":"legacy-reference");
  if(baseline) {
    GeglNode *old=gegl_node_new();
    gegl_node_set(old,"operation","nanotank:glass","preserve-alpha",FALSE,
      "alpha-shape",TRUE,"glass-opacity",.15,"thickness",12.0,NULL);
    char *xml=gegl_node_to_xml(old,"");g_assert(xml);
    g_assert(g_file_set_contents("output/glass-p13/p12-effect.xml",xml,-1,NULL));
    g_free(xml);g_object_unref(old);
    g_object_unref(root);g_object_unref(input);g_object_unref(output);gegl_exit();return 0;
  }
  if(g_file_test("output/glass-p13/p12-effect.xml",G_FILE_TEST_IS_REGULAR)) {
    char *xml=NULL;g_assert(g_file_get_contents("output/glass-p13/p12-effect.xml",&xml,NULL,NULL));
    GeglNode *old_graph=gegl_node_new_from_xml(xml,"");GeglNode *old_effect=find_glass(old_graph);
    g_assert(old_effect);gboolean enabled=TRUE;
    gegl_node_get(old_effect,"pixel-geometry",&enabled,NULL);g_assert(!enabled);
    memcpy(reference,pixels,sizeof pixels);GeglNode *current=glass;
    gegl_node_link(input_node,old_effect);glass=old_effect;render();
    double delta=0;
    for(int i=0;i<N;i++) delta=fmax(delta,fabs(pixels[i]-reference[i]));
    printf("legacy XML max float delta %.7f (GEGL colour serialization)\n",delta);
    g_assert(delta<.001);
    glass=current;g_object_unref(old_graph);g_free(xml);
    puts("PASS: P1.2 GEGL XML without new properties keeps legacy behavior");
  }
  gegl_node_set(glass,"pixel-geometry",TRUE,"edge-radius",0.0,"surface-extent",100.0,NULL);
  const double heights[]={0,10,50,100,150,-5,-20};
  for(int h=0;h<7;h++) {
    shape(0);gegl_node_set(glass,"surface-height",heights[h],NULL);render();
    char name[80];snprintf(name,sizeof name,"circle-%+04d",(int)heights[h]);save(name);
    if(h==0) {int i=(140*W+192)*4;g_assert(pixels[i+3]<.04f);}
  }
  shape(1);gegl_node_set(glass,"surface-height",10.0,"surface-extent",15.0,"edge-radius",4.0,NULL);
  render();save("rectangle-15");
  gegl_node_set(glass,"surface-extent",100.0,NULL);render();save("rectangle-100");
  gegl_node_set(glass,"surface-height",-10.0,"surface-extent",100.0,"edge-radius",6.0,NULL);
  render();save("concave-key");
  for(int type=2;type<=5;type++) {
    shape(type);gegl_node_set(glass,"surface-height",20.0,"surface-extent",100.0,"edge-radius",2.0,NULL);
    render();save(type==2?"organic":type==3?"separate":type==4?"strokes":"ellipse");
  }
  shape(1);gegl_node_set(glass,"preserve-alpha",TRUE,"surface-height",35.0,"edge-radius",4.0,NULL);
  render();save("coating");
  for(int i=3;i<N;i+=4) g_assert(pixels[i]==source[i]);
  /* Deterministic results on arbitrary requested GEGL output regions. */
  memcpy(reference,pixels,sizeof pixels);
  for(int y=0;y<H;y+=37) for(int x=0;x<W;x+=53) {
    GeglRectangle r={x,y,MIN(53,W-x),MIN(37,H-y)};
    gegl_node_blit(glass,1,&r,babl_format("RGBA float"),pixels+(y*W+x)*4,W*4*sizeof(float),GEGL_BLIT_DEFAULT);
  }
  for(int i=0;i<N;i++) g_assert(fabsf(pixels[i]-reference[i])<2e-5f);
  /* Translation in the GEGL graph must not change the surface. */
  shape(3);gegl_node_set(glass,"preserve-alpha",FALSE,NULL);render();
  memcpy(reference,pixels,sizeof pixels);
  GeglNode *move=gegl_node_new_child(root,"operation","gegl:translate","x",47.0,"y",-23.0,NULL);
  gegl_node_link(input_node,move);gegl_node_link(move,glass);
  GeglRectangle shifted={47,-23,W,H};gegl_buffer_set_extent(output,&shifted);
  gegl_node_blit_buffer(glass,output,&shifted,0,GEGL_ABYSS_NONE);
  gegl_buffer_get(output,&shifted,1,babl_format("RGBA float"),pixels,GEGL_AUTO_ROWSTRIDE,GEGL_ABYSS_NONE);
  for(int i=0;i<N;i++) g_assert(fabsf(pixels[i]-reference[i])<3e-5f);
  gegl_node_link(input_node,glass);gegl_buffer_set_extent(output,&extent);
  shape(1);gegl_node_set(glass,"preserve-alpha",TRUE,NULL);
  /* Every sign uses the same persisted properties after GEGL XML roundtrip. */
  gegl_node_set(glass,"surface-height",-20.0,"surface-extent",15.0,NULL);render();
  memcpy(reference,pixels,sizeof pixels);
  GeglNode *single=gegl_node_new();
  gegl_node_set(single,"operation","nanotank:glass","pixel-geometry",TRUE,
    "surface-height",-20.0,"surface-extent",15.0,"edge-radius",4.0,
    "preserve-alpha",TRUE,"alpha-shape",TRUE,NULL);
  char *xml=gegl_node_to_xml(single,"");g_assert(xml);
  GeglNode *loaded=gegl_node_new_from_xml(xml,"");g_assert(loaded);
  GeglNode *restored=find_glass(loaded);
  g_assert(restored);
  gboolean enabled=FALSE;double h=0,p=0;
  gegl_node_get(restored,"pixel-geometry",&enabled,"surface-height",&h,"surface-extent",&p,NULL);
  g_assert(enabled && h==-20.0 && p==15.0);
  gegl_node_link(input_node,restored);glass=restored;render();
  double xml_delta=0;
  for(int i=0;i<N;i++) xml_delta=fmax(xml_delta,fabs(pixels[i]-reference[i]));
  printf("pixel-profile XML max float delta %.7f\n",xml_delta);
  g_assert(xml_delta<.001);
  g_free(xml);g_object_unref(single);g_object_unref(loaded);
  /* Partial alpha and HDR remain independent of the new geometry. */
  glass=gegl_node_new_child(root,"operation","nanotank:glass","pixel-geometry",TRUE,
    "preserve-alpha",TRUE,"alpha-shape",TRUE,"surface-height",-20.0,
    "surface-extent",15.0,"edge-radius",4.0,"reflection",0.0,
    "tint-strength",0.0,"refraction",0.0,NULL);
  gegl_node_link(input_node,glass);
  for(int i=0;i<N;i+=4) {source[i]=1.8f;source[i+3]*=.37f;}
  gegl_buffer_set(input,&extent,0,babl_format("RGBA float"),source,GEGL_AUTO_ROWSTRIDE);render();
  for(int i=0;i<N;i+=4) {
    g_assert(pixels[i+3]==source[i+3]);
    if(source[i+3]>0) g_assert(fabsf(pixels[i]-source[i])<1e-6f);
  }
  puts("PASS P1.3 GEGL: signed profiles, shapes, alpha, finite pixels, requested tiles, XML roundtrip");
  g_object_unref(root);g_object_unref(input);g_object_unref(output);gegl_exit();return 0;
}
