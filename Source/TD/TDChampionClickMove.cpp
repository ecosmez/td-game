#include "TDChampionClickMove.h"

ETDChampionClickIntent FTDChampionClickMove::ClassifyHit(
	bool bHitActorIsChampion,
	bool bEnableAttack,
	bool bHitActorIsAttackableEnemy)
{
	if (bHitActorIsChampion)
	{
		return ETDChampionClickIntent::SkipHit;
	}
	if (bEnableAttack && bHitActorIsAttackableEnemy)
	{
		return ETDChampionClickIntent::Attack;
	}
	return ETDChampionClickIntent::MoveToHit;
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
