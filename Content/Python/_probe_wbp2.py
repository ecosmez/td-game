import unreal

names = [x for x in dir(unreal) if ('Widget' in x and ('Editor' in x or 'Blueprint' in x or 'UMG' in x or 'Tree' in x)) or x.startswith('UMG') or 'WidgetBlueprint' in x]
print('classes', names[:80])

for name in ['WidgetBlueprintLibrary', 'UMGEditorSubsystem', 'WidgetBlueprintEditorUtils', 'EditorUtilityLibrary', 'AssetEditorSubsystem']:
    print(name, hasattr(unreal, name))

# Try loading WidgetTree as subobject
paths = [
    '/Game/TopDown/TowersWidgetBlueprint.TowersWidgetBlueprint:WidgetTree',
    '/Game/TopDown/TowersWidgetBlueprint.WidgetTree',
    '/Game/TopDown/TowersWidgetBlueprint.TowersWidgetBlueprint.WidgetTree',
]
for p in paths:
    obj = unreal.load_object(None, p)
    print('load', p, obj)

# Generated class default object widget tree?
gen = unreal.EditorAssetLibrary.load_asset('/Game/TopDown/TowersWidgetBlueprint').generated_class()
print('gen props try')
try:
    cdo = unreal.get_default_object(gen)
    print('cdo', cdo)
    print('cdo attrs', [x for x in dir(cdo) if 'tree' in x.lower() or 'root' in x.lower() or 'widget' in x.lower()][:40])
except Exception as e:
    print('cdo err', e)

# Search subsystems
subs = [x for x in dir(unreal) if 'Subsystem' in x and ('Widget' in x or 'UMG' in x or 'Asset' in x)]
print('subs', subs)
