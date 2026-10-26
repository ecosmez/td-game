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

const TCHAR* FTDCaptureBaseLogic::BaseBuildingTypeName()
{
	return TEXT("Base");
}

bool FTDCaptureBaseLogic::IsBaseTypeName(const FString& Value)
{
	return Value.TrimStartAndEnd().Equals(BaseBuildingTypeName(), ESearchCase::IgnoreCase);
}

bool FTDCaptureBaseLogic::IsBaseBuildingClassPath(const FString& ClassPath)
{
	FString Name = ClassPath.TrimStartAndEnd();
	int32 Slash = INDEX_NONE;
	if (Name.FindLastChar(TEXT('/'), Slash))
	{
		Name = Name.Mid(Slash + 1);
	}
	int32 Dot = INDEX_NONE;
	if (Name.FindChar(TEXT('.'), Dot))
	{
		Name = Name.Mid(Dot + 1);
	}
	return Name.Equals(TEXT("BP_Tower_Base_C"), ESearchCase::IgnoreCase)
		|| Name.Equals(TEXT("BP_Building_Base_C"), ESearchCase::IgnoreCase);
}

bool FTDCaptureBaseLogic::IsBaseBuildingType(const FTDBuildingTypeQuery& Query)
{
	if (!Query.bSelectionActive)
	{
		return false;
	}

	if (!Query.DeclaredType.IsEmpty())
	{
		return IsBaseTypeName(Query.DeclaredType);
	}

	const bool bNameNamesAnotherTower = !Query.SelectedName.IsEmpty() && !IsBaseTypeName(Query.SelectedName);
	if (bNameNamesAnotherTower)
	{
		return false;
	}

	if (IsBaseTypeName(Query.SelectedName))
	{
		return true;
	}
	return IsBaseBuildingClassPath(Query.ClassPath);
}

bool FTDCaptureBaseLogic::CanPlaceBuildingOnPad(bool bIsStarterPad, const FTDBuildingTypeQuery& Query)
{
	if (!bIsStarterPad)
	{
		return true;
	}
	return IsBaseBuildingType(Query);
}

FString FTDCaptureBaseLogic::ResolveRememberedBuildingType(
	const FTDRememberedBuildingType& Remembered,
	const FString& SelectedName,
	const FString& ClassPath)
{
	const bool bHasName = !Remembered.SelectedName.IsEmpty();
	const bool bHasPath = !Remembered.ClassPath.IsEmpty();
	if (!bHasName && !bHasPath)
	{
		return FString();
	}

	const bool bNameOk = !bHasName
		|| Remembered.SelectedName.Equals(SelectedName, ESearchCase::IgnoreCase);
	const bool bPathOk = !bHasPath
		|| Remembered.ClassPath.Equals(ClassPath, ESearchCase::IgnoreCase);
	if (bNameOk && bPathOk)
	{
		return Remembered.BuildingType;
	}
	return FString();
}

bool FTDCaptureBaseLogic::IsPadLikeClassName(const FString& ClassName)
{
	return ClassName.Contains(TEXT("HexPad")) || ClassName.Contains(TEXT("TowerPad"));
}

bool FTDCaptureBaseLogic::ShouldCountOccupyingTower(
	bool bSameAsPad,
	bool bPadLike,
	bool bGhost,
	bool bHasBuiltFlag,
	bool bIsBuilt,
	float Dist2D,
	float OccupancyRadius)
{
	if (bSameAsPad || bPadLike || bGhost)
	{
		return false;
	}
	if (bHasBuiltFlag && !bIsBuilt)
	{
		return false;
	}
	return Dist2D <= OccupancyRadius;
}

FTDPadClickCollision FTDCaptureBaseLogic::ResolvePadClickCollision(bool bBuildable)
{
	FTDPadClickCollision Out;
	if (bBuildable)
	{
		Out.bActorCollision = true;
		Out.bQueryOnly = true;
		Out.bBlockVisibility = true;
	}
	return Out;
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
	Out.bExtraPadsVisible = Out.bHeld;
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
