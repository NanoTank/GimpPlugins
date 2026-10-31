# SPDX-License-Identifier: MIT — NanoTank
# Real GIMP non-destructive filters and forced visible projection, no user data.
import os,time,json
from pathlib import Path
from gi.repository import Gimp,Gegl,Gio
Gegl.init(None)
root=Path(os.environ['NANOTANK_GLASS_ROOT'])
rows=[]
for w,h,kind in [(1024,768,0),(512,512,1),(1024,768,2),(1920,1080,0),(3840,2160,0)]:
    image=Gimp.Image.new(w,h,Gimp.ImageBaseType.RGB)
    background=Gimp.Layer.new(image,'Synthetic background',w,h,Gimp.ImageType.RGBA_IMAGE,100,Gimp.LayerMode.NORMAL)
    layer=Gimp.Layer.new(image,'Synthetic alpha objects',w,h,Gimp.ImageType.RGBA_IMAGE,100,Gimp.LayerMode.NORMAL)
    image.insert_layer(background,None,0);background.fill(Gimp.FillType.WHITE)
    image.insert_layer(layer,None,0);layer.fill(Gimp.FillType.TRANSPARENT)
    if kind==1:image.select_ellipse(Gimp.ChannelOps.REPLACE,w/2-100,h/2-100,200,200)
    elif kind==2:
        image.select_rectangle(Gimp.ChannelOps.REPLACE,w/2-230,h/2-55,140,110)
        image.select_ellipse(Gimp.ChannelOps.ADD,w/2+30,h/2-90,180,180)
    else:image.select_rectangle(Gimp.ChannelOps.REPLACE,w/2-159,h/2-89,319,179)
    Gimp.context_set_foreground(Gegl.Color.new('red'));layer.edit_fill(Gimp.FillType.FOREGROUND);Gimp.Selection.none(image)
    effect=Gimp.DrawableFilter.new(layer,'nanotank:glass','NanoTank performance')
    config=effect.get_config()
    for key,value in {'alpha-shape':True,'pixel-geometry':True,'preserve-alpha':False}.items():config.set_property(key,value)
    t=time.perf_counter();layer.append_filter(effect)
    projection=Gimp.Layer.new_from_visible(image,image,'Measurement');pixel=projection.get_pixel(w//2,h//2).get_rgba();projection.delete()
    rows.append([w,h,kind,'cold',time.perf_counter()-t]);print(rows[-1],flush=True)
    assert pixel[1]>.5,'visible projection did not apply transparent material'
    for phase,key,values in [('geometry','surface-height',[-20.0]),('material','reflection',[.6]),('repeat','light-angle',[48.,49.,50.,51.])]:
        for value in values:
            t=time.perf_counter();config.set_property(key,value);effect.update()
            projection=Gimp.Layer.new_from_visible(image,image,'Measurement');projection.get_pixel(w//2,h//2);projection.delete()
            rows.append([w,h,kind,phase,time.perf_counter()-t]);print(rows[-1],flush=True)
    # Confirm reopening/reload with persisted properties; only synthetic XCF.
    if kind==1:
        f=Gio.File.new_for_path(str(root/'output/glass-p142/performance.xcf'))
        assert Gimp.file_save(Gimp.RunMode.NONINTERACTIVE,image,f)
        loaded=Gimp.file_load(Gimp.RunMode.NONINTERACTIVE,f)
        filters=loaded.get_layers()[0].get_filters();assert len(filters)==1
        assert filters[0].get_config().get_property('surface-height')==-20
        p=Gimp.Layer.new_from_visible(loaded,loaded,'Reload measurement');assert p.get_pixel(w//2,h//2);p.delete();loaded.delete()
        print('NORMAL_FILTER_XCF_RELOAD_OK',flush=True)
    image.delete()
(root/('output/glass-p142/gimp-'+os.environ['NANOTANK_PERF_TAG']+'.json')).write_text(json.dumps(rows,indent=2)+'\n')
print('GIMP_PERFORMANCE_OK',flush=True)
