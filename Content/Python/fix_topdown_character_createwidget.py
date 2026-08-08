"""
Fix BP_TopDownCharacter Create Widget compile errors (Class pin wiped).

Uses ObjectIterator to find K2Node_CreateWidget when graph arrays are empty (UE 5.8).
Sets WidgetClass aggressively; if that fails, destroys the node via its EdGraph outer.
"""
import unreal

CHAR = "/Game/TopDown/Blueprints/BP_TopDownCharacter"
WIDGET_BP = "/Game/TD/UI/WBP_AbilityBar"
CPP_CLASS = "/Script/TD.AbilityBarWidget"


def _resolve_widget_class():
    bp = unreal.EditorAssetLibrary.load_asset(WIDGET_BP)
    if bp is not None:
        for call in (
            lambda: bp.generated_class(),
            lambda: bp.get_editor_property("generated_class"),
        ):
            try:
                gen = call()
                if gen is not None:
                    return gen
            except Exception:
                pass
    cls = unreal.load_class(None, CPP_CLASS)
    if cls is None:
        cls = unreal.load_object(None, CPP_CLASS)
    return cls


def _find_create_widget_nodes():
    found = []
    seen = set()

    def consider(obj):
        try:
            path = obj.get_path_name()
        except Exception:
            return
        if "BP_TopDownCharacter" not in path or path in seen:
            return
        try:
            cname = obj.get_class().get_name()
        except Exception:
            return
        title = ""
        try:
            title = str(obj.get_node_title(unreal.NodeTitleType.FULL_TITLE))
        except Exception:
            pass
        if "CreateWidget" not in cname and "Create Widget" not in title:
            return
        seen.add(path)
        found.append(obj)
        unreal.log("Found {} ({}) outer={}".format(
            path,
            title or cname,
            obj.get_outer().get_path_name() if obj.get_outer() else "?",
        ))

    # Specific class iterator first
    try:
        cw = unreal.load_class(None, "/Script/UMGEditor.K2Node_CreateWidget")
        if cw:
            for obj in unreal.ObjectIterator(cw):
                consider(obj)
    except Exception as exc:
        unreal.log_warning("CreateWidget iterator: {}".format(exc))

    try:
        for obj in unreal.ObjectIterator(unreal.K2Node):
            consider(obj)
    except Exception as exc:
        unreal.log_warning("K2Node iterator: {}".format(exc))

    return found


def _set_widget_class(node, widget_cls):
    for prop in ("widget_class", "WidgetClass", "class_to_spawn", "ClassToSpawn"):
        try:
            node.set_editor_property(prop, widget_cls)
            try:
                node.reconstruct_node()
            except Exception:
                pass
            unreal.log("Set property {} on {}".format(prop, node.get_name()))
            return True
        except Exception as exc:
            unreal.log_warning("prop {}: {}".format(prop, exc))

    try:
        pins = list(node.get_editor_property("pins"))
    except Exception:
        try:
            pins = list(node.pins)
        except Exception:
            pins = []

    for pin in pins:
        try:
            pname = str(pin.pin_name)
        except Exception:
            continue
        if pname.lower() not in ("class", "widgetclass", "class to spawn"):
            continue

        # Schema helpers (most reliable in graph editor)
        schema = None
        try:
            schema = unreal.EdGraphSchema_K2.get_default_object()
        except Exception:
            try:
                schema = unreal.EdGraphSchema_K2()
            except Exception:
                schema = None

        if schema is not None:
            for args in (
                (pin, widget_cls, True),
                (pin, widget_cls),
            ):
                try:
                    schema.try_set_default_object(*args)
                    try:
                        node.reconstruct_node()
                    except Exception:
                        pass
                    unreal.log("schema.try_set_default_object on {}".format(pname))
                    return True
                except Exception as exc:
                    unreal.log_warning("schema set: {}".format(exc))

        for label, fn in (
            ("default_object", lambda: pin.set_editor_property("default_object", widget_cls)),
            ("DefaultObject", lambda: pin.set_editor_property("DefaultObject", widget_cls)),
            ("default_value", lambda: pin.set_editor_property("default_value", widget_cls.get_path_name())),
        ):
            try:
                fn()
                try:
                    node.reconstruct_node()
                except Exception:
                    pass
                unreal.log("Set pin {} via {}".format(pname, label))
                return True
            except Exception as exc:
                unreal.log_warning("pin {} {}: {}".format(pname, label, exc))
    return False


