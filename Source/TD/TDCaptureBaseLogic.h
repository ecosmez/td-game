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

struct FTDPadClickCollision
{
	bool bActorCollision = false;
	bool bQueryOnly = false;
	bool bBlockVisibility = false;
};

/** The building the player is currently trying to place. */
struct FTDBuildingTypeQuery
{
	/** False after cancel, or when no tower is selected. */
	bool bSelectionActive = false;
	FString SelectedName;
	FString ClassPath;
	/** Store BuildingType, or the tower BuildingType / TowerTypeName. */
	FString DeclaredType;
};

/** Last store selection, so a catalog BuildingType stays tied to that tower. */
struct FTDRememberedBuildingType
{
	FString SelectedName;
	FString ClassPath;
	FString BuildingType;
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

	/**
	 * Building type name for the towers that may be placed on a capture base's
	 * 3 starter pads. Extra pads and pads that do not belong to a base ignore it.
	 */
	static const TCHAR* BaseBuildingTypeName();

	/** True when Value is the Base building type (case-insensitive, trimmed). */
	static bool IsBaseTypeName(const FString& Value);

	/** True when ClassPath is a dedicated Base building class, not BP_Tower or BP_Trap_Base. */
	static bool IsBaseBuildingClassPath(const FString& ClassPath);

	/** True when the active selection is a Base-type building. */
	static bool IsBaseBuildingType(const FTDBuildingTypeQuery& Query);

	/**
	 * Starter pads accept only the Base building type.
	 * Every other pad accepts whatever building is selected.
	 */
	static bool CanPlaceBuildingOnPad(bool bIsStarterPad, const FTDBuildingTypeQuery& Query);

	/**
	 * Returns the remembered store BuildingType when the current selection still
	 * matches that store click. Empty when the selection has moved on.
	 */
	static FString ResolveRememberedBuildingType(
		const FTDRememberedBuildingType& Remembered,
		const FString& SelectedName,
		const FString& ClassPath);

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

	/**
	 * Click traces for Arrow-style pad placement use ECC_Visibility.
	 * Buildable pads must re-assert QueryOnly + Visibility block after
	 * SetActorEnableCollision(true), because HideAllPads can save NoCollision
	 * if it runs before BP_HexPad BeginPlay.
	 */
	static FTDPadClickCollision ResolvePadClickCollision(bool bBuildable);

	static FTDCaptureBaseOutput Step(const FTDCaptureBaseInput& Input);
};

struct FTDVisionSourceList
{
	/** Remove every matching actor key. Returns how many entries were removed. */
	static int32 Unregister(TArray<uint64>& Keys, uint64 ActorKey);
};
