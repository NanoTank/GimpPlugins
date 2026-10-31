/* SPDX-License-Identifier: MIT — NanoTank */
#include <gegl.h>
#include <stdio.h>
#include <string.h>
#include "../../gegl/nanotank-glass/glass-geometry.h"
#include "../../gegl/nanotank-glass/glass-geometry-cache.h"
#define W 73
#define H 61
#define N (W*H)
static void check(GObject *owner,float *mask,double height,double extent,double edge,double thickness)
{
 float a[4][N],b[4][N];
 g_assert(nt_geometry_build(mask,W,H,height,extent,edge,thickness,a[0],a[1],a[2],a[3],NULL));
 g_assert(nt_geometry_cached((GeglOperation*)owner,mask,W,H,height,extent,edge,thickness,b[0],b[1],b[2],b[3]));
 for(int k=0;k<4;k++)g_assert(memcmp(a[k],b[k],sizeof a[k])==0);
}
static gpointer concurrent(gpointer data)
{
 float mask[N]={0};
 for(int y=8;y<45;y++)for(int x=10;x<55;x++)mask[y*W+x]=1;
 for(int k=0;k<6;k++)check(data,mask,k*3-8,15+k*10,3,12+k);
 return NULL;
}
int main(void)
{
 GObject *owner=g_object_new(G_TYPE_OBJECT,NULL);float mask[N]={0};
 for(int y=8;y<45;y++)for(int x=10;x<55;x++)mask[y*W+x]=1;
 check(owner,mask,10,100,4,12);
 NtGeometryCache *cache=nt_geometry_cache((GeglOperation*)owner);
 g_assert(cache->field_builds==1 && cache->profile_builds==1);
 check(owner,mask,10,100,4,12);g_assert(cache->field_builds==1 && cache->profile_builds==1);
 check(owner,mask,-20,15,6,30);g_assert(cache->field_builds==1 && cache->profile_builds==2);
 mask[20*W+20]=.3;check(owner,mask,-20,15,6,30);g_assert(cache->field_builds==1);
 mask[20*W+20]=.0005;check(owner,mask,-20,15,6,30);g_assert(cache->field_builds==2);
 mask[20*W+20]=1;check(owner,mask,-20,15,6,30);g_assert(cache->field_builds==3);
 memset(mask,0,sizeof mask);check(owner,mask,10,100,4,12);
 mask[3*W+4]=1;mask[40*W+60]=1;check(owner,mask,-5,100,2,1);
 GThread *threads[4];for(int i=0;i<4;i++)threads[i]=g_thread_new("geometry-cache",concurrent,owner);
 for(int i=0;i<4;i++)g_thread_join(threads[i]);
 g_object_unref(owner);
 puts("PASS: cache hit counts, signed profile-only updates, alpha threshold edits/restore, empty/tiny components, concurrent instance access; bit-identical to uncached full field");
 return 0;
}
