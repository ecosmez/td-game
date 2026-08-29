#pragma once

#include "CoreMinimal.h"

/** Pure sky-drop landing math used by AMobaPlayerController. */
struct FTDChampionDropLand
{
	/**
	 * Point next to the main crystal, on the side opposite AwayFromHintLocation
	 * (typically the primary enemy spawner, so the drop is on the home/base side).
	 * If the hint is missing or coincident, offset along +X. Z is copied from the crystal.
	 */
	static FVector ComputeLocationNearCrystal(
		const FVector& CrystalLocation,
		bool bHasAwayFromHint,
		const FVector& AwayFromHintLocation,
		float OffsetDistance);
};
