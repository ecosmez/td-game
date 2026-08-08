import unreal

# Can we duplicate_object a Button under WidgetTree and add to HorizontalBox?
tree = unreal.load_object(None, "/Game/TopDown/TowersWidgetBlueprint.TowersWidgetBlueprint:WidgetTree")
btn = unreal.find_object(None, "/Game/TopDown/TowersWidgetBlueprint.TowersWidgetBlueprint:WidgetTree.Button01")
hbox = unreal.find_object(None, "/Game/TopDown/TowersWidgetBlueprint.TowersWidgetBlueprint:WidgetTree.HorizontalContentBox")
print("btn", btn, "hbox", hbox)
try:
    dup = unreal.duplicate_object(btn, tree, "AbilityBtn_Q")
    print("dup", dup)
except Exception as e:
    print("dup err", e)

# Check CanvasPanel add_child methods on HorizontalBox
print("hbox add methods", [x for x in dir(hbox) if "add" in x.lower() or "child" in x.lower()])

# Try creating entirely new Widget BP via create_blueprint_asset_with_parent
try:
    path = "/Game/TD/UI/WBP_AbilityBar2"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    bp = unreal.BlueprintEditorLibrary.create_blueprint_asset_with_parent(path, unreal.UserWidget)
    print("created bp", bp)
except Exception as e:
    print("create bp err", e)
