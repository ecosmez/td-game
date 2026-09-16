"""Keep BP_SlowZone walkable: visual mesh must not block or carve NavMesh.

The overlap sphere still detects enemies. The ground cylinder is display-only.
Run in Unreal Editor:
  py Content/Python/setup_slow_zone_collision.py
"""

import unreal

BLUEPRINT_PATH = "/Game/TD/BP_SlowZone"


def _component_name(component):
    return component.get_name().removesuffix("_GEN_VARIABLE")


def _get_component_object(handle):
    data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
    return unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)


def _configure_visual(mesh):
    mesh.set_collision_profile_name("NoCollision")
    mesh.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    mesh.set_collision_response_to_all_channels(unreal.CollisionResponse.ECR_IGNORE)
    mesh.set_generate_overlap_events(False)
    mesh.set_editor_property("can_ever_affect_navigation", False)
    mesh.set_editor_property("can_character_step_up_on", unreal.CanBeCharacterBase.ECB_NO)


def _configure_volume(volume):
    volume.set_collision_profile_name("OverlapAllDynamic")
    volume.set_collision_enabled(unreal.CollisionEnabled.QUERY_ONLY)
    volume.set_collision_response_to_all_channels(unreal.CollisionResponse.ECR_OVERLAP)
    volume.set_generate_overlap_events(True)
    volume.set_hidden_in_game(True)
    volume.set_editor_property("can_ever_affect_navigation", False)
    volume.set_editor_property("can_character_step_up_on", unreal.CanBeCharacterBase.ECB_NO)


def setup():
    blueprint = unreal.EditorAssetLibrary.load_asset(BLUEPRINT_PATH)
    if not blueprint:
        raise RuntimeError("Missing {}".format(BLUEPRINT_PATH))

    subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
    handles = subsystem.k2_gather_subobject_data_for_blueprint(blueprint)
    updated = []
    for handle in handles:
        component = _get_component_object(handle)
        if not component:
            continue
        name = _component_name(component)
        if name == "ZoneMesh" and isinstance(component, unreal.StaticMeshComponent):
            _configure_visual(component)
            updated.append(name)
        elif name == "SlowVolume" and isinstance(component, unreal.SphereComponent):
            _configure_volume(component)
            updated.append(name)

    if not updated:
        raise RuntimeError("BP_SlowZone is missing ZoneMesh/SlowVolume")

    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_asset(BLUEPRINT_PATH)
    unreal.log("SLOW_ZONE_COLLISION_SETUP updated={}".format(updated))
    return updated


if __name__ == "__main__":
    setup()
