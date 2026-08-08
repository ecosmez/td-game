"""
Ensure WBP_TowerStore subclasses TowerStoreWidget (C++).

C++ parent builds:
  - Always-visible PLUS button (bottom-left) that toggles the store panel
  - Per-tower cards with translucent mesh projection + stats
  - Resource readout + affordability
  - On card click: BP_BuildManager Select*Tower

Run from Unreal: File > Execute Python Script, or auto via init_unreal.py after module compile.
"""
import unreal

DST = "/Game/TD/UI/WBP_TowerStore"
DST_DIR = "/Game/TD/UI"
CPP_PARENT = "/Script/TD.TowerStoreWidget"


def _tower_store_class():
    cls = unreal.load_class(None, CPP_PARENT)
    if cls is None:
        cls = unreal.load_object(None, CPP_PARENT)
    return cls


def _close_editors():
    try:
        aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
        if unreal.EditorAssetLibrary.does_asset_exist(DST):
            aes.close_all_editors_for_asset(unreal.EditorAssetLibrary.load_asset(DST))
    except Exception as exc:
        unreal.log_warning("close editors: {}".format(exc))


def _create_fresh(parent_cls):
    if not unreal.EditorAssetLibrary.does_directory_exist(DST_DIR):
        unreal.EditorAssetLibrary.make_directory(DST_DIR)

    if unreal.EditorAssetLibrary.does_asset_exist(DST):
        unreal.EditorAssetLibrary.delete_asset(DST)

    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent_cls)
    bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "WBP_TowerStore", DST_DIR, unreal.WidgetBlueprint, factory
    )
    return bp


def setup(force=False):
    parent_cls = _tower_store_class()
    if parent_cls is None:
        unreal.log_warning(
            "TowerStoreWidget C++ class not loaded yet. "
            "Compile the TD module, restart the editor, then re-run setup_tower_store_ui."
        )
        return False

    _close_editors()

    needs_recreate = force or (not unreal.EditorAssetLibrary.does_asset_exist(DST))
    bp = None

    if not needs_recreate:
        bp = unreal.EditorAssetLibrary.load_asset(DST)
        try:
            current_parent = bp.parent_class
        except Exception:
            current_parent = None
        # Old text-button store fights the C++ runtime UI — recreate when parent mismatches.
        if current_parent != parent_cls:
            needs_recreate = True

    if needs_recreate:
        bp = _create_fresh(parent_cls)
        if bp is None:
            unreal.log_error("Failed to create WBP_TowerStore")
            return False
        unreal.log("Created fresh WBP_TowerStore with TowerStoreWidget parent")
    else:
        unreal.log("WBP_TowerStore already subclasses TowerStoreWidget")

    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    except Exception as exc:
        unreal.log_warning("compile: {}".format(exc))

    unreal.EditorAssetLibrary.save_asset(DST)
    unreal.log("WBP_TowerStore ready (PLUS open + translucent mesh previews + stats)")
    return True


if __name__ == "__main__":
    setup(force=True)
