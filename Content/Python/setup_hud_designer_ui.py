"""
Seed HUD Widget Blueprints under /Game/TD/UI so they are editable in UMG.

UE 5.8 no longer exposes WidgetBlueprint.widget_tree to Python. Use
EditorUtilityLibrary.add_source_widget / find_source_widget_by_name instead.

Creates missing named widgets. Designer edits win: existing names are left alone.
"""
import unreal

DST_DIR = "/Game/TD/UI"


def _load_cpp(path):
    cls = unreal.load_class(None, path)
    if cls is None:
        cls = unreal.load_object(None, path)
    return cls


def _close(path):
    try:
        aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
        if unreal.EditorAssetLibrary.does_asset_exist(path):
            aes.close_all_editors_for_asset(unreal.EditorAssetLibrary.load_asset(path))
    except Exception as exc:
        unreal.log_warning("close editors {}: {}".format(path, exc))


def _ensure_wbp(asset_path, asset_name, cpp_parent):
    parent_cls = _load_cpp(cpp_parent)
    if parent_cls is None:
        unreal.log_warning("C++ parent not loaded: {}".format(cpp_parent))
        return None

    if not unreal.EditorAssetLibrary.does_directory_exist(DST_DIR):
        unreal.EditorAssetLibrary.make_directory(DST_DIR)

    if unreal.EditorAssetLibrary.does_asset_exist(asset_path):
        return unreal.EditorAssetLibrary.load_asset(asset_path)

    factory = unreal.WidgetBlueprintFactory()
    factory.set_editor_property("parent_class", parent_cls)
    bp = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        asset_name, DST_DIR, unreal.WidgetBlueprint, factory
    )
    if bp:
        unreal.log("Created {}".format(asset_path))
    return bp


def _find(bp, name):
    try:
        found = unreal.EditorUtilityLibrary.find_source_widget_by_name(bp, name)
        if found:
            return found
    except Exception:
        pass
    return None


def _has(bp, name):
    return _find(bp, name) is not None


def _has_any(bp, names):
    for name in names:
        if _has(bp, name):
            return True
    return False


def _add(bp, widget_type, name, parent_name=""):
    existing = _find(bp, name)
    if existing:
        return existing
    parent = parent_name if parent_name else "None"
    try:
        return unreal.EditorUtilityLibrary.add_source_widget(bp, widget_type, name, parent)
    except Exception as exc:
        unreal.log_warning("add_source_widget {} under {}: {}".format(name, parent, exc))
        return None


def _compile_save(bp, path):
    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    except Exception as exc:
        unreal.log_warning("compile {}: {}".format(path, exc))
    unreal.EditorAssetLibrary.save_asset(path)


def _text(bp, name, parent, value, size=13, justify=None):
    block = _add(bp, unreal.TextBlock, name, parent)
    if not block:
        return None
    block.set_text(unreal.Text(value))
    if justify is not None:
        try:
            block.set_justification(justify)
        except Exception:
            pass
    try:
        font = block.get_editor_property("font")
        font.size = size
        block.set_editor_property("font", font)
    except Exception:
        pass
    return block


def _size_box(bp, name, parent, width, height):
    box = _add(bp, unreal.SizeBox, name, parent)
    if not box:
        return None
    try:
        box.set_width_override(width)
        box.set_height_override(height)
    except Exception:
        box.set_editor_property("width_override", width)
        box.set_editor_property("height_override", height)
        box.set_editor_property("b_override_width_override", True)
        box.set_editor_property("b_override_height_override", True)
    return box


def _canvas_slot(widget, anchors_min, anchors_max, align, offsets, z=10, auto_size=True):
    if not widget:
        return None
    slot = widget.slot
    if not slot:
        return None
    try:
        slot.set_anchors(unreal.Anchors(minimum=anchors_min, maximum=anchors_max))
        slot.set_alignment(align)
        slot.set_offsets(offsets)
        slot.set_z_order(z)
        if auto_size:
            slot.set_auto_size(True)
    except Exception as exc:
        unreal.log_warning("canvas slot {}: {}".format(widget.get_name(), exc))
    return slot


