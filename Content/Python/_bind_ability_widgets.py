import unreal
tree = "/Game/TD/UI/WBP_AbilityBar.WBP_AbilityBar:WidgetTree"
names = []
for key in ("Q", "W", "E"):
    names += ["Btn_{}".format(key), "CDBar_{}".format(key), "CDText_{}".format(key), "Key_{}".format(key), "Size_{}".format(key)]
for name in names:
    w = unreal.find_object(None, tree + "." + name)
    if not w:
        print("missing", name)
        continue
    for prop in ["bIsVariable", "IsVariable", "bIsVariable_DEPRECATED"]:
        try:
            w.set_editor_property(prop, True)
            print("set", name, prop, "ok")
            break
        except Exception as e:
            pass
    else:
        # list props that look like variable
        props = [p for p in dir(w) if "ariable" in p.lower() or "bind" in p.lower()]
        print(name, "no bIsVariable", props[:20])
        try:
            print(name, "get_editor_property dump attempt")
            # try common
            print("  bIsVariable", w.get_editor_property("bIsVariable"))
        except Exception as e:
            print("  err", e)

bp = unreal.EditorAssetLibrary.load_asset("/Game/TD/UI/WBP_AbilityBar")
unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_asset("/Game/TD/UI/WBP_AbilityBar")
print("compiled")
print("vars", unreal.BlueprintEditorLibrary.list_member_variable_names(bp))
