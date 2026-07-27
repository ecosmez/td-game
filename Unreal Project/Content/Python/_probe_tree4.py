import unreal

bp = unreal.EditorAssetLibrary.load_asset('/Game/TopDown/TowersWidgetBlueprint')
tree = unreal.load_object(None, '/Game/TopDown/TowersWidgetBlueprint.TowersWidgetBlueprint:WidgetTree')
canvas = unreal.load_object(None, '/Game/TopDown/TowersWidgetBlueprint.TowersWidgetBlueprint:WidgetTree.RootCanvas')
print('canvas', canvas)

# Try every call_method name that might set root
for method in ['SetRootWidget', 'ReplaceWidget', 'RenameWidget', 'RemoveWidget', 'FindWidget', 'ForWidgetAndChildren']:
    try:
        print(method, tree.call_method(method, (canvas,)))
    except Exception as e:
        print(method, '->', str(e)[:120])

# Try Blueprint side
for method in ['SetRootWidget', 'GetRootWidget', 'GetWidgetFromName']:
    try:
        print('bp', method, bp.call_method(method, (canvas,) if 'Set' in method else ()))
    except Exception as e:
        print('bp', method, '->', str(e)[:120])

# Duplicate audio button with correct signature
try:
    ok = unreal.EditorAssetLibrary.duplicate_asset('/AudioWidgets/AudioButtonToggle/AudioButtonToggle', '/Game/TopDown/TowersWidget_FromAudio')
    print('dup2arg', ok)
except Exception as e:
    print('dup2arg err', e)

try:
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    print('tools', tools)
    # duplicate_asset(asset_name, package_path, original_object)
    src = unreal.EditorAssetLibrary.load_asset('/AudioWidgets/AudioButtonToggle/AudioButtonToggle')
    dup = tools.duplicate_asset('TowersWidget_FromAudio', '/Game/TopDown', src)
    print('tools dup', dup)
except Exception as e:
    print('tools dup err', e)
