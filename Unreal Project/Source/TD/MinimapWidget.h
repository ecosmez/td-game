#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MinimapWidget.generated.h"

class UCanvasPanel;
class UBorder;
class UImage;
class USizeBox;
class UCanvasPanelSlot;
class UTextureRenderTarget2D;
class USceneCaptureComponent2D;
class ASceneCapture2D;
class AMobaCameraPawn;

/**
 * LoL-style minimap anchored bottom-right.
 * Top-down orthographic SceneCapture + champion / camera markers.
 * Left-click pans the free camera (or recenters view target) to the world point.
 */
UCLASS()
class TD_API UMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UMinimapWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** Outer map frame size (slate units). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap", meta = (ClampMin = "80.0"))
	float MinimapSize = 220.0f;

	/** Distance from bottom-right corner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FVector2D ScreenMargin = FVector2D(24.0f, 24.0f);

	/** World XY bounds drawn on the minimap (cm). Synced from free camera when available. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|World")
	FVector2D WorldMin = FVector2D(-8000.0f, -8000.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|World")
	FVector2D WorldMax = FVector2D(8000.0f, 8000.0f);

	/** Height of the top-down capture cam above map center. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Capture", meta = (ClampMin = "100.0"))
	float CaptureHeight = 25000.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Capture", meta = (ClampMin = "32"))
	int32 RenderTargetSize = 256;

	/** Update scene capture every N seconds (0 = every frame). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Capture", meta = (ClampMin = "0.0"))
	float CaptureInterval = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	bool bShowChampionMarker = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	bool bShowCameraMarker = true;

	/** LMB click / drag on minimap moves free camera to that world point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	bool bClickToPanCamera = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FLinearColor FrameColor = FLinearColor(0.04f, 0.07f, 0.10f, 0.94f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FLinearColor BorderColor = FLinearColor(0.55f, 0.65f, 0.75f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FLinearColor ChampionMarkerColor = FLinearColor(0.25f, 0.85f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FLinearColor CameraMarkerColor = FLinearColor(1.0f, 0.92f, 0.35f, 0.95f);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetWorldBounds(FVector2D InMin, FVector2D InMax);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void RefreshCaptureSettings();

protected:
	void EnsureBuilt();
	void BuildDefaultUI();
	void EnsureCapture();
	void DestroyCapture();
	void SyncBoundsFromCamera();
	void UpdateCaptureTransform();
	void UpdateMarkers();
	void UpdateCapture(float DeltaTime);
	bool LocalToWorld(const FVector2D& LocalPos, const FGeometry& Geometry, FVector& OutWorld) const;
	FVector2D WorldToNormalized(const FVector& WorldLoc) const;
	void PanCameraToWorld(const FVector& WorldLoc);
	APawn* ResolveChampion() const;
	AMobaCameraPawn* ResolveCameraPawn() const;
	void PlaceMarker(UBorder* Marker, UCanvasPanelSlot* Slot, const FVector2D& Normalized, float HalfSize);

	UPROPERTY()
	TObjectPtr<UCanvasPanel> RootCanvas = nullptr;

	UPROPERTY()
	TObjectPtr<USizeBox> FrameSizeBox = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> FrameBorder = nullptr;

	UPROPERTY()
	TObjectPtr<UImage> MapImage = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> ChampionMarker = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> CameraMarker = nullptr;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> ChampionSlot = nullptr;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> CameraSlot = nullptr;

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> RenderTarget = nullptr;

	UPROPERTY()
	TObjectPtr<ASceneCapture2D> CaptureActor = nullptr;

	float CaptureTimer = 0.0f;
	bool bBuilt = false;
	bool bDragging = false;
	bool bCaptureReady = false;
};
