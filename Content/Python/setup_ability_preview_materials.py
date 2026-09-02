"""Tune champion ability preview transparency and glow strength.

Run in Unreal Editor:
  py Content/Python/setup_ability_preview_materials.py
"""

import unreal

MATERIAL_FOLDER = "/Game/TD/Materials"
PARENT_MAT = f"{MATERIAL_FOLDER}/M_AbilityIndicator"

# Softer, more transparent previews (range ring, placement disc, ghost mesh, slow zone).
INSTANCE_OPACITY = {
    "MI_AbilityRange": 0.50,
    "MI_AbilityPlace": 0.38,
    "MI_AbilityGhost": 0.32,
    "MI_SlowZone": 0.30,
}

EMISSIVE_SCALE = 0.35


def _load_material(path):
    return unreal.EditorAssetLibrary.load_asset(path)


def _set_instance_opacity(name, opacity):
    path = f"{MATERIAL_FOLDER}/{name}"
    instance = _load_material(path)
    if not instance:
        unreal.log_warning(f"Missing material instance {path}")
        return False
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
        instance, "Opacity", opacity
    )
    unreal.EditorAssetLibrary.save_asset(path)
    return True


def _ensure_emissive_scaled(parent):
    """Scale emissive by Opacity * EmissiveScale so glow matches translucency."""
    mel = unreal.MaterialEditingLibrary
    expressions = list(parent.get_editor_property("expressions") or [])

    def _param_name(expr):
        try:
            return expr.get_editor_property("parameter_name")
        except Exception:
            return None

    has_emissive_scale = any(_param_name(expr) == "EmissiveScale" for expr in expressions)
    color_expr = next((expr for expr in expressions if _param_name(expr) == "Color"), None)
    opacity_expr = next((expr for expr in expressions if _param_name(expr) == "Opacity"), None)
    if not color_expr or not opacity_expr:
        unreal.log_warning("M_AbilityIndicator is missing Color/Opacity parameters")
        return False

    if not has_emissive_scale:
        emissive_scale = mel.create_material_expression(
            parent, unreal.MaterialExpressionScalarParameter, 80, 40
        )
        emissive_scale.set_editor_property("parameter_name", "EmissiveScale")
        emissive_scale.set_editor_property("default_value", EMISSIVE_SCALE)

        color_opacity = mel.create_material_expression(
            parent, unreal.MaterialExpressionMultiply, 300, -120
        )
        scaled_emissive = mel.create_material_expression(
            parent, unreal.MaterialExpressionMultiply, 520, -120
        )

        mel.connect_material_expressions(color_expr, "", color_opacity, "A")
        mel.connect_material_expressions(opacity_expr, "", color_opacity, "B")
        mel.connect_material_expressions(color_opacity, "", scaled_emissive, "A")
        mel.connect_material_expressions(emissive_scale, "", scaled_emissive, "B")
        mel.connect_material_property(
            scaled_emissive, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR
        )
    else:
        for expr in expressions:
            if _param_name(expr) == "EmissiveScale":
                expr.set_editor_property("default_value", EMISSIVE_SCALE)

    mel.recompile_material(parent)
    unreal.EditorAssetLibrary.save_asset(PARENT_MAT)
    return True


def setup():
    parent = _load_material(PARENT_MAT)
    if not parent:
        raise RuntimeError(f"Missing parent material {PARENT_MAT}")

    _ensure_emissive_scaled(parent)

    updated = 0
    for name, opacity in INSTANCE_OPACITY.items():
        if _set_instance_opacity(name, opacity):
            updated += 1

    unreal.log(
        f"ABILITY_PREVIEW_MATERIALS_SETUP_COMPLETE ({updated} instances, EmissiveScale={EMISSIVE_SCALE})"
    )
    return True


if __name__ == "__main__":
    setup()
