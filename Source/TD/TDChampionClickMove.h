#pragma once

#include "CoreMinimal.h"

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
	 * Enemy hits issue an attack and Landscape hits issue a move. Any other first
	 * blocking hit rejects the click so objects cannot expose the ground below them.
	 */
	static ETDChampionClickIntent ClassifyHit(
		bool bHitActorIsChampion,
		bool bEnableAttack,
		bool bHitActorIsAttackableEnemy,
		bool bHitActorIsLandscape,
		bool bHitEnemyIsVisible = true);

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
};