def _required(bp, names):
    for name in names:
        if not _has(bp, name):
            return False
    return True


def seed_capture_channel(force=False):
    path = "/Game/TD/UI/WBP_CaptureChannel"
    _close(path)
    bp = _ensure_wbp(path, "WBP_CaptureChannel", "/Script/TD.CaptureChannelWidget")
    if bp is None:
        return False
    if (not force) and _required(bp, ("ChannelBar", "BarSize")):
        unreal.log("WBP_CaptureChannel already has designer widgets")
        _compile_save(bp, path)
        return True

    size = _size_box(bp, "BarSize", "", 80.0, 10.0)
    _add(bp, unreal.ProgressBar, "ChannelBar", "BarSize")
    if size:
        try:
            size.set_percent
        except Exception:
            pass
    bar = _find(bp, "ChannelBar")
    if bar:
        try:
            bar.set_percent(0.0)
        except Exception:
            pass
    _compile_save(bp, path)
    unreal.log("Seeded WBP_CaptureChannel")
    return True


def seed_floating_damage(force=False):
    path = "/Game/TD/UI/WBP_FloatingDamage"
    _close(path)
    bp = _ensure_wbp(path, "WBP_FloatingDamage", "/Script/TD.FloatingDamageTextWidget")
    if bp is None:
        return False
    if (not force) and _has(bp, "FloatingDamageLabel"):
        unreal.log("WBP_FloatingDamage already has designer widgets")
        _compile_save(bp, path)
        return True

    _text(bp, "FloatingDamageLabel", "", "0", 22, unreal.TextJustify.CENTER)
    _compile_save(bp, path)
    unreal.log("Seeded WBP_FloatingDamage")
    return True


def _add_ability_slot(bp, key):
    _size_box(bp, "Size_{}".format(key), "AbilitySlotRow", 56.0, 56.0)
    frame = _add(bp, unreal.Border, "Frame_{}".format(key), "Size_{}".format(key))
    if frame:
        frame.set_padding(unreal.Margin(3.0, 3.0, 3.0, 3.0))
        frame.set_brush_color(unreal.LinearColor(0.45, 0.85, 0.95, 1.0))
    _add(bp, unreal.Overlay, "Slot_{}".format(key), "Frame_{}".format(key))
    btn = _add(bp, unreal.Button, "Btn_{}".format(key), "Slot_{}".format(key))
    if btn:
        btn.set_background_color(unreal.LinearColor(0.12, 0.22, 0.32, 0.96))
    clip = _size_box(bp, "CDClip_{}".format(key), "Slot_{}".format(key), 56.0, 0.0)
    if clip:
        clip.set_visibility(unreal.SlateVisibility.COLLAPSED)
    fill = _add(bp, unreal.Border, "CDFill_{}".format(key), "CDClip_{}".format(key))
    if fill:
        fill.set_brush_color(unreal.LinearColor(0.02, 0.03, 0.06, 0.82))
    labels = _add(bp, unreal.VerticalBox, "Labels_{}".format(key), "Slot_{}".format(key))
    if labels:
        labels.set_visibility(unreal.SlateVisibility.HIT_TEST_INVISIBLE)
    _text(bp, "Key_{}".format(key), "Labels_{}".format(key), key, 18, unreal.TextJustify.CENTER)
    cd_text = _text(bp, "CDText_{}".format(key), "Labels_{}".format(key), "", 16, unreal.TextJustify.CENTER)
    if cd_text:
        cd_text.set_visibility(unreal.SlateVisibility.COLLAPSED)
    lock = _text(bp, "Lock_{}".format(key), "Labels_{}".format(key), "LOCKED", 10, unreal.TextJustify.CENTER)
    if lock:
        lock.set_visibility(unreal.SlateVisibility.COLLAPSED)


