"""
Fix BP_TopDownCharacter::ShowAbilityHUD Create Widget Class pin.

Recreating WBP_AbilityBar (setup_ability_bar_ui) clears the Class pin and leaves:
  - Spawn node Create Widget must have a class specified
  - Invalid pin connection Input Object / Return Value

Sets UK2Node_CreateWidget.WidgetClass to WBP_AbilityBar (falls back to AbilityBarWidget C++).
"""
import unreal

CHAR = "/Game/TopDown/Blueprints/BP_TopDownCharacter"
WIDGET_BP = "/Game/TD/UI/WBP_AbilityBar"
CPP_CLASS = "/Script/TD.AbilityBarWidget"


def _close_editors():
    try:
        aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
        if unreal.EditorAssetLibrary.does_asset_exist(CHAR):
            aes.close_all_editors_for_asset(unreal.EditorAssetLibrary.load_asset(CHAR))
    except Exception as exc:
        unreal.log_warning("close editors: {}".format(exc))


def _resolve_widget_class():
    bp = unreal.EditorAssetLibrary.load_asset(WIDGET_BP)
    if bp is not None:
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
    graphs = []
    try:
        if hasattr(bp, "ubergraph_pages") and bp.ubergraph_pages:
            graphs.extend(list(bp.ubergraph_pages))
    except Exception:
        pass
    try:
        if hasattr(bp, "function_graphs") and bp.function_graphs:
            graphs.extend(list(bp.function_graphs))
    except Exception:
        pass
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
    _close_editors()

    if not unreal.EditorAssetLibrary.does_asset_exist(CHAR):
        unreal.log_error("Missing {}".format(CHAR))
        return False

    widget_cls = _resolve_widget_class()
    if widget_cls is None:
        unreal.log_error("Could not resolve ability bar widget class")
        return False
    unreal.log("Using widget class: {}".format(widget_cls.get_path_name()))

    bp = unreal.EditorAssetLibrary.load_asset(CHAR)
    if bp is None:
        unreal.log_error("Failed to load BP_TopDownCharacter")
        return False

    nodes = _find_create_widget_nodes(bp)
    unreal.log("Found {} CreateWidget node(s) on BP_TopDownCharacter".format(len(nodes)))
    if not nodes:
        unreal.log_warning("No K2Node_CreateWidget nodes found (ShowAbilityHUD may already be OK)")
        return True

    fixed = 0
    for graph, node in nodes:
        try:
            node.set_editor_property("widget_class", widget_cls)
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

    ok = unreal.EditorAssetLibrary.save_asset(CHAR)
    unreal.log("Saved BP_TopDownCharacter: {}".format(ok))
    return ok


if __name__ == "__main__":
    setup()
