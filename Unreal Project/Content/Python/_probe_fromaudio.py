import unreal

# Inspect FromAudio package objects via loading known patterns / AssetRegistry
asset = unreal.EditorAssetLibrary.load_asset('/Game/TopDown/TowersWidget_FromAudio')
pkg_name = asset.get_package().get_name()
print('pkg', pkg_name)

# Use unreal.EditorAssetLibrary.find_package_assets - may not exist
# Iterate with AssetRegistry get_assets_by_package_name
ar = unreal.AssetRegistryHelpers.get_asset_registry()
assets = ar.get_assets_by_package_name(pkg_name)
print('assets_by_pkg', assets)

# Soft object paths inside - use get_dependencies from editor
deps = unreal.EditorAssetLibrary.find_package_assets('/Game/TopDown', True) if hasattr(unreal.EditorAssetLibrary, 'find_package_assets') else None
print('find_package', deps)

# Use ObjectExporter / save text
# Try Console: obj dump
try:
    unreal.SystemLibrary.execute_console_command(None, 'obj list class=CanvasPanel')
except Exception as e:
    print('console', e)

# Check Python for dumpobject
print('has dump', hasattr(unreal, 'dump_object'))

# Read generated class CDO default root via WidgetTree on generated class
gen = asset.generated_class()
print('gen', gen)
# WidgetBlueprintGeneratedClass has WidgetTreeProperty?
attrs = [x for x in dir(gen) if not x.startswith('_')]
print('gen attrs sample', [a for a in attrs if 'idget' in a.lower() or 'tree' in a.lower() or 'Root' in a])
try:
    print('gen WidgetTree', gen.get_editor_property('WidgetTree'))
except Exception as e:
    print('gen WT', e)
