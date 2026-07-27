import unreal

def walk(w, depth=0):
    if w is None or depth > 8:
        return
    print("  "*depth + w.get_name() + " [" + w.get_class().get_name() + "]")
    try:
        for c in w.get_all_children():
            walk(c, depth+1)
    except Exception:
        pass

# Find likely roots via ObjectIterator
widgets = []
for obj in unreal.ObjectIterator(unreal.Widget):
    pkg = obj.get_package()
    if pkg and pkg.get_name() == "/Game/TD/UI/WBP_AbilityBar":
        if "WidgetTree." in obj.get_path_name() and "_C:" not in obj.get_path_name():
            widgets.append(obj)
print("count", len(widgets))
# Find widgets with no parent or CanvasPanel
for w in widgets:
    try:
        p = w.get_parent()
    except Exception:
        p = "ERR"
    if p is None:
        print("ROOT?", w.get_name(), w.get_class().get_name())
        walk(w)
