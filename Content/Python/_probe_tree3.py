import unreal

# Search for anything related to setting root widget
hits = []
for name in dir(unreal):
    low = name.lower()
    if 'rootwidget' in low or 'widgettree' in low or ('setroot' in low) or ('addwidget' in low and 'blue' in low):
        hits.append(name)
print('hits', hits)

# Try BlueprintEditorLibrary
print('BlueprintEditorLibrary', hasattr(unreal, 'BlueprintEditorLibrary'))
if hasattr(unreal, 'BlueprintEditorLibrary'):
    print([x for x in dir(unreal.BlueprintEditorLibrary) if not x.startswith('_')])

# Try to use ObjectMixer or factory recreate
# Duplicate AudioButtonToggle into TD and inspect if we can replace Towers content

# Use AssetRegistry / copy RootWidget via duplicate then rename
dst = '/Game/TopDown/TowersWidget_FromAudio'
if unreal.EditorAssetLibrary.does_asset_exist(dst):
    unreal.EditorAssetLibrary.delete_asset(dst)
ok = unreal.EditorAssetLibrary.duplicate_asset('/AudioWidgets/AudioButtonToggle/AudioButtonToggle', '/Game/TopDown', 'TowersWidget_FromAudio')
print('dup', ok)

# After duplicate, load tree and see root via get_editor_property with force?
tree = unreal.load_object(None, '/Game/TopDown/TowersWidget_FromAudio.TowersWidget_FromAudio:WidgetTree')
print('dup tree', tree)
# Try accessing via __dict__ or get_class
# Use unreal.Object.get_editor_property with Generated class

# Check if Canvas we created is listed somehow - compile and see hierarchy
bp = unreal.EditorAssetLibrary.load_asset('/Game/TopDown/TowersWidgetBlueprint')
try:
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    print('compiled')
except Exception as e:
    print('compile', e)
