#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UObject/SoftObjectPath.h"
#include "MinimapWidget.generated.h"

class UCanvasPanel;
class UBorder;
class UImage;
class USizeBox;
class UCanvasPanelSlot;
class UTextureRenderTarget2D;
class UTexture2D;
class USceneCaptureComponent2D;
class ASceneCapture2D;
class AMobaCameraPawn;
class UMapDiscoveryComponent;

/**
 * LoL-style minimap anchored bottom-right.
 * Top-down orthographic SceneCapture of the playable level (lit scene color),
 * champion / camera markers, LMB camera pan, and optional discovery fog.
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
	int32 RenderTargetSize = 512;

	/** Update scene capture every N seconds (0 = every frame). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Capture", meta = (ClampMin = "0.0"))
	float CaptureInterval = 0.1f;

	/**
	 * Fit minimap orthographic bounds to placed level actors (pads, meshes, etc.).
	 * Avoids empty solid-color maps when free-cam defaults use huge world clamps.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|World")
	bool bAutoFitBoundsToLevel = true;

	/** Padding added around auto-fit actor bounds (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|World", meta = (ClampMin = "0.0", EditCondition = "bAutoFitBoundsToLevel"))
	float AutoFitBoundsPadding = 800.0f;

	/** Re-scan level bounds every N seconds while auto-fit is on (0 = only once). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|World", meta = (ClampMin = "0.0", EditCondition = "bAutoFitBoundsToLevel"))
	float AutoFitRescanInterval = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	bool bShowChampionMarker = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	bool bShowCameraMarker = true;

	/** LMB click / drag on minimap moves free camera to that world point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	bool bClickToPanCamera = true;

	/**
	 * RMB click on minimap issues a move order to the controlled champion
	 * (same as world right-click-to-move).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	bool bClickToMoveChampion = true;

	/**
	 * SceneCapture top-down RTs are mirrored on X vs world XY mapping.
	 * When true, mirror the capture image horizontally so it lines up with markers/clicks.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Capture")
	bool bFlipCaptureImageU = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FLinearColor FrameColor = FLinearColor(0.04f, 0.07f, 0.10f, 0.94f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FLinearColor BorderColor = FLinearColor(0.55f, 0.65f, 0.75f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FLinearColor ChampionMarkerColor = FLinearColor(0.25f, 0.85f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FLinearColor CameraMarkerColor = FLinearColor(1.0f, 0.92f, 0.35f, 0.95f);

	/** Crystal (goal) marker — green. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Landmarks")
	bool bShowCrystalMarker = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Landmarks")
	FLinearColor CrystalMarkerColor = FLinearColor(0.15f, 0.95f, 0.35f, 1.0f);

	/** Half-size of the crystal marker on the minimap (slate units). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Landmarks", meta = (ClampMin = "2.0"))
	float CrystalMarkerHalfSize = 7.0f;

	/** First enemy spawner marker — red. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Landmarks")
	bool bShowEnemySpawnMarker = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Landmarks")
	FLinearColor EnemySpawnMarkerColor = FLinearColor(0.95f, 0.18f, 0.15f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Landmarks", meta = (ClampMin = "2.0"))
	float EnemySpawnMarkerHalfSize = 7.0f;

	/**
	 * World radius (cm) permanently revealed around crystal + first enemy spawn
	 * (clears minimap fog and 3D FOW at those sites).
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Landmarks", meta = (ClampMin = "100.0"))
	float LandmarkRevealRadius = 2200.0f;

	/** Soft class path for BP_Crystal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Landmarks")
	FSoftClassPath CrystalActorClass =
		FSoftClassPath(TEXT("/Game/TD/BP_Crystal.BP_Crystal_C"));

	/** Soft class path for BP_EnemySpawner (first found = initial spawn). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Landmarks")
	FSoftClassPath EnemySpawnerActorClass =
		FSoftClassPath(TEXT("/Game/TD/BP_EnemySpawner.BP_EnemySpawner_C"));

	/**
	 * Diablo-style map discovery: unexplored areas stay fogged on the minimap.
	 * Prefer the shared UMapDiscoveryComponent from the player controller when set.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Discovery")
	bool bMapDiscoveryEnabled = true;

	/** World-space radius (cm) revealed around the champion while exploring. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Discovery", meta = (ClampMin = "100.0", EditCondition = "bMapDiscoveryEnabled"))
	float DiscoveryRadius = 1800.0f;

	/** Soft edge width as a fraction of DiscoveryRadius (0 = hard circle, 1 = fully soft). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Discovery", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bMapDiscoveryEnabled"))
	float DiscoverySoftness = 0.35f;

	/** Resolution of the persistent reveal fog mask (square). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Discovery", meta = (ClampMin = "64", ClampMax = "1024", EditCondition = "bMapDiscoveryEnabled"))
	int32 DiscoveryMaskSize = 256;

	/** Champion must move at least this far (cm) before stamping another reveal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Discovery", meta = (ClampMin = "0.0", EditCondition = "bMapDiscoveryEnabled"))
	float DiscoveryStampDistance = 80.0f;

	/** Fog tint for unexplored minimap regions (alpha = opacity over terrain). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Discovery", meta = (EditCondition = "bMapDiscoveryEnabled"))
	FLinearColor UndiscoveredColor = FLinearColor(0.02f, 0.03f, 0.04f, 0.96f);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void SetWorldBounds(FVector2D InMin, FVector2D InMax);

	UFUNCTION(BlueprintCallable, Category = "Minimap")
	void RefreshCaptureSettings();

	/** True when screen pos is over the map frame (not the full-viewport root). */
	UFUNCTION(BlueprintPure, Category = "Minimap")
	bool IsScreenPosOverMap(FVector2D ScreenPos) const;

	/** Permanently reveal a circle at the given world location (uses DiscoveryRadius). */
	UFUNCTION(BlueprintCallable, Category = "Minimap|Discovery")
	void RevealAtWorldLocation(const FVector& WorldLocation);

	/** Clear all exploration (full fog). Current champion position is re-stamped on next tick. */
	UFUNCTION(BlueprintCallable, Category = "Minimap|Discovery")
	void ResetMapDiscovery();

	/** Enable/disable discovery fog at runtime. */
	UFUNCTION(BlueprintCallable, Category = "Minimap|Discovery")
	void SetMapDiscoveryEnabled(bool bEnabled);

	/** Bind a shared discovery mask (from AMobaPlayerController). */
	UFUNCTION(BlueprintCallable, Category = "Minimap|Discovery")
	void SetDiscoverySource(UMapDiscoveryComponent* InDiscovery);

	/** Find crystal / first spawner, place fog reveals, and bind marker actors. */
	UFUNCTION(BlueprintCallable, Category = "Minimap|Landmarks")
	void RefreshLandmarks();

