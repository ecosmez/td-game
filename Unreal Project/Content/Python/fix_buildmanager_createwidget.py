"""
Fix BP_BuildManager ShowBuildHUD Create Widget Class pin.
Sets UK2Node_CreateWidget.WidgetClass to WBP_TowerStore (falls back to C++ class).
"""
import unreal

BM = "/Game/TD/BP_BuildManager"
WIDGET_BP = "/Game/TD/UI/WBP_TowerStore"
CPP_CLASS = "/Script/TD.TowerStoreWidget"


def _close_bm_editors():
    try:
        aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
        if unreal.EditorAssetLibrary.does_asset_exist(BM):
            aes.close_all_editors_for_asset(unreal.EditorAssetLibrary.load_asset(BM))
    except Exception as exc:
        unreal.log_warning("close editors: {}".format(exc))


def _resolve_widget_class():
    # Prefer generated BP class so designer overrides stick; fall back to C++.
    bp = unreal.EditorAssetLibrary.load_asset(WIDGET_BP)
    if bp is not None:
        # WidgetBlueprint.generated_class is the correct soft-target
        try:
            gen = bp.generated_class()
            if gen is not None:
                return gen
        except Exception:
            pass
        try:
            gen = bp.get_editor_property("generated_class")
            if gen is not None:
                return gen
        except Exception:
            pass

    cls = unreal.load_class(None, CPP_CLASS)
    if cls is None:
        cls = unreal.load_object(None, CPP_CLASS)
    return cls


def _find_create_widget_nodes(bp):
    nodes = []
    uber = bp.ubergraph_pages if hasattr(bp, "ubergraph_pages") else None
    graphs = list(bp.function_graphs) if hasattr(bp, "function_graphs") else []
    if uber:
        graphs = list(uber) + graphs
    # Also try BlueprintEditorLibrary / all graphs via iteration
    try:
        for g in unreal.BlueprintEditorLibrary.get_all_graphs(bp):
            if g not in graphs:
                graphs.append(g)
    except Exception:
        pass

    for graph in graphs:
        try:
            for node in graph.nodes:
                if node.get_class().get_name() == "K2Node_CreateWidget":
                    nodes.append((graph, node))
        except Exception as exc:
            unreal.log_warning("scan graph {}: {}".format(graph, exc))
    return nodes


def setup():
    _close_bm_editors()

    if not unreal.EditorAssetLibrary.does_asset_exist(BM):
        unreal.log_error("Missing {}".format(BM))
        return False

    widget_cls = _resolve_widget_class()
    if widget_cls is None:
        unreal.log_error("Could not resolve tower store widget class")
        return False
    unreal.log("Using widget class: {}".format(widget_cls.get_path_name()))

    bp = unreal.EditorAssetLibrary.load_asset(BM)
    if bp is None:
        unreal.log_error("Failed to load BP_BuildManager")
        return False

    nodes = _find_create_widget_nodes(bp)
    unreal.log("Found {} CreateWidget node(s)".format(len(nodes)))
    if not nodes:
        unreal.log_error("No K2Node_CreateWidget nodes found on BP_BuildManager")
        return False

    fixed = 0
    for graph, node in nodes:
        try:
            node.set_editor_property("widget_class", widget_cls)
            # Reconstruct pins after class change
            try:
                node.reconstruct_node()
            except Exception:
                pass
            fixed += 1
            unreal.log(
                "Set WidgetClass on {} in graph {}".format(
                    node.get_name(), graph.get_name()
                )
            )
        except Exception as exc:
            # Try alternate property access
            try:
                node.set_editor_property("WidgetClass", widget_cls)
                fixed += 1
            except Exception as exc2:
                unreal.log_error("Failed to set WidgetClass: {} / {}".format(exc, exc2))

    if fixed == 0:
        return False

    try:
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    except Exception as exc:
        unreal.log_warning("compile: {}".format(exc))

    # Re-check errors
    try:
        status = unreal.BlueprintEditorLibrary.get_blueprint_status(bp)
        unreal.log("BP_BuildManager status after fix: {}".format(status))
    except Exception:
        pass

    ok = unreal.EditorAssetLibrary.save_asset(BM)
    unreal.log("Saved BP_BuildManager: {}".format(ok))
    return ok


if __name__ == "__main__":
    setup()