def seed_ability_bar(force=False):
    path = "/Game/TD/UI/WBP_AbilityBar"
    _close(path)
    bp = _ensure_wbp(path, "WBP_AbilityBar", "/Script/TD.AbilityBarWidget")
    if bp is None:
        return False
    if (not force) and _required(bp, ("AbilitySlotRow", "Btn_Q")) and _has_any(bp, ("StorePlusButton", "PlusButton")):
        unreal.log("WBP_AbilityBar already has designer widgets")
        _compile_save(bp, path)
        return True

    root = _add(bp, unreal.CanvasPanel, "AbilityBarRoot", "")
    chrome = _add(bp, unreal.Border, "AbilityBarChrome", "AbilityBarRoot")
    if chrome:
        chrome.set_padding(unreal.Margin(6.0, 4.0, 6.0, 4.0))
        chrome.set_brush_color(unreal.LinearColor(0.05, 0.08, 0.12, 0.72))
        _canvas_slot(
            chrome,
            unreal.Vector2D(0.0, 1.0),
            unreal.Vector2D(0.0, 1.0),
            unreal.Vector2D(0.0, 1.0),
            unreal.Margin(24.0, -138.0, 0.0, 0.0),
            10,
        )
    _size_box(bp, "AbilityBarSize", "AbilityBarChrome", 312.0, 56.0)
    _add(bp, unreal.HorizontalBox, "AbilitySlotRow", "AbilityBarSize")

    _size_box(bp, "StorePlusSize", "AbilitySlotRow", 56.0, 56.0)
    plus_frame = _add(bp, unreal.Border, "StorePlusFrame", "StorePlusSize")
    if plus_frame:
        plus_frame.set_brush_color(unreal.LinearColor(0.45, 0.78, 0.88, 1.0))
    plus_btn = _add(bp, unreal.Button, "StorePlusButton", "StorePlusFrame")
    if plus_btn:
        plus_btn.set_background_color(unreal.LinearColor(0.12, 0.42, 0.55, 0.95))
    _text(bp, "StorePlusLabel", "StorePlusButton", "+", 22, unreal.TextJustify.CENTER)

    for key in ("Q", "W", "E", "R"):
        _add_ability_slot(bp, key)

    _compile_save(bp, path)
    unreal.log("Seeded WBP_AbilityBar")
    return True


