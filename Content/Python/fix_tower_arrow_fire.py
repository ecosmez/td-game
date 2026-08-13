"""
Fix towers that stop firing after FirePoints are moved onto the mesh.

1. Tag any SceneComponent named FirePoint* with Component Tag "FirePoint"
   (GatherFirePoints only collects by tag, not by name).
2. Make SelectVisibleTarget ignore the firing tower so LoS does not die
   inside TowerMesh collision.
"""
import unreal

TOWER_BP = "/Game/TD/BP_Tower"
CHILD_BPS = (
    "/Game/TD/Towers/BP_Tower_Arrow",
    "/Game/TD/Towers/BP_Tower_Cannon",
)
FIRE_TAG = "FirePoint"


def log(msg):
    unreal.log("[fix_tower_fire] {}".format(msg))
    print(msg)


def all_graphs(bp):
    graphs = []
    for attr in ("ubergraph_pages", "function_graphs", "macro_graphs"):
        try:
            pages = getattr(bp, attr, None)
            if pages:
                graphs.extend(list(pages))
        except Exception:
            pass
    try:
        for g in unreal.BlueprintEditorLibrary.get_all_graphs(bp):
            if g not in graphs:
                graphs.append(g)
    except Exception:
        pass
    return graphs


def node_title(node):
    try:
        return str(node.get_node_title(unreal.NodeTitleType.FULL_TITLE))
    except Exception:
        try:
            return node.get_name()
        except Exception:
            return ""


def node_pins(node):
    try:
        return list(node.pins)
    except Exception:
        pass
    try:
        return list(node.get_editor_property("pins") or [])
    except Exception:
        return []


def pin_name(pin):
    for attr in ("pin_name", "PinName"):
        try:
            return str(getattr(pin, attr))
        except Exception:
            pass
    try:
        return str(pin.get_editor_property("pin_name"))
    except Exception:
        return ""


def set_bool_pin(pin, value):
    text = "true" if value else "false"
    errors = []
    for setter in (
        lambda: setattr(pin, "default_value", text),
        lambda: pin.set_editor_property("default_value", text),
        lambda: setattr(pin, "default_value", "1" if value else "0"),
    ):
        try:
            setter()
            return True
        except Exception as exc:
            errors.append(str(exc))
    log("  could not set pin {}: {}".format(pin_name(pin), errors[-1] if errors else "?"))
    return False


def patch_ignore_self(bp):
    changed = 0
    found = 0
    for graph in all_graphs(bp):
        gname = graph.get_name()
        try:
            nodes = list(graph.nodes)
        except Exception:
            continue
        for node in nodes:
            title = node_title(node)
            cname = ""
            try:
                cname = node.get_class().get_name()
            except Exception:
                pass
            if "LineTraceByChannel" not in title and "LineTraceByChannel" not in cname:
                continue
            found += 1
            log("  found {} in {}".format(title, gname))
            for pin in node_pins(node):
                pname = pin_name(pin)
                if pname in ("bIgnoreSelf", "IgnoreSelf", "Ignore Self"):
                    if set_bool_pin(pin, True):
                        changed += 1
                        log("    set {} = true".format(pname))
                    try:
                        node.reconstruct_node()
                    except Exception:
                        pass
    log("LineTrace nodes={} pins_changed={}".format(found, changed))
    return changed > 0 or found > 0


def ensure_tags_on_template(tmpl, label):
    if tmpl is None:
        return False
    try:
        tags = list(tmpl.get_editor_property("component_tags") or [])
    except Exception:
        try:
            tags = list(tmpl.component_tags or [])
        except Exception:
            tags = []
    tag_strs = [str(t) for t in tags]
    if FIRE_TAG in tag_strs:
        log("  {} already tagged loc={}".format(label, tmpl.relative_location))
        return False
    tags.append(FIRE_TAG)
    try:
        tmpl.set_editor_property("component_tags", tags)
        log("  tagged {} loc={}".format(label, tmpl.relative_location))
        return True
    except Exception as exc:
        log("  tag failed {}: {}".format(label, exc))
        return False


