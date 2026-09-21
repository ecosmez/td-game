"""
Ensure WBP_WavePowerUp subclasses WavePowerUpWidget (between-wave 3-card overlay).

C++ fills the three cards at runtime. This only creates the WBP parent so
CreateWidget can load /Game/TD/UI/WBP_WavePowerUp.
"""
import unreal

DST = "/Game/TD/UI/WBP_WavePowerUp"
DST_DIR = "/Game/TD/UI"
CPP_PARENT = "/Script/TD.WavePowerUpWidget"


def _wave_powerup_class():
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
        "WBP_WavePowerUp", DST_DIR, unreal.WidgetBlueprint, factory
    )
    return bp


def setup(force=False):
    parent_cls = _wave_powerup_class()
    if parent_cls is None:
        unreal.log_warning(
            "WavePowerUpWidget C++ class not loaded yet. "
            "Compile the TD module, restart the editor, then re-run setup_wave_powerup_ui."
        )
        return False

    _close_editors()

    if unreal.EditorAssetLibrary.does_asset_exist(DST) and not force:
        bp = unreal.EditorAssetLibrary.load_asset(DST)
        try:
            unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        except Exception as exc:
            unreal.log_warning("compile existing WBP_WavePowerUp: {}".format(exc))
        unreal.EditorAssetLibrary.save_asset(DST)
        unreal.log("WBP_WavePowerUp already exists — leave in place (no recreate)")
        return True

    bp = _create_fresh(parent_cls)
    if bp is None:
        unreal.log_error("Failed to create WBP_WavePowerUp")
        return False
    unreal.log("Created fresh WBP_WavePowerUp with WavePowerUpWidget parent")

    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    except Exception as exc:
        unreal.log_warning("compile: {}".format(exc))

    unreal.EditorAssetLibrary.save_asset(DST)
    unreal.log("WBP_WavePowerUp ready (between-wave power-up cards)")
    return True


if __name__ == "__main__":
    setup(force=False)
