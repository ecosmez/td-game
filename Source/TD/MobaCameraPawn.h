#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MobaCameraPawn.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UFloatingPawnMovement;
class UInputAction;
class UInputMappingContext;
class UEnhancedInputComponent;
struct FInputActionValue;

/**
 * Independent RTS/MOBA camera pawn (LoL-style).
 * Not attached to the champion. Driven by Enhanced Input and free movement.
 */
UCLASS(Blueprintable, BlueprintType)
class TD_API AMobaCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	AMobaCameraPawn();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void UnPossessed() override;

	// ---- Components ----
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moba Camera|Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moba Camera|Components")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moba Camera|Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Moba Camera|Components")
	TObjectPtr<UFloatingPawnMovement> FloatingPawnMovement;

	// ---- Framing ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Framing")
	float CameraPitch = -55.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Framing")
	float CameraYaw = 45.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Framing", meta = (ClampMin = "1.0"))
	float DefaultZoom = 1800.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Framing", meta = (ClampMin = "1.0"))
	float MinimumZoom = 900.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Framing", meta = (ClampMin = "1.0"))
	float MaximumZoom = 2600.0f;

	// ---- Speeds ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Movement", meta = (ClampMin = "0.0"))
	float KeyboardMoveSpeed = 1800.0f;

	/**
	 * Grounded free-cam policy (after sky drop lands): mouse edge/drag/zoom + Space lock only.
	 * WASD/QE keys do not pan the camera once the champion is no longer dropping.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Movement")
	bool bEnableKeyboardCameraMoveWhenGrounded = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Movement", meta = (ClampMin = "0.0"))
	float EdgeScrollSpeed = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Movement", meta = (ClampMin = "0.0"))
	float DragMoveSpeed = 2.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Movement", meta = (ClampMin = "0.0"))
	float RotationSpeed = 70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Movement", meta = (ClampMin = "0.0"))
	float ZoomSpeed = 250.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Movement", meta = (ClampMin = "0.01"))
	float MovementInterpolationSpeed = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Movement", meta = (ClampMin = "0.01"))
	float ZoomInterpolationSpeed = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Movement", meta = (ClampMin = "0.01"))
	float RotationInterpolationSpeed = 8.0f;

	/** How quickly the camera eases toward a minimap click/drag target (lower = slower pan). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Movement", meta = (ClampMin = "0.01"))
	float MinimapPanInterpolationSpeed = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Movement", meta = (ClampMin = "0.0"))
	float EdgeScrollThreshold = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Movement")
	bool bScaleMovementWithZoom = true;

	// ---- Edge / UI / Drag ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Edge Scroll")
	bool bEnableEdgeScrolling = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Edge Scroll")
	bool bBlockCameraInputOverUI = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Drag")
	bool bHideCursorWhileDragging = false;

	// ---- Focus ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Focus", meta = (ClampMin = "0.01"))
	float ChampionFollowInterpolationSpeed = 12.0f;

	/**
	 * Sticky LoL-style lock: free camera keeps XY over the champion until the player pans.
	 * Defaults on so sky-drop and post-land framing stay on the player.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Focus")
	bool bLockedToChampion = true;

	/** While the champion has bIsDropping, re-lock and follow even if the player had panned away. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Focus")
	bool bFollowWhileChampionDropping = true;

	/** Snap XY onto the champion every tick while dropping (avoids lag during freefall steer). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Focus")
	bool bSnapFollowWhileDropping = true;

	/** MMB drag / minimap pan clear lock (Space re-locks). Edge scroll only when unlocked. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Focus")
	bool bManualMovementCancelsFocus = true;

	// ---- Rotation ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Rotation")
	bool bEnableCameraRotation = false;

	// ---- Bounds ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Bounds")
	bool bConstrainToWorldBounds = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Bounds")
	FVector2D MinimumWorldBounds = FVector2D(-50000.0f, -50000.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Bounds")
	FVector2D MaximumWorldBounds = FVector2D(50000.0f, 50000.0f);

	/** Optional bounds volume/actor – world bounds are derived from its bounds when set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Bounds")
	TObjectPtr<AActor> WorldBoundsSource;

	// ---- Enhanced Input (assign IMC_MobaCamera + IA_* assets, or leave empty for runtime defaults) ----
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Input")
	TObjectPtr<UInputMappingContext> CameraMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Input")
	TObjectPtr<UInputAction> ZoomAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Input")
	TObjectPtr<UInputAction> RotateAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Input")
	TObjectPtr<UInputAction> DragAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Input")
	TObjectPtr<UInputAction> FocusChampionAction;

	/** Arrow keys pan the camera regardless of bEnableKeyboardCameraMoveWhenGrounded (WASD is reserved for abilities). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Input")
	TObjectPtr<UInputAction> ArrowMoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Input")
	int32 MappingContextPriority = 1;

	// ---- Blueprint API ----
	UFUNCTION(BlueprintCallable, Category = "Moba Camera")
	void SetFocusingChampion(bool bFocusing);

	UFUNCTION(BlueprintPure, Category = "Moba Camera")
	bool IsFocusingChampion() const { return ShouldFollowChampion(); }

	UFUNCTION(BlueprintCallable, Category = "Moba Camera")
	void SetLockedToChampion(bool bLocked);

	UFUNCTION(BlueprintPure, Category = "Moba Camera")
	bool IsLockedToChampion() const { return bLockedToChampion; }

	UFUNCTION(BlueprintCallable, Category = "Moba Camera")
	void SetTargetZoom(float NewZoom);

	UFUNCTION(BlueprintPure, Category = "Moba Camera")
	float GetCurrentZoom() const { return CurrentZoom; }

	/** Current interpolated orbit yaw (degrees). Independent of Q/E keyboard rotate. */
	UFUNCTION(BlueprintPure, Category = "Moba Camera|Rotation")
	float GetOrbitYaw() const { return CurrentYaw; }

	/** Set orbit yaw around the camera pivot (pitch/height unchanged). */
	UFUNCTION(BlueprintCallable, Category = "Moba Camera|Rotation")
	void SetOrbitYaw(float YawDegrees, bool bInstant = false);

	/** Add a yaw delta around the camera pivot (pitch/height unchanged). */
	UFUNCTION(BlueprintCallable, Category = "Moba Camera|Rotation")
	void AddOrbitYawDelta(float DeltaDegrees);

	UFUNCTION(BlueprintCallable, Category = "Moba Camera")
	void RecenterOnChampion(bool bInstant = false);

	/** Eases the free camera pivot toward a world XY at MinimapPanInterpolationSpeed (keeps current height). Used by minimap. */
	UFUNCTION(BlueprintCallable, Category = "Moba Camera")
	void SnapToWorldXY(const FVector& WorldLocation);

	UFUNCTION(BlueprintCallable, Category = "Moba Camera")
	void RefreshBoundsFromSource();

