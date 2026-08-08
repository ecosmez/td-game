"""Aggressive patch for BP_TopDownCharacter Create Widget Class pin."""
import unreal

CHAR = "/Game/TopDown/Blueprints/BP_TopDownCharacter"
WIDGET_BP = "/Game/TD/UI/WBP_AbilityBar"
CPP_CLASS = "/Script/TD.AbilityBarWidget"


def resolve_widget_class():
    bp = unreal.EditorAssetLibrary.load_asset(WIDGET_BP)
    if bp:
        for call in (
            lambda: bp.generated_class(),
            lambda: bp.get_editor_property("generated_class"),
        ):
            try:
                gen = call()
                if gen:
                    return gen
            except Exception:
                pass
    cls = unreal.load_class(None, CPP_CLASS)
    return cls or unreal.load_object(None, CPP_CLASS)


def find_create_widget_nodes():
    found = []
    candidates = []
    # Prefer specific CreateWidget class
    for path in (
        "/Script/UMGEditor.K2Node_CreateWidget",
        "/Script/BlueprintGraph.K2Node_GenericCreateObject",
    ):
        cls = unreal.load_class(None, path)
        if cls is None:
            continue
        try:
            for obj in unreal.ObjectIterator(cls):
                candidates.append(obj)
        except Exception as exc:
            unreal.log_warning("iterate {}: {}".format(path, exc))
    # Fallback: any K2Node titled Create Widget
    try:
        for obj in unreal.ObjectIterator(unreal.K2Node):
            candidates.append(obj)
    except Exception as exc:
        unreal.log_warning("K2Node iter: {}".format(exc))

    seen = set()
    for obj in candidates:
        try:
            path = obj.get_path_name()
        except Exception:
            continue
        if "BP_TopDownCharacter" not in path:
            continue
        if path in seen:
            continue
        cname = obj.get_class().get_name()
        title = ""
        try:
            title = str(obj.get_node_title(unreal.NodeTitleType.FULL_TITLE))
        except Exception:
            pass
        if "CreateWidget" in cname or "Create Widget" in title:
            seen.add(path)
            found.append(obj)
            unreal.log("NODE {} class={} title={} outer={}".format(
                path, cname, title, obj.get_outer().get_path_name() if obj.get_outer() else None
            ))
            # Dump all properties
            try:
                for prop in dir(obj):
                    if "widget" in prop.lower() or "class" in prop.lower():
                        try:
                            val = getattr(obj, prop)
                            unreal.log("  attr {}.{} = {}".format(cname, prop, val))
                        except Exception:
                            pass
            except Exception:
                pass
            try:
                pins = list(obj.get_editor_property("pins"))
            except Exception:
                try:
                    pins = list(obj.pins)
                except Exception:
                    pins = []
            for pin in pins:
                try:
                    pname = str(pin.pin_name)
                    unreal.log(
                        "  pin {} dir={} def_obj={} def_val={} subtype={}".format(
                            pname,
                            getattr(pin, "direction", "?"),
                            getattr(pin, "default_object", None),
                            getattr(pin, "default_value", None),
                            getattr(pin, "pin_type", None),
                        )
                    )
                except Exception as exc:
                    unreal.log("  pin dump fail: {}".format(exc))
    return found


def set_class(node, widget_cls):
    attempts = []
    # Property setters
    for prop in (
        "widget_class",
        "WidgetClass",
        "class_to_spawn",
        "ClassToSpawn",
        "spawn_class",
        "SpawnClass",
    ):
        try:
            node.set_editor_property(prop, widget_cls)
            attempts.append("set_editor_property({}) ok".format(prop))
            try:
                node.reconstruct_node()
            except Exception:
                pass
            return True, attempts
        except Exception as exc:
            attempts.append("set_editor_property({}) fail: {}".format(prop, exc))
        try:
            setattr(node, prop, widget_cls)
            attempts.append("setattr({}) ok".format(prop))
            return True, attempts
        except Exception as exc:
            attempts.append("setattr({}) fail: {}".format(prop, exc))

    # Pins
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
        if pname.lower() not in ("class", "widgetclass", "class to spawn", "classtospawn"):
            continue
        for label, fn in (
            ("default_object attr", lambda: setattr(pin, "default_object", widget_cls)),
            ("default_object prop", lambda: pin.set_editor_property("default_object", widget_cls)),
            ("DefaultObject prop", lambda: pin.set_editor_property("DefaultObject", widget_cls)),
            ("default_value path", lambda: pin.set_editor_property("default_value", widget_cls.get_path_name())),
            ("default_value attr", lambda: setattr(pin, "default_value", widget_cls.get_path_name())),
            ("autogenerated_default_value", lambda: pin.set_editor_property("autogenerated_default_value", widget_cls.get_path_name())),
        ):
            try:
                fn()
                attempts.append("pin {} via {} ok".format(pname, label))
                try:
                    node.reconstruct_node()
                except Exception:
                    pass
                # Also try schema default value
                try:
                    schema = unreal.EdGraphSchema_K2.get_default_object()
                    schema.try_set_default_object(pin, widget_cls, True)
                    attempts.append("schema.try_set_default_object ok")
                except Exception as exc:
                    attempts.append("schema.try_set_default_object: {}".format(exc))
                try:
                    schema = unreal.EdGraphSchema_K2()
                    schema.try_set_default_object(pin, widget_cls)
                    attempts.append("schema() try_set ok")
                except Exception as exc:
                    attempts.append("schema() try_set: {}".format(exc))
                return True, attempts
            except Exception as exc:
                attempts.append("pin {} via {} fail: {}".format(pname, label, exc))
    return False, attempts


