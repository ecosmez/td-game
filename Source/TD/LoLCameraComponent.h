#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InputCoreTypes.h"
#include "LoLCameraComponent.generated.h"
class USpringArmComponent;
class UCameraComponent;
class APlayerController;
/**
 * Locked: camera sticks to champion (mouse ignored).
 * Unlocked: pan only when cursor is on screen edges (no champion follow).
 * Space = center + lock. Y = toggle lock/unlock.
 * Landing: LoL-style arm punch-zoom + pitch recover when champion lands from sky drop.
 */
UCLASS(ClassGroup = (Camera), meta = (BlueprintSpawnableComponent))
class TD_API ULoLCameraComponent : public UActorComponent
{
	GENERATED_BODY()
public:
	ULoLCameraComponent();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LoL Camera")
	bool bCameraEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LoL Camera")
	bool bDisableWhileDropping = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LoL Camera")
	bool bCameraLocked = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LoL Camera", meta = (ClampMin = "1.0"))
	float EdgeMargin = 22.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LoL Camera", meta = (ClampMin = "0.0"))
	float EdgeScrollSpeed = 1000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LoL Camera")
	FName SpringArmComponentName = NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LoL Camera")
	FName CameraComponentName = NAME_None;
	UPROPERTY(BlueprintReadOnly, Category = "LoL Camera")
	FVector CameraFocusLocation = FVector::ZeroVector;
	// --- Landing zoom (sky drop -> ground) ---
	/** If true, auto-plays landing zoom when owner bIsDropping goes true->false. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LoL Camera|Landing")
	bool bAutoPlayLandingZoom = true;
	/** Total duration of the land punch + settle. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LoL Camera|Landing", meta = (ClampMin = "0.05"))
	float LandingZoomDuration = 0.72f;
	/** Fraction of duration spent zooming into the peak (closer) before settling out. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LoL Camera|Landing",
		meta = (ClampMin = "0.05", ClampMax = "0.9"))
	float LandingZoomPunchTime = 0.32f;
	/** How much closer the boom pulls at the land peak (TargetArmLength decrease). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LoL Camera|Landing", meta = (ClampMin = "0.0"))
	float LandingZoomPunchAmount = 1700.f;
	/** Extra FOV degrees at punch peak (0 = no FOV kick). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LoL Camera|Landing", meta = (ClampMin = "0.0"))
	float LandingFovKick = 5.f;
	/**
	 * Pitch used when auto-playing (settle target). BP can pass a different pitch via PlayLandingZoom.
	 * Matches typical top-down framing (~-55).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LoL Camera|Landing")
	float LandingSettlePitch = -55.f;
	UFUNCTION(BlueprintCallable, Category = "LoL Camera")
	void SetCameraEnabled(bool bEnabled);
	UFUNCTION(BlueprintCallable, Category = "LoL Camera")
	void LockToChampion();
	UFUNCTION(BlueprintCallable, Category = "LoL Camera")
	void UnlockCamera();
	UFUNCTION(BlueprintCallable, Category = "LoL Camera")
	void ToggleCameraLock();
	/**
	 * Play LoL-style land zoom: ease pitch/arm from the current drop framing toward
	 * settle values, with a brief closer punch at impact.
	 */
	UFUNCTION(BlueprintCallable, Category = "LoL Camera|Landing")
	void PlayLandingZoom(float SettleArmLength, float SettlePitch);
	UFUNCTION(BlueprintCallable, Category = "LoL Camera|Landing")
	void StopLandingZoom(bool bSnapToSettle = true);
	UFUNCTION(BlueprintPure, Category = "LoL Camera|Landing")
	bool IsLandingZoomActive() const { return bLandingZoomActive; }

	/**
	 * Hotkey checked while owner has bIsDropping. Calls champion SkipSkyDrop / CompleteDropLanding
	 * when available, else snaps to ground and clears drop state (default: Enter).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LoL Camera|Landing")
	FKey SkipSkyDropKey = EKeys::Enter;

	/** If true, SkipSkyDropKey ends freefall while the owner is dropping. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LoL Camera|Landing")
	bool bEnableSkipSkyDropHotkey = true;

protected:
	UPROPERTY()
	TObjectPtr<USpringArmComponent> SpringArm;
	UPROPERTY()
	TObjectPtr<UCameraComponent> Camera;
	bool bSavedCameraLag = false;
	bool bHasSavedCameraLag = false;
	/** After unlock, wait until cursor leaves edges before scrolling (avoids jump). */
	bool bEdgeScrollArmed = true;
	bool bWasOwnerDropping = false;
	bool bLandingZoomActive = false;
	float LandingZoomElapsed = 0.f;
	float LandingZoomStartArm = 0.f;
	float LandingZoomStartPitch = -90.f;
	float LandingZoomPeakArm = 0.f;
	float LandingZoomSettleArm = 7800.f;
	float LandingZoomSettlePitchVal = -55.f;
	float LandingZoomBaseFov = 40.f;
	bool ResolveSpringArm();
	bool ResolveCamera();
	bool IsOwnerDropping() const;
	APlayerController* GetOwnerPlayerController() const;
	bool GetViewportMouse(APlayerController* PC, FVector2D& OutMouse, FVector2D& OutSize) const;
	bool IsOnAnyEdge(const FVector2D& Mouse, const FVector2D& Size) const;
	void UpdateEdgePan(float DeltaTime, APlayerController* PC);
	void ApplyFocusToSpringArm();
	void SetSpringArmLagEnabled(bool bEnabled);
	void SyncFocusFromSpringArm();
	void UpdateLandingZoom(float DeltaTime);
	void ApplyLandingZoomFrame(float NormalizedTime);
	void TrySkipSkyDropFromHotkey(APlayerController* PC);
	static float SmoothStep01(float T);
};
