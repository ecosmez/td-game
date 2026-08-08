"""
Creates / rewires M_WorldFogOfWar_Plane — unlit translucent world fog sheet.

WorldPosition + MapCenterOrtho UV mapping matches UMapDiscoveryComponent::WorldToNormalized
and M_WorldFogOfWar_PP so reveals line up under the champion.

Run in Unreal Editor:
  py Content/Python/setup_world_fow_plane_material.py
"""

import unreal


ASSET_PATH = "/Game/TD/Materials/M_WorldFogOfWar_Plane"
FOLDER = "/Game/TD/Materials"
NAME = "M_WorldFogOfWar_Plane"


def _load_or_create_material():
    existing = unreal.EditorAssetLibrary.load_asset(ASSET_PATH)
    if existing:
        return existing
    asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
    factory = unreal.MaterialFactoryNew()
    return asset_tools.create_asset(NAME, FOLDER, unreal.Material, factory)


def _clear_expressions(mat):
    try:
        exprs = list(mat.get_editor_property("expressions") or [])
        for e in exprs:
            unreal.MaterialEditingLibrary.delete_material_expression(mat, e)
    except Exception as exc:
        unreal.log_warning(f"Could not clear expressions: {exc}")


def setup():
    mat = _load_or_create_material()
    if not mat:
        unreal.log_error("Failed to create/load M_WorldFogOfWar_Plane")
        return False

    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_SURFACE)
    mat.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    mat.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    mat.set_editor_property("two_sided", True)
    try:
        mat.set_editor_property("disable_depth_test", True)
    except Exception:
        pass

    _clear_expressions(mat)
    mel = unreal.MaterialEditingLibrary

    world_pos = mel.create_material_expression(mat, unreal.MaterialExpressionWorldPosition, -1000, 0)
    map_params = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -1000, 220)
    map_params.set_editor_property("parameter_name", "MapCenterOrtho")
    map_params.set_editor_property("default_value", unreal.LinearColor(0.0, 0.0, 24000.0, 1.0))

    fog_color = mel.create_material_expression(mat, unreal.MaterialExpressionVectorParameter, -200, 520)
    fog_color.set_editor_property("parameter_name", "FogColor")
    fog_color.set_editor_property("default_value", unreal.LinearColor(0.02, 0.03, 0.04, 0.96))

    intensity = mel.create_material_expression(mat, unreal.MaterialExpressionScalarParameter, -200, 680)
    intensity.set_editor_property("parameter_name", "FogIntensity")
    intensity.set_editor_property("default_value", 1.0)

    fog_tex = mel.create_material_expression(mat, unreal.MaterialExpressionTextureSampleParameter2D, 200, 160)
    fog_tex.set_editor_property("parameter_name", "FogMask")
    fog_tex.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_COLOR)

    world_xy = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -800, 0)
    world_xy.set_editor_property("r", True)
    world_xy.set_editor_property("g", True)
    world_xy.set_editor_property("b", False)
    world_xy.set_editor_property("a", False)

    center_xy = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -800, 160)
    center_xy.set_editor_property("r", True)
    center_xy.set_editor_property("g", True)
    center_xy.set_editor_property("b", False)
    center_xy.set_editor_property("a", False)

    ortho = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, -800, 300)
    ortho.set_editor_property("r", False)
    ortho.set_editor_property("g", False)
    ortho.set_editor_property("b", True)
    ortho.set_editor_property("a", False)

    sub = mel.create_material_expression(mat, unreal.MaterialExpressionSubtract, -600, 40)
    div = mel.create_material_expression(mat, unreal.MaterialExpressionDivide, -400, 40)
    half = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 180)
    half.set_editor_property("r", 0.5)
    add_n = mel.create_material_expression(mat, unreal.MaterialExpressionAdd, -200, 40)

    u_mask = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, 0, 0)
    u_mask.set_editor_property("r", True)
    u_mask.set_editor_property("g", False)
    u_mask.set_editor_property("b", False)
    u_mask.set_editor_property("a", False)

    v_mask = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, 0, 120)
    v_mask.set_editor_property("r", False)
    v_mask.set_editor_property("g", True)
    v_mask.set_editor_property("b", False)
    v_mask.set_editor_property("a", False)

    one_minus = mel.create_material_expression(mat, unreal.MaterialExpressionOneMinus, 120, 120)
    append_uv = mel.create_material_expression(mat, unreal.MaterialExpressionAppendVector, 40, 60)
    mul_a = mel.create_material_expression(mat, unreal.MaterialExpressionMultiply, 420, 280)

    color_rgb = mel.create_material_expression(mat, unreal.MaterialExpressionComponentMask, 0, 520)
    color_rgb.set_editor_property("r", True)
    color_rgb.set_editor_property("g", True)
    color_rgb.set_editor_property("b", True)
    color_rgb.set_editor_property("a", False)

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
    mel.connect_material_expressions(fog_color, "", color_rgb, "")
    mel.connect_material_expressions(fog_tex, "A", mul_a, "A")
    mel.connect_material_expressions(intensity, "", mul_a, "B")

    mel.connect_material_property(color_rgb, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    mel.connect_material_property(mul_a, "", unreal.MaterialProperty.MP_OPACITY)

    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(ASSET_PATH)
    unreal.log("M_WorldFogOfWar_Plane ready (world-position FOW mask).")
    return True


if __name__ == "__main__":
    setup()
