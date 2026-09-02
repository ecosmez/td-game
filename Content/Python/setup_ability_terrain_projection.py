"""Make ability range and target indicators project onto uneven ground."""

import unreal


BLUEPRINT_PATH = "/Game/TD/BP_AbilityAimPreview"
MATERIAL_FOLDER = "/Game/TD/Materials"


def _load_or_create_material(name):
    path = f"{MATERIAL_FOLDER}/{name}"
    material = unreal.EditorAssetLibrary.load_asset(path)
    if material:
        return material
    return unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        name, MATERIAL_FOLDER, unreal.Material, unreal.MaterialFactoryNew()
    )


def _clear_material(material):
    try:
        for expression in list(material.get_editor_property("expressions") or []):
            unreal.MaterialEditingLibrary.delete_material_expression(material, expression)
    except Exception:
        pass


def _make_decal_material(name, color, ring):
    material = _load_or_create_material(name)
    material.set_editor_property("material_domain", unreal.MaterialDomain.MD_DEFERRED_DECAL)
    material.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    _clear_material(material)

    mel = unreal.MaterialEditingLibrary
    uv = mel.create_material_expression(material, unreal.MaterialExpressionTextureCoordinate, -700, 0)
    center = mel.create_material_expression(material, unreal.MaterialExpressionConstant2Vector, -700, 140)
    center.set_editor_property("r", 0.5)
    center.set_editor_property("g", 0.5)

    outer = mel.create_material_expression(material, unreal.MaterialExpressionSphereMask, -450, 0)
    outer.set_editor_property("attenuation_radius", 0.49)
    outer.set_editor_property("hardness_percent", 92.0)
    mel.connect_material_expressions(uv, "", outer, "A")
    mel.connect_material_expressions(center, "", outer, "B")

    mask = outer
    if ring:
        inner = mel.create_material_expression(material, unreal.MaterialExpressionSphereMask, -450, 180)
        inner.set_editor_property("attenuation_radius", 0.455)
        inner.set_editor_property("hardness_percent", 92.0)
        mel.connect_material_expressions(uv, "", inner, "A")
        mel.connect_material_expressions(center, "", inner, "B")
        mask = mel.create_material_expression(material, unreal.MaterialExpressionSubtract, -200, 60)
        mel.connect_material_expressions(outer, "", mask, "A")
        mel.connect_material_expressions(inner, "", mask, "B")

    tint = mel.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, -100, -140)
    tint.set_editor_property("constant", unreal.LinearColor(*color, 1.0))
    alpha = mel.create_material_expression(material, unreal.MaterialExpressionMultiply, 80, 100)
    opacity = mel.create_material_expression(material, unreal.MaterialExpressionConstant, -80, 220)
    opacity.set_editor_property("r", 0.42 if ring else 0.25)
    mel.connect_material_expressions(mask, "", alpha, "A")
    mel.connect_material_expressions(opacity, "", alpha, "B")
    mel.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(alpha, "", unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(material)
    unreal.EditorAssetLibrary.save_asset(f"{MATERIAL_FOLDER}/{name}")
    return material


def _component_name(component):
    return component.get_name().removesuffix("_GEN_VARIABLE")


def _get_component_object(handle):
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    return unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)


def _find_handle(handles, name):
    for handle in handles:
        component = _get_component_object(handle)
        if component and _component_name(component) == name:
            return handle
    return None


def _ensure_decal(subsystem, blueprint, parent_handle, name, material):
    handles = subsystem.k2_gather_subobject_data_for_blueprint(blueprint)
    handle = _find_handle(handles, name)
    if not handle:
        params = unreal.AddNewSubobjectParams(
            parent_handle=parent_handle,
            new_class=unreal.DecalComponent,
            blueprint_context=blueprint,
        )
        result = subsystem.add_new_subobject(params=params)
        handle = result[0]
        failure = result[1]
        if not _get_component_object(handle):
            raise RuntimeError(f"Could not add {name}: {failure}")
        subsystem.rename_subobject(handle=handle, new_name=unreal.Text(name))

    if not subsystem.attach_subobject(parent_handle, handle):
        raise RuntimeError(f"Could not attach {name} to the preview root")

    decal = _get_component_object(handle)
    decal.set_editor_property("decal_material", material)
    decal.set_editor_property("decal_size", unreal.Vector(10000.0, 50.0, 50.0))
    decal.set_editor_property("relative_rotation", unreal.Rotator(0.0, -90.0, 0.0))
    decal.set_editor_property("sort_order", 50)
    decal.set_editor_property("fade_screen_size", 0.0)
    return decal


def _ensure_sync_component(subsystem, blueprint, owner_handle):
    handles = subsystem.k2_gather_subobject_data_for_blueprint(blueprint)
    for handle in handles:
        component = _get_component_object(handle)
        if component and component.get_class().get_name() == "AbilityTerrainProjectionComponent":
            return component

    params = unreal.AddNewSubobjectParams(
        parent_handle=owner_handle,
        new_class=unreal.AbilityTerrainProjectionComponent,
        blueprint_context=blueprint,
    )
    handle, failure = subsystem.add_new_subobject(params=params)
    component = _get_component_object(handle)
    if not component:
        raise RuntimeError(f"Could not add terrain projection sync component: {failure}")
    subsystem.rename_subobject(handle=handle, new_name=unreal.Text("TerrainProjectionSync"))
    return component


def setup():
    range_material = _make_decal_material(
        "M_AbilityRangeTerrainDecal", (0.05, 0.35, 1.0), ring=True
    )
    aim_material = _make_decal_material(
        "M_AbilityPlaceTerrainDecal", (1.0, 0.38, 0.04), ring=False
    )

    blueprint = unreal.EditorAssetLibrary.load_asset(BLUEPRINT_PATH)
    if not blueprint:
        raise RuntimeError(f"Missing {BLUEPRINT_PATH}")
    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_blueprint(blueprint)

    range_handle = _find_handle(handles, "RangeRing")
    aim_handle = _find_handle(handles, "AimMarker")
    root_handle = _find_handle(handles, "DefaultSceneRoot")
    if not range_handle or not aim_handle or not root_handle:
        raise RuntimeError("Ability preview is missing RangeRing or AimMarker")

    for handle in (range_handle, aim_handle):
        mesh = _get_component_object(handle)
        mesh.set_editor_property("hidden_in_game", False)
        mesh.set_editor_property("static_mesh", None)
        mesh.set_editor_property("cast_shadow", False)
        mesh.set_editor_property("receives_decals", False)

    _ensure_decal(subsystem, blueprint, root_handle, "RangeTerrainDecal", range_material)
    _ensure_decal(subsystem, blueprint, root_handle, "AimTerrainDecal", aim_material)
    _ensure_sync_component(subsystem, blueprint, handles[0])

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        raise RuntimeError(f"Failed to save {BLUEPRINT_PATH}")
    unreal.log("ABILITY_TERRAIN_PROJECTION_SETUP_COMPLETE")
    return True


if __name__ == "__main__":
    setup()
