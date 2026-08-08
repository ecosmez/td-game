#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "TDUIInputLibrary.generated.h"

/**
 * Shared helpers: block world click / move when the cursor is over UMG (store, HUD)
 * or when a tower drag/selection is active on a BuildManager.
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

	/** True when world click-to-move / 3D click events should be ignored. */
	UFUNCTION(BlueprintPure, Category = "TD|Input", meta = (WorldContext = "WorldContextObject"))
	static bool ShouldBlockWorldClickInput(const UObject* WorldContextObject, bool bCheckUI = true);

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
};
