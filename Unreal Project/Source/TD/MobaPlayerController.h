#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MobaPlayerController.generated.h"

class AMobaCameraPawn;
class UMinimapWidget;
class UInputMappingContext;

/**
 * MOBA player controller: free RTS camera + separately tracked champion pawn.
 * Does not attach the camera to the champion.
 */
UCLASS(Blueprintable, BlueprintType)
class TD_API AMobaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMobaPlayerController();

	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void OnPossess(APawn* InPawn) override;
	virtual void OnUnPossess() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void PlayerTick(float DeltaTime) override;

	/** Gameplay champion (not necessarily the possessed pawn when using free camera). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Champion")
	TObjectPtr<APawn> ControlledChampion;

	/** Camera pawn class used when spawning a free camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera")
	TSubclassOf<AMobaCameraPawn> CameraPawnClass;

	/**
	 * Optional champion class. When set and ControlledChampion is null at begin play,
	 * a champion is spawned so Space focus / gameplay have a target while the camera is possessed.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Champion")
	TSubclassOf<APawn> ChampionClass;

	/** RMB click-to-move for the controlled champion (uses AI move). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Champion")
	bool bEnableClickToMoveChampion = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Champion")
	TEnumAsByte<ECollisionChannel> ClickMoveTraceChannel = ECC_Visibility;

	/** When true, controller possesses the camera pawn and keeps the champion via ControlledChampion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera")
	bool bPossessCameraPawn = true;

	/** Spawn a camera pawn if none is possessed / already exists. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera")
	bool bAutoSpawnCameraPawn = true;

	/** Place spawned camera above the champion on begin play. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera")
	bool bSnapCameraToChampionOnStart = true;

	/** Optional height for the free-camera pivot (world Z). 0 = use champion Z / current. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera")
	float CameraPivotHeight = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera")
	bool bBlockCameraInputOverUI = true;

	/** IMC for MOBA camera (optional – camera pawn also loads / creates defaults). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Input")
	TObjectPtr<UInputMappingContext> CameraMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Input")
	int32 CameraMappingPriority = 1;

	/** Show LoL-style minimap (bottom-right). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba HUD")
	bool bShowMinimap = true;

	/** Optional custom minimap class; defaults to UMinimapWidget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba HUD")
	TSubclassOf<UMinimapWidget> MinimapWidgetClass;

	UFUNCTION(BlueprintCallable, Category = "Moba HUD")
	UMinimapWidget* ShowMinimap();

	UFUNCTION(BlueprintCallable, Category = "Moba HUD")
	void HideMinimap();

	UFUNCTION(BlueprintPure, Category = "Moba HUD")
	UMinimapWidget* GetMinimapWidget() const { return MinimapWidget; }

	UFUNCTION(BlueprintPure, Category = "Moba Camera")
	APawn* GetControlledChampion() const;

	UFUNCTION(BlueprintCallable, Category = "Moba Camera")
	void SetControlledChampion(APawn* NewChampion);

	UFUNCTION(BlueprintPure, Category = "Moba Camera")
	AMobaCameraPawn* GetMobaCameraPawn() const;

	UFUNCTION(BlueprintCallable, Category = "Moba Camera")
	AMobaCameraPawn* GetOrSpawnCameraPawn();

	/** True when pointer is over hit-testable UI (store, HUD, minimap, etc.). */
	UFUNCTION(BlueprintPure, Category = "Moba Camera")
	bool ShouldBlockCameraInputForUI() const;

	/** True when world click/drag input should be ignored (UI or store interaction). */
	UFUNCTION(BlueprintPure, Category = "Moba Camera")
	bool ShouldBlockWorldClickInput() const;

	/** Use GameAndUI so HUD / abilities remain clickable with a free camera. */
	UFUNCTION(BlueprintCallable, Category = "Moba Camera")
	void ApplyMobaInputMode();

	/** Issue a ground move to the controlled champion (SimpleMoveToLocation). */
	UFUNCTION(BlueprintCallable, Category = "Moba Camera")
	void MoveChampionToLocation(const FVector& WorldLocation);

protected:
	void InitializeMobaCamera();
	void WireChampionFromPawn(APawn* InPawn);
	void HandleClickToMoveChampion();
	void EnsureChampionHasAIController(APawn* Champion);

	UPROPERTY(Transient)
	TObjectPtr<AMobaCameraPawn> CachedCameraPawn;

	UPROPERTY(Transient)
	TObjectPtr<UMinimapWidget> MinimapWidget;
};
