#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CameraOrbitGizmoWidget.generated.h"

class UCanvasPanel;
class UBorder;
class USizeBox;
class UOverlay;
class UCanvasPanelSlot;
class AMobaCameraPawn;

/**
 * Circular on-screen gizmo that orbits the free camera yaw around its current
 * pivot without changing pitch or height.
 */
UCLASS()
class TD_API UCameraOrbitGizmoWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCameraOrbitGizmoWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** Outer gizmo diameter (slate units). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Orbit Gizmo", meta = (ClampMin = "64.0"))
	float GizmoSize = 120.0f;

	/** Distance from bottom-left corner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Orbit Gizmo")
	FVector2D ScreenMargin = FVector2D(24.0f, 24.0f);

	/** Radius of the handle ring as a fraction of half-size (0–1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Orbit Gizmo", meta = (ClampMin = "0.2", ClampMax = "0.95"))
	float HandleRadiusFraction = 0.72f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera Orbit Gizmo", meta = (ClampMin = "4.0"))
	float HandleSize = 18.0f;

	/** True when ScreenPos is over the gizmo frame (for world-click blocking). */
	UFUNCTION(BlueprintPure, Category = "Camera Orbit Gizmo")
	bool IsScreenPosOverGizmo(FVector2D ScreenPos) const;

protected:
	void EnsureBuilt();
	void BuildDefaultUI();
	void ApplyHitTestPolicy();
	void BindPointerEvents();
	void UpdateHandleFromYaw(float YawDegrees);
	bool TryPointerAngle(const FPointerEvent& MouseEvent, float& OutScreenAngleDegrees) const;
	AMobaCameraPawn* ResolveCameraPawn() const;

	UFUNCTION()
	FEventReply OnGizmoMouseButtonDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent);

	UFUNCTION()
	FEventReply OnGizmoMouseMove(FGeometry MyGeometry, const FPointerEvent& MouseEvent);

	UFUNCTION()
	FEventReply OnGizmoMouseButtonUp(FGeometry MyGeometry, const FPointerEvent& MouseEvent);

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> RootCanvas;

	UPROPERTY(Transient)
	TObjectPtr<USizeBox> FrameSizeBox;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> RingBorder;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> HubBorder;

	UPROPERTY(Transient)
	TObjectPtr<UBorder> HandleBorder;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanel> HandleCanvas;

	UPROPERTY(Transient)
	TObjectPtr<UCanvasPanelSlot> HandleSlot;

	bool bBuilt = false;
	bool bDragging = false;

	/** TargetYaw - ScreenAngle captured on mouse-down (relative drag). */
	float DragYawOffset = 0.0f;
};
