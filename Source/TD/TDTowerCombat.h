#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TDTowerCombat.generated.h"

/** One actor hit along a tower shot line, already resolved to Blueprint flags. */
struct FTDTowerShotHitInfo
{
	FString ClassName;
	bool bIsSelf = false;
	bool bIsGhost = false;
	bool bHasBuiltFlag = false;
	bool bIsBuilt = true;
};

/**
 * Tower combat LoS: other finished towers block shots. Terrain, pads, ghosts,
 * and the firing tower itself do not.
 */
struct FTDTowerCombat
{
	/** True for placed tower Blueprints (Arrow, Wall, …), not pads or widgets. */
	static bool IsTowerShotBlockerClassName(const FString& ClassName);

	/** True when this hit should stop the shot from reaching an enemy. */
	static bool DoesHitBlockTowerShot(
		const FString& ClassName,
		bool bIsSelf,
		bool bIsGhost,
		bool bHasBuiltFlag,
		bool bIsBuilt);

	/** Walks hits from nearest to farthest; any blocking tower occludes the shot. */
	static bool IsShotBlockedByHits(TArrayView<const FTDTowerShotHitInfo> Hits);
};

UCLASS()
class TD_API UTDTowerCombatLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * True when another finished tower sits on the Visibility trace from Origin
	 * to TargetLocation. Rocky pads and terrain are ignored so they cannot
	 * silence in-range enemies.
	 */
	UFUNCTION(BlueprintPure, Category = "TD|TowerCombat", meta = (WorldContext = "WorldContextObject", DefaultToSelf = "WorldContextObject"))
	static bool IsShotBlockedByOtherTower(
		const UObject* WorldContextObject,
		FVector Origin,
		FVector TargetLocation);
};
