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

/**
 * Editor / play debug rings for crystal and capture-base vision circles.
 * Fog itself is XY; the preview uses the same plane so designers can see coverage.
 */
struct FTDVisionPreview
{
	static constexpr float DefaultCrystalRadiusCm = 12000.f;
	static constexpr float DefaultCaptureBaseRadiusCm = 10000.f;
	static constexpr float GroundHeightOffsetCm = 20.f;
	static constexpr int32 SegmentCount = 64;
	static constexpr float Thickness = 12.f;

	static bool ShouldDraw(bool bIsGameWorld, bool bEnabledInEditor, bool bEnabledInPlay);
	static FVector CircleAxisX();
	static FVector CircleAxisY();
	static FVector GroundLocation(const FVector& ActorLocation);
	static FColor CrystalColor();
	static FColor CaptureBaseColor();

	static void DrawCircle(const UWorld* World, const FVector& ActorLocation, float RadiusCm, const FColor& Color);
};