def seed_crystal_health(force=False):
    path = "/Game/TD/UI/WBP_CrystalHealthBar"
    _close(path)
    bp = _ensure_wbp(path, "WBP_CrystalHealthBar", "/Script/TD.CrystalHealthBarWidget")
    if bp is None:
        return False
    if (not force) and _required(bp, ("TopHudStack", "BaseHealthBar", "NextWaveCrystalImpact")) and _has_any(bp, ("WaveLabel", "WaveTitle")) and _has_any(bp, ("NextWaveButton", "PlayNextWave")) and _has_any(bp, ("WaveDotsBox", "WaveDotsHost")):
        unreal.log("WBP_CrystalHealthBar already has designer widgets")
        _compile_save(bp, path)
        return True

    _add(bp, unreal.CanvasPanel, "CrystalHealthRoot", "")
    stack = _add(bp, unreal.VerticalBox, "TopHudStack", "CrystalHealthRoot")
    _canvas_slot(
        stack,
        unreal.Vector2D(0.5, 0.0),
        unreal.Vector2D(0.5, 0.0),
        unreal.Vector2D(0.5, 0.0),
        unreal.Margin(0.0, 10.0, 0.0, 0.0),
        10,
    )

    top_chrome = _add(bp, unreal.Border, "TopBarChrome", "TopHudStack")
    if top_chrome:
        top_chrome.set_padding(unreal.Margin(14.0, 10.0, 14.0, 10.0))
        top_chrome.set_brush_color(unreal.LinearColor(0.03, 0.04, 0.06, 0.94))
    _add(bp, unreal.HorizontalBox, "TopBarRow", "TopBarChrome")

    _add(bp, unreal.Border, "BaseHealthChrome", "TopBarRow")
    _add(bp, unreal.VerticalBox, "BaseHealthColumn", "BaseHealthChrome")
    _add(bp, unreal.HorizontalBox, "BaseHealthHeader", "BaseHealthColumn")
    _text(bp, "BaseHealthTitle", "BaseHealthHeader", "BASE HEALTH", 13)
    _text(bp, "BaseHealthValue", "BaseHealthHeader", "100 / 100", 13, unreal.TextJustify.RIGHT)
    _size_box(bp, "BaseHealthSize", "BaseHealthColumn", 240.0, 12.0)
    _add(bp, unreal.Overlay, "BaseHealthOverlay", "BaseHealthSize")
    bar = _add(bp, unreal.ProgressBar, "BaseHealthBar", "BaseHealthOverlay")
    if bar:
        bar.set_percent(1.0)

    _add(bp, unreal.Border, "WaveStripChrome", "TopBarRow")
    _add(bp, unreal.VerticalBox, "WaveColumn", "WaveStripChrome")
    _text(bp, "WaveLabel", "WaveColumn", "WAVE  0 / 7", 13)
    _add(bp, unreal.HorizontalBox, "WaveStripRow", "WaveColumn")
    _add(bp, unreal.HorizontalBox, "WaveDotsBox", "WaveStripRow")

    enemies = _add(bp, unreal.Border, "EnemiesCountChrome", "TopBarRow")
    _text(bp, "WaveEnemiesCount", "EnemiesCountChrome", "ENEMIES\n0", 16, unreal.TextJustify.CENTER)
    _text(bp, "WaveTimer", "TopBarRow", "0:00", 20, unreal.TextJustify.CENTER)

    _size_box(bp, "NextWaveSize", "TopBarRow", 36.0, 36.0)
    _add(bp, unreal.Border, "NextWaveFrame", "NextWaveSize")
    _add(bp, unreal.Button, "NextWaveButton", "NextWaveFrame")
    _text(bp, "NextWavePlayIcon", "NextWaveButton", ">", 16, unreal.TextJustify.CENTER)

    threat = _add(bp, unreal.Border, "CrystalThreatChrome", "TopHudStack")
    if threat:
        threat.set_padding(unreal.Margin(14.0, 7.0, 14.0, 7.0))
    _add(bp, unreal.VerticalBox, "CrystalThreatColumn", "CrystalThreatChrome")
    _text(bp, "NextWaveCrystalImpact", "CrystalThreatColumn", "NEXT WAVE", 15, unreal.TextJustify.CENTER)
    _text(bp, "EnemyCrystalAccumulation", "CrystalThreatColumn", "", 11, unreal.TextJustify.CENTER)

    _compile_save(bp, path)
    unreal.log("Seeded WBP_CrystalHealthBar")
    return True


