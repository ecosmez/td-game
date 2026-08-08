import unreal

bp = unreal.EditorAssetLibrary.load_asset('/Game/TopDown/TowersWidgetBlueprint')
print('type', type(bp))
print('attrs', [x for x in dir(bp) if ('idget' in x.lower()) or ('tree' in x.lower()) or ('Tree' in x) or ('Root' in x)])
try:
    print('WidgetTree prop', bp.get_editor_property('WidgetTree'))
except Exception as e:
    print('WidgetTree err', e)
# try Subobject
try:
    print('widget_tree', getattr(bp, 'widget_tree', 'NO'))
except Exception as e:
    print(e)
# Generated class CDO?
try:
    gen = bp.generated_class()
    print('gen', gen)
except Exception as e:
    print('gen err', e)
