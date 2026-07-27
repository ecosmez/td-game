import unreal

tree = unreal.load_object(None, '/Game/TopDown/TowersWidgetBlueprint.TowersWidgetBlueprint:WidgetTree')
# get all objects with this outer
# Use AssetRegistry / package
pkg = unreal.load_package('/Game/TopDown/TowersWidgetBlueprint')
# iterate export names via find
candidates = []
for name in ['Button01', 'ValueText', 'ScaleBox_0', 'Overlay_0', 'HorizontalBox_0', 'VerticalBox_0', 'Border_0', 'SizeBox_0', 'NamedSlot_0', 'CanvasPanel_0', 'Image_0', 'Background', 'Root']:
    obj = unreal.find_object(None, '/Game/TopDown/TowersWidgetBlueprint.TowersWidgetBlueprint:WidgetTree.' + name)
    if obj:
        vis = None
        try:
            vis = obj.get_editor_property('visibility')
        except Exception:
            pass
        candidates.append((name, obj.get_class().get_name(), str(vis), str(obj.get_outer().get_name()) if obj.get_outer() else None))
print('widgets', candidates)

# Set button back to Visible - use OnClicked for begin drag, and BuildManager Tick for mouse release drop
btn = unreal.find_object(None, '/Game/TopDown/TowersWidgetBlueprint.TowersWidgetBlueprint:WidgetTree.Button01')
if btn:
    btn.set_editor_property('visibility', unreal.SlateVisibility.VISIBLE)
    print('button visibility restored Visible')
