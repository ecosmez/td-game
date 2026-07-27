import unreal

# Inspect WidgetTree UFunctions via get_class
tree = unreal.load_object(None, "/Game/TD/UI/WBP_AbilityBar.WBP_AbilityBar:WidgetTree")
cls = tree.get_class()
print("class", cls)
# list functions from reflection
try:
    funcs = unreal.ObjectIterator or None
except Exception:
    pass

# Try EditorAssetLibrary / BlueprintEditorLibrary methods related to widgets
bel = [x for x in dir(unreal.BlueprintEditorLibrary) if not x.startswith("_")]
print("BEL", bel)
eul = [x for x in dir(unreal.EditorUtilityLibrary) if ("idget" in x.lower() or "lueprint" in x.lower()) and not x.startswith("_")]
print("EUL", eul)

# Try UserWidgetBlueprint
print("UserWidgetBlueprint", hasattr(unreal, "UserWidgetBlueprint"))

# Duplicate MainMenu which already has a designed tree, then inspect children via find_object
src = "/Game/TopDown/MainMenuWidgetBlueprint"
dst = "/Game/TD/UI/WBP_AbilityBar_FromMenu"
if unreal.EditorAssetLibrary.does_asset_exist(dst):
    unreal.EditorAssetLibrary.delete_asset(dst)
dup = unreal.EditorAssetLibrary.duplicate_asset(src, dst)
print("dup", dup)
mm_tree_path = dst + ".WBP_AbilityBar_FromMenu:WidgetTree"
# After duplicate the asset name is WBP_AbilityBar_FromMenu
tree2 = unreal.load_object(None, "/Game/TD/UI/WBP_AbilityBar_FromMenu.WBP_AbilityBar_FromMenu:WidgetTree")
print("tree2", tree2)
# Find known widgets
for name in ["RootCanvas", "Backdrop", "MenuBox", "PlayButton", "ExitButton", "TitleText", "PlayButton_Label", "ExitButton_Label"]:
    obj = unreal.find_object(None, "/Game/TD/UI/WBP_AbilityBar_FromMenu.WBP_AbilityBar_FromMenu:WidgetTree." + name)
    print(name, obj)

# Can we new_object children under existing canvas if we find it?
canvas = unreal.find_object(None, "/Game/TD/UI/WBP_AbilityBar_FromMenu.WBP_AbilityBar_FromMenu:WidgetTree.RootCanvas")
print("canvas", canvas)
if canvas:
    print("canvas methods", [x for x in dir(canvas) if "add" in x.lower() or "child" in x.lower() or "slot" in x.lower()])
    try:
        border = unreal.new_object(unreal.Border, tree2, "TestBorder")
        print("border", border)
        slot = canvas.add_child_to_canvas(border)
        print("slot", slot)
    except Exception as e:
        print("add child err", e)