protected:
	void EnsureInputAssets();
	void BindEnhancedInput(UEnhancedInputComponent* EIC);
	void AddMappingContext();
	void RemoveMappingContext();

	void OnMove(const FInputActionValue& Value);
	void OnZoom(const FInputActionValue& Value);
	void OnRotate(const FInputActionValue& Value);
	void OnDragStarted(const FInputActionValue& Value);
	void OnDragCompleted(const FInputActionValue& Value);
	void OnFocusStarted(const FInputActionValue& Value);
	void OnFocusCompleted(const FInputActionValue& Value);
	void OnArrowMove(const FInputActionValue& Value);

	void UpdatePlanarAxes();
	void UpdateKeyboardMove(float DeltaTime);
	void UpdateEdgeScroll(float DeltaTime);
	void UpdateMiddleMouseDrag(float DeltaTime);
	void UpdateChampionFocus(float DeltaTime);
	void UpdateMinimapPan(float DeltaTime);
	void UpdateZoom(float DeltaTime);
	void UpdateRotation(float DeltaTime);
	void ApplyVelocity(float DeltaTime);
	void ConstrainLocation(FVector& Location) const;
	void CancelChampionFollow();
	bool ShouldFollowChampion() const;
	bool IsChampionDropping() const;
	/** True when WASD/QE are allowed for free-cam (disabled on ground by default). */
	bool IsKeyboardCameraMoveEnabled() const;

	class AMobaPlayerController* GetMobaController() const;
	APawn* ResolveFocusChampion() const;
	bool ShouldBlockCameraInputForUI() const;
	bool HasMouseFocus() const;
	float GetZoomSpeedScale() const;

	// Runtime state
	FVector2D MoveInput = FVector2D::ZeroVector;
	/** Arrow-key pan input — combined with MoveInput but not gated by the WASD grounded toggle. */
	FVector2D ArrowMoveInput = FVector2D::ZeroVector;
	float RotateInput = 0.0f;
	FVector DesiredVelocity = FVector::ZeroVector;
	FVector CurrentVelocity = FVector::ZeroVector;

	float TargetZoom = 1800.0f;
	float CurrentZoom = 1800.0f;
	float TargetYaw = 45.0f;
	float CurrentYaw = 45.0f;

	FVector ForwardPlanar = FVector::ForwardVector;
	FVector RightPlanar = FVector::RightVector;

	bool bIsDragging = false;
	/** After a fresh lock (Space / recenter), require the cursor to leave the screen edge once before edge-scroll can act again. */
	bool bEdgeScrollArmed = true;
	bool bFocusChampionHeld = false;
	/** True while easing toward a minimap click/drag target (cleared on arrival or manual input). */
	bool bMinimapPanning = false;
	FVector MinimapPanTarget = FVector::ZeroVector;
	FVector2D LastDragMousePos = FVector2D::ZeroVector;
	bool bSavedCursorVisibility = true;
	float CachedPawnHeight = 0.0f;
	bool bInputBound = false;
};
