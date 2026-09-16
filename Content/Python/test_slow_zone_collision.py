"""Verify BP_SlowZone collision cannot block enemy movement or carve NavMesh."""

import unreal

BLUEPRINT_PATH = "/Game/TD/BP_SlowZone"


def _component_name(component):
    return component.get_name().removesuffix("_GEN_VARIABLE")


def _get_component_object(handle):
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    return unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)


def test():
    blueprint = unreal.EditorAssetLibrary.load_asset(BLUEPRINT_PATH)
    if not blueprint:
        raise RuntimeError("Missing {}".format(BLUEPRINT_PATH))

    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_blueprint(blueprint)
    found = {}
    for handle in handles:
        component = _get_component_object(handle)
        if not component:
            continue
        found[_component_name(component)] = component

    mesh = found.get("ZoneMesh")
    volume = found.get("SlowVolume")
    if not isinstance(mesh, unreal.StaticMeshComponent):
        raise RuntimeError("ZoneMesh missing")
    if not isinstance(volume, unreal.SphereComponent):
        raise RuntimeError("SlowVolume missing")

    mesh_enabled = mesh.get_collision_enabled()
    if mesh_enabled != unreal.CollisionEnabled.NO_COLLISION:
        raise RuntimeError("ZoneMesh must be NoCollision, got {}".format(mesh_enabled))
    if mesh.get_editor_property("can_ever_affect_navigation"):
        raise RuntimeError("ZoneMesh must not carve NavMesh")
    if mesh.get_generate_overlap_events():
        raise RuntimeError("ZoneMesh must not generate overlaps")

    vol_enabled = volume.get_collision_enabled()
    if vol_enabled != unreal.CollisionEnabled.QUERY_ONLY:
        raise RuntimeError("SlowVolume must be QueryOnly, got {}".format(vol_enabled))
    if volume.get_editor_property("can_ever_affect_navigation"):
        raise RuntimeError("SlowVolume must not carve NavMesh")
    if not volume.get_generate_overlap_events():
        raise RuntimeError("SlowVolume must generate overlaps")
    if volume.get_collision_response_to_channel(unreal.CollisionChannel.PAWN) != unreal.CollisionResponse.ECR_OVERLAP:
        raise RuntimeError("SlowVolume must overlap pawns")

    unreal.log("SLOW_ZONE_COLLISION_TEST_PASSED")
    return True


if __name__ == "__main__":
    test()
