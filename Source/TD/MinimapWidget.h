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
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
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

	/**
	 * Tightens the orthographic capture around the world-bounds center (1.0 = fit exactly,
	 * smaller = zoom in / show less area). LoL-style minimaps read as a close, filled-frame
	 * crop rather than the whole level with empty margins.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|World", meta = (ClampMin = "0.1", ClampMax = "1.0"))
	float MinimapZoomFactor = 0.6f;

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

	/** Live enemy blips — red dots for every walking BP_Enemy, updated as they move. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Enemies")
	bool bShowEnemyMarkers = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Enemies")
	FLinearColor EnemyMarkerColor = FLinearColor(0.95f, 0.12f, 0.10f, 1.0f);

	/** Half-size of each enemy dot on the minimap (slate units). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Enemies", meta = (ClampMin = "1.0"))
	float EnemyMarkerHalfSize = 4.0f;

	/** Soft class path for BP_Enemy (and subclasses) tracked as walking blips. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Enemies")
	FSoftClassPath EnemyActorClass = FSoftClassPath(TEXT("/Game/TD/BP_Enemy.BP_Enemy_C"));

	/** Re-scan live enemies every N seconds (0 = every frame). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Enemies", meta = (ClampMin = "0.0"))
	float EnemyMarkerUpdateInterval = 0.1f;

	/** Blue blips for every placed tower (BP_Tower and subclasses). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Towers")
	bool bShowTowerMarkers = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Towers")
	FLinearColor TowerMarkerColor = FLinearColor(0.15f, 0.55f, 0.98f, 1.0f);

	/** Half-size of each tower blip on the minimap (slate units). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Towers", meta = (ClampMin = "1.0"))
	float TowerMarkerHalfSize = 5.0f;

	/** Soft class path for BP_Tower (and subclasses: arrow, cannon, sniper, ...). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Towers")
	FSoftClassPath TowerActorClass = FSoftClassPath(TEXT("/Game/TD/BP_Tower.BP_Tower_C"));

	/** Re-scan placed towers every N seconds (0 = every frame). Towers are static, so this can be slower than enemy scanning. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Towers", meta = (ClampMin = "0.0"))
	float TowerMarkerUpdateInterval = 0.5f;

	/** White ring behind the champion marker so it reads as a distinct "avatar" icon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	bool bShowChampionAvatarFrame = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	FLinearColor ChampionAvatarFrameColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.9f);

	/** How much larger the avatar frame is than the champion marker itself (slate units, added to half-size). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap", meta = (ClampMin = "0.0"))
	float ChampionAvatarFramePadding = 3.0f;

	/** Draws a quad on the minimap outlining the ground area currently visible on screen (LoL-style camera box). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|View Frustum")
	bool bShowViewFrustum = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|View Frustum")
	FLinearColor ViewFrustumColor = FLinearColor(1.0f, 1.0f, 1.0f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|View Frustum", meta = (ClampMin = "0.5"))
	float ViewFrustumThickness = 1.5f;

	/**
	 * Unused for live vision (crystal radius lives on UMapDiscoveryComponent).
	 * Kept so existing Blueprint defaults still serialize.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Landmarks", meta = (ClampMin = "100.0"))
	float LandmarkRevealRadius = 8000.0f;

	/** Soft class path for BP_Crystal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Landmarks")
	FSoftClassPath CrystalActorClass =
		FSoftClassPath(TEXT("/Game/TD/BP_Crystal.BP_Crystal_C"));

	/** Soft class path for BP_EnemySpawner (first found = initial spawn). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Landmarks")
	FSoftClassPath EnemySpawnerActorClass =
		FSoftClassPath(TEXT("/Game/TD/BP_EnemySpawner.BP_EnemySpawner_C"));

	/**
	 * League-style live vision overlay on the minimap (dim map, clear around vision).
	 * Prefer the shared UMapDiscoveryComponent from the player controller when set.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Discovery")
	bool bMapDiscoveryEnabled = true;

	/** World-space radius (cm) of current vision around the champion. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Discovery", meta = (ClampMin = "100.0", EditCondition = "bMapDiscoveryEnabled"))
	float DiscoveryRadius = 4000.0f;

	/** Soft edge width as a fraction of DiscoveryRadius (0 = hard circle, 1 = fully soft). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Discovery", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bMapDiscoveryEnabled"))
	float DiscoverySoftness = 0.35f;

	/** Resolution of the persistent reveal fog mask (square). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Discovery", meta = (ClampMin = "64", ClampMax = "1024", EditCondition = "bMapDiscoveryEnabled"))
	int32 DiscoveryMaskSize = 256;

	/** Champion must move at least this far (cm) before stamping another reveal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Discovery", meta = (ClampMin = "0.0", EditCondition = "bMapDiscoveryEnabled"))
	float DiscoveryStampDistance = 0.0f;

	/** Fog tint for dim (no-vision) minimap regions (alpha = overlay opacity). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap|Discovery", meta = (EditCondition = "bMapDiscoveryEnabled"))
	FLinearColor UndiscoveredColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.32f);

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
	void UpdateEnemyMarkers(float DeltaTime);
	UBorder* GetOrCreateEnemyMarker(int32 Index);
	void UpdateTowerMarkers(float DeltaTime);
	UBorder* GetOrCreateTowerMarker(int32 Index);
	/** Deprojects the four screen corners onto the ground plane and returns their minimap-normalized UVs. */
	bool ComputeViewFrustumCorners(TArray<FVector2D>& OutNormalizedCorners) const;
	/** Maps a minimap-normalized point into AllottedGeometry's local paint space (for NativePaint). */
	FVector2D NormalizedToRootLocal(const FVector2D& Normalized, const FGeometry& AllottedGeometry) const;
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
	/** Align discovery fog overlay with inverted minimap U mapping. */
	void ApplyMinimapAxisFlip() const;
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

	/** Dim overlay outside current vision; alpha punched out by champion/crystal vision. */
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

	/** Map canvas hosting all markers — kept so enemy blips can be added after initial build. */
	UPROPERTY()
	TObjectPtr<UCanvasPanel> MapCanvas = nullptr;

	/** Pooled live-enemy blip widgets, one per concurrently tracked enemy. */
	UPROPERTY()
	TArray<TObjectPtr<UBorder>> EnemyMarkers;

	UPROPERTY()
	TArray<TObjectPtr<UCanvasPanelSlot>> EnemyMarkerSlots;

	TWeakObjectPtr<UClass> CachedEnemyClass;

	float EnemyMarkerTimer = 0.0f;

	/** Pooled placed-tower blip widgets (blue), one per concurrently tracked tower. */
	UPROPERTY()
	TArray<TObjectPtr<UBorder>> TowerMarkers;

	UPROPERTY()
	TArray<TObjectPtr<UCanvasPanelSlot>> TowerMarkerSlots;

	TWeakObjectPtr<UClass> CachedTowerClass;

	float TowerMarkerTimer = 0.0f;

	/** White ring behind ChampionMarker so it reads as an avatar icon rather than a plain square. */
	UPROPERTY()
	TObjectPtr<UBorder> ChampionMarkerFrame = nullptr;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> ChampionFrameSlot = nullptr;

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
