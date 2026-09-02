#pragma once

#include "CoreMinimal.h"

class AActor;

enum class ETDChampionClickIntent : uint8
{
	IgnoreClick,
	Attack,
	MoveToHit,
	ContinueTrace
};

enum class ETDChampionGroundMoveMode : uint8
{
	NavMesh,
	DirectXY
};

/** Pure click-to-move decisions used by AMobaPlayerController. */
struct FTDChampionClickMove
{
	/**
	 * Champion / overlay / world-geometry hits continue so the trace can reach
	 * Landscape. Visible enemies issue an attack. Only Landscape issues movement.
	 */
	static ETDChampionClickIntent ClassifyHit(
		bool bHitActorIsChampion,
		bool bEnableAttack,
		bool bHitActorIsAttackableEnemy,
		bool bHitActorIsLandscape,
		bool bHitEnemyIsVisible = true,
		bool bHitActorIsClickThrough = false,
		bool bHitActorIsEnvironmentOccluder = false);

	/**
	 * Overlay actors (crystal collision hull, fog plane, aim preview) sit above
	 * Landscape and must not consume click or ability ground traces.
	 */
	static bool IsClickThroughActorName(const FString& ActorName, const FString& ClassName);

	/** True for placed kit meshes that should block the pawn; false for sky spheres. */
	static bool IsKitEnvironmentActorName(const FString& ActorName);

	/** Terrain collision is authored persistently; mutating it in Play would rebuild NavMesh. */
	static bool ShouldConfigureEnvironmentCollisionAtRuntime();

	/** Disable blocking collision so overlay hulls cannot intercept ground traces. */
	static void StripActorTraceCollision(AActor* Actor);

	/**
	 * 3DKit simple hulls are oversized blobs above Landscape. Use the render mesh
	 * as collision so the canyon floor stays walkable and the rocks stay solid.
	 */
	static void UseComplexCollisionOnEnvironmentMesh(AActor* Actor);

	/**
	 * Prefer a complete NavMesh path to the Landscape click. Otherwise steer in XY
	 * toward that exact point (cliff drops and unreachable nav both use DirectXY).
	 */
	static ETDChampionGroundMoveMode ChooseMoveMode(
		bool bHasCompleteNavPath,
		bool bHasNavProjection,
		float ChampionZ,
		float DestinationZ,
		float CliffDropFallbackZ);

	/** The move order is always the Landscape click, regardless of path mode. */
	static FVector ResolveMoveDestination(
		const FVector& ClickLocation,
		ETDChampionGroundMoveMode Mode,
		bool bHasNavProjection,
		const FVector& ProjectedNavLocation);

	/** Reject NavMesh candidates from a different floor, cliff top, or nearby mountain. */
	static bool IsNavProjectionNearClick(
		const FVector& ClickLocation,
		const FVector& ProjectedNavLocation,
		float MaxHorizontalDistance,
		float MaxVerticalDistance);

	/** Exact point first, followed by evenly spaced rings used to escape isolated NavMesh polygons. */
	static TArray<FVector2D> BuildNavSearchOffsets(
		float MaxRadius,
		float RadiusStep,
		int32 SamplesPerRing);

	/** Stable text used by RMB ray diagnostics in both the screen overlay and Output Log. */
	static FString BuildTraceDiagnostic(
		const FString& Stage,
		const FString& ActorName,
		const FString& ComponentName,
		const FString& ClassName,
		const FVector& ImpactPoint,
		bool bBlockingHit);

	/** Returns the expanding/fading frame for a short-lived move destination pulse. */
	static bool CalculateMoveIndicatorFrame(
		float Elapsed,
		float Duration,
		float& OutScale,
		float& OutIntensity);
};
