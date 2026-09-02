#pragma once

#include "CoreMinimal.h"

/**
 * Top-down minimap projection: world XY ↔ texture UV, including a yaw offset
 * so the landscape can be shown rotated (45°) inside the square frame.
 */
struct FTDMinimapProjection
{
	/** SceneCapture pitch that looks straight down at the ground. */
	static constexpr float CapturePitch = -90.f;

	/** Legacy capture yaw used by the unrotated minimap (mirrors world X). */
	static constexpr float BaseCaptureYaw = -90.f;

	/** Capture rotator: look-down pitch, BaseCaptureYaw + MapYawOffset. */
	static FRotator MakeCaptureRotation(float MapYawOffsetDegrees);

	/**
	 * Square orthographic footprint covering WorldMin/WorldMax. A non-zero yaw
	 * grows the ortho by the rotated AABB so a square landscape still fits
	 * after a 45-degree turn (diamond in the frame).
	 */
	static void GetOrthoWorldRect(
		FVector2D WorldMin,
		FVector2D WorldMax,
		float ZoomFactor,
		float MapYawOffsetDegrees,
		float& OutCenterX,
		float& OutCenterY,
		float& OutOrthoWidth);

	/** World location → minimap UV in [0,1]. Matches MakeCaptureRotation. */
	static FVector2D WorldToNormalized(
		const FVector& WorldLoc,
		float CenterX,
		float CenterY,
		float Ortho,
		float MapYawOffsetDegrees);

	/** Inverse of WorldToNormalized (XY only). */
	static FVector2D NormalizedToWorldXY(
		const FVector2D& Normalized,
		float CenterX,
		float CenterY,
		float Ortho,
		float MapYawOffsetDegrees);

	/** Landscape actor bounds → padded minimap WorldMin/WorldMax. */
	static FBox2D FitLandscapeBounds(const FBox& LandscapeBounds, float Padding);
};
