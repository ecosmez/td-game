"""
Make TowersWidget HitArea SelfHitTestInvisible so tower buttons receive clicks,
and empty space passes through to the world only where intended.
Store drag still works via Button hover + LMB (EventTick) and drag flags.
"""
import unreal

ASSET_PATH = "/Game/TopDown/TowersWidgetBlueprint"


def setup():
    if not unreal.EditorAssetLibrary.does_asset_exist(ASSET_PATH):
        unreal.log_error("Missing {}".format(ASSET_PATH))
        return False

    widget_bp = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if not widget_bp:
        unreal.log_error("Failed to load {}".format(ASSET_PATH))
        return False

    tree = widget_bp.widget_tree
    if not tree or not tree.root_widget:
        unreal.log_error("Widget has no root")
        return False

    changed = False

    def visit(w):
        nonlocal changed
        if not w:
            return
        name = w.get_name()
        # HitArea fullscreen border – let children hit; do not permanently eat world input.
        if name == "HitArea" or (isinstance(w, unreal.Border) and name.lower().find("hit") >= 0):
            try:
                # ESlateVisibility::SelfHitTestInvisible = 3 in some APIs; use enum
                w.set_visibility(unreal.SlateVisibility.SELF_HIT_TEST_INVISIBLE)
                changed = True
                unreal.log("Set {} Visibility = SelfHitTestInvisible".format(name))
            except Exception as exc:
                unreal.log_warning("Could not set visibility on {}: {}".format(name, exc))
        # Children
        if hasattr(w, "get_all_children"):
            for c in w.get_all_children():
                visit(c)
        # Canvas / panel common API
        if hasattr(w, "get_children_count"):
            try:
                n = w.get_children_count()
                for i in range(n):
                    visit(w.get_child_at(i))
            except Exception:
                pass

    visit(tree.root_widget)

    # Also ensure button-named widgets are Visible
    def visit_buttons(w):
        if not w:
            return
        if isinstance(w, unreal.Button):
            try:
                w.set_visibility(unreal.SlateVisibility.VISIBLE)
            except Exception:
                pass
        if hasattr(w, "get_all_children"):
            for c in w.get_all_children():
                visit_buttons(c)
        if hasattr(w, "get_children_count"):
            try:
                n = w.get_children_count()
                for i in range(n):
                    visit_buttons(w.get_child_at(i))
            except Exception:
                pass

    visit_buttons(tree.root_widget)

    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(widget_bp)
    except Exception as exc:
        unreal.log_warning("Compile warning: {}".format(exc))

    unreal.EditorAssetLibrary.save_asset(ASSET_PATH)
    unreal.log("TowersWidget HitArea pass-through fix applied (changed={})".format(changed))
    return True


if __name__ == "__main__":
    setup()
