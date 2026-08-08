import unreal
bp = unreal.EditorAssetLibrary.load_asset('/Game/TopDown/TowersWidgetBlueprint')
print('parent', unreal.BlueprintEditorLibrary.get_blueprint_parent_class(bp))
# compile and print errors
try:
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
except Exception as e:
    print('compile', e)
# list member variables
print('vars', unreal.BlueprintEditorLibrary.list_member_variable_names(bp))
print('funcs', [f for f in unreal.BlueprintEditorLibrary.list_functions(bp)])
# try get WidgetTree subobjects by scanning package
pkg = bp.get_package()
# use EditorAssetLibrary.load_asset on known paths from earlier RootCanvas attempt
for name in ['Button01', 'ValueText', 'RootCanvas', 'HitArea', 'Overlay_0', 'Border_0', 'SizeBox_0', 'Button_0']:
    obj = unreal.find_object(None, '/Game/TopDown/TowersWidgetBlueprint.TowersWidgetBlueprint:WidgetTree.' + name)
    print('find', name, obj)
