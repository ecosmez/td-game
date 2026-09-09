"""
Ensure WBP_CaptureChannel subclasses CaptureChannelWidget.

Do not delete+recreate an existing WBP unless force=True.
Recreate wiped the designer tree (BarSize / ChannelBar) on editor start.

Enemy HP, tower HP, and capture-base bars all load this widget.
Edit size on BarSize (Width/Height Override). Do not rename ChannelBar.

Run from Unreal: File > Execute Python Script, or auto via init_unreal.py.
"""
import unreal

DST = "/Game/TD/UI/WBP_CaptureChannel"
DST_DIR = "/Game/TD/UI"
CPP_PARENT = "/Script/TD.CaptureChannelWidget"


def _parent_class():
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
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "WBP_CaptureChannel", DST_DIR, unreal.WidgetBlueprint, factory
    )


def setup(force=False):
    parent_cls = _parent_class()
    if parent_cls is None:
        unreal.log_warning(
            "CaptureChannelWidget C++ class not loaded yet. "
            "Compile the TD module, restart the editor, then re-run setup_capture_channel_ui."
        )
        return False

    if unreal.EditorAssetLibrary.does_asset_exist(DST) and not force:
        unreal.log("WBP_CaptureChannel already exists — leave in place (no recreate)")
        return True

    _close_editors()
    bp = _create_fresh(parent_cls)
    if bp is None:
        unreal.log_error("Failed to create WBP_CaptureChannel")
        return False
    unreal.log("Created fresh WBP_CaptureChannel with CaptureChannelWidget parent")

    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    except Exception as exc:
        unreal.log_warning("compile: {}".format(exc))

    unreal.EditorAssetLibrary.save_asset(DST)
    unreal.log("WBP_CaptureChannel ready — add BarSize + ChannelBar in the UMG designer")
    return True


if __name__ == "__main__":
    setup(force=False)
