"""
Ensure WBP_AbilityBar subclasses AbilityBarWidget (C++ LoL QWER HUD).

IMPORTANT: Never delete+recreate an existing WBP unless force=True.
Recreate wiped Create Widget Class pins on BP_TopDownCharacter every editor start.
"""
import unreal

DST = "/Game/TD/UI/WBP_AbilityBar"
DST_DIR = "/Game/TD/UI"
CPP_PARENT = "/Script/TD.AbilityBarWidget"


def _ability_bar_class():
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
        "WBP_AbilityBar", DST_DIR, unreal.WidgetBlueprint, factory
    )
    return bp


def setup(force=False):
    parent_cls = _ability_bar_class()
    if parent_cls is None:
        unreal.log_warning(
            "AbilityBarWidget C++ class not loaded yet. "
            "Compile the TD module, restart the editor, then re-run setup_ability_bar_ui."
        )
        return False

    _close_editors()

    if unreal.EditorAssetLibrary.does_asset_exist(DST) and not force:
        bp = unreal.EditorAssetLibrary.load_asset(DST)
        # Keep existing asset. Parent mismatch is rare after first setup; do not wipe.
        try:
            unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        except Exception as exc:
            unreal.log_warning("compile existing WBP_AbilityBar: {}".format(exc))
        unreal.EditorAssetLibrary.save_asset(DST)
        unreal.log("WBP_AbilityBar already exists — leave in place (no recreate)")
        return True

    bp = _create_fresh(parent_cls)
    if bp is None:
        unreal.log_error("Failed to create WBP_AbilityBar")
        return False
    unreal.log("Created fresh WBP_AbilityBar with AbilityBarWidget parent")

    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    except Exception as exc:
        unreal.log_warning("compile: {}".format(exc))

    unreal.EditorAssetLibrary.save_asset(DST)
    unreal.log("WBP_AbilityBar ready (LoL QWER ability HUD)")

    try:
        import fix_topdown_character_createwidget

        fix_topdown_character_createwidget.setup()
    except Exception as exc:
        unreal.log_warning("fix_topdown_character_createwidget failed: {}".format(exc))

    return True


if __name__ == "__main__":
    setup(force=False)
