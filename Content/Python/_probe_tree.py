import unreal

tree = unreal.load_object(None, '/Game/TopDown/TowersWidgetBlueprint.TowersWidgetBlueprint:WidgetTree')
print('tree', tree)
print('dir', [x for x in dir(tree) if not x.startswith('_')])
for prop in ['RootWidget', 'root_widget', 'AllWidgets']:
    try:
        print(prop, tree.get_editor_property(prop))
    except Exception as e:
        print(prop, 'ERR', e)

# construct widget
try:
    canvas = tree.construct_widget(unreal.CanvasPanel, 'RootCanvas')
    print('constructed', canvas)
except Exception as e:
    print('construct err', e)

try:
    canvas = unreal.CanvasPanel()
    print('new canvas', canvas)
except Exception as e:
    print('new err', e)
