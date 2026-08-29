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
class UFloatingDamageTextWidget;

/** One active floating combat-text number: owning widget, world anchor, and lifetime. */
USTRUCT()
struct FTDFloatingDamageEntry
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UFloatingDamageTextWidget> Widget = nullptr;

	FVector WorldLocation = FVector::ZeroVector;
	float Elapsed = 0.0f;
	float Duration = 1.0f;
};

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

	/**
	 * Place the champion's sky-drop over a point next to the main crystal
	 * instead of PlayerStart / world origin.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba|Sky Drop")
	bool bLandChampionNearMainCrystal = true;

	/** Horizontal distance from the main crystal to the drop (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba|Sky Drop", meta = (ClampMin = "0.0"))
	float ChampionCrystalLandOffset = 500.0f;

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

	/** Horizontal search radius when snapping a click onto NavMesh (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Champion", meta = (ClampMin = "1.0"))
	float NavProjectHorizontalExtent = 500.0f;

	/** Vertical search radius when snapping a click onto NavMesh (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Champion", meta = (ClampMin = "1.0"))
	float NavProjectVerticalExtent = 1000.0f;

	/** Right-click on an enemy attacks it instead of moving there (walks into range first). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Champion|Attack")
	bool bEnableChampionAttack = true;

	/** Reach of the champion's basic attack (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Champion|Attack", meta = (ClampMin = "1.0"))
	float ChampionAttackRange = 250.0f;

	/** Damage dealt per basic attack. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Champion|Attack", meta = (ClampMin = "0.0"))
	float ChampionAttackDamage = 15.0f;

	/** Seconds between basic attacks while the target is in range. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Champion|Attack", meta = (ClampMin = "0.05"))
	float ChampionAttackInterval = 1.0f;

	/** Current champion basic-attack target, if any (set by right-clicking an enemy). */
	UFUNCTION(BlueprintPure, Category = "Moba Camera|Champion|Attack")
	AActor* GetChampionAttackTarget() const { return ChampionAttackTarget.Get(); }

	/** Show a LoL-style floating damage number that rises and fades over this many seconds. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Champion|Attack", meta = (ClampMin = "0.1"))
	float FloatingDamageDuration = 1.0f;

	/** Upward drift speed of floating damage numbers (cm/s). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Champion|Attack")
	float FloatingDamageRiseSpeed = 70.0f;

	/** Floating text color for damage the champion deals to enemies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Champion|Attack")
	FLinearColor OutgoingDamageColor = FLinearColor(1.0f, 0.87f, 0.25f, 1.0f);

	/** Floating text color for damage the champion takes from enemies. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Champion|Attack")
	FLinearColor IncomingDamageColor = FLinearColor(1.0f, 0.18f, 0.16f, 1.0f);

	/** Clear the current champion attack order (does not issue a new move). */
	UFUNCTION(BlueprintCallable, Category = "Moba Camera|Champion|Attack")
	void StopChampionAttack();

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
	 * League-style live vision + 3D fog of war (dim map, clear around champion/crystal).
	 * Shared with the minimap fog overlay when enabled.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Discovery")
	TObjectPtr<UMapDiscoveryComponent> MapDiscovery;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Map Discovery")
	TObjectPtr<UWorldFogOfWarComponent> WorldFogOfWar;

	/** When true, discovery runs even without minimap fog. Drives 3D FOW. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery")
	bool bEnableMapDiscovery = true;

	/** When true, apply dim world fog outside current vision. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery")
	bool bEnableWorldFogOfWar = true;

	/** Also paint discovery fog on the minimap. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery")
	bool bEnableMinimapDiscoveryFog = true;

	/**
	 * Hotkey that toggles 3D fog of war (and matching minimap fog visuals).
	 * Discovery continues under the fog so vision sources keep updating.
	 * Default: J
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery")
	FKey ToggleFogOfWarKey = EKeys::J;

	UFUNCTION(BlueprintCallable, Category = "Map Discovery")
	void ToggleWorldFogOfWar();

	UFUNCTION(BlueprintCallable, Category = "Map Discovery")
	void SetWorldFogOfWarEnabled(bool bEnabled);

	/** Hotkey that opens/closes the tower store. Default: B */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store")
	FKey ToggleStoreKeyPrimary = EKeys::B;

	/** Second hotkey that opens/closes the tower store. Default: S */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store")
	FKey ToggleStoreKeySecondary = EKeys::S;

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
	 * Projects the click onto NavMesh when possible. Uses path following for
	 * reachable ground; otherwise steers in XY toward lower / off-mesh clicks
	 * so the champion can walk off cliffs instead of stalling.
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
	AActor* FindMainCrystal() const;
	bool TryGetChampionDropLocationNearMainCrystal(FVector& OutLocation) const;
	void PlaceChampionNearMainCrystal(APawn* Champion);
	void HandleClickToMoveChampion();
	void UpdateChampionAttack(float DeltaTime);
	void BeginChampionAttack(APawn* Champion, AActor* Target);
	void UpdateChampionDamageTaken();
	void SpawnFloatingDamageText(const FVector& WorldLocation, float Amount, const FLinearColor& Color);
	void UpdateFloatingDamageTexts(float DeltaTime);
	void HandleSkipSkyDropInput();
	void HandleToggleFogOfWarInput();
	void HandleToggleStoreInput();
	void ApplyFogOfWarVisualState();
	void EnsureChampionHasAIController(APawn* Champion);
	void UpdateSkyDropCamera(float DeltaTime);
	void ApplyWarzoneDropFraming(APawn* Champion, float DeltaTime);
	void SwitchViewToMobaCamera(bool bBlend);

	/** True when a complete (non-partial) NavMesh path exists to Dest. */
	bool HasCompleteNavPathTo(APawn* Champion, const FVector& Dest) const;

	/** First-hit cursor trace: enemies attack, Landscape moves, everything else rejects the click. */
	bool TraceChampionClick(APawn* Champion, FHitResult& OutHit, AActor*& OutAttackTarget) const;

	/** Snap a world click onto NavMesh within NavProject* extents. */
	bool ProjectClickToNavMesh(APawn* Champion, const FVector& Point, FVector& OutProjected) const;

	/** Path-follow to Dest, projecting the goal onto NavMesh so off-mesh clicks still move. */
	void IssueChampionNavMove(APawn* Champion, const FVector& Dest);

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

	/** Enemy the champion is currently attack-moving toward / attacking. */
	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> ChampionAttackTarget;

	UPROPERTY(Transient)
	float ChampionAttackCooldownRemaining = 0.0f;

	/** Throttles re-issuing SimpleMoveToLocation while chasing a moving attack target. */
	UPROPERTY(Transient)
	float ChampionAttackRepathCooldown = 0.0f;

	UPROPERTY(Transient)
	FVector ChampionAttackLastChaseTarget = FVector::ZeroVector;

	/** Active floating damage numbers currently rising/fading in the viewport. */
	UPROPERTY(Transient)
	TArray<FTDFloatingDamageEntry> FloatingDamageEntries;

	/** Champion CurrentHealth last tick, used to detect incoming hits for red floating text. -1 = not yet sampled. */
	UPROPERTY(Transient)
	float LastKnownChampionHealth = -1.0f;

	UPROPERTY(Transient)
	TWeakObjectPtr<APawn> LastKnownChampionForHealth;
};
