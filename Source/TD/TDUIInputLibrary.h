#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TDUIInputLibrary.generated.h"

/**
 * Shared helpers: block world click events over UMG, and block champion
 * click-to-move over UMG or while a tower is selected/dragging.
 */
UCLASS()
class TD_API UTDUIInputLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** True when cursor is over a hit-testable UMG / interactive widget. */
	UFUNCTION(BlueprintPure, Category = "TD|Input", meta = (WorldContext = "WorldContextObject"))
	static bool IsPointerOverHitTestableUI(const UObject* WorldContextObject);

	/** True when BP_BuildManager is dragging or has a tower selected. */
	UFUNCTION(BlueprintPure, Category = "TD|Input", meta = (WorldContext = "WorldContextObject"))
	static bool IsTowerBuildInteractionActive(const UObject* WorldContextObject);

	/**
	 * True when 3D actor click events (pad OnClicked) should be ignored.
	 * UI under the cursor only — tower selection must keep click events enabled
	 * so pads can receive LMB place.
	 */
	UFUNCTION(BlueprintPure, Category = "TD|Input", meta = (WorldContext = "WorldContextObject"))
	static bool ShouldBlockWorldClickInput(const UObject* WorldContextObject, bool bCheckUI = true);

	/**
	 * True when champion click-to-move / set-destination should be ignored.
	 * Blocks over hit-testable UI and while a tower is selected/dragging so RMB
	 * can cancel placement without issuing a move order.
	 */
	UFUNCTION(BlueprintPure, Category = "TD|Input", meta = (WorldContext = "WorldContextObject"))
	static bool ShouldBlockChampionClickToMove(const UObject* WorldContextObject, bool bCheckUI = true);

	/**
	 * Create the tower store HUD (WBP_TowerStore if available, else TowerStoreWidget C++).
	 * Avoids BP "Create Widget must have a class specified" compile failures.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|UI", meta = (WorldContext = "WorldContextObject"))
	static UUserWidget* CreateTowerStoreWidget(UObject* WorldContextObject, APlayerController* OwningPlayer = nullptr);

	/**
	 * Create the tower store, add to viewport (closed strip), GameAndUI + show cursor.
	 * Opens via ability bar + / SetStoreOpen. Returns widget for BuildManager cache.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|UI", meta = (WorldContext = "WorldContextObject"))
	static UUserWidget* CreateAndShowTowerStore(
		UObject* WorldContextObject,
		APlayerController* OwningPlayer = nullptr,
		int32 ZOrder = 120);

	/**
	 * Create the ability HUD (WBP_AbilityBar if available, else AbilityBarWidget C++).
	 * Avoids BP "Create Widget must have a class specified" compile failures on ShowAbilityHUD.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|UI", meta = (WorldContext = "WorldContextObject"))
	static UUserWidget* CreateAbilityBarWidget(UObject* WorldContextObject, APlayerController* OwningPlayer = nullptr);

	/**
	 * Create the ability bar and add it to the viewport (ZOrder default 100).
	 * Prefer this in BP ShowAbilityHUD / landing flow over Create Widget nodes.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|UI", meta = (WorldContext = "WorldContextObject"))
	static UUserWidget* CreateAndShowAbilityBar(
		UObject* WorldContextObject,
		APlayerController* OwningPlayer = nullptr,
		int32 ZOrder = 100);
};
