"""
One-shot remote fix for BP_TopDownCharacter Create Widget compile errors.
Works against the already-running editor (does not need new C++ for the Class pin fix).
"""
import json
import os
import sys
import time

# UE remote execution client (must match Project Settings > Python)
ENGINE_REMOTE = r"C:\Program Files\Epic Games\UE_5.8\Engine\Plugins\Experimental\PythonScriptPlugin\Content\Python"
sys.path.insert(0, ENGINE_REMOTE)

from remote_execution import RemoteExecution, MODE_EXEC_FILE, MODE_EXEC_STATEMENT  # noqa: E402

FIX_CMD = r'''
import unreal

CHAR = "/Game/TopDown/Blueprints/BP_TopDownCharacter"
WIDGET_BP = "/Game/TD/UI/WBP_AbilityBar"
CPP_CLASS = "/Script/TD.AbilityBarWidget"

def all_graphs(bp):
    graphs = []
    for attr in ("ubergraph_pages", "function_graphs", "macro_graphs", "delegate_signature_graphs"):
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

def resolve_widget_class():
    bp = unreal.EditorAssetLibrary.load_asset(WIDGET_BP)
    if bp:
        try:
            gen = bp.generated_class()
            if gen:
                return gen
        except Exception:
            pass
        try:
            gen = bp.get_editor_property("generated_class")
            if gen:
                return gen
        except Exception:
            pass
    cls = unreal.load_class(None, CPP_CLASS)
    if cls is None:
        cls = unreal.load_object(None, CPP_CLASS)
    return cls

def node_title(node):
    try:
        return str(node.get_node_title(unreal.NodeTitleType.FULL_TITLE))
    except Exception:
        try:
            return node.get_name()
        except Exception:
            return ""

def is_create_widget(node):
    try:
        cname = node.get_class().get_name()
    except Exception:
        return False
    if "CreateWidget" in cname or cname in ("K2Node_CreateWidget", "K2Node_GenericCreateObject"):
        return True
    t = node_title(node)
    return "Create Widget" in t

def set_widget_class(node, widget_cls):
    for prop in ("widget_class", "WidgetClass"):
        try:
            node.set_editor_property(prop, widget_cls)
            try:
                node.reconstruct_node()
            except Exception:
                pass
            return True
        except Exception:
            pass
    try:
        for pin in list(node.pins):
            try:
                pname = str(pin.pin_name)
            except Exception:
                continue
            if pname.lower() != "class":
                continue
            try:
                pin.default_object = widget_cls
            except Exception:
                try:
                    pin.set_editor_property("default_object", widget_cls)
                except Exception:
                    continue
            try:
                node.reconstruct_node()
            except Exception:
                pass
            return True
    except Exception as exc:
        unreal.log_warning("class pin set failed: {}".format(exc))
    return False

def remove_node(graph, node):
    try:
        graph.remove_node(node)
        return True
    except Exception:
        pass
    try:
        node.destroy_node()
        return True
    except Exception:
        return False

# Close editor windows so package saves cleanly
try:
    aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
    if unreal.EditorAssetLibrary.does_asset_exist(CHAR):
        aes.close_all_editors_for_asset(unreal.EditorAssetLibrary.load_asset(CHAR))
except Exception as exc:
    unreal.log_warning("close editors: {}".format(exc))

if not unreal.EditorAssetLibrary.does_asset_exist(CHAR):
    unreal.log_error("Missing " + CHAR)
    raise SystemExit(1)

bp = unreal.EditorAssetLibrary.load_asset(CHAR)
widget_cls = resolve_widget_class()
unreal.log("Widget class: {}".format(widget_cls.get_path_name() if widget_cls else None))

# Dump graph names first
for g in all_graphs(bp):
    try:
        unreal.log("GRAPH: " + g.get_name())
    except Exception:
        pass

fixed = 0
removed = 0
for graph in all_graphs(bp):
    try:
        nodes = list(graph.nodes)
    except Exception:
        continue
    for node in nodes:
        if not is_create_widget(node):
            continue
        unreal.log("Found CreateWidget in {} :: {}".format(graph.get_name(), node_title(node)))
        if widget_cls and set_widget_class(node, widget_cls):
            fixed += 1
            unreal.log("  -> set Class to widget")
        else:
            if remove_node(graph, node):
                removed += 1
                unreal.log("  -> removed broken CreateWidget (C++ land path will spawn HUD)")
            else:
                unreal.log_error("  -> could not fix or remove")

# Also break dangling set-variable links that only existed for CreateWidget return
# Compile and report status
try:
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
except Exception as exc:
    unreal.log_warning("compile: {}".format(exc))

try:
    st = bp.status
    unreal.log("Blueprint status: {}".format(st))
except Exception:
    pass
try:
    st = bp.get_editor_property("status")
    unreal.log("Blueprint status prop: {}".format(st))
except Exception:
    pass

ok = unreal.EditorAssetLibrary.save_asset(CHAR)
unreal.log("RESULT fixed={} removed={} saved={}".format(fixed, removed, ok))
'''


def main():
    remote = RemoteExecution()
    remote.start()
    try:
        # Wait for editor multicast ping
        node_id = None
        for attempt in range(40):
            nodes = remote.remote_nodes
            if nodes:
                # remote_nodes can be dict id->node or list
                if isinstance(nodes, dict):
                    node_id = next(iter(nodes.keys()))
                else:
                    node_id = nodes[0].get("node_id") if isinstance(nodes[0], dict) else list(nodes)[0]
                break
            time.sleep(0.25)
        if not node_id:
            print("ERROR: No Unreal remote-execution node found. Is the editor open with Python remote execution enabled?")
            print("remote_nodes=", remote.remote_nodes)
            return 2

        print("Connecting to remote node:", node_id)
        remote.open_command_connection(node_id)

        # Prefer exec as statement block (inline). MODE_EXEC_FILE needs a path the editor can read.
        result = remote.run_command(FIX_CMD, exec_mode=MODE_EXEC_STATEMENT, unattended=True)
        print("RESULT:", json.dumps(result, indent=2, default=str) if not isinstance(result, str) else result)
        # Some versions return dict with success/output
        if isinstance(result, dict):
            success = result.get("success", result.get("Success", None))
            if success is False:
                return 1
        return 0
    finally:
        try:
            remote.stop()
        except Exception:
            pass


if __name__ == "__main__":
    raise SystemExit(main())
