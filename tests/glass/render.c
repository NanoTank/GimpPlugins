/* SPDX-License-Identifier: MIT
 * Real GEGL render checks; build/run inside the matching GIMP Flatpak SDK.
 */
#include <gegl.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#define W 384
#define H 240
static float input[W*H*4], output[W*H*4], reference[W*H*4];
static void render(GeglNode *glass, GeglBuffer *dst) {
  gegl_node_blit_buffer(glass,dst,NULL,0,GEGL_ABYSS_NONE);
  gegl_buffer_get(dst,GEGL_RECTANGLE(0,0,W,H),1,babl_format("RGBA float"),output,GEGL_AUTO_ROWSTRIDE,GEGL_ABYSS_NONE);
  for(int i=0;i<W*H*4;i++) g_assert(isfinite(output[i]));
}
int main(int argc,char **argv) {
  gegl_init(&argc,&argv);
  g_assert(gegl_has_operation("nanotank:glass"));
  GeglRectangle r={0,0,W,H};
  GeglBuffer *src=gegl_buffer_new(&r,babl_format("RGBA float"));
  GeglBuffer *dst=gegl_buffer_new(&r,babl_format("RGBA float"));
  GeglNode *root=gegl_node_new();
  GeglNode *source=gegl_node_new_child(root,"operation","gegl:buffer-source","buffer",src,NULL);
  GeglNode *glass=gegl_node_new_child(root,"operation","nanotank:glass",NULL);
  gegl_node_link(source,glass);
  for(int shape=0;shape<3;shape++) {
    for(int y=0;y<H;y++) for(int x=0;x<W;x++) {
      int i=(y*W+x)*4;
      float d=hypotf((x-W*.5f)/1.45f,y-H*.5f);
      input[i]=.08f+.2f*x/W; input[i+1]=.12f+.15f*y/H; input[i+2]=.18f;
      input[i+3]=shape==0?1:shape==1?CLAMP(88-d,0,1):0;
      if(shape==1 && input[i+3]==0) {input[i]=20;input[i+1]=0;input[i+2]=8;}
    }
    gegl_buffer_set(src,&r,0,babl_format("RGBA float"),input,GEGL_AUTO_ROWSTRIDE);
    gegl_node_set(glass,"preserve-alpha",shape!=2,"alpha-shape",shape==1,NULL);
    render(glass,dst);
    memcpy(reference,output,sizeof output);
    for(int yy=0;yy<H;yy+=37) for(int xx=0;xx<W;xx+=53) {
      GeglRectangle part={xx,yy,MIN(53,W-xx),MIN(37,H-yy)};
      gegl_node_blit(glass,1,&part,babl_format("RGBA float"),
                     output+((yy*W+xx)*4),W*4*sizeof(float),GEGL_BLIT_DEFAULT);
    }
    for(int i=0;i<W*H*4;i++) g_assert(fabsf(output[i]-reference[i])<2e-5f);
    if(shape!=2) for(int i=3;i<W*H*4;i+=4) g_assert(output[i]==input[i]);
    else for(int i=3;i<W*H*4;i+=4) g_assert(output[i]>=.32f && output[i]<=1);
    if(shape==1) for(int i=0;i<W*H*4;i+=4) if(input[i+3]>.1f) g_assert(output[i]<1.1f);
    /* Save checker-composited preview in sRGB through GEGL. */
    GeglBuffer *preview=gegl_buffer_new(&r,babl_format("RGBA float"));
    float composite[W*H*4];
    for(int y=0;y<H;y++) for(int x=0;x<W;x++) {
      int i=(y*W+x)*4; float bg=((x/24+y/24)%2)?.65f:.16f;
      for(int k=0;k<3;k++) composite[i+k]=output[i+k]*output[i+3]+bg*(1-output[i+3]);
      composite[i+3]=1;
    }
    gegl_buffer_set(preview,&r,0,babl_format("RGBA float"),composite,GEGL_AUTO_ROWSTRIDE);
    char path[128]; snprintf(path,sizeof path,"output/glass-p1-%d.png",shape);
    GeglNode *ps=gegl_node_new_child(root,"operation","gegl:buffer-source","buffer",preview,NULL);
    GeglNode *save=gegl_node_new_child(root,"operation","gegl:png-save","path",path,NULL);
    gegl_node_link(ps,save); gegl_node_process(save); g_object_unref(preview);
  }
  /* IOR=1 disables interface reflection and internal displacement. */
  gegl_node_set(glass,"preserve-alpha",TRUE,"alpha-shape",FALSE,"ior",1.0,"tint-strength",0.0,"roundness",0.0,NULL);
  for(int i=0;i<W*H;i++) {input[i*4]=.2f;input[i*4+1]=.3f;input[i*4+2]=.4f;input[i*4+3]=1;}
  gegl_buffer_set(src,&r,0,babl_format("RGBA float"),input,GEGL_AUTO_ROWSTRIDE);
  render(glass,dst); memcpy(reference,output,sizeof output);
  gegl_node_set(glass,"ior",1.5,NULL);render(glass,dst);
  g_assert(fabs(output[0]-reference[0])>1e-4);
  /* Partial-alpha shape including tiny objects and non-square offsets. */
  for(int n=1;n<=9;n+=4) {
    memset(input,0,sizeof input);
    for(int y=13;y<13+n;y++) for(int x=19;x<19+n;x++) {
      int i=(y*W+x)*4;input[i]=.3f;input[i+1]=.4f;input[i+2]=.5f;input[i+3]=.37f;
    }
    gegl_buffer_set(src,&r,0,babl_format("RGBA float"),input,GEGL_AUTO_ROWSTRIDE);
    gegl_node_set(glass,"alpha-shape",TRUE,NULL);render(glass,dst);
    for(int i=3;i<W*H*4;i+=4) g_assert(output[i]==input[i]);
  }
  puts("PASS: registration, finite pixels, opaque/partial alpha, empty glass, hidden RGB, IOR, tiny shapes");
  g_object_unref(root);g_object_unref(src);g_object_unref(dst);gegl_exit();return 0;
}
