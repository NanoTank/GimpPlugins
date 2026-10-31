# SPDX-License-Identifier: MIT – NanoTank
# Isolated GIMP batch integration test, not a test against user images.
import os
import runpy
from pathlib import Path
from gi.repository import Gio
workspace=Path(os.environ["NANOTANK_GLASS_ROOT"])
from gi.repository import Gimp, Gegl
helper=runpy.run_path(str(workspace / "plugins/nanotank-glass-background/nanotank-glass-background.py"),run_name='nanotank_probe')
assert Gimp.get_pdb().lookup_procedure(helper['PROCEDURE']) is not None
print('PLUGIN_REGISTRATION_OK')
img=Gimp.Image.new(256,192,Gimp.ImageBaseType.RGB)
bg=Gimp.Layer.new(img,'Background',256,192,Gimp.ImageType.RGBA_IMAGE,100,Gimp.LayerMode.NORMAL)
shape=Gimp.Layer.new(img,'Shape',256,192,Gimp.ImageType.RGBA_IMAGE,100,Gimp.LayerMode.NORMAL)
img.insert_layer(bg,None,0);img.insert_layer(shape,None,0)
bg.fill(Gimp.FillType.WHITE);shape.fill(Gimp.FillType.TRANSPARENT)
root=Gegl.Node.new();checker=root.create_child('gegl:checkerboard')
checker.set_property('x',16);checker.set_property('y',16)
checker.blit_buffer(bg.get_buffer(),Gegl.Rectangle.new(0,0,256,192),0,Gegl.AbyssPolicy.NONE)
bg.update(0,0,256,192)
img.select_ellipse(Gimp.ChannelOps.REPLACE,48,16,160,160)
shape.edit_fill(Gimp.FillType.WHITE);Gimp.Selection.none(img)
above=Gimp.Layer.new(img,'Above',32,32,Gimp.ImageType.RGBA_IMAGE,100,Gimp.LayerMode.NORMAL)
img.insert_layer(above,None,0)
Gimp.context_set_foreground(Gegl.Color.new('red'));above.edit_fill(Gimp.FillType.FOREGROUND)
img.select_rectangle(Gimp.ChannelOps.REPLACE,0,0,10,10)
selection_before=Gimp.Selection.bounds(img)
previous=Gimp.DrawableFilter.new(shape,'nanotank:glass','Existing P1.3')
previous.get_config().set_property('pixel-geometry',True)
previous.get_config().set_property('alpha-shape',True)
previous.get_config().set_property('preserve-alpha',False)
shape.append_filter(previous)
assert len(shape.get_filters())==1
settings={row[0]:row[4] for row in helper['DOUBLES']}
settings.update({'tint':Gegl.Color.new('#bcdff7'),'preserve-alpha':False,'surface-height':80.0})
result=helper['make_snapshot'](img,shape,settings,show=False)
assert shape.get_visible() and bg.get_visible() and above.get_visible()
assert previous.get_visible()
assert Gimp.Selection.bounds(img)==selection_before
assert result.get_layers()[0].get_visible()
assert tuple(result.get_selected_layers()[0].get_pixel(5,5).get_rgba()) != tuple(above.get_pixel(5,5).get_rgba())
assert not result.get_layers()[-1].get_visible()  # no duplicate background overlay
assert result.get_parasite(helper['MARKER']) is not None
changes=0
for y in range(32,160,16):
    for x in range(64,200,16):
        a=tuple(bg.get_pixel(x,y).get_rgba())
        b=tuple(result.get_selected_layers()[0].get_pixel(x,y).get_rgba())
        changes += max(abs(c-d) for c,d in zip(a,b)) > .01
assert changes>5, ('native render did not change background', changes)
print('NATIVE_PIXEL_CHANGES',changes)
print('HELPER_OK',len(result.get_layers()),result.get_selected_layers()[0].get_name())
try:
    helper['make_snapshot'](result,result.get_selected_layers()[0],settings,show=False)
    raise AssertionError('feedback was not refused')
except ValueError:
    print('FEEDBACK_REFUSED')
# Save only synthetic output, verify snapshot persistence and native pixels.
path=workspace / "output/glass-p14/gimp-snapshot.xcf"
assert Gimp.file_save(Gimp.RunMode.NONINTERACTIVE,result,Gio.File.new_for_path(str(path)))
loaded=Gimp.file_load(Gimp.RunMode.NONINTERACTIVE,Gio.File.new_for_path(str(path)))
assert loaded and loaded.get_parasite(helper['MARKER'])
assert loaded.get_width()==256 and loaded.get_height()==192
before=result.get_selected_layers()[0].get_pixel(95,80).get_rgba()
after=loaded.get_layers()[1].get_pixel(95,80).get_rgba()
assert max(abs(a-b) for a,b in zip(before,after))<1e-6
print('XCF_SNAPSHOT_RELOAD_OK')
# Exercise the registered procedure, not only its helper function.
proc=Gimp.get_pdb().lookup_procedure(helper['PROCEDURE'])
config=proc.create_config()
config.set_property('run-mode',Gimp.RunMode.NONINTERACTIVE)
config.set_property('image',img)
config.set_core_object_array('drawables',[shape])
for key,value in settings.items(): config.set_property(key,value)
before={item.get_id() for item in Gimp.get_images()}
status=proc.run(config)
assert status.index(0)==Gimp.PDBStatusType.SUCCESS
created=[item for item in Gimp.get_images() if item.get_id() not in before]
assert len(created)==1 and created[0].get_parasite(helper['MARKER'])
created[0].delete()
print('REGISTERED_PROCEDURE_OK')
loaded.delete();result.delete();img.delete()
