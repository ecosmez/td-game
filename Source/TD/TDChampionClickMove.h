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
	 * Champion / overlay hits continue so the trace can reach walkable ground.
	 * Visible enemies issue an attack. Remaining blocking hits (Landscape, pads,
	 * kit meshes) issue a move to that impact — not a reject.
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
	 * Prefer a complete NavMesh path. If the click is much lower and unreachable,
	 * steer in XY so the champion can walk off a ledge. Otherwise, if the click
	 * projected onto NavMesh, use that — never fall through to an unprojected
	 * SimpleMove that fails silently. Last resort is DirectXY toward the click.
	 */
	static ETDChampionGroundMoveMode ChooseMoveMode(
		bool bHasCompleteNavPath,
		bool bHasNavProjection,
		float ChampionZ,
		float DestinationZ,
		float CliffDropFallbackZ);

	/** DirectXY keeps the raw click. NavMesh prefers the projected walkable point. */
	static FVector ResolveMoveDestination(
		const FVector& ClickLocation,
		ETDChampionGroundMoveMode Mode,
		bool bHasNavProjection,
		const FVector& ProjectedNavLocation);

	/** Returns the expanding/fading frame for a short-lived move destination pulse. */
	static bool CalculateMoveIndicatorFrame(
		float Elapsed,
		float Duration,
		float& OutScale,
		float& OutIntensity);
};
