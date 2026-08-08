"""
Creates / rewires M_WorldFogOfWar_PP — post-process material for 3D map FOW.

Run in Unreal Editor (Output Log Python, or File → Execute Python Script):
  Content/Python/setup_world_fow_material.py

Parameters expected by UWorldFogOfWarComponent:
  FogMask          (Texture2D)  — remaining fog alpha
  MapCenterOrtho   (Vector)     — X=CenterX, Y=CenterY, Z=OrthoWidth
  FogColor         (Vector)     — dark fog tint
  FogIntensity     (Scalar)     — overall strength
"""

import unreal


ASSET_PATH = "/Game/TD/Materials/M_WorldFogOfWar_PP"
FOLDER = "/Game/TD/Materials"
NAME = "M_WorldFogOfWar_PP"


def _load_or_create_material():
    existing = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if existing:
        return existing
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.MaterialFactoryNew()
    mat = asset_tools.create_asset(NAME, FOLDER, unreal.Material, factory)
    return mat


def _clear_expressions(mat):
    # Best-effort wipe; MaterialEditingLibrary API varies by version.
    try:
        exprs = list(mat.get_editor_property("expressions") or [])
        for e in exprs:
            unreal.MaterialEditingLibrary.delete_material_expression(mat, e)
    except Exception as exc:
        unreal.log_warning(f"Could not clear expressions: {exc}")


def setup():
    mat = _load_or_create_material()
    if not mat:
        unreal.log_error("Failed to create/load M_WorldFogOfWar_PP")
        return False

    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    try:
        mat.set_editor_property("blendable_location", unreal.BlendableLocation.BL_AFTER_TONEMAPPING)
    except Exception:
        pass

    _clear_expressions(mat)

    # Build graph via MaterialEditingLibrary when available.
    mel = unreal.MaterialEditingLibrary

    scene = mel.create_material_expression(mat, unreal.MaterialExpressionSceneTexture, -900, 0)
    scene.set_editor_property("scene_texture_id", unreal.SceneTextureId.PPI_POST_PROCESS_INPUT0)

    world_pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -900, 250)
    map_params = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -900, 420)
    map_params.set_editor_property("parameter_name", "MapCenterOrtho")
    map_params.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 24000.0, 1.0))

    fog_color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -200, 650)
    fog_color.set_editor_property("parameter_name", "FogColor")
    fog_color.set_editor_property("default_value", unreal.LinearColor(0.02, 0.03, 0.04, 1.0))

    intensity = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -200, 780)
    intensity.set_editor_property("parameter_name", "FogIntensity")
    intensity.set_editor_property("default_value", 1.0)

    fog_tex = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, 120, 280)
    fog_tex.set_editor_property("parameter_name", "FogMask")
    fog_tex.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)

    world_xy = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -700, 250)
    world_xy.set_editor_property("r", True)
    world_xy.set_editor_property("g", True)
    world_xy.set_editor_property("b", False)
    world_xy.set_editor_property("a", False)

    center_xy = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -700, 400)
    center_xy.set_editor_property("r", True)
    center_xy.set_editor_property("g", True)
    center_xy.set_editor_property("b", False)
    center_xy.set_editor_property("a", False)

    ortho = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -700, 520)
    ortho.set_editor_property("r", False)
    ortho.set_editor_property("g", False)
    ortho.set_editor_property("b", True)
    ortho.set_editor_property("a", False)

    sub = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, -500, 300)
    div = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, -300, 300)
    half = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -300, 420)
    half.set_editor_property("r", 0.5)
    add_n = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, -100, 300)

    u_mask = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, 50, 220)
    u_mask.set_editor_property("r", True)
    u_mask.set_editor_property("g", False)
    u_mask.set_editor_property("b", False)
    u_mask.set_editor_property("a", False)

    v_mask = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, 50, 360)
    v_mask.set_editor_property("r", False)
    v_mask.set_editor_property("g", True)
    v_mask.set_editor_property("b", False)
    v_mask.set_editor_property("a", False)

    one_minus = mel.create_material_expression(mat, unreal.MaterialExpressionOneMinus, 200, 360)
    append_uv = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, -20, 280)
    mul_alpha = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 350, 480)
    lerp = mel.create_material_expression(mat, unreal.MaterialExpressionLinearInterpolate, 550, 120)

    mel.connect_material_expressions(world_pos, "", world_xy, "")
    mel.connect_material_expressions(map_params, "", center_xy, "")
    mel.connect_material_expressions(map_params, "", ortho, "")
    mel.connect_material_expressions(world_xy, "", sub, "A")
    mel.connect_material_expressions(center_xy, "", sub, "B")
    mel.connect_material_expressions(sub, "", div, "A")
    mel.connect_material_expressions(ortho, "", div, "B")
    mel.connect_material_expressions(div, "", add_n, "A")
    mel.connect_material_expressions(half, "", add_n, "B")
    mel.connect_material_expressions(add_n, "", u_mask, "")
    mel.connect_material_expressions(add_n, "", v_mask, "")
    mel.connect_material_expressions(v_mask, "", one_minus, "")
    mel.connect_material_expressions(u_mask, "", append_uv, "A")
    mel.connect_material_expressions(one_minus, "", append_uv, "B")
    mel.connect_material_expressions(append_uv, "", fog_tex, "UVs")
    mel.connect_material_expressions(fog_tex, "A", mul_alpha, "A")
    mel.connect_material_expressions(intensity, "", mul_alpha, "B")
    mel.connect_material_expressions(scene, "Color", lerp, "A")
    mel.connect_material_expressions(fog_color, "", lerp, "B")
    mel.connect_material_expressions(mul_alpha, "", lerp, "Alpha")
    mel.connect_material_property(lerp, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)

    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(ASSET_PATH)
    unreal.log("M_WorldFogOfWar_PP ready.")
    return True


if __name__ == "__main__":
    setup()
