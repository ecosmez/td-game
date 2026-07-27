"""
Build WBP_AbilityBar by cloning a widget that already has a root tree,
then replacing its children with Q/W/E cooldown slots.
"""
import unreal

SRC = "/Game/TopDown/TowersWidgetBlueprint"
DST = "/Game/TD/UI/WBP_AbilityBar"
SLOT_SIZE = 78.0


def _tree_path(asset_path, asset_name):
    return "{}.{}:WidgetTree".format(asset_path, asset_name)


def _find(tree_path, name):
    return unreal.find_object(None, tree_path + "." + name)


def _new(tree, cls, name):
    return unreal.new_object(cls, tree, name)


def _make_slot(tree, parent_hbox, key):
    size = _new(tree, unreal.SizeBox, "Size_{}".format(key))
    try:
        size.set_width_override(SLOT_SIZE)
        size.set_height_override(SLOT_SIZE)
    except Exception:
        pass

    overlay = _new(tree, unreal.Overlay, "Slot_{}".format(key))
    size.set_content(overlay)

    btn = _new(tree, unreal.Button, "Btn_{}".format(key))
    try:
        btn.set_background_color(unreal.LinearColor(0.08, 0.12, 0.18, 0.95))
    except Exception:
        pass
    oslot = overlay.add_child_to_overlay(btn)
    oslot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
    oslot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_FILL)

    cd = _new(tree, unreal.ProgressBar, "CDBar_{}".format(key))
    cd.set_percent(0.0)
    try:
        cd.set_fill_color_and_opacity(unreal.LinearColor(0.02, 0.02, 0.05, 0.75))
        cd.set_bar_fill_type(unreal.ProgressBarFillType.TOP_TO_BOTTOM)
        cd.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    except Exception:
        pass
    oslot = overlay.add_child_to_overlay(cd)
    oslot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
    oslot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_FILL)

    vbox = _new(tree, unreal.VerticalBox, "Labels_{}".format(key))
    try:
        vbox.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    except Exception:
        pass

    key_lbl = _new(tree, unreal.TextBlock, "Key_{}".format(key))
    key_lbl.set_text(unreal.Text(key))
    try:
        key_lbl.set_justification(unreal.TextJustify.CENTER)
        key_lbl.set_color_and_opacity(unreal.SlateColor(unreal.LinearColor(0.92, 0.96, 1.0, 1.0)))
    except Exception:
        pass
    vslot = vbox.add_child_to_vertical_box(key_lbl)
    vslot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
    vslot.set_padding(unreal.Margin(0.0, 10.0, 0.0, 0.0))

    cd_txt = _new(tree, unreal.TextBlock, "CDText_{}".format(key))
    cd_txt.set_text(unreal.Text(""))
    try:
        cd_txt.set_justification(unreal.TextJustify.CENTER)
        cd_txt.set_color_and_opacity(unreal.SlateColor(unreal.LinearColor(1.0, 0.85, 0.3, 1.0)))
    except Exception:
        pass
    vslot = vbox.add_child_to_vertical_box(cd_txt)
    vslot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)

    oslot = overlay.add_child_to_overlay(vbox)
    oslot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_FILL)
    oslot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_FILL)

    hslot = parent_hbox.add_child_to_horizontal_box(size)
    hslot.set_padding(unreal.Margin(8.0, 0.0, 8.0, 0.0))
    hslot.set_vertical_alignment(unreal.VerticalAlignment.V_ALIGN_CENTER)
    return size


def setup(force=True):
    # Close editors
    try:
        aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
        for p in (DST, "/Game/TD/UI/WBP_AbilityBar2", "/Game/TD/UI/WBP_AbilityBar_FromMenu"):
            if unreal.EditorAssetLibrary.does_asset_exist(p):
                aes.close_all_editors_for_asset(unreal.EditorAssetLibrary.load_asset(p))
    except Exception as e:
        unreal.log_warning("close editors: {}".format(e))

    if unreal.EditorAssetLibrary.does_asset_exist(DST):
        unreal.EditorAssetLibrary.delete_asset(DST)
    for p in ("/Game/TD/UI/WBP_AbilityBar2", "/Game/TD/UI/WBP_AbilityBar_FromMenu"):
        if unreal.EditorAssetLibrary.does_asset_exist(p):
            unreal.EditorAssetLibrary.delete_asset(p)

    dup = unreal.EditorAssetLibrary.duplicate_asset(SRC, DST)
    if not dup:
        unreal.log_error("Failed to duplicate {} -> {}".format(SRC, DST))
        return False

    tree_path = _tree_path(DST, "WBP_AbilityBar")
    tree = unreal.load_object(None, tree_path)
    hbox = _find(tree_path, "HorizontalContentBox")
    if hbox is None:
        unreal.log_error("HorizontalContentBox missing after duplicate")
        return False

    # Remove existing children (audio UI chrome)
    try:
        while hbox.get_children_count() > 0:
            child = hbox.get_child_at(0)
            hbox.remove_child_at(0)
            # leave orphan; compile will clean or keep unused
    except Exception as e:
        unreal.log_warning("clear children: {}".format(e))
        try:
            hbox.clear_children()
        except Exception as e2:
            unreal.log_warning("clear_children: {}".format(e2))

    for key in ("Q", "W", "E"):
        _make_slot(tree, hbox, key)

    # Hide leftover towers chrome widgets if present
    for name in (
        "TextBorder", "ValueText", "Button01",
        "LeftCurveVerticalBox", "RightCurveVerticalBox",
        "LeftBoxTop", "LeftBoxBottom", "RightBoxTop", "RightBoxBottom",
        "LeftCurveTop", "LeftCurveBottom", "RightCurveTop", "RightCurveBottom",
        "WidgetSwitchLeftTop", "WidgetSwitchLeftBottom",
        "WidgetSwitchRightCurveTop", "WidgetSwitchRightCurveBottom",
    ):
        w = _find(tree_path, name)
        if w is not None:
            try:
                w.set_visibility(unreal.SlateVisibility.COLLAPSED)
            except Exception:
                pass

    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(dup)
    except Exception as e:
        unreal.log_warning("compile: {}".format(e))

    unreal.EditorAssetLibrary.save_asset(DST)
    unreal.log("WBP_AbilityBar rebuilt with Q/W/E cooldown slots")
    return True


if __name__ == "__main__":
    setup(force=True)
