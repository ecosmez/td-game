"""Apply SelectVisibleTarget so other towers occlude shots (not terrain)."""
import os
import unreal
from editor_toolset.toolsets.blueprint import BlueprintTools

TOWER_BP = "/Game/TD/BP_Tower"
GRAPH_NAME = "SelectVisibleTarget"
DSL_PATH = os.path.join(
    os.path.dirname(os.path.abspath(__file__)),
    "..",
    "TD",
    "Tower_SelectVisibleTarget.dsl.txt",
)
KEEP_CLASSES = (
    "K2Node_FunctionEntry",
    "K2Node_FunctionResult",
    "K2Node_Tunnel",
)


def log(msg):
    unreal.log("[tower_shot_block] {}".format(msg))
    print(msg)


def load_dsl():
    path = os.path.normpath(DSL_PATH)
    with open(path, "r", encoding="utf-8") as handle:
        return handle.read()


def wipe_function_body(graph):
    deleted = 0
    for node in list(BlueprintTools.find_nodes(graph)):
        cname = node.get_class().get_name()
        if cname in KEEP_CLASSES:
            log("keep {}".format(cname))
            continue
        log("delete {} {}".format(cname, node.get_name()))
        BlueprintTools.delete_node(node)
        deleted += 1
    log("deleted {} leftover nodes".format(deleted))
    return deleted


def main():
    log("start")
    bp = unreal.EditorAssetLibrary.load_asset(TOWER_BP)
    if not bp:
        raise RuntimeError("missing " + TOWER_BP)

    graph = BlueprintTools.get_graph(bp, GRAPH_NAME)
    types = BlueprintTools.find_node_types(graph, "IsShotBlocked")
    log("node types: {}".format(types))
    log("before:\n{}".format(BlueprintTools.read_graph_dsl(graph)))

    wipe_function_body(graph)
    log("after wipe:\n{}".format(BlueprintTools.read_graph_dsl(graph)))

    code = load_dsl()
    log("writing DSL from {}".format(os.path.normpath(DSL_PATH)))
    BlueprintTools.write_graph_dsl(graph, code)
    BlueprintTools.compile_blueprint(bp, warnings_as_errors=False)
    ok = unreal.EditorAssetLibrary.save_asset(TOWER_BP)
    log("saved {} {}".format(TOWER_BP, ok))

    infos = BlueprintTools.get_node_infos(BlueprintTools.find_nodes(graph))
    type_ids = [info.type_id for info in infos]
    if type_ids.count("TD|TowerCombat|IsShotBlockedByOtherTower") != 1:
        raise RuntimeError("expected one IsShotBlockedByOtherTower node, got {}: {}".format(
            type_ids.count("TD|TowerCombat|IsShotBlockedByOtherTower"), type_ids))
    if type_ids.count("Actor|GetAllActorsOfClass") != 1:
        raise RuntimeError("expected one GetAllActorsOfClass after wipe, got {}".format(type_ids))
    log("ok nodes={}".format(type_ids))


main()
