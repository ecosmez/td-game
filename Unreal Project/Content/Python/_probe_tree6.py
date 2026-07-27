import unreal

# Search set object property helpers
hits = [n for n in dir(unreal.SystemLibrary) if 'ropert' in n.lower() or 'Object' in n][:50]
print('syslib', hits)
hits2 = [n for n in dir(unreal) if 'set_object' in n.lower() or 'ObjectProperty' in n or 'EditorProperty' in n]
print('hits2', hits2[:40])

# Create brand new widget via factory and inspect for auto root
factory = unreal.WidgetBlueprintFactory()
factory.set_editor_property('ParentClass', unreal.UserWidget)
tools = unreal.AssetToolsHelpers.get_asset_tools()
path = '/Game/TD/UI'
name = 'WBP_AutoRootTest'
full = path + '/' + name
if unreal.EditorAssetLibrary.does_asset_exist(full):
    unreal.EditorAssetLibrary.delete_asset(full)
asset = tools.create_asset(name, path, unreal.WidgetBlueprint, factory)
print('created', asset)
tree = unreal.load_object(None, full.replace('/Game/', '/Game/').replace(name, name) )
# proper path
tree = unreal.load_object(None, '/Game/TD/UI/WBP_AutoRootTest.WBP_AutoRootTest:WidgetTree')
print('new tree', tree)
# list package objects via unreal.EditorAssetLibrary.find_asset_data?
# Use AssetRegistry get dependencies
# Try save and check file size vs empty

# Use Export
try:
    task = unreal.AssetExportTask()
    task.set_editor_property('object', unreal.EditorAssetLibrary.load_asset('/Game/TopDown/TowersWidgetBlueprint'))
    task.set_editor_property('filename', r'C:\Users\Eduardo Cosme\Documents\Unreal Projects\TD\Saved\TowersWidgetBlueprint.json')
    task.set_editor_property('selected', False)
    task.set_editor_property('replace_identical', True)
    task.set_editor_property('prompt', False)
    task.set_editor_property('automated', True)
    task.set_editor_property('use_file_archive', False)
    task.set_editor_property('write_empty_files', True)
    # find exporter
    exporters = [n for n in dir(unreal) if 'Exporter' in n and 'Widget' in n]
    print('exporters', exporters)
    exporters2 = [n for n in dir(unreal) if n.endswith('Exporter')][:40]
    print('exporters2', exporters2)
except Exception as e:
    print('export setup', e)
