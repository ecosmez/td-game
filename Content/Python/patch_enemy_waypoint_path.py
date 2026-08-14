"""
Rewire BP_Enemy / BP_EnemySpawner to waypoint-only C++ pathing.

Requires the TD C++ module compiled (UTDEnemyPathLibrary).

Run in Unreal:
  py "Content/Python/patch_enemy_waypoint_path.py"
or remote:
  python Content/Python/_run_patch_enemy_path_remote.py
"""
import unreal

ENEMY_BP = "/Game/TD/BP_Enemy"
SPAWNER_BP = "/Game/TD/BP_EnemySpawner"
LIB_PATH = "/Script/TD.TDEnemyPathLibrary"


def log(msg):
    unreal.log("[patch_enemy_path] {}".format(msg))


def warn(msg):
    unreal.log_warning("[patch_enemy_path] {}".format(msg))


def load_bp(path):
    bp = unreal.EditorAssetLibrary.load_asset(path)
    if not bp:
        warn("missing asset {}".format(path))
    return bp


def all_graphs(bp):
    graphs = []
    for attr in ("function_graphs", "ubergraph_pages", "macro_graphs"):
        try:
            pages = getattr(bp, attr, None)
            if pages:
                graphs.extend(list(pages))
        except Exception:
            pass
    try:
        for graph in unreal.BlueprintEditorLibrary.get_all_graphs(bp):
            if graph not in graphs:
                graphs.append(graph)
    except Exception:
        pass
    return graphs


def graph_name(graph):
    for attr in ("get_name", "get_graph_name"):
        try:
            val = getattr(graph, attr)()
            if val:
                return str(val)
        except Exception:
            pass
    try:
        return str(graph.get_editor_property("graph_guid"))
    except Exception:
        return str(graph)


def find_graph(bp, wanted):
    wanted_l = wanted.lower()
    for graph in all_graphs(bp):
        name = graph_name(graph)
        if name.lower() == wanted_l or name.lower().endswith(wanted_l):
            return graph
    return None


def get_nodes(graph):
    try:
        return list(graph.get_editor_property("nodes") or [])
    except Exception:
        return []


def is_entry_or_result(node):
    cname = node.get_class().get_name()
    return cname in ("K2Node_FunctionEntry", "K2Node_FunctionResult", "K2Node_Tunnel")


def get_pins(node):
    try:
        return list(node.get_pins())
    except Exception:
        return []


def pin_name(pin):
    try:
        return str(pin.pin_name)
    except Exception:
        try:
            return str(pin.get_editor_property("pin_name"))
        except Exception:
            return ""


def find_pin(node, names):
    wanted = {n.lower() for n in names}
    for pin in get_pins(node):
        if pin_name(pin).lower() in wanted:
            return pin
    return None


def find_exec_out(node):
    pin = find_pin(node, ("then", "execute"))
    if pin:
        try:
            if str(pin.direction) == "EGPD_OUTPUT" or "OUTPUT" in str(pin.direction):
                return pin
        except Exception:
            return pin
    for pin in get_pins(node):
        try:
            cat = str(pin.pin_type.pin_category)
        except Exception:
            cat = ""
        if cat.lower() == "exec" and "OUTPUT" in str(pin.direction):
            return pin
    return None


def find_exec_in(node):
    pin = find_pin(node, ("execute", "exec"))
    if pin:
        return pin
    for pin in get_pins(node):
        try:
            cat = str(pin.pin_type.pin_category)
        except Exception:
            cat = ""
        if cat.lower() == "exec" and "INPUT" in str(pin.direction):
            return pin
    return None


def connect(schema, a, b):
    if not a or not b:
        return False
    try:
        return bool(schema.try_create_connection(a, b))
    except Exception as exc:
        warn("connect failed: {}".format(exc))
        return False


def get_schema(graph):
    try:
        schema = graph.get_editor_property("schema")
        if schema:
            return schema
    except Exception:
        pass
    try:
        return graph.schema
    except Exception:
        pass
    return unreal.EdGraphSchema_K2()


def resolve_lib_function(func_name):
    lib = unreal.load_class(None, LIB_PATH)
    if lib is None:
        try:
            lib = unreal.TDEnemyPathLibrary.static_class()
        except Exception:
            lib = None
    if lib is None:
        warn("UTDEnemyPathLibrary not found — compile C++ first")
        return None, None
    func = None
    try:
        func = lib.find_function_by_name(func_name)
    except Exception:
        pass
    if func is None:
        try:
            func = unreal.find_object(lib, func_name)
        except Exception:
            pass
    if func is None:
        warn("function {} missing on library".format(func_name))
    return lib, func


def spawn_call_node(graph, func, x=384, y=0):
    node = unreal.new_object(unreal.K2Node_CallFunction, graph)
    try:
        graph.add_node(node, True, False)
    except Exception:
        try:
            graph.add_node(node)
        except Exception as exc:
            warn("add_node failed: {}".format(exc))
            return None
    try:
        node.set_editor_property("node_pos_x", x)
        node.set_editor_property("node_pos_y", y)
    except Exception:
        pass

    applied = False
    if func is not None:
        for call in (
            lambda: node.set_from_function(func),
            lambda: node.set_editor_property("function_reference", func),
        ):
            try:
                call()
                applied = True
                break
            except Exception:
                pass
        if not applied:
            try:
                ref = node.get_editor_property("function_reference")
                ref.set_member_name(func.get_name())
                applied = True
            except Exception as exc:
                warn("set function ref failed: {}".format(exc))

    for recon in ("allocate_default_pins", "reconstruct_node", "reallocate_pins_during_reconstruction"):
        try:
            getattr(node, recon)()
        except Exception:
            pass
    return node


