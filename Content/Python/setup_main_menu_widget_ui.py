"""
Build Play/Exit UI for MainMenuWidgetBlueprint.
Run in Unreal: File > Execute Python Script, or auto via init_unreal.py.
"""
import unreal


ASSET_PATH = "/Game/TopDown/MainMenuWidgetBlueprint"
MARKER = "main_menu_widget_ui_ready.txt"


def _set_button_text(tree, button, text):
    # Prefer existing text child; else create one.
    children = []
    try:
        children = list(button.get_all_children())
    except Exception:
        pass
    text_block = None
    for child in children:
        if isinstance(child, unreal.TextBlock):
            text_block = child
            break
    if text_block is None:
        text_block = tree.construct_widget(unreal.TextBlock, button.get_name() + "_Label")
        button.set_content(text_block)
    text_block.set_text(unreal.Text(text))
    try:
        text_block.set_justification(unreal.TextJustify.CENTER)
    except Exception:
        pass
    return text_block


def setup(force=False):
    saved = unreal.Paths.project_saved_dir()
    marker_path = saved + MARKER
    if (not force) and unreal.Paths.file_exists(marker_path):
        unreal.log("MainMenu widget UI marker present; skip")
        return True

    if not unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        unreal.log_error("Missing {}".format(ASSET_PATH))
        return False

    widget_bp = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if widget_bp is None:
        unreal.log_error("Failed to load {}".format(ASSET_PATH))
        return False

    tree = widget_bp.widget_tree

    # Always rebuild root for a clean menu layout.
    canvas = tree.construct_widget(unreal.CanvasPanel, "RootCanvas")
    tree.root_widget = canvas

    # Fullscreen dark backdrop
    backdrop = tree.construct_widget(unreal.Border, "Backdrop")
    backdrop.set_brush_color(unreal.LinearColor(0.02, 0.04, 0.08, 0.92))
    slot = canvas.add_child_to_canvas(backdrop)
    slot.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0.0, 0.0), maximum=unreal.Vector2D(1.0, 1.0)))
    slot.set_offsets(unreal.Margin(0.0, 0.0, 0.0, 0.0))
    slot.set_alignment(unreal.Vector2D(0.0, 0.0))

    # Center vertical box
    vbox = tree.construct_widget(unreal.VerticalBox, "MenuBox")
    vslot = canvas.add_child_to_canvas(vbox)
    vslot.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0.5, 0.5), maximum=unreal.Vector2D(0.5, 0.5)))
    vslot.set_alignment(unreal.Vector2D(0.5, 0.5))
    vslot.set_auto_size(True)
    vslot.set_offsets(unreal.Margin(0.0, 0.0, 0.0, 0.0))

    title = tree.construct_widget(unreal.TextBlock, "TitleText")
    title.set_text(unreal.Text("TOWER DEFENSE"))
    try:
        title.set_justification(unreal.TextJustify.CENTER)
    except Exception:
        pass
    title_slot = vbox.add_child_to_vertical_box(title)
    title_slot.set_padding(unreal.Margin(0.0, 0.0, 0.0, 36.0))
    title_slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)

    play_btn = tree.construct_widget(unreal.Button, "PlayButton")
    _set_button_text(tree, play_btn, "PLAY")
    play_slot = vbox.add_child_to_vertical_box(play_btn)
    play_slot.set_padding(unreal.Margin(0.0, 0.0, 0.0, 16.0))
    play_slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
    try:
        play_btn.set_desired_size_override(unreal.Vector2D(280.0, 64.0))
    except Exception:
        pass

    exit_btn = tree.construct_widget(unreal.Button, "ExitButton")
    _set_button_text(tree, exit_btn, "EXIT")
    exit_slot = vbox.add_child_to_vertical_box(exit_btn)
    exit_slot.set_padding(unreal.Margin(0.0, 0.0, 0.0, 0.0))
    exit_slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
    try:
        exit_btn.set_desired_size_override(unreal.Vector2D(280.0, 64.0))
    except Exception:
        pass

    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(widget_bp)
    except Exception as exc:
        unreal.log_warning("Compile warning: {}".format(exc))

    unreal.EditorAssetLibrary.save_asset(ASSET_PATH)

    try:
        with open(marker_path, "w", encoding="utf-8") as f:
            f.write("ok\n")
    except Exception as exc:
        unreal.log_warning("Could not write marker: {}".format(exc))

    unreal.log("MainMenuWidgetBlueprint UI ready (Play/Exit)")
    return True


if __name__ == "__main__":
    setup(force=True)
