# Auto-run project Python helpers when the interactive Unreal Editor starts.
# UI-oriented helpers can assert while Slate is unavailable in commandlets.
import unreal
if "-run=" in unreal.SystemLibrary.get_command_line().lower():
    unreal.log("init_unreal: skipping UI setup during commandlet execution")
else:
    try:
        import setup_terrain_collision
    except Exception as exc:
        unreal.log_warning("init_unreal setup_terrain_collision failed: {}".format(exc))

    try:
        import setup_towers_widget_ui
        setup_towers_widget_ui.setup(force=False)
    except Exception as exc:
        unreal.log_warning("init_unreal setup_towers_widget_ui failed: {}".format(exc))

    try:
        import setup_main_menu_widget_ui
        setup_main_menu_widget_ui.setup(force=False)
    except Exception as exc:
        unreal.log_warning("init_unreal setup_main_menu_widget_ui failed: {}".format(exc))

    try:
        import setup_ability_bar_ui
        setup_ability_bar_ui.setup(force=False)
    except Exception as exc:
        unreal.log_warning("init_unreal setup_ability_bar_ui failed: {}".format(exc))

    try:
        # Ensure ShowAbilityHUD Create Widget still points at WBP_AbilityBar after setup.
        import fix_topdown_character_createwidget
        fix_topdown_character_createwidget.setup()
    except Exception as exc:
        unreal.log_warning("init_unreal fix_topdown_character_createwidget failed: {}".format(exc))

    try:
        import setup_tower_store_ui
        setup_tower_store_ui.setup(force=False)
    except Exception as exc:
        unreal.log_warning("init_unreal setup_tower_store_ui failed: {}".format(exc))

    try:
        import setup_crystal_health_bar_ui
        setup_crystal_health_bar_ui.setup(force=False)
    except Exception as exc:
        unreal.log_warning("init_unreal setup_crystal_health_bar_ui failed: {}".format(exc))

    try:
        import setup_ability_preview_materials
        setup_ability_preview_materials.setup()
    except Exception as exc:
        unreal.log_warning("init_unreal setup_ability_preview_materials failed: {}".format(exc))
