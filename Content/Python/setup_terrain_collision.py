"""Configure accurate collision for the playable terrain static meshes."""

import unreal


TERRAIN_MESHES = (
    "/Game/TD/Assets/Terrain/TDProject_Sculpted_A",
    "/Game/TD/Assets/Terrain/TDProject_Greybox_A",
    "/Game/TD/Assets/Terrain/TDProject_Greybox_MainGround_A1",
)


def setup():
    updated = []
    for asset_path in TERRAIN_MESHES:
        mesh = unreal.EditorAssetLibrary.load_asset(asset_path)
        if not isinstance(mesh, unreal.StaticMesh):
            raise RuntimeError("Terrain static mesh not found: {}".format(asset_path))

        body_setup = mesh.get_editor_property("body_setup")
        if not body_setup:
            raise RuntimeError("Terrain mesh has no body setup: {}".format(asset_path))

        desired = unreal.CollisionTraceFlag.CTF_USE_COMPLEX_AS_SIMPLE
        current = body_setup.get_editor_property("collision_trace_flag")
        if current != desired:
            mesh.modify()
            body_setup.set_editor_property("collision_trace_flag", desired)
            if not unreal.EditorAssetLibrary.save_loaded_asset(mesh, only_if_is_dirty=False):
                raise RuntimeError("Failed to save terrain collision: {}".format(asset_path))
            updated.append(asset_path)

        unreal.log(
            "TERRAIN_COLLISION_SETUP asset={} trace_flag={}".format(
                mesh.get_path_name(),
                body_setup.get_editor_property("collision_trace_flag"),
            )
        )

    unreal.log("TERRAIN_COLLISION_SETUP updated={}".format(len(updated)))
    return updated


setup()
