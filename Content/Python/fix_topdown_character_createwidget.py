"""
Diagnose + fix BP_TopDownCharacter::ShowAbilityHUD.

Windows often ends up with 0 CreateWidget nodes (node deleted / pin broken after
WBP_AbilityBar recreate). This script:
  1) Logs every graph/node + blueprint compile status
  2) Sets WidgetClass on any existing CreateWidget nodes
  3) If none exist, rebuilds ShowAbilityHUD with CreateWidget -> WBP_AbilityBar

Run: Tools > Execute Python Script
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
        for getter in ("generated_class",):
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


def _all_graphs(bp):
    graphs = []
    try:
        if getattr(bp, "ubergraph_pages", None):
            graphs.extend(list(bp.ubergraph_pages))
    except Exception:
        pass
    try:
        if getattr(bp, "function_graphs", None):
            graphs.extend(list(bp.function_graphs))
    except Exception:
        pass
    try:
        for g in unreal.BlueprintEditorLibrary.get_all_graphs(bp):
            if g not in graphs:
                graphs.append(g)
    except Exception:
        pass
    return graphs


def _dump_graphs(bp):
    for graph in _all_graphs(bp):
        try:
            gname = graph.get_name()
        except Exception:
            gname = str(graph)
        unreal.log("=== GRAPH {} ===".format(gname))
        try:
            nodes = list(graph.nodes)
        except Exception as exc:
            unreal.log_warning("  cannot read nodes: {}".format(exc))
            continue
        for node in nodes:
            try:
                cls = node.get_class().get_name()
                title = ""
                try:
                    title = str(node.get_node_title(unreal.NodeTitleType.FULL_TITLE))
                except Exception:
                    try:
                        title = node.get_name()
                    except Exception:
                        title = "?"
                unreal.log("  - {} | {}".format(cls, title))
            except Exception as exc:
                unreal.log_warning("  node dump fail: {}".format(exc))


def _find_create_widget_nodes(bp):
    found = []
    for graph in _all_graphs(bp):
        try:
            for node in graph.nodes:
                cname = node.get_class().get_name()
                if cname in ("K2Node_CreateWidget", "K2Node_GenericCreateObject"):
                    # Prefer real CreateWidget
                    if cname == "K2Node_CreateWidget" or "Widget" in cname:
                        found.append((graph, node))
                # Also match by title text
                try:
                    title = str(node.get_node_title(unreal.NodeTitleType.FULL_TITLE))
                    if "Create Widget" in title:
                        found.append((graph, node))
                except Exception:
                    pass
        except Exception as exc:
            unreal.log_warning("scan {}: {}".format(graph, exc))
    # de-dupe by object
    uniq = []
    seen = set()
    for g, n in found:
        key = n.get_path_name()
        if key not in seen:
            seen.add(key)
            uniq.append((g, n))
    return uniq


def _find_graph_by_name(bp, name):
    for graph in _all_graphs(bp):
        try:
            gname = graph.get_name()
            if gname == name or gname.endswith(name) or name in gname:
                return graph
        except Exception:
            pass
        try:
            path = graph.get_path_name()
            if path.endswith(":" + name) or path.endswith("." + name):
                return graph
        except Exception:
            pass
    return None


def _new_graph_node(graph, class_path, x, y):
    """Spawn an EdGraph node under the graph outer."""
    node_cls = unreal.load_class(None, class_path)
    if node_cls is None:
        unreal.log_error("Cannot load node class {}".format(class_path))
        return None
    node = unreal.new_object(node_cls, graph)
    if node is None:
        return None
    try:
        node.node_pos_x = int(x)
        node.node_pos_y = int(y)
    except Exception:
        pass
    try:
        graph.add_node(node, False, False)
    except Exception as exc:
        # Some UE builds use different add_node signatures
        try:
            graph.add_node(node)
        except Exception as exc2:
            unreal.log_error("add_node failed: {} / {}".format(exc, exc2))
            return None
    try:
        node.create_new_guid()
    except Exception:
        pass
    try:
        node.post_placed_new_node()
    except Exception:
        pass
    try:
        node.allocate_default_pins()
    except Exception:
        pass
    try:
        node.reconstruct_node()
    except Exception:
        pass
    return node


def _pin(node, name, direction=None):
    """Find a pin by name (direction optional: 'input'/'output')."""
    try:
        pins = list(node.pins)
    except Exception:
        try:
            pins = node.get_editor_property("pins")
        except Exception:
            return None
    name_l = name.lower()
    for p in pins:
        try:
            pname = str(p.pin_name)
        except Exception:
            try:
                pname = str(p.get_editor_property("pin_name"))
            except Exception:
                continue
        if pname.lower() != name_l and pname.replace(" ", "").lower() != name_l.replace(" ", ""):
            # also allow friendly names without spaces
            if pname.replace(" ", "").lower() != name.replace(" ", "").lower():
                continue
        if direction:
            try:
                d = p.direction
                dname = str(d)
            except Exception:
                try:
                    dname = str(p.get_editor_property("direction"))
                except Exception:
                    dname = ""
            if direction == "input" and "Output" in dname:
                continue
            if direction == "output" and "Input" in dname:
                continue
        return p
    return None


def _link(schema, a_node, a_name, b_node, b_name, a_dir="output", b_dir="input"):
    a = _pin(a_node, a_name, a_dir)
    b = _pin(b_node, b_name, b_dir)
    if not a or not b:
        unreal.log_warning(
            "Missing pins for link {}::{} -> {}::{} (a={} b={})".format(
                a_node.get_name(), a_name, b_node.get_name(), b_name, a, b
            )
        )
        return False
    try:
        schema.try_create_connection(a, b)
        return True
    except Exception as exc:
        try:
            # alternate API
            if schema.create_connection(a, b):
                return True
        except Exception as exc2:
            unreal.log_warning("link failed: {} / {}".format(exc, exc2))
    return False


def _clear_graph_nodes(graph, keep_entry=True):
    """Remove nodes from a function graph (optionally keep FunctionEntry)."""
    try:
        nodes = list(graph.nodes)
    except Exception:
        return
    for node in nodes:
        cname = node.get_class().get_name()
        if keep_entry and cname in ("K2Node_FunctionEntry", "K2Node_FunctionResult"):
            continue
        try:
            graph.remove_node(node)
        except Exception:
            try:
                node.destroy_node()
            except Exception as exc:
                unreal.log_warning("remove_node {}: {}".format(cname, exc))


def _set_widget_class(node, widget_cls):
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
    # Fallback: set the Class input pin default object (matches editor Class dropdown).
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
        unreal.log_warning("Class pin fallback failed: {}".format(exc))
    return False


def _rebuild_show_ability_hud(bp, widget_cls):
    graph = _find_graph_by_name(bp, "ShowAbilityHUD")
    if graph is None:
        unreal.log_error("ShowAbilityHUD graph not found")
        return False

    unreal.log("Rebuilding ShowAbilityHUD graph…")
    _clear_graph_nodes(graph, keep_entry=True)

    entry = None
    for node in list(graph.nodes):
        if node.get_class().get_name() == "K2Node_FunctionEntry":
            entry = node
            break
    if entry is None:
        unreal.log_error("No FunctionEntry in ShowAbilityHUD")
        return False

    # Layout: Entry -> GetPC -> IsValid -> CreateWidget -> IsValid -> SetVar -> AddToViewport
    get_pc = _new_graph_node(graph, "/Script/BlueprintGraph.K2Node_CallFunction", 200, 0)
    create = _new_graph_node(graph, "/Script/UMGEditor.K2Node_CreateWidget", 700, 0)
    is_valid_pc = _new_graph_node(graph, "/Script/BlueprintGraph.K2Node_IfThenElse", 450, 0)
    # Prefer Macro IsValid if available; IfThenElse needs a bool — use IsValid macro instance instead.
    # K2Node_MacroInstance for IsValid is awkward; use CallFunction IsValid + Branch.

    # Simpler reliable path using CallFunction nodes we can configure:
    # Clear and use a more targeted approach with CreateWidget + known helpers.

    # Re-clear experimental failed nodes and use MacroInstance IsValid + CreateWidget
    _clear_graph_nodes(graph, keep_entry=True)
    entry = None
    for node in list(graph.nodes):
        if node.get_class().get_name() == "K2Node_FunctionEntry":
            entry = node
            break

    get_pc = _new_graph_node(graph, "/Script/BlueprintGraph.K2Node_CallFunction", 250, 50)
    create = _new_graph_node(graph, "/Script/UMGEditor.K2Node_CreateWidget", 900, 50)
    set_var = _new_graph_node(graph, "/Script/BlueprintGraph.K2Node_VariableSet", 1300, 0)
    add_vp = _new_graph_node(graph, "/Script/BlueprintGraph.K2Node_CallFunction", 1600, 0)

    if not all([get_pc, create]):
        unreal.log_error("Failed to spawn core nodes")
        return False

    # Configure GetPlayerController
    try:
        # Function: GameplayStatics.GetPlayerController
        fn = unreal.GameplayStatics.static_class().find_function_by_name("GetPlayerController")
        if fn:
            get_pc.set_from_function(fn)
        else:
            get_pc.set_editor_property(
                "function_reference",
                unreal.MemberReference(
                    member_parent=unreal.GameplayStatics.static_class(),
                    member_name="GetPlayerController",
                ),
            )
        get_pc.reconstruct_node()
    except Exception as exc:
        unreal.log_warning("configure GetPlayerController: {}".format(exc))
        # Fallback: set member name string props used by some UE versions
        try:
            get_pc.set_editor_property("function_reference", unreal.load_object(None, "/Script/Engine.Default__GameplayStatics"))
        except Exception:
            pass

    if not _set_widget_class(create, widget_cls):
        unreal.log_error("Could not set CreateWidget class")
        return False

    # Configure AddToViewport
    try:
        fn = unreal.Widget.static_class().find_function_by_name("AddToViewport")
        if fn:
            add_vp.set_from_function(fn)
            add_vp.reconstruct_node()
    except Exception as exc:
        unreal.log_warning("configure AddToViewport: {}".format(exc))

    # Configure Set AbilityHUDWidget
    try:
        set_var.set_editor_property("variable_name", "AbilityHUDWidget")
        set_var.reconstruct_node()
    except Exception as exc:
        unreal.log_warning("configure Set AbilityHUDWidget: {}".format(exc))

    schema = unreal.EdGraphSchema_K2()
    # Exec: Entry -> CreateWidget (skip IsValid for robustness; OwningPlayer can be None briefly)
    # Better: Entry then GetPC (pure) into Create OwningPlayer; Entry exec into Create
    _link(schema, entry, "then", create, "execute", "output", "input")
    # Data: GetPC return -> Create OwningPlayer
    # GetPC is pure (no exec) once configured
    _link(schema, get_pc, "ReturnValue", create, "OwningPlayer", "output", "input")
    # Create then -> Set var
    if set_var:
        _link(schema, create, "then", set_var, "execute", "output", "input")
        _link(schema, create, "ReturnValue", set_var, "AbilityHUDWidget", "output", "input")
        if add_vp:
            _link(schema, set_var, "then", add_vp, "execute", "output", "input")
            _link(schema, create, "ReturnValue", add_vp, "self", "output", "input")
            # ZOrder pin if present
            z = _pin(add_vp, "ZOrder", "input")
            if z is not None:
                try:
                    z.default_value = "100"
                except Exception:
                    try:
                        z.set_editor_property("default_value", "100")
                    except Exception:
                        pass
    else:
        if add_vp:
            _link(schema, create, "then", add_vp, "execute", "output", "input")
            _link(schema, create, "ReturnValue", add_vp, "self", "output", "input")

    unreal.log("ShowAbilityHUD rebuild attempted")
    return True


def _log_compile_status(bp):
    try:
        status = bp.status
        unreal.log("Blueprint status: {}".format(status))
    except Exception:
        pass
    try:
        status = bp.get_editor_property("status")
        unreal.log("Blueprint status prop: {}".format(status))
    except Exception:
        pass
    # Pull recent Blueprint compile errors from message log if possible
    try:
        # Force compile
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    except Exception as exc:
        unreal.log_warning("compile_blueprint: {}".format(exc))
    try:
        status = bp.status
        unreal.log("Blueprint status after compile: {}".format(status))
    except Exception:
        pass


def setup():
    _close_editors()

    if not unreal.EditorAssetLibrary.does_asset_exist(CHAR):
        unreal.log_error("Missing {}".format(CHAR))
        return False
    if not unreal.EditorAssetLibrary.does_asset_exist(WIDGET_BP):
        unreal.log_error("Missing {} — run setup_ability_bar_ui.py first".format(WIDGET_BP))
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

    unreal.log("--- DIAGNOSTIC dump ---")
    _dump_graphs(bp)
    _log_compile_status(bp)

    nodes = _find_create_widget_nodes(bp)
    unreal.log("Found {} CreateWidget-like node(s)".format(len(nodes)))

    fixed = 0
    for graph, node in nodes:
        if _set_widget_class(node, widget_cls):
            fixed += 1
            unreal.log(
                "Set WidgetClass on {} in {}".format(node.get_name(), graph.get_name())
            )

    if fixed == 0:
        unreal.log_warning("No CreateWidget to patch — rebuilding ShowAbilityHUD")
        if not _rebuild_show_ability_hud(bp, widget_cls):
            unreal.log_error(
                "Auto-rebuild failed. Open BP_TopDownCharacter > ShowAbilityHUD and "
                "add Create Widget (Class=WBP_AbilityBar) manually."
            )
            return False

    _log_compile_status(bp)
    ok = unreal.EditorAssetLibrary.save_asset(CHAR)
    unreal.log("Saved BP_TopDownCharacter: {}".format(ok))

    # Final node count check
    nodes2 = _find_create_widget_nodes(bp)
    unreal.log("CreateWidget nodes after fix: {}".format(len(nodes2)))
    return ok


if __name__ == "__main__":
    setup()
