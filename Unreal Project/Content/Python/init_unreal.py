# Auto-run project Python helpers when the Unreal Editor starts.
try:
    import setup_towers_widget_ui
    setup_towers_widget_ui.setup(force=False)
except Exception as exc:
    import unreal
    unreal.log_warning("init_unreal setup_towers_widget_ui failed: {}".format(exc))

try:
    import setup_main_menu_widget_ui
    setup_main_menu_widget_ui.setup(force=False)
except Exception as exc:
    import unreal
    unreal.log_warning("init_unreal setup_main_menu_widget_ui failed: {}".format(exc))

try:
    import setup_ability_bar_ui
    setup_ability_bar_ui.setup(force=False)
except Exception as exc:
    import unreal
    unreal.log_warning("init_unreal setup_ability_bar_ui failed: {}".format(exc))

try:
    import setup_tower_store_ui
    setup_tower_store_ui.setup(force=False)
except Exception as exc:
    import unreal
    unreal.log_warning("init_unreal setup_tower_store_ui failed: {}".format(exc))

try:
    import setup_crystal_health_bar_ui
    setup_crystal_health_bar_ui.setup(force=False)
except Exception as exc:
    import unreal
    unreal.log_warning("init_unreal setup_crystal_health_bar_ui failed: {}".format(exc))
