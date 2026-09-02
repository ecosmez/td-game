#include "TDChampionDropLand.h"

FVector FTDChampionDropLand::ComputeLocationNearCrystal(
	const FVector& CrystalLocation,
	bool bHasAwayFromHint,
	const FVector& AwayFromHintLocation,
	float OffsetDistance)
{
	const float Offset = FMath::Max(0.f, OffsetDistance);
	FVector Direction(1.f, 0.f, 0.f);
	if (bHasAwayFromHint)
	{
		FVector Delta = CrystalLocation - AwayFromHintLocation;
		Delta.Z = 0.f;
		if (!Delta.IsNearlyZero())
		{
			Direction = Delta.GetSafeNormal();
		}
	}

	FVector Result = CrystalLocation + Direction * Offset;
	Result.Z = CrystalLocation.Z;
	return Result;
}
