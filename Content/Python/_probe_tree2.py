import unreal

tree = unreal.load_object(None, '/Game/TopDown/TowersWidgetBlueprint.TowersWidgetBlueprint:WidgetTree')

# Try call_method variants
for args in [
    ('ConstructWidget', (unreal.CanvasPanel.static_class(), 'RootCanvas')),
    ('ConstructWidget', {'WidgetClass': unreal.CanvasPanel.static_class(), 'WidgetName': 'RootCanvas'}),
]:
    try:
        print('try', args[0], tree.call_method(args[0], tuple(args[1]) if isinstance(args[1], tuple) else args[1]))
    except Exception as e:
        print('fail', args, e)

# Try creating widget as subobject with outer=tree
try:
    canvas = unreal.new_object(unreal.CanvasPanel, tree, 'RootCanvas')
    print('new_object', canvas, canvas.get_outer())
    try:
        tree.set_editor_property('RootWidget', canvas)
        print('set RootWidget ok')
    except Exception as e:
        print('set RootWidget', e)
except Exception as e:
    print('new_object err', e)

# Editor asset subsystem rename / duplicate from AudioButton which has hierarchy
src = '/AudioWidgets/AudioButtonToggle/AudioButtonToggle'
print('src exists', unreal.EditorAssetLibrary.does_asset_exist(src))