def seed_champion_frame(force=False):
    path = "/Game/TD/UI/WBP_ChampionFrame"
    _close(path)
    bp = _ensure_wbp(path, "WBP_ChampionFrame", "/Script/TD.ChampionFrameWidget")
    if bp is None:
        return False
    if (not force) and _required(bp, ("ChampionFrameChrome", "ChampionHpBar")):
        unreal.log("WBP_ChampionFrame already has designer widgets")
        _compile_save(bp, path)
        return True

    _add(bp, unreal.CanvasPanel, "ChampionFrameRoot", "")
    chrome = _add(bp, unreal.Border, "ChampionFrameChrome", "ChampionFrameRoot")
    if chrome:
        chrome.set_padding(unreal.Margin(10.0, 8.0, 14.0, 8.0))
        chrome.set_brush_color(unreal.LinearColor(0.03, 0.04, 0.06, 0.94))
        _canvas_slot(
            chrome,
            unreal.Vector2D(0.0, 1.0),
            unreal.Vector2D(0.0, 1.0),
            unreal.Vector2D(0.0, 1.0),
            unreal.Margin(24.0, 0.0, 0.0, 24.0),
            10,
        )
    _add(bp, unreal.HorizontalBox, "ChampionFrameRow", "ChampionFrameChrome")

    _size_box(bp, "ChampionAvatarSize", "ChampionFrameRow", 92.0, 92.0)
    _add(bp, unreal.Overlay, "ChampionAvatarOverlay", "ChampionAvatarSize")
    _add(bp, unreal.Border, "ChampionAvatarFrame", "ChampionAvatarOverlay")
    _add(bp, unreal.Overlay, "ChampionPortraitOverlay", "ChampionAvatarFrame")
    img = _add(bp, unreal.Image, "ChampionAvatarImage", "ChampionPortraitOverlay")
    if img:
        img.set_visibility(unreal.SlateVisibility.COLLAPSED)
    _text(bp, "ChampionAvatarLetter", "ChampionPortraitOverlay", "C", 36, unreal.TextJustify.CENTER)

    _size_box(bp, "ChampionLevelSize", "ChampionAvatarOverlay", 26.0, 26.0)
    _add(bp, unreal.Border, "ChampionLevelFrame", "ChampionLevelSize")
    _add(bp, unreal.Overlay, "ChampionLevelOverlay", "ChampionLevelFrame")
    _text(bp, "ChampionLevelLabel", "ChampionLevelOverlay", "1", 12, unreal.TextJustify.CENTER)

    _add(bp, unreal.VerticalBox, "ChampionStats", "ChampionFrameRow")
    _text(bp, "ChampionName", "ChampionStats", "CHAMPION", 14)
    _size_box(bp, "ChampionHpSize", "ChampionStats", 196.0, 22.0)
    _add(bp, unreal.Overlay, "ChampionHpOverlay", "ChampionHpSize")
    bar = _add(bp, unreal.ProgressBar, "ChampionHpBar", "ChampionHpOverlay")
    if bar:
        bar.set_percent(1.0)
    _text(bp, "ChampionHpValue", "ChampionHpOverlay", "- / -", 12, unreal.TextJustify.CENTER)

    _compile_save(bp, path)
    unreal.log("Seeded WBP_ChampionFrame")
    return True


def seed_minimap(force=False):
    path = "/Game/TD/UI/WBP_Minimap"
    _close(path)
    bp = _ensure_wbp(path, "WBP_Minimap", "/Script/TD.MinimapWidget")
    if bp is None:
        return False
    if (not force) and _required(bp, ("MinimapFrame", "MinimapImage")) and _has_any(bp, ("ChampionMarker", "ChampionBlip")):
        unreal.log("WBP_Minimap already has designer widgets")
        _compile_save(bp, path)
        return True

    _add(bp, unreal.CanvasPanel, "MinimapRoot", "")
    size = _size_box(bp, "MinimapSizeBox", "MinimapRoot", 220.0, 220.0)
    _canvas_slot(
        size,
        unreal.Vector2D(1.0, 1.0),
        unreal.Vector2D(1.0, 1.0),
        unreal.Vector2D(1.0, 1.0),
        unreal.Margin(0.0, 0.0, 24.0, 24.0),
        50,
        auto_size=False,
    )
    frame = _add(bp, unreal.Border, "MinimapFrame", "MinimapSizeBox")
    if frame:
        frame.set_padding(unreal.Margin(3.0, 3.0, 3.0, 3.0))
        frame.set_brush_color(unreal.LinearColor(0.55, 0.65, 0.75, 1.0))
    inner = _add(bp, unreal.Border, "MinimapInner", "MinimapFrame")
    if inner:
        inner.set_brush_color(unreal.LinearColor(0.04, 0.07, 0.10, 0.94))
    _add(bp, unreal.CanvasPanel, "MinimapCanvas", "MinimapInner")

    def _map_image(name, z):
        image = _add(bp, unreal.Image, name, "MinimapCanvas")
        _canvas_slot(
            image,
            unreal.Vector2D(0.0, 0.0),
            unreal.Vector2D(1.0, 1.0),
            unreal.Vector2D(0.0, 0.0),
            unreal.Margin(0.0, 0.0, 0.0, 0.0),
            z,
            auto_size=False,
        )
        return image

    _map_image("MinimapImage", 0)
    fog = _map_image("MinimapFog", 1)
    if fog:
        fog.set_visibility(unreal.SlateVisibility.COLLAPSED)

    def _marker(name, z, color):
        border = _add(bp, unreal.Border, name, "MinimapCanvas")
        if border:
            border.set_brush_color(color)
        slot = _canvas_slot(
            border,
            unreal.Vector2D(0.0, 0.0),
            unreal.Vector2D(0.0, 0.0),
            unreal.Vector2D(0.5, 0.5),
            unreal.Margin(0.0, 0.0, 0.0, 0.0),
            z,
            auto_size=False,
        )
        if slot:
            try:
                slot.set_size(unreal.Vector2D(14.0, 14.0))
            except Exception:
                pass
        return border

    _marker("ChampionMarkerFrame", 2, unreal.LinearColor(1.0, 1.0, 1.0, 0.9))
    _marker("ChampionMarker", 3, unreal.LinearColor(0.25, 0.85, 1.0, 1.0))
    _marker("CameraMarker", 3, unreal.LinearColor(1.0, 0.92, 0.35, 0.95))
    crystal = _marker("CrystalMarker", 4, unreal.LinearColor(0.15, 0.95, 0.35, 1.0))
    if crystal:
        crystal.set_visibility(unreal.SlateVisibility.COLLAPSED)
    spawn = _marker("EnemySpawnMarker", 4, unreal.LinearColor(0.95, 0.18, 0.15, 1.0))
    if spawn:
        spawn.set_visibility(unreal.SlateVisibility.COLLAPSED)

    _compile_save(bp, path)
    unreal.log("Seeded WBP_Minimap")
    return True


