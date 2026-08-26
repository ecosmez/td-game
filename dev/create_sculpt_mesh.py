import unreal


SOURCE_ASSET = "/Game/TD/Assets/Terrain/TDProject_Greybox_A"
DESTINATION_PATH = "/Game/TD/Assets/Terrain"
DESTINATION_NAME = "TDProject_Sculpted_A"


def main():
    source = unreal.load_asset(SOURCE_ASSET)
    if not source:
        raise RuntimeError(f"Source mesh not found: {SOURCE_ASSET}")
    if not isinstance(source, unreal.StaticMesh):
        raise RuntimeError(f"Source asset is not a StaticMesh: {SOURCE_ASSET}")

    destination = f"{DESTINATION_PATH}/{DESTINATION_NAME}"
    existing = unreal.load_asset(destination)
    if existing:
        unreal.log(f"Sculpt mesh already exists: {destination}")
        return

    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    duplicate = asset_tools.duplicate_asset(
        DESTINATION_NAME,
        DESTINATION_PATH,
        source,
    )
    if not duplicate:
        raise RuntimeError(f"Could not create sculpt mesh: {destination}")

    duplicate.set_editor_property("allow_cpu_access", True)
    unreal.EditorAssetLibrary.set_metadata_tag(
        duplicate,
        "SculptSource",
        SOURCE_ASSET,
    )
    unreal.EditorAssetLibrary.set_metadata_tag(
        duplicate,
        "SculptSpace",
        "Exact duplicate: pivot, bounds, and local coordinates preserved",
    )
    unreal.EditorAssetLibrary.save_loaded_asset(duplicate, False)
    unreal.log(f"Created sculpt-ready mesh: {destination}")


main()
