import unreal
tree = unreal.load_object(None, "/Game/TD/UI/WBP_AbilityBar.WBP_AbilityBar:WidgetTree")
print("tree", tree)
methods = [x for x in dir(tree) if not x.startswith("_")]
print("methods", methods)
# try ConstructWidget variants
for args in [
    ("ConstructWidget", (unreal.CanvasPanel.static_class(), "RootCanvas")),
]:
    try:
        print("call", args[0], tree.call_method(args[0], args[1]))
    except Exception as e:
        print("fail", e)
# check main menu tree which supposedly worked
mm = unreal.EditorAssetLibrary.load_asset("/Game/TopDown/MainMenuWidgetBlueprint")
try:
    print("mm widget_tree", getattr(mm, "widget_tree", None))
    print("mm WidgetTree prop", mm.get_editor_property("WidgetTree"))
except Exception as e:
    print("mm err", e)
mm_tree = unreal.load_object(None, "/Game/TopDown/MainMenuWidgetBlueprint.MainMenuWidgetBlueprint:WidgetTree")
print("mm_tree", mm_tree)
if mm_tree:
    print("mm methods has construct", hasattr(mm_tree, "construct_widget"), "ConstructWidget" in dir(mm_tree))
    print([x for x in dir(mm_tree) if "onstruct" in x.lower() or "Widget" in x or "root" in x.lower()])