protected:
	void EnsureBuilt();
	void BuildDefaultUI();
	void EnsureCapture();
	void DestroyCapture();
	void SyncBoundsFromCamera();
	void RefitBoundsFromLevel();
	void UpdateCaptureTransform();
	void ConfigureSceneCapture();
	void UpdateMarkers();
	void UpdateCapture(float DeltaTime);
	void UpdateMapDiscovery();
	void EnsureDiscoveryFog();
	void ClearDiscoveryFog();
	void FlushDiscoveryFogTexture();
	void StampDiscoveryAtNormalized(const FVector2D& NormalizedUV);
	/** Square orthographic footprint centered on WorldMin/WorldMax (matches capture ↔ markers). */
	void GetOrthoWorldRect(float& OutCenterX, float& OutCenterY, float& OutOrthoWidth) const;
	bool LocalToWorld(const FVector2D& LocalPos, const FGeometry& Geometry, FVector& OutWorld) const;
	/** Map pointer to world using the frame widget geometry (not full-viewport root). */
	bool TryPointerToWorld(const FPointerEvent& MouseEvent, FVector& OutWorld) const;
	FVector2D WorldToNormalized(const FVector& WorldLoc) const;
	void PanCameraToWorld(const FVector& WorldLoc);
	void MoveChampionToWorld(const FVector& WorldLoc);
	/** Apply texture to a minimap image brush. */
	void ApplyCaptureBrush(UImage* TargetImage, UTexture* Texture, const FVector2D& ImageSize) const;
	/** Mirror the scene-capture image so it matches WorldToNormalized markers/clicks. */
	void ApplyCaptureImageFlip() const;
	APawn* ResolveChampion() const;
	AMobaCameraPawn* ResolveCameraPawn() const;
	void PlaceMarker(UBorder* Marker, UCanvasPanelSlot* Slot, const FVector2D& Normalized, float HalfSize);
	AActor* FindFirstActorOfSoftClass(const FSoftClassPath& ClassPath) const;
	void RegisterLandmarkFogReveals();
	/** Apply hit-test policy so only the map frame eats input. */
	void ApplyHitTestPolicy();
	void BindMapPointerEvents();
	/** Shared LMB pan / RMB champion-move for frame/image delegates and native handlers. */
	FReply HandleMapPointerDown(const FPointerEvent& MouseEvent);
	FReply HandleMapPointerMove(const FPointerEvent& MouseEvent);
	FReply HandleMapPointerUp(const FPointerEvent& MouseEvent);

	UFUNCTION()
	FEventReply OnMapMouseButtonDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent);

	UFUNCTION()
	FEventReply OnMapMouseMove(FGeometry MyGeometry, const FPointerEvent& MouseEvent);

	UFUNCTION()
	FEventReply OnMapMouseButtonUp(FGeometry MyGeometry, const FPointerEvent& MouseEvent);

	UPROPERTY()
	TObjectPtr<UCanvasPanel> RootCanvas = nullptr;

	UPROPERTY()
	TObjectPtr<USizeBox> FrameSizeBox = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> FrameBorder = nullptr;

	UPROPERTY()
	TObjectPtr<UImage> MapImage = nullptr;

	/** Black fog over unexplored regions; alpha punched out by champion discovery. */
	UPROPERTY()
	TObjectPtr<UImage> FogImage = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> ChampionMarker = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> CameraMarker = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> CrystalMarker = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> EnemySpawnMarker = nullptr;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> ChampionSlot = nullptr;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> CameraSlot = nullptr;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> CrystalSlot = nullptr;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> EnemySpawnSlot = nullptr;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CachedCrystalActor;

	UPROPERTY(Transient)
	TWeakObjectPtr<AActor> CachedEnemySpawnActor;

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> RenderTarget = nullptr;

	UPROPERTY()
	TObjectPtr<ASceneCapture2D> CaptureActor = nullptr;

	/** CPU-backed fog texture: RGB = UndiscoveredColor, A = remaining fog (255 = fully hidden). */
	UPROPERTY()
	TObjectPtr<UTexture2D> DiscoveryFogTexture = nullptr;

	/** Shared discovery from player controller when available. */
	UPROPERTY(Transient)
	TObjectPtr<UMapDiscoveryComponent> DiscoverySource = nullptr;

	TArray<FColor> DiscoveryFogPixels;
	FVector LastDiscoveryStampLocation = FVector(ForceInitToZero);
	bool bHasDiscoveryStamp = false;
	bool bDiscoveryFogDirty = false;
	int32 DiscoveryFogTextureSize = 0;

	float CaptureTimer = 0.0f;
	float AutoFitTimer = 0.0f;
	float LandmarkRefreshTimer = 0.0f;
	bool bBuilt = false;
	bool bDragging = false;
	bool bCaptureReady = false;
	bool bDidAutoFitBounds = false;
	bool bLandmarksRegistered = false;
};
