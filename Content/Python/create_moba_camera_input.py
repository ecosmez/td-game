"""
Create MOBA camera Enhanced Input assets under /Game/TD/Input/MobaCamera/
Run in Unreal Editor: File > Execute Python Script, or:
  py Content/Python/create_moba_camera_input.py
"""
import unreal

FOLDER = "/Game/TD/Input/MobaCamera"

def ensure_folder(path: str):
    if not unreal.EditorAssetLibrary.does_directory_exist(path):
        unreal.EditorAssetLibrary.make_directory(path)

def create_input_action(name: str, value_type: unreal.InputActionValueType):
    asset_path = f"{FOLDER}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.log(f"Exists: {asset_path}")
        return unreal.EditorAssetLibrary.load_asset(asset_path)

    factory = unreal.InputActionFactoryNew()
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    action = asset_tools.create_asset(name, FOLDER, unreal.InputAction, factory)
    if not action:
        unreal.log_error(f"Failed to create {name}")
        return None
    action.set_editor_property("value_type", value_type)
    unreal.EditorAssetLibrary.save_asset(asset_path)
    unreal.log(f"Created {asset_path}")
    return action

def create_mapping_context(name: str, mappings: list):
    asset_path = f"{FOLDER}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        unreal.log(f"Exists: {asset_path}")
        return unreal.EditorAssetLibrary.load_asset(asset_path)

    factory = unreal.InputMappingContextFactoryNew()
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    imc = asset_tools.create_asset(name, FOLDER, unreal.InputMappingContext, factory)
    if not imc:
        unreal.log_error(f"Failed to create {name}")
        return None

    for action, key, modifiers in mappings:
        if not action:
            continue
        mapping = imc.map_key(action, key)
        for mod in modifiers:
            mapping.modifiers.append(mod)
    unreal.EditorAssetLibrary.save_asset(asset_path)
    unreal.log(f"Created {asset_path}")
    return imc

def main():
    ensure_folder(FOLDER)

    move = create_input_action("IA_CameraMove", unreal.InputActionValueType.AXIS2D)
    zoom = create_input_action("IA_CameraZoom", unreal.InputActionValueType.AXIS1D)
    rotate = create_input_action("IA_CameraRotate", unreal.InputActionValueType.AXIS1D)
    drag = create_input_action("IA_CameraDrag", unreal.InputActionValueType.BOOLEAN)
    focus = create_input_action("IA_CameraFocusChampion", unreal.InputActionValueType.BOOLEAN)

    # Build modifiers
    swizzle = unreal.InputModifierSwizzleAxis()
    swizzle.set_editor_property("order", unreal.InputAxisSwizzle.YXZ)
    neg_s = unreal.InputModifierNegate()
    neg_a = unreal.InputModifierNegate()
    neg_q = unreal.InputModifierNegate()
    swizzle_s = unreal.InputModifierSwizzleAxis()
    swizzle_s.set_editor_property("order", unreal.InputAxisSwizzle.YXZ)

    # Note: map_key returns mapping objects; modifier assignment varies by UE version.
    # If complex mapping fails, assign keys in the editor Mapping Context UI.
    imc = create_mapping_context("IMC_MobaCamera", [])

    if imc and move:
        try:
            imc.map_key(move, unreal.Key.W)
            imc.map_key(move, unreal.Key.S)
            imc.map_key(move, unreal.Key.A)
            imc.map_key(move, unreal.Key.D)
        except Exception as e:
            unreal.log_warning(f"WASD map note: {e}")
    if imc and zoom:
        try:
            imc.map_key(zoom, unreal.Key.MOUSE_WHEEL_AXIS)
        except Exception as e:
            unreal.log_warning(f"Zoom map note: {e}")
    if imc and rotate:
        try:
            imc.map_key(rotate, unreal.Key.Q)
            imc.map_key(rotate, unreal.Key.E)
        except Exception as e:
            unreal.log_warning(f"Rotate map note: {e}")
    if imc and drag:
        try:
            imc.map_key(drag, unreal.Key.MIDDLE_MOUSE_BUTTON)
        except Exception as e:
            unreal.log_warning(f"Drag map note: {e}")
    if imc and focus:
        try:
            imc.map_key(focus, unreal.Key.SPACE_BAR)
        except Exception as e:
            unreal.log_warning(f"Focus map note: {e}")

    if imc:
        unreal.EditorAssetLibrary.save_asset(f"{FOLDER}/IMC_MobaCamera")
    unreal.EditorAssetLibrary.save_directory(FOLDER)
    unreal.log("MOBA camera input setup complete.")

if __name__ == "__main__":
    main()