def _destroy_node(node):
    # Find UEdGraph outer
    outer = node.get_outer()
    graph = None
    depth = 0
    while outer is not None and depth < 8:
        try:
            cname = outer.get_class().get_name()
        except Exception:
            cname = ""
        unreal.log("outer[{}]={}".format(depth, cname))
        if "Graph" in cname:
            graph = outer
            break
        try:
            outer = outer.get_outer()
        except Exception:
            break
        depth += 1

    if graph is not None:
        for kwargs in (
            {"node": node},
            {},
        ):
            try:
                if kwargs:
                    graph.remove_node(node)
                else:
                    graph.remove_node(node, True)
                unreal.log("Removed node via graph.remove_node")
                return True
            except TypeError:
                try:
                    graph.remove_node(node, True, True)
                    unreal.log("Removed node via graph.remove_node(3args)")
                    return True
                except Exception as exc:
                    unreal.log_warning("remove_node args: {}".format(exc))
            except Exception as exc:
                unreal.log_warning("remove_node: {}".format(exc))

    try:
        node.modify(True)
        node.destroy_node()
        unreal.log("Destroyed node via destroy_node()")
        return True
    except Exception as exc:
        unreal.log_warning("destroy_node: {}".format(exc))
    return False


def _compile(bp):
    try:
        unreal.KismetEditorUtilities.compile_blueprint(bp)
    except Exception:
        try:
            unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        except Exception as exc:
            unreal.log_warning("compile: {}".format(exc))
    try:
        st = bp.status
    except Exception:
        try:
            st = bp.get_editor_property("status")
        except Exception:
            st = "?"
    unreal.log("Blueprint status: {}".format(st))
    return str(st)


def setup():
    try:
        aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
        if unreal.EditorAssetLibrary.does_asset_exist(CHAR):
            aes.close_all_editors_for_asset(unreal.EditorAssetLibrary.load_asset(CHAR))
    except Exception as exc:
        unreal.log_warning("close editors: {}".format(exc))

    if not unreal.EditorAssetLibrary.does_asset_exist(CHAR):
        unreal.log_error("Missing {}".format(CHAR))
        return False
    if not unreal.EditorAssetLibrary.does_asset_exist(WIDGET_BP):
        unreal.log_error("Missing {} — run setup_ability_bar_ui once".format(WIDGET_BP))
        return False

    widget_cls = _resolve_widget_class()
    if widget_cls is None:
        unreal.log_error("Could not resolve ability bar widget class")
        return False
    unreal.log("Widget class: {}".format(widget_cls.get_path_name()))

    bp = unreal.EditorAssetLibrary.load_asset(CHAR)
    if bp is None:
        unreal.log_error("Failed to load BP_TopDownCharacter")
        return False

    # Open then close so package fully loads nodes into memory
    try:
        aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
        aes.open_editor_for_assets([bp])
        aes.close_all_editors_for_asset(bp)
    except Exception as exc:
        unreal.log_warning("open/close: {}".format(exc))

    nodes = _find_create_widget_nodes()
    unreal.log("CreateWidget count: {}".format(len(nodes)))

    fixed = 0
    removed = 0
    for node in nodes:
        if _set_widget_class(node, widget_cls):
            fixed += 1
        elif _destroy_node(node):
            removed += 1
            unreal.log("Removed unpatchable Create Widget (HUD still comes from C++ land path)")
        else:
            unreal.log_error("Could not fix {}".format(node.get_name()))

    status = _compile(bp)

    if "ERROR" in status.upper():
        # Nuclear second pass — destroy anything left
        for node in _find_create_widget_nodes():
            if _destroy_node(node):
                removed += 1
        status = _compile(bp)

    ok = unreal.EditorAssetLibrary.save_asset(CHAR)
    unreal.log("RESULT fixed={} removed={} saved={} status={}".format(fixed, removed, ok, status))
    return ok


if __name__ == "__main__":
    setup()
