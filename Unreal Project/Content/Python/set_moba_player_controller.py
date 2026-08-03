"""
Force AMobaPlayerController on all gameplay GameModes and maps.

Run in Unreal Editor (Output Log):
  py Content/Python/set_moba_player_controller.py
"""
import unreal

MOBA_PC = unreal.load_class(None, "/Script/TD.MobaPlayerController")
MOBA_CAM = unreal.load_class(None, "/Script/TD.MobaCameraPawn")
CHAMP_CLASS_PATH = "/Game/TopDown/Blueprints/BP_TopDownCharacter.BP_TopDownCharacter_C"
GM_PATH = "/Game/TopDown/Blueprints/BP_TopDownController"
TOPDOWN_GM = "/Game/TopDown/Blueprints/BP_TopDownGameMode"
TOPDOWN_PC_BP = "/Game/TopDown/Blueprints/BP_TopDownController"
MAPS = [
    "/Game/Level0",
    "/Game/TopDown/Lvl_TopDown",
]


def _class_path(cls) -> str:
    if not cls:
        return "None"
    return cls.get_path_name()


def set_game_mode_classes():
    if not MOBA_PC:
        unreal.log_error("AMobaPlayerController not found — is TD module compiled?")
        return {"ok": False, "reason": "missing_moba_pc"}

    results = {}

    gm = unreal.EditorAssetLibrary.load_asset(TOPDOWN_GM)
    if not gm:
        unreal.log_error(f"Missing {TOPDOWN_GM}")
        return {"ok": False, "reason": "missing_topdown_gm"}

    # Blueprint asset -> generated class CDO
    generated = gm.generated_class()
    cdo = unreal.get_default_object(generated)

    cdo.set_editor_property("player_controller_class", MOBA_PC)
    # Keep champion as default pawn; MobaPlayerController will wire it then possess free camera.
    # Do NOT switch DefaultPawn to camera unless ChampionClass is also configured on PC.
    unreal.EditorAssetLibrary.save_asset(TOPDOWN_GM)
    results["BP_TopDownGameMode"] = {
        "player_controller_class": _class_path(cdo.get_editor_property("player_controller_class")),
        "default_pawn_class": _class_path(cdo.get_editor_property("default_pawn_class")),
    }
    unreal.log(
        f"BP_TopDownGameMode PlayerControllerClass -> {results['BP_TopDownGameMode']['player_controller_class']}"
    )

    # Reparent legacy controller so any leftover spawn remains IsA(AMobaPlayerController).
    pc_bp = unreal.EditorAssetLibrary.load_asset(TOPDOWN_PC_BP)
    if pc_bp and MOBA_PC:
        try:
            parent = pc_bp.parent_class if hasattr(pc_bp, "parent_class") else None
            # UBlueprint.parent_class
            current_parent = pc_bp.get_editor_property("parent_class")
            unreal.log(f"BP_TopDownController current parent: {_class_path(current_parent)}")
            if current_parent != MOBA_PC:
                unreal.BlueprintEditorLibrary.reparent_blueprint(pc_bp, MOBA_PC)
                unreal.BlueprintEditorLibrary.compile_blueprint(pc_bp)
                unreal.EditorAssetLibrary.save_asset(TOPDOWN_PC_BP)
                results["BP_TopDownController_reparent"] = _class_path(MOBA_PC)
                unreal.log("Reparented BP_TopDownController -> AMobaPlayerController")
            else:
                results["BP_TopDownController_reparent"] = "already_moba"
        except Exception as e:
            results["BP_TopDownController_reparent"] = f"error: {e}"
            unreal.log_warning(f"BP_TopDownController reparent failed: {e}")

    return {"ok": True, "results": results}


def ensure_map_game_mode(map_path: str, gm_class):
    # Load each map and set World Settings DefaultGameMode.
    if not unreal.EditorLoadingAndSavingUtils.load_map(map_path):
        # UE5 API
        try:
            unreal.EditorLevelLibrary.load_level(map_path)
        except Exception as e:
            unreal.log_warning(f"Could not load map {map_path}: {e}")
            return f"load_failed: {e}"

    world = unreal.EditorLevelLibrary.get_editor_world()
    if not world:
        return "no_world"

    ws = world.get_world_settings()
    if not ws:
        return "no_world_settings"

    before = _class_path(ws.get_editor_property("default_game_mode"))
    ws.set_editor_property("default_game_mode", gm_class)
    after = _class_path(ws.get_editor_property("default_game_mode"))

    try:
        unreal.EditorLevelLibrary.save_current_level()
    except Exception:
        unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

    return {"before": before, "after": after}


def main():
    out = set_game_mode_classes()

    gm_asset = unreal.EditorAssetLibrary.load_asset(TOPDOWN_GM)
    gm_class = gm_asset.generated_class() if gm_asset else None
    map_results = {}
    if gm_class:
        for m in MAPS:
            map_results[m] = ensure_map_game_mode(m, gm_class)
            unreal.log(f"Map {m}: {map_results[m]}")

    out["maps"] = map_results
    unreal.log(f"set_moba_player_controller done: {out}")
    return out


if __name__ == "__main__":
    main()