def seed_orbit_gizmo(force=False):
    path = "/Game/TD/UI/WBP_CameraOrbitGizmo"
    _close(path)
    bp = _ensure_wbp(path, "WBP_CameraOrbitGizmo", "/Script/TD.CameraOrbitGizmoWidget")
    if bp is None:
        return False
    if (not force) and _required(bp, ("OrbitRing", "OrbitHandle")):
        unreal.log("WBP_CameraOrbitGizmo already has designer widgets")
        _compile_save(bp, path)
        return True

    _add(bp, unreal.CanvasPanel, "OrbitGizmoRoot", "")
    size = _size_box(bp, "OrbitGizmoSizeBox", "OrbitGizmoRoot", 88.0, 88.0)
    _canvas_slot(
        size,
        unreal.Vector2D(1.0, 1.0),
        unreal.Vector2D(1.0, 1.0),
        unreal.Vector2D(1.0, 1.0),
        unreal.Margin(0.0, 0.0, 216.0, 216.0),
        60,
        auto_size=False,
    )
    _add(bp, unreal.Overlay, "OrbitGizmoOverlay", "OrbitGizmoSizeBox")
    _add(bp, unreal.Border, "OrbitRing", "OrbitGizmoOverlay")
    _size_box(bp, "OrbitHubSize", "OrbitGizmoOverlay", 28.0, 28.0)
    _add(bp, unreal.Border, "OrbitHub", "OrbitHubSize")
    _add(bp, unreal.CanvasPanel, "OrbitHandleCanvas", "OrbitGizmoOverlay")
    handle = _add(bp, unreal.Border, "OrbitHandle", "OrbitHandleCanvas")
    hslot = _canvas_slot(
        handle,
        unreal.Vector2D(0.0, 0.0),
        unreal.Vector2D(0.0, 0.0),
        unreal.Vector2D(0.5, 0.5),
        unreal.Margin(0.0, 0.0, 0.0, 0.0),
        1,
        auto_size=False,
    )
    if hslot:
        try:
            hslot.set_size(unreal.Vector2D(18.0, 18.0))
        except Exception:
            pass

    _compile_save(bp, path)
    unreal.log("Seeded WBP_CameraOrbitGizmo")
    return True


