import unreal

tree_path = "/Game/TD/UI/WBP_AbilityBar.WBP_AbilityBar:WidgetTree"
hbox = unreal.find_object(None, tree_path + ".HorizontalContentBox")
print("hbox", hbox, "parent", hbox.get_parent() if hbox else None)
if hbox:
    parent = hbox.get_parent()
    print("parent type", parent.get_class() if parent else None, parent.get_name() if parent else None)
    slot = hbox.slot
    print("slot", slot, type(slot) if slot else None)
    if slot:
        print("slot methods", [x for x in dir(slot) if "anchor" in x.lower() or "offset" in x.lower() or "align" in x.lower() or "position" in x.lower() or "size" in x.lower()])
        try:
            # Canvas slot?
            if hasattr(slot, "set_anchors"):
                slot.set_anchors(unreal.Anchors(minimum=unreal.Vector2D(0.5, 1.0), maximum=unreal.Vector2D(0.5, 1.0)))
                slot.set_alignment(unreal.Vector2D(0.5, 1.0))
                slot.set_auto_size(True)
                slot.set_offsets(unreal.Margin(0.0, 0.0, 0.0, 36.0))
                print("canvas anchors set")
            elif hasattr(slot, "set_horizontal_alignment"):
                slot.set_horizontal_alignment(unreal.HorizontalAlignment.H_ALIGN_CENTER)
                print("hbox align set")
        except Exception as e:
            print("slot err", e)

# Ensure CD bars start empty and buttons look ready
for key in ("Q", "W", "E"):
    cd = unreal.find_object(None, tree_path + ".CDBar_{}".format(key))
    btn = unreal.find_object(None, tree_path + ".Btn_{}".format(key))
    if cd:
        cd.set_percent(0.0)
    if btn:
        try:
            btn.set_is_enabled(True)
            btn.set_background_color(unreal.LinearColor(0.1, 0.35, 0.55, 0.95))
        except Exception as e:
            print("btn", key, e)

bp = unreal.EditorAssetLibrary.load_asset("/Game/TD/UI/WBP_AbilityBar")
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_asset("/Game/TD/UI/WBP_AbilityBar")
unreal.EditorAssetLibrary.save_asset("/Game/TopDown/Blueprints/BP_TopDownCharacter")
print("saved")
