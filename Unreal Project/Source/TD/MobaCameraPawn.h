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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Focus")
	bool bManualMovementCancelsFocus = false;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Moba Camera|Input")
	int32 MappingContextPriority = 1;

	// ---- Blueprint API ----
	UFUNCTION(BlueprintCallable, Category = "Moba Camera")
	void SetFocusingChampion(bool bFocusing);

	UFUNCTION(BlueprintPure, Category = "Moba Camera")
	bool IsFocusingChampion() const { return bFocusChampionHeld; }

	UFUNCTION(BlueprintCallable, Category = "Moba Camera")
	void SetTargetZoom(float NewZoom);

	UFUNCTION(BlueprintPure, Category = "Moba Camera")
	float GetCurrentZoom() const { return CurrentZoom; }

	UFUNCTION(BlueprintCallable, Category = "Moba Camera")
	void RecenterOnChampion(bool bInstant = false);

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

	void UpdatePlanarAxes();
	void UpdateKeyboardMove(float DeltaTime);
	void UpdateEdgeScroll(float DeltaTime);
	void UpdateMiddleMouseDrag(float DeltaTime);
	void UpdateChampionFocus(float DeltaTime);
	void UpdateZoom(float DeltaTime);
	void UpdateRotation(float DeltaTime);
	void ApplyVelocity(float DeltaTime);
	void ConstrainLocation(FVector& Location) const;

	class AMobaPlayerController* GetMobaController() const;
	APawn* ResolveFocusChampion() const;
	bool ShouldBlockCameraInputForUI() const;
	bool HasMouseFocus() const;
	float GetZoomSpeedScale() const;

	// Runtime state
	FVector2D MoveInput = FVector2D::ZeroVector;
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
	bool bFocusChampionHeld = false;
	FVector2D LastDragMousePos = FVector2D::ZeroVector;
	bool bSavedCursorVisibility = true;
	float CachedPawnHeight = 0.0f;
	bool bInputBound = false;
};
