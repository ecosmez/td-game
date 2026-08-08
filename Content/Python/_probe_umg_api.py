import unreal
names = [n for n in dir(unreal) if ("Widget" in n and ("Editor" in n or "Blueprint" in n or "Tree" in n or "Factory" in n)) or "UMG" in n]
print("names", names)
for n in ["WidgetBlueprintLibrary", "UMGEditorSubsystem", "WidgetBlueprintEditorUtils", "EditorUtilityLibrary", "AssetEditorSubsystem", "WidgetTree", "WidgetBlueprint"]:
    print(n, hasattr(unreal, n))
# Try UMGEditorSubsystem
if hasattr(unreal, "UMGEditorSubsystem"):
    sub = unreal.get_engine_subsystem(unreal.UMGEditorSubsystem) if hasattr(unreal, "get_engine_subsystem") else None
    print("sub", sub)
    if sub:
        print([x for x in dir(sub) if not x.startswith("_")])
# Try creating widget as subobject
tree = unreal.load_object(None, "/Game/TD/UI/WBP_AbilityBar.WBP_AbilityBar:WidgetTree")
try:
    canvas = unreal.new_object(unreal.CanvasPanel, tree, "RootCanvas")
    print("new_object canvas", canvas)
    tree.set_editor_property("RootWidget", canvas)
    print("set root ok", tree.get_editor_property("RootWidget"))
except Exception as e:
    print("new_object err", e)
# Inspect existing main menu children
mm_tree = unreal.load_object(None, "/Game/TopDown/MainMenuWidgetBlueprint.MainMenuWidgetBlueprint:WidgetTree")
try:
    root = mm_tree.get_editor_property("RootWidget")
    print("mm root", root, root.get_name() if root else None)
    if root and hasattr(root, "get_all_children"):
        print("children", [c.get_name() for c in root.get_all_children()])
except Exception as e:
    print("mm root err", e)
