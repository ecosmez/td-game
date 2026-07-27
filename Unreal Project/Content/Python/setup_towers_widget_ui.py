"""
Build a fullscreen hit-test root for TowersWidgetBlueprint so drag/drop UI works.
Safe to run multiple times. In Unreal: File > Execute Python Script, or it auto-runs via init_unreal.py.
"""
import unreal


ASSET_PATH = "/Game/TopDown/TowersWidgetBlueprint"
MARKER = "/Game/TopDown/.towers_widget_ui_ready"


def _ensure_root(widget_bp):
    tree = widget_bp.widget_tree
    root = tree.root_widget
    if root is not None:
        unreal.log("TowersWidgetBlueprint already has root: {}".format(root.get_name()))
        return root, False

    canvas = tree.construct_widget(unreal.CanvasPanel, "RootCanvas")
    tree.root_widget = canvas

    # Fullscreen transparent border so the UserWidget receives mouse events.
    border = tree.construct_widget(unreal.Border, "HitArea")
    border.set_brush_color(unreal.LinearColor(0.0, 0.0, 0.0, 0.01))
    slot = canvas.add_child_to_canvas(border)
    slot.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0.0, 0.0), maximum=unreal.Vector2D(1.0, 1.0)))
    slot.set_offsets(unreal.Margin(0.0, 0.0, 0.0, 0.0))
    slot.set_alignment(unreal.Vector2D(0.0, 0.0))

    unreal.log("Created RootCanvas + HitArea for TowersWidgetBlueprint")
    return canvas, True


def setup(force=False):
    if (not force) and unreal.EditorAssetLibrary.does_asset_exist(MARKER):
        unreal.log("Towers widget UI marker present; skip (pass force=True to rebuild)")
        return True

    if not unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        unreal.log_error("Missing asset {}".format(ASSET_PATH))
        return False

    widget_bp = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if widget_bp is None:
        unreal.log_error("Failed to load {}".format(ASSET_PATH))
        return False

    _ensure_root(widget_bp)

    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(widget_bp)
    except Exception as exc:
        unreal.log_warning("Compile warning: {}".format(exc))

    unreal.EditorAssetLibrary.save_asset(ASSET_PATH)

    # Marker asset folder touch via empty data asset is heavy; use a tiny text marker file under Saved instead.
    try:
        saved = unreal.Paths.project_saved_dir()
        marker_path = saved + "towers_widget_ui_ready.txt"
        with open(marker_path, "w", encoding="utf-8") as f:
            f.write("ok\n")
    except Exception as exc:
        unreal.log_warning("Could not write marker: {}".format(exc))

    unreal.log("TowersWidgetBlueprint UI root ready")
    return True


if __name__ == "__main__":
    setup(force=True)