def spawn_self_node(graph, x=128, y=128):
    try:
        node = unreal.new_object(unreal.K2Node_Self, graph)
        graph.add_node(node, True, False)
        node.set_editor_property("node_pos_x", x)
        node.set_editor_property("node_pos_y", y)
        try:
            node.reconstruct_node()
        except Exception:
            pass
        return node
    except Exception as exc:
        warn("K2Node_Self failed: {}".format(exc))
        return None


def clear_graph_body(graph):
    removed = 0
    for node in list(get_nodes(graph)):
        if is_entry_or_result(node):
            continue
        try:
            graph.remove_node(node)
            removed += 1
        except Exception:
            try:
                node.destroy_node()
                removed += 1
            except Exception as exc:
                warn("could not remove {}: {}".format(node, exc))
    return removed


def find_entry_result(graph):
    entry = None
    result = None
    for node in get_nodes(graph):
        cname = node.get_class().get_name()
        if cname == "K2Node_FunctionEntry":
            entry = node
        elif cname == "K2Node_FunctionResult":
            result = node
    return entry, result


def replace_function_with_call(bp, graph_name_wanted, lib_func_name, extra_pins=None):
    extra_pins = extra_pins or {}
    graph = find_graph(bp, graph_name_wanted)
    if graph is None:
        warn("{}: graph '{}' not found. Graphs: {}".format(
            bp.get_name(), graph_name_wanted, [graph_name(g) for g in all_graphs(bp)]))
        return False

    lib, func = resolve_lib_function(lib_func_name)
    if func is None and lib is None:
        return False

    removed = clear_graph_body(graph)
    log("{}: cleared {} nodes from {}".format(bp.get_name(), removed, graph_name(graph)))

    entry, result = find_entry_result(graph)
    if entry is None:
        warn("{}: no FunctionEntry in {}".format(bp.get_name(), graph_name(graph)))
        return False

    call = spawn_call_node(graph, func)
    if call is None:
        return False

    schema = get_schema(graph)
    ok = connect(schema, find_exec_out(entry), find_exec_in(call))
    if result:
        connect(schema, find_exec_out(call), find_exec_in(result))

    self_node = spawn_self_node(graph)
    target_pin = find_pin(call, ("Enemy", "Spawner", "self", "Target"))
    if self_node and target_pin:
        self_out = find_pin(self_node, ("self", "ReturnValue", "Output")) or (get_pins(self_node)[0] if get_pins(self_node) else None)
        if connect(schema, self_out, target_pin):
            ok = True

    for src_name, dest_name in extra_pins.items():
        src = find_pin(entry, (src_name,))
        dest = find_pin(call, (dest_name, src_name))
        if src and dest:
            connect(schema, src, dest)

    log("{}: {} -> {} pins: {}".format(
        bp.get_name(), graph_name_wanted, lib_func_name,
        [(pin_name(p), str(p.direction)) for p in get_pins(call)]))
    return ok


def compile_bp(bp):
    for compile_fn in (
        lambda: unreal.KismetEditorUtilities.compile_blueprint(bp),
        lambda: unreal.BlueprintEditorLibrary.compile_blueprint(bp),
    ):
        try:
            compile_fn()
            log("compiled {}".format(bp.get_name()))
            return
        except Exception:
            pass
    warn("could not compile {}".format(bp.get_name()))


def setup():
    enemy = load_bp(ENEMY_BP)
    spawner = load_bp(SPAWNER_BP)
    if enemy:
        log("Enemy graphs: {}".format([graph_name(g) for g in all_graphs(enemy)]))
        replace_function_with_call(enemy, "ChoosePath", "ChooseEnemyPath")
        replace_function_with_call(enemy, "BuildNavPath", "ChooseEnemyPath")
        replace_function_with_call(
            enemy, "AdvanceAlongPath", "AdvanceEnemyAlongPath",
            extra_pins={"DeltaSeconds": "DeltaSeconds"})
        compile_bp(enemy)
        unreal.EditorAssetLibrary.save_asset(ENEMY_BP)
    if spawner:
        log("Spawner graphs: {}".format([graph_name(g) for g in all_graphs(spawner)]))
        replace_function_with_call(spawner, "SpawnEnemyInner", "SpawnNextWaveEnemy")
        replace_function_with_call(spawner, "StartWaveSpawning", "BeginWaveSpawning")
        replace_function_with_call(spawner, "CheckWaveClear", "CheckWaveEnemiesCleared")
        replace_function_with_call(spawner, "StartWaves", "AnnounceWaveIfPrimary")
        replace_function_with_call(spawner, "ForceStartNextWave", "ForceStartNextWave")
        compile_bp(spawner)
        unreal.EditorAssetLibrary.save_asset(SPAWNER_BP)
    log("done")


if __name__ == "__main__":
    setup()