def destroy_node(node):
    attempts = []
    # Walk outers to find EdGraph
    outer = node.get_outer()
    graph = None
    while outer is not None:
        try:
            cname = outer.get_class().get_name()
        except Exception:
            cname = ""
        attempts.append("outer {}".format(cname))
        if "EdGraph" in cname or cname.endswith("Graph"):
            graph = outer
            break
        outer = outer.get_outer() if hasattr(outer, "get_outer") else None

    if graph is not None:
        for call in (
            lambda: graph.remove_node(node),
            lambda: graph.remove_node(node, True),
            lambda: graph.remove_node(node, True, True),
        ):
            try:
                call()
                attempts.append("graph.remove_node ok")
                return True, attempts
            except Exception as exc:
                attempts.append("graph.remove_node: {}".format(exc))

    try:
        node.destroy_node()
        attempts.append("destroy_node ok")
        return True, attempts
    except Exception as exc:
        attempts.append("destroy_node: {}".format(exc))

    try:
        unreal.SystemLibrary.begin_transaction("Remove CreateWidget")
        node.modify()
        node.destroy_node()
        unreal.SystemLibrary.end_transaction()
        attempts.append("tx destroy ok")
        return True, attempts
    except Exception as exc:
        attempts.append("tx destroy: {}".format(exc))

    # Last resort: mark broken pin ignored? not possible.
    return False, attempts


def compile_bp(bp):
    try:
        unreal.KismetEditorUtilities.compile_blueprint(bp)
    except Exception:
        try:
            unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        except Exception as exc:
            unreal.log_warning("compile fail: {}".format(exc))
    try:
        return str(bp.status)
    except Exception:
        try:
            return str(bp.get_editor_property("status"))
        except Exception:
            return "?"


def setup():
    try:
        aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
        if unreal.EditorAssetLibrary.does_asset_exist(CHAR):
            aes.close_all_editors_for_asset(unreal.EditorAssetLibrary.load_asset(CHAR))
    except Exception:
        pass

    widget_cls = resolve_widget_class()
    unreal.log("widget_cls={}".format(widget_cls.get_path_name() if widget_cls else None))
    if not widget_cls:
        return False

    bp = unreal.EditorAssetLibrary.load_asset(CHAR)
    # Force full load
    try:
        aes = unreal.get_editor_subsystem(unreal.AssetEditorSubsystem)
        aes.open_editor_for_assets([bp])
        aes.close_all_editors_for_asset(bp)
    except Exception as exc:
        unreal.log_warning("open editor: {}".format(exc))

    nodes = find_create_widget_nodes()
    unreal.log("found {}".format(len(nodes)))
    fixed = 0
    removed = 0
    for node in nodes:
        ok, attempts = set_class(node, widget_cls)
        for a in attempts:
            unreal.log("  set: {}".format(a))
        if ok:
            fixed += 1
        else:
            ok2, attempts2 = destroy_node(node)
            for a in attempts2:
                unreal.log("  del: {}".format(a))
            if ok2:
                removed += 1

    status = compile_bp(bp)
    unreal.log("status after patch: {}".format(status))

    # If still error, try destroy all found again
    if "ERROR" in status.upper():
        nodes = find_create_widget_nodes()
        for node in nodes:
            ok2, attempts2 = destroy_node(node)
            for a in attempts2:
                unreal.log("  del2: {}".format(a))
            if ok2:
                removed += 1
        status = compile_bp(bp)

    # Break remaining pin links that reference None widget by refreshing nodes
    try:
        unreal.BlueprintEditorLibrary.refresh_all_nodes(bp)
    except Exception:
        pass
    status = compile_bp(bp)

    ok = unreal.EditorAssetLibrary.save_asset(CHAR)
    unreal.log("HARDFIX fixed={} removed={} saved={} status={}".format(fixed, removed, ok, status))
    return "ERROR" not in status.upper()


if __name__ == "__main__":
    setup()
