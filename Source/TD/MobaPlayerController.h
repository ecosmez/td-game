#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "MobaPlayerController.generated.h"

class AMobaCameraPawn;
class UMinimapWidget;
class UCameraOrbitGizmoWidget;
class UInputMappingContext;
class UMapDiscoveryComponent;
class UWorldFogOfWarComponent;

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

	/**
	 * If NavMesh has no complete path and the click is at least this much lower (cm),
	 * steer directly in XY so the champion can walk off a ledge and fall.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Champion", meta = (ClampMin = "0.0"))
	float CliffDropFallbackZ = 80.0f;

	/** Stop direct cliff/fall moves when XY distance to the click is within this (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Champion", meta = (ClampMin = "1.0"))
	float DirectMoveAcceptanceRadius = 90.0f;

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

	/** Show circular camera-orbit gizmo (docked to the minimap). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba HUD")
	bool bShowCameraOrbitGizmo = true;

	/** Optional custom orbit gizmo class; defaults to UCameraOrbitGizmoWidget. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba HUD")
	TSubclassOf<UCameraOrbitGizmoWidget> CameraOrbitGizmoWidgetClass;

	/**
	 * Persistent map discovery + 3D fog of war (Diablo-style permanent reveal).
	 * Shared with the minimap fog overlay when enabled.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Discovery")
	TObjectPtr<UMapDiscoveryComponent> MapDiscovery;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Discovery")
	TObjectPtr<UWorldFogOfWarComponent> WorldFogOfWar;

	/** When true, discovery runs even without minimap fog. Drives 3D FOW. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery")
	bool bEnableMapDiscovery = true;

	/** When true, apply dark world fog over unexplored terrain. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery")
	bool bEnableWorldFogOfWar = true;

	/** Also paint discovery fog on the minimap. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery")
	bool bEnableMinimapDiscoveryFog = true;

	/**
	 * Hotkey that toggles 3D fog of war (and matching minimap fog visuals).
	 * Discovery continues under the fog so reveal progress is preserved.
	 * Default: J
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery")
	FKey ToggleFogOfWarKey = EKeys::J;

	UFUNCTION(BlueprintCallable, Category = "Map Discovery")
	void ToggleWorldFogOfWar();

	UFUNCTION(BlueprintCallable, Category = "Map Discovery")
	void SetWorldFogOfWarEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Map Discovery")
	bool IsWorldFogOfWarEnabled() const { return bEnableWorldFogOfWar; }

	UFUNCTION(BlueprintCallable, Category = "Moba HUD")
	UMinimapWidget* ShowMinimap();

	UFUNCTION(BlueprintCallable, Category = "Moba HUD")
	void HideMinimap();

	UFUNCTION(BlueprintPure, Category = "Moba HUD")
	UMinimapWidget* GetMinimapWidget() const { return MinimapWidget; }

	UFUNCTION(BlueprintCallable, Category = "Moba HUD")
	UCameraOrbitGizmoWidget* ShowCameraOrbitGizmo();

	UFUNCTION(BlueprintCallable, Category = "Moba HUD")
	void HideCameraOrbitGizmo();

	UFUNCTION(BlueprintPure, Category = "Moba HUD")
	UCameraOrbitGizmoWidget* GetCameraOrbitGizmoWidget() const { return CameraOrbitGizmoWidget; }

	UFUNCTION(BlueprintPure, Category = "Map Discovery")
	UMapDiscoveryComponent* GetMapDiscovery() const { return MapDiscovery; }

	UFUNCTION(BlueprintPure, Category = "Map Discovery")
	UWorldFogOfWarComponent* GetWorldFogOfWar() const { return WorldFogOfWar; }

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

	/**
	 * Issue a ground move to the controlled champion.
	 * Uses NavMesh when a complete path exists; otherwise steers in XY toward
	 * lower ground so the champion can walk off cliffs and fall.
	 */
	UFUNCTION(BlueprintCallable, Category = "Moba Camera")
	void MoveChampionToLocation(const FVector& WorldLocation);

	/**
	 * Instantly end sky-drop / freefall: snap champion to the ground under them,
	 * clear bIsDropping, restore movement/camera defaults, show ability HUD.
	 * Bound to SkipSkyDropKey while the champion is dropping.
	 */
	UFUNCTION(BlueprintCallable, Category = "Moba|Sky Drop")
	bool TrySkipSkyDrop();

	/** Hotkey that skips the landing/sky-drop phase (default: Enter). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba|Sky Drop")
	FKey SkipSkyDropKey = EKeys::Enter;

	/**
	 * While true, RMB champion click-to-move is disabled (sky drop / freefall).
	 * Call from character BPs instead of the old BP_TopDownController path.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba|Sky Drop")
	bool bDropMode = false;

	UFUNCTION(BlueprintCallable, Category = "Moba|Sky Drop")
	void SetDropMode(bool bInDropMode);

	UFUNCTION(BlueprintPure, Category = "Moba|Sky Drop")
	bool IsDropMode() const { return bDropMode; }

	/**
	 * Overhead freefall: pure top-down view of the landing area while dropping
	 * (mouse aims steer target on the ground), then blend to free MOBA camera on land.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba|Sky Drop|Camera")
	bool bUseWarzoneDropCamera = true;

	/**
	 * Spring-arm length while freefalling (cm).
	 * Longer boom = more of the landing field in frame for mouse guidance.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba|Sky Drop|Camera", meta = (ClampMin = "100.0"))
	float DropCameraArmLength = 2800.0f;

	/**
	 * Spring-arm pitch while freefalling (degrees).
	 * -90 = dead top-down so the ground under the cursor is the landing aim.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba|Sky Drop|Camera")
	float DropCameraPitch = -90.0f;

	/** Fixed world yaw for the overhead drop cam (stable map orientation for mouse aim). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba|Sky Drop|Camera")
	float DropCameraYaw = 0.0f;

	/** Socket Z lift (usually 0 for pure top-down). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba|Sky Drop|Camera")
	float DropCameraSocketOffsetZ = 0.0f;

	/** Blend time when switching from freefall view to free MOBA camera. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba|Sky Drop|Camera", meta = (ClampMin = "0.0"))
	float DropToMobaCameraBlendTime = 0.85f;

	/** True while the player is viewing the champion freefall camera. */
	UFUNCTION(BlueprintPure, Category = "Moba|Sky Drop|Camera")
	bool IsInDropCamera() const { return bInDropCamera; }

	/** Manually enter freefall 3rd-person view (also called automatically while bIsDropping). */
	UFUNCTION(BlueprintCallable, Category = "Moba|Sky Drop|Camera")
	void EnterDropCameraMode();

	/** Manually leave freefall view and switch to free MOBA camera. */
	UFUNCTION(BlueprintCallable, Category = "Moba|Sky Drop|Camera")
	void ExitDropCameraMode();