def seed_tower_store(force=False):
    path = "/Game/TD/UI/WBP_TowerStore"
    _close(path)
    bp = _ensure_wbp(path, "WBP_TowerStore", "/Script/TD.TowerStoreWidget")
    if bp is None:
        return False
    if (not force) and _required(bp, ("CategoryTab_All",)) and _has_any(bp, ("StorePanel", "StoreChrome")) and _has_any(bp, ("CardRow", "CardHost")):
        unreal.log("WBP_TowerStore already has designer widgets")
        _compile_save(bp, path)
        return True

    _add(bp, unreal.CanvasPanel, "TowerStoreRoot", "")
    panel = _add(bp, unreal.Border, "StorePanel", "TowerStoreRoot")
    if panel:
        panel.set_padding(unreal.Margin(12.0, 8.0, 12.0, 8.0))
        panel.set_brush_color(unreal.LinearColor(0.04, 0.06, 0.09, 0.96))
        _canvas_slot(
            panel,
            unreal.Vector2D(0.5, 1.0),
            unreal.Vector2D(0.5, 1.0),
            unreal.Vector2D(0.5, 1.0),
            unreal.Margin(0.0, -36.0, 0.0, 0.0),
            55,
        )
    _add(bp, unreal.VerticalBox, "PanelVBox", "StorePanel")
    _add(bp, unreal.HorizontalBox, "StoreHeader", "PanelVBox")
    _text(bp, "StoreTitle", "StoreHeader", "TOWERS", 16)
    _text(bp, "ResourceText", "StoreHeader", "0", 16, unreal.TextJustify.RIGHT)

    _add(bp, unreal.HorizontalBox, "CategoryTabsRow", "PanelVBox")
    for name in ("All", "Attack", "Defense", "Support"):
        _size_box(bp, "CategoryTabSize_{}".format(name), "CategoryTabsRow", 88.0, 26.0)
        _add(bp, unreal.Button, "CategoryTab_{}".format(name), "CategoryTabSize_{}".format(name))
        _text(bp, "CategoryTabLabel_{}".format(name), "CategoryTab_{}".format(name), name, 11, unreal.TextJustify.CENTER)

    _size_box(bp, "StripSize", "PanelVBox", 780.0, 86.0)
    scroll = _add(bp, unreal.ScrollBox, "CardScroll", "StripSize")
    if scroll:
        try:
            scroll.set_orientation(unreal.Orientation.ORIENT_HORIZONTAL)
        except Exception:
            pass
    _add(bp, unreal.HorizontalBox, "CardRow", "CardScroll")

    hover = _size_box(bp, "HoverFloat", "TowerStoreRoot", 420.0, 180.0)
    _canvas_slot(
        hover,
        unreal.Vector2D(0.5, 1.0),
        unreal.Vector2D(0.5, 1.0),
        unreal.Vector2D(0.5, 1.0),
        unreal.Margin(0.0, -196.0, 0.0, 0.0),
        80,
        auto_size=False,
    )
    if hover:
        hover.set_visibility(unreal.SlateVisibility.COLLAPSED)
    _add(bp, unreal.HorizontalBox, "HoverRow", "HoverFloat")
    _size_box(bp, "HoverMeshBox", "HoverRow", 160.0, 160.0)
    _add(bp, unreal.Image, "HoverMeshImage", "HoverMeshBox")
    _add(bp, unreal.VerticalBox, "HoverInfo", "HoverRow")
    _add(bp, unreal.HorizontalBox, "HoverTitleRow", "HoverInfo")
    _text(bp, "HoverName", "HoverTitleRow", "-", 20)
    _text(bp, "HoverCost", "HoverTitleRow", "", 18)
    _text(bp, "HoverStats", "HoverInfo", "", 13)

    _compile_save(bp, path)
    unreal.log("Seeded WBP_TowerStore")
    return True


def setup(force=False):
    ok = True
    ok = seed_capture_channel(force) and ok
    ok = seed_floating_damage(force) and ok
    ok = seed_ability_bar(force) and ok
    ok = seed_crystal_health(force) and ok
    ok = seed_champion_frame(force) and ok
    ok = seed_minimap(force) and ok
    ok = seed_orbit_gizmo(force) and ok
    ok = seed_tower_store(force) and ok
    return ok


if __name__ == "__main__":
    setup(force=False)
