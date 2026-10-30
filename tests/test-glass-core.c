/* SPDX-License-Identifier: MIT */
#include "../plugins/nanotank-glass/glass-core.h"
#include <assert.h>
#include <stdio.h>

static int close_enough(float a, float b) { return fabsf(a-b)<0.00001f; }
int main(void)
{
  GlassSurface p={.opacity=.1f,.tint_strength=.2f,.reflection=.8f,
                  .ior=1.5f,.tint={.4f,.6f,.8f}};
  GlassPixel opaque={.3f,.4f,.5f,1.0f};
  GlassPixel clear={0,0,0,0};
  GlassPixel a=glass_shade(opaque,1,1,1,&p);
  GlassPixel b=glass_shade(clear,1,1,1,&p);
  GlassPixel c=glass_shade(opaque,0,1,1,&p);
  GlassPixel d=glass_shade(clear,0,1,1,&p);
  assert(close_enough(a.a,1.0f)); /* no accidental transparency */
  assert(b.a>0.0f && b.a<1.0f); /* visible translucent glass */
  assert(close_enough(c.a,opaque.a));
  assert(close_enough(d.a,clear.a));
  assert(close_enough(glass_fresnel(1,1.5f),.04f));
  assert(glass_fresnel(.1f,1.5f)>glass_fresnel(1,1.5f));
  GlassPixel red={1,0,0,.5f}, blue={0,0,1,.5f};
  GlassPixel over=glass_over(red,blue);
  assert(close_enough(over.a,.75f));
  assert(close_enough(over.r,2.0f/3));
  assert(close_enough(over.b,1.0f/3));
  p.opacity=0;p.reflection=0;
  GlassPixel identity=glass_shade(opaque,1,1,1,&p);
  assert(close_enough(identity.r,opaque.r));
  assert(close_enough(identity.a,opaque.a));
  puts("Glass core: 11 assertions passed");
  return 0;
}
