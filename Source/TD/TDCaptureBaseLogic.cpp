#include "TDCaptureBaseLogic.h"

bool FTDCaptureBaseLogic::ShouldFillChannel(
	bool bChampionInRadius,
	int32 EnemiesInRadius,
	int32 StarterTowersAlive,
	bool bChannelCompletedThisLife)
{
	return bChampionInRadius
		&& EnemiesInRadius <= 0
		&& StarterTowersAlive <= 0
		&& !bChannelCompletedThisLife;
}

float FTDCaptureBaseLogic::AdvanceChannel(float Progress, float DeltaSeconds, float Duration, bool bFilling)
{
	if (!bFilling)
	{
		return Progress;
	}

	const float SafeDuration = FMath::Max(Duration, KINDA_SMALL_NUMBER);
	return FMath::Clamp(Progress + DeltaSeconds / SafeDuration, 0.f, 1.f);
}

bool FTDCaptureBaseLogic::IsInsideRadius(const FVector& Origin, const FVector& Point, float RadiusCm)
{
	if (RadiusCm <= KINDA_SMALL_NUMBER)
	{
		return false;
	}
	return FVector::DistSquared2D(Origin, Point) <= FMath::Square(RadiusCm);
}

bool FTDCaptureBaseLogic::ArePadAssignmentsValid(
	int32 StarterCount,
	int32 ExtraCount,
	int32 UniqueCount,
	bool bAnyNull)
{
	if (bAnyNull || StarterCount != 3 || ExtraCount < 0)
	{
		return false;
	}
	return UniqueCount == StarterCount + ExtraCount;
}

bool FTDCaptureBaseLogic::IsPadBuildable(bool bIsStarter, bool bOccupied, const FTDCaptureBaseOutput& State)
{
	if (bOccupied)
	{
		return false;
	}
	return bIsStarter ? State.bStarterPadsBuildable : State.bExtraPadsBuildable;
}

FTDCaptureBaseOutput FTDCaptureBaseLogic::Step(const FTDCaptureBaseInput& Input)
{
	FTDCaptureBaseOutput Out;
	Out.bHeld = Input.bHeld;
	Out.bExtraPadsUnlocked = Input.bExtraPadsUnlocked;
	Out.bChannelCompletedThisLife = Input.bChannelCompletedThisLife;
	Out.ChannelProgress = Input.ChannelProgress;

	const int32 Starters = FMath::Max(0, Input.StarterTowersAlive);
	const int32 Previous = FMath::Max(0, Input.PreviousStarterTowersAlive);

	if (Previous > 0 && Starters == 0)
	{
		Out.bHeld = false;
		Out.bChannelCompletedThisLife = false;
		Out.ChannelProgress = 0.f;
	}

	if (Starters >= 3)
	{
		Out.bHeld = true;
		Out.bExtraPadsUnlocked = true;
	}

	const bool bFilling = ShouldFillChannel(
		Input.bChampionInRadius,
		Input.EnemiesInRadius,
		Starters,
		Out.bChannelCompletedThisLife);
	Out.ChannelProgress = AdvanceChannel(
		Out.ChannelProgress,
		Input.DeltaSeconds,
		Input.ChannelDuration,
		bFilling);
	if (Out.ChannelProgress >= 1.f - KINDA_SMALL_NUMBER)
	{
		Out.ChannelProgress = 1.f;
		if (Starters == 0)
		{
			Out.bChannelCompletedThisLife = true;
		}
	}

	Out.bVisionOn = Out.bHeld;
	Out.bExtraTowersPowered = Out.bHeld;
	Out.bStarterPadsBuildable = Out.bChannelCompletedThisLife && Starters < 3;
	Out.bStarterPadsVisible = Out.bStarterPadsBuildable;
	Out.bExtraPadsVisible = Out.bExtraPadsUnlocked;
	Out.bExtraPadsBuildable = Out.bHeld;
	Out.bShowChannelBar = Starters == 0
		&& (Out.ChannelProgress > KINDA_SMALL_NUMBER || Input.bChampionInRadius);
	Out.bShouldRegisterVision = Out.bHeld && !Input.bHeld;
	Out.bShouldUnregisterVision = !Out.bHeld && Input.bHeld;
	return Out;
}

int32 FTDVisionSourceList::Unregister(TArray<uint64>& Keys, uint64 ActorKey)
{
	if (ActorKey == 0)
	{
		return 0;
	}

	const int32 Before = Keys.Num();
	Keys.RemoveAll([ActorKey](uint64 Key)
	{
		return Key == ActorKey;
	});
	return Before - Keys.Num();
}
