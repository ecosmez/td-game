import unreal
import shutil
import os

# Close editors for TowersWidgetBlueprint
aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
path = '/Game/TopDown/TowersWidgetBlueprint'
print('close', aes.close_all_editors_for_asset(unreal.EditorAssetLibrary.load_asset(path)))

# Also close from audio if open
fa = '/Game/TopDown/TowersWidget_FromAudio'
if unreal.EditorAssetLibrary.does_asset_exist(fa):
    aes.close_all_editors_for_asset(unreal.EditorAssetLibrary.load_asset(fa))

# Save/unload
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

src = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir() + 'TopDown/TowersWidget_FromAudio.uasset')
dst = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir() + 'TopDown/TowersWidgetBlueprint.uasset')
print('src', src, os.path.exists(src))
print('dst', dst, os.path.exists(dst))

# Backup current TowersWidgetBlueprint
bak = dst + '.bak'
shutil.copy2(dst, bak)
print('backed up')

# Replace with FromAudio asset file then rename inside? 
# Better: delete TowersWidgetBlueprint asset properly, duplicate FromAudio onto that name
ok = unreal.EditorAssetLibrary.delete_asset(path)
print('deleted', ok)
dup = unreal.EditorAssetLibrary.duplicate_asset(fa, path)
print('dup', dup)

# Compile
if dup:
    unreal.BlueprintEditorLibrary.compile_blueprint(dup)
    unreal.EditorAssetLibrary.save_asset(path)
    print('saved new TowersWidgetBlueprint with root from AudioButton')
