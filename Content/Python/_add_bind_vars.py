import unreal

bp = unreal.EditorAssetLibrary.load_asset("/Game/TD/UI/WBP_AbilityBar")
# Reparent to UserWidget
try:
    unreal.BlueprintEditorLibrary.reparent_blueprint(bp, unreal.UserWidget)
    print("reparented")
except Exception as e:
    print("reparent err", e)

# Inspect add_member_variable signature via help
print([x for x in dir(unreal.BlueprintEditorLibrary) if "variable" in x.lower() or "member" in x.lower()])

# Try adding object member variables
btn_type = unreal.BlueprintEditorLibrary.get_object_reference_type(unreal.Button.static_class())
pb_type = unreal.BlueprintEditorLibrary.get_object_reference_type(unreal.ProgressBar.static_class())
tb_type = unreal.BlueprintEditorLibrary.get_object_reference_type(unreal.TextBlock.static_class())
print("btn_type", btn_type)

for key in ("Q", "W", "E"):
    for name, typ in [
        ("Btn_{}".format(key), btn_type),
        ("CDBar_{}".format(key), pb_type),
        ("CDText_{}".format(key), tb_type),
    ]:
        try:
            ok = unreal.BlueprintEditorLibrary.add_member_variable(bp, name, typ)
            print("add", name, ok)
        except Exception as e:
            print("add err", name, e)

# Try set expose / category - look for metadata APIs
print([x for x in dir(unreal.BlueprintEditorLibrary) if "meta" in x.lower() or "expose" in x.lower() or "bind" in x.lower()])

unreal.BlueprintEditorLibrary.compile_blueprint(bp)
unreal.EditorAssetLibrary.save_asset("/Game/TD/UI/WBP_AbilityBar")
print("vars", [v for v in unreal.BlueprintEditorLibrary.list_member_variable_names(bp) if not v.startswith("/Script")])
