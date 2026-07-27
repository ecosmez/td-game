import unreal

def list_pkg_widgets(asset_path, asset_name):
    pkg = unreal.load_package(asset_path)
    print("===", asset_path, "pkg", pkg)
    # iterate objects in package
    objs = []
    try:
        # unreal.EditorAssetLibrary.find_package_assets?
        for obj in unreal.ObjectIterator(unreal.Widget):
            if obj.get_package() and obj.get_package().get_name() == asset_path:
                objs.append(obj.get_path_name())
    except Exception as e:
        print("ObjectIterator err", e)
    print("widgets", objs[:80], "count", len(objs))
    # Also try AllWidgets via generated class CDO
    bp = unreal.EditorAssetLibrary.load_asset(asset_path)
    try:
        gen = bp.generated_class()
        cdo = unreal.get_default_object(gen)
        print("gen", gen, "cdo", cdo)
        print("cdo props sample", [p for p in dir(cdo) if "idget" in p.lower() or "Root" in p][:40])
    except Exception as e:
        print("gen err", e)

for path, name in [
    ("/Game/TopDown/MainMenuWidgetBlueprint", "MainMenuWidgetBlueprint"),
    ("/Game/TopDown/TowersWidgetBlueprint", "TowersWidgetBlueprint"),
    ("/Game/TD/UI/WBP_BuildHUD2", "WBP_BuildHUD2"),
    ("/Game/TD/UI/WBP_AbilityBar", "WBP_AbilityBar"),
]:
    list_pkg_widgets(path, name)
    print()