protected:
	void InitializeMobaCamera();
	void WireChampionFromPawn(APawn* InPawn);
	void HandleClickToMoveChampion();
	void HandleSkipSkyDropInput();
	void HandleToggleFogOfWarInput();
	void ApplyFogOfWarVisualState();
	void EnsureChampionHasAIController(APawn* Champion);
	void UpdateSkyDropCamera(float DeltaTime);
	void ApplyWarzoneDropFraming(APawn* Champion, float DeltaTime);
	void SwitchViewToMobaCamera(bool bBlend);

	/** True when a complete (non-partial) NavMesh path exists to Dest. */
	bool HasCompleteNavPathTo(APawn* Champion, const FVector& Dest) const;

	/** Abort PathFollowing on the champion's AI controller. */
	void AbortChampionPathFollowing(APawn* Champion);

	/** Start / stop XY-steer fallback used for cliff drops. */
	void StartDirectMoveTo(const FVector& WorldLocation);
	void StopDirectMove();
	void UpdateDirectMoveChampion(float DeltaTime);

	UPROPERTY(Transient)
	TObjectPtr<AMobaCameraPawn> CachedCameraPawn;

	UPROPERTY(Transient)
	TObjectPtr<UMinimapWidget> MinimapWidget;

	UPROPERTY(Transient)
	TObjectPtr<UCameraOrbitGizmoWidget> CameraOrbitGizmoWidget;

	/** Runtime: freefall view is active. */
	UPROPERTY(Transient)
	bool bInDropCamera = false;

	/** Last-frame drop flag for edge detection. */
	UPROPERTY(Transient)
	bool bWasChampionDropping = false;

	/** True while steering in XY toward a click that NavMesh cannot reach (cliff fall). */
	UPROPERTY(Transient)
	bool bDirectMoveActive = false;

	UPROPERTY(Transient)
	FVector DirectMoveTarget = FVector::ZeroVector;
};