def tag_firepoint_components(bp):
    tagged = 0
    try:
        scs = bp.simple_construction_script
        nodes = list(scs.get_all_nodes())
    except Exception as exc:
        log("  scs error: {}".format(exc))
        nodes = []
    for node in nodes:
        try:
            varname = str(node.get_editor_property("variable_name"))
        except Exception:
            try:
                varname = str(node.variable_name)
            except Exception:
                varname = node.get_name()
        if FIRE_TAG not in varname:
            continue
        try:
            tmpl = node.component_template
        except Exception:
            tmpl = None
        if ensure_tags_on_template(tmpl, varname):
            tagged += 1
    try:
        ich = bp.get_editor_property("inheritable_component_handler")
    except Exception:
        ich = None
    if ich:
        templates = []
        for meth in (
            "get_overriden_component_templates",
            "get_overridden_component_templates",
        ):
            try:
                templates = list(getattr(ich, meth)() or [])
                break
            except Exception:
                pass
        for tmpl in templates:
            try:
                name = tmpl.get_name()
            except Exception:
                name = str(tmpl)
            if FIRE_TAG in name:
                if ensure_tags_on_template(tmpl, "ICH " + name):
                    tagged += 1
    return tagged


def dump_cdo_firepoints(path):
    bp = unreal.EditorAssetLibrary.load_asset(path)
    if not bp:
        log("missing {}".format(path))
        return
    try:
        cdo = unreal.get_default_object(bp.generated_class())
    except Exception as exc:
        log("CDO error {}: {}".format(path, exc))
        return
    try:
        aspeed = cdo.get_editor_property("attack_speed")
    except Exception:
        try:
            aspeed = cdo.get_editor_property("AttackSpeed")
        except Exception:
            aspeed = "?"
    try:
        can = cdo.get_editor_property("can_attack")
    except Exception:
        try:
            can = cdo.get_editor_property("CanAttack")
        except Exception:
            can = "?"
    log("{} CanAttack={} AttackSpeed={}".format(path, can, aspeed))
    tagged = []
    try:
        tagged = list(cdo.get_components_by_tag(unreal.SceneComponent, FIRE_TAG))
    except Exception as exc:
        log("  get_components_by_tag: {}".format(exc))
    if not tagged:
        log("  NO tagged FirePoint components")
    for c in tagged:
        loc = c.relative_location
        xy = (loc.x * loc.x + loc.y * loc.y) ** 0.5
        log(
            "  tagged {} class={} rel=({:.1f},{:.1f},{:.1f}) xy={:.1f}".format(
                c.get_name(), c.get_class().get_name(), loc.x, loc.y, loc.z, xy
            )
        )
    try:
        comps = list(cdo.get_components_by_class(unreal.SceneComponent))
    except Exception:
        comps = []
    for c in comps:
        name = c.get_name()
        if FIRE_TAG not in name:
            continue
        if c in tagged:
            continue
        loc = c.relative_location
        log(
            "  UNTAGGED {} rel=({:.1f},{:.1f},{:.1f})".format(
                name, loc.x, loc.y, loc.z
            )
        )


def compile_save(path):
    bp = unreal.EditorAssetLibrary.load_asset(path)
    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    except Exception:
        try:
            unreal.KismetEditorUtilities.compile_blueprint(bp)
        except Exception as exc:
            log("compile failed {}: {}".format(path, exc))
    ok = unreal.EditorAssetLibrary.save_asset(path)
    log("saved {} {}".format(path, ok))


def main():
    log("start")
    tower = unreal.EditorAssetLibrary.load_asset(TOWER_BP)
    if tower:
        patch_ignore_self(tower)
        compile_save(TOWER_BP)
    else:
        log("could not load " + TOWER_BP)

    for path in CHILD_BPS:
        bp = unreal.EditorAssetLibrary.load_asset(path)
        if not bp:
            log("missing " + path)
            continue
        n = tag_firepoint_components(bp)
        log("{} tagged {}".format(path, n))
        compile_save(path)
        dump_cdo_firepoints(path)

    log("done")


main()
