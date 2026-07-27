import unreal

tree = unreal.load_object(None, '/Game/TopDown/TowersWidgetBlueprint.TowersWidgetBlueprint:WidgetTree')
canvas = unreal.load_object(None, '/Game/TopDown/TowersWidgetBlueprint.TowersWidgetBlueprint:WidgetTree.RootCanvas')

# Property access notify modes
modes = [x for x in dir(unreal) if 'PropertyAccess' in x or 'NotifyMode' in x]
print('modes', modes)

# Try set with notify / bypass
for kwargs in [
    {},
]:
    pass

# Inspect set_editor_property signature via help
help_txt = tree.set_editor_property.__doc__
print('doc', help_txt)

# Try unreal.ValueOrError / EditorScripting
# Use duplicate FromAudio - clear its graphs and rename over Towers? 

# Check FromAudio hierarchy by compiling and reading tags / exporting
from_audio = unreal.EditorAssetLibrary.load_asset('/Game/TopDown/TowersWidget_FromAudio')
tree2 = unreal.load_object(None, '/Game/TopDown/TowersWidget_FromAudio.TowersWidget_FromAudio:WidgetTree')
print('from_audio tree', tree2)
# list subobjects of tree2
# Get all objects in package
pkg = from_audio.get_package()
print('pkg', pkg)
# iterate assets in package via AssetRegistry
ar = unreal.AssetRegistryHelpers.get_asset_registry()
# Use EditorAssetLibrary.find_package_assets?
try:
    assets = unreal.EditorAssetLibrary.list_assets('/Game/TopDown/TowersWidget_FromAudio', True, False)
    print('list_assets', assets)
except Exception as e:
    print('list', e)

# Find subobjects
try:
    # unreal.ObjectIterator not available; use find_object
    for name in ['RootWidget', 'CanvasPanel_0', 'Button_0', 'Overlay_0', 'Border_0', 'SizeBox_0', 'Image_0', 'TextBlock_0']:
        obj = unreal.find_object(tree2, name)
        print('find', name, obj)
except Exception as e:
    print('find err', e)

# Try bypassing protection via call_method on UObject SetProperty
try:
    print(tree.call_method('SetEditorProperty', ('RootWidget', canvas)))
except Exception as e:
    print('SetEditorProperty method', e)
