"""Stop kit rocks/pads from carving Landscape NavMesh holes.

Keeps physical collision (pawn still bumps rocks) but sets
CanEverAffectNavigation=False so Recast builds continuous green on Landscape.
"""

import unreal


NEEDLES = ("BigCliff", "TallRock", "Placement")


def setup():
	world = unreal.EditorLevelLibrary.get_editor_world()
	if not world:
		raise RuntimeError("No editor world")

	updated = 0
	for actor in unreal.EditorLevelLibrary.get_all_level_actors():
		if not actor:
			continue
		label = actor.get_actor_label()
		if not any(needle in label for needle in NEEDLES):
			continue
		for comp in actor.get_components_by_class(unreal.StaticMeshComponent):
			if not comp:
				continue
			if comp.get_editor_property("can_ever_affect_navigation"):
				comp.set_editor_property("can_ever_affect_navigation", False)
				updated += 1
				unreal.NavigationSystemV1.update_component_data(comp)

	unreal.log("DISABLE_TERRAIN_NAV_CARVE updated_comps={}".format(updated))
	return updated


setup()
