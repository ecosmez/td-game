import unreal
# Find root of Towers
for name in ["RootCanvas", "CanvasPanel_0", "Overlay_0", "HorizontalContentBox", "Button01"]:
    obj = unreal.find_object(None, "/Game/TopDown/TowersWidgetBlueprint.TowersWidgetBlueprint:WidgetTree." + name)
    print(name, obj)
# try get AllWidgets via call_method
tree = unreal.load_object(None, "/Game/TopDown/TowersWidgetBlueprint.TowersWidgetBlueprint:WidgetTree")
for m in ["GetAllWidgets", "ForWidgetAndChildren", "FindWidget", "RemoveWidget", "GetRootWidget"]:
    try:
        print(m, tree.call_method(m))
    except Exception as e:
        print(m, "->", str(e)[:160])
# Try FindWidget with name
try:
    print("FindWidget Button01", tree.call_method("FindWidget", ("Button01",)))
except Exception as e:
    print("FindWidget err", e)
