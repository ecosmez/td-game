#pragma once

#include "CoreMinimal.h"

struct FTDCaptureBaseInput
{
	bool bChampionInRadius = false;
	int32 EnemiesInRadius = 0;
	float DeltaSeconds = 0.f;
	float ChannelDuration = 5.f;
	int32 StarterTowersAlive = 0;
	int32 PreviousStarterTowersAlive = 0;
	float ChannelProgress = 0.f;
	bool bHeld = false;
	bool bExtraPadsUnlocked = false;
	bool bChannelCompletedThisLife = false;
};

struct FTDCaptureBaseOutput
{
	float ChannelProgress = 0.f;
	bool bHeld = false;
	bool bExtraPadsUnlocked = false;
	bool bChannelCompletedThisLife = false;
	bool bVisionOn = false;
	bool bExtraTowersPowered = false;
	bool bStarterPadsVisible = false;
	bool bStarterPadsBuildable = false;
	bool bExtraPadsVisible = false;
	bool bExtraPadsBuildable = false;
	bool bShowChannelBar = false;
	bool bShouldRegisterVision = false;
	bool bShouldUnregisterVision = false;
};

struct FTDCaptureBaseLogic
{
	static bool ShouldFillChannel(
		bool bChampionInRadius,
		int32 EnemiesInRadius,
		int32 StarterTowersAlive,
		bool bChannelCompletedThisLife);

	static float AdvanceChannel(float Progress, float DeltaSeconds, float Duration, bool bFilling);

	static bool IsInsideRadius(const FVector& Origin, const FVector& Point, float RadiusCm);

	static bool ArePadAssignmentsValid(
		int32 StarterCount,
		int32 ExtraCount,
		int32 UniqueCount,
		bool bAnyNull);

	static bool IsPadBuildable(bool bIsStarter, bool bOccupied, const FTDCaptureBaseOutput& State);

	/** True when an actor is a pad, not a placed tower (HexPad / TowerPad). */
	static bool IsPadLikeClassName(const FString& ClassName);

	/** True when a nearby actor should occupy a capture pad. */
	static bool ShouldCountOccupyingTower(
		bool bSameAsPad,
		bool bPadLike,
		bool bGhost,
		bool bHasBuiltFlag,
		bool bIsBuilt,
		float Dist2D,
		float OccupancyRadius);

	static FTDCaptureBaseOutput Step(const FTDCaptureBaseInput& Input);
};

struct FTDVisionSourceList
{
	/** Remove every matching actor key. Returns how many entries were removed. */
	static int32 Unregister(TArray<uint64>& Keys, uint64 ActorKey);
};
