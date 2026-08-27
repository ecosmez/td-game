import unreal


BLUEPRINT_PATH = "/Game/TD/BP_AbilityAimPreview"
EXPECTED_DECALS = {
    "RangeTerrainDecal": "/Game/TD/Materials/M_AbilityRangeTerrainDecal",
    "AimTerrainDecal": "/Game/TD/Materials/M_AbilityPlaceTerrainDecal",
}


def _component_base_name(component):
    return component.get_name().removesuffix("_GEN_VARIABLE")


def verify():
    blueprint = unreal.EditorAssetLibrary.load_asset(BLUEPRINT_PATH)
    assert blueprint, f"Missing {BLUEPRINT_PATH}"

    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    library = unreal.SubobjectDataBlueprintFunctionLibrary
    components = []
    component_handles = {}
    for handle in subsystem.k2_gather_subobject_data_for_blueprint(blueprint):
        component = library.get_object(library.get_data(handle))
        if isinstance(component, unreal.ActorComponent):
            components.append(component)
            component_handles[_component_base_name(component)] = handle

    decals = {
        _component_base_name(component): component
        for component in components
        if isinstance(component, unreal.DecalComponent)
    }
    for component_name, material_path in EXPECTED_DECALS.items():
        assert component_name in decals, (
            f"{component_name} must be a DecalComponent so the indicator conforms to terrain"
        )
        decal = decals[component_name]
        material = decal.get_editor_property("decal_material")
        assert material, f"{component_name} has no decal material"
        assert material.get_path_name().split(".")[0] == material_path, (
            f"{component_name} uses {material.get_path_name()}, expected {material_path}"
        )
        assert decal.get_editor_property("decal_size").x >= 500.0, (
            f"{component_name} projection depth is too shallow for uneven terrain"
        )

    expected_parents = {
        "RangeTerrainDecal": "RangeRing",
        "AimTerrainDecal": "AimMarker",
    }
    for component_name, expected_parent_name in expected_parents.items():
        data = library.get_data(component_handles[component_name])
        parent_handle = library.get_parent_handle(data)
        parent = library.get_object(library.get_data(parent_handle))
        actual_parent_name = _component_base_name(parent) if parent else "None"
        assert actual_parent_name == expected_parent_name, (
            f"{component_name} must inherit transforms from {expected_parent_name}; "
            f"actual parent is {actual_parent_name}"
        )

    legacy_meshes = {
        _component_base_name(component): component
        for component in components
        if isinstance(component, unreal.StaticMeshComponent)
    }
    for component_name in ("RangeRing", "AimMarker"):
        assert component_name in legacy_meshes, f"Missing legacy driver {component_name}"
        assert not legacy_meshes[component_name].get_editor_property("hidden_in_game"), (
            f"{component_name} must not hide its attached decal branch"
        )
        assert legacy_meshes[component_name].get_editor_property("static_mesh") is None, (
            f"{component_name} must retain transforms but render no legacy mesh geometry"
        )

    unreal.log("ABILITY_TERRAIN_PROJECTION_TEST_PASS")
    return True


if __name__ == "__main__":
    verify()
