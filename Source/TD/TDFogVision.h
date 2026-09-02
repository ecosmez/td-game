#pragma once

#include "CoreMinimal.h"

/** One circular vision source (champion, crystal, etc.). */
struct FTDFogVisionSource
{
	FVector Location = FVector::ZeroVector;
	float RadiusCm = 0.f;
};

/**
 * League-style live vision math: a point is clear only while inside a source
 * circle. Everything else is dim map — never a third "unexplored black" layer.
 */
struct FTDFogVision
{
	/** True when Location is inside any source's XY radius. */
	static bool IsLocationVisible(const FVector& Location, TArrayView<const FTDFogVisionSource> Sources);

	/** Champion vision requires both range and an unobstructed environment trace. */
	static bool IsChampionLocationVisible(bool bInsideRadius, bool bEnvironmentBlocksLineOfSight);

	/**
	 * Fog texel alpha: 0 inside the hard vision radius, DimAlpha outside,
	 * interpolated across the soft rim.
	 */
	static uint8 CompositeFogAlpha(float Distance2D, float RadiusCm, float Softness, uint8 DimAlpha);

	/** Minions are hidden when they are not in current vision. */
	static bool ShouldHideEnemy(bool bLocationVisible);

	/** Click traces skip fogged minions so the ground behind them can still be ordered. */
	static bool ShouldSkipClickThroughFoggedEnemy(bool bIsAttackableEnemy, bool bLocationVisible);
};
