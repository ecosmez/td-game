#include "TDChampionClickMove.h"
#include "TDFogVision.h"

ETDChampionClickIntent FTDChampionClickMove::ClassifyHit(
	bool bHitActorIsChampion,
	bool bEnableAttack,
	bool bHitActorIsAttackableEnemy,
	bool bHitActorIsLandscape,
	bool bHitEnemyIsVisible)
{
	if (bHitActorIsChampion)
	{
		return ETDChampionClickIntent::IgnoreClick;
	}
	if (FTDFogVision::ShouldSkipClickThroughFoggedEnemy(bHitActorIsAttackableEnemy, bHitEnemyIsVisible))
	{
		return ETDChampionClickIntent::ContinueTrace;
	}
	if (bEnableAttack && bHitActorIsAttackableEnemy)
	{
		return ETDChampionClickIntent::Attack;
	}
	if (bHitActorIsLandscape)
	{
		return ETDChampionClickIntent::MoveToHit;
	}
	return ETDChampionClickIntent::IgnoreClick;
}

ETDChampionGroundMoveMode FTDChampionClickMove::ChooseMoveMode(
	bool bHasCompleteNavPath,
	bool bHasNavProjection,
	float ChampionZ,
	float DestinationZ,
	float CliffDropFallbackZ)
{
	if (bHasCompleteNavPath)
	{
		return ETDChampionGroundMoveMode::NavMesh;
	}
	if ((ChampionZ - DestinationZ) >= CliffDropFallbackZ)
	{
		return ETDChampionGroundMoveMode::DirectXY;
	}
	if (bHasNavProjection)
	{
		return ETDChampionGroundMoveMode::NavMesh;
	}
	return ETDChampionGroundMoveMode::DirectXY;
}

FVector FTDChampionClickMove::ResolveMoveDestination(
	const FVector& ClickLocation,
	ETDChampionGroundMoveMode Mode,
	bool bHasNavProjection,
	const FVector& ProjectedNavLocation)
{
	if (Mode == ETDChampionGroundMoveMode::DirectXY)
	{
		return ClickLocation;
	}
	if (bHasNavProjection)
	{
		return ProjectedNavLocation;
	}
	return ClickLocation;
}
