#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TowerStoreWidget.generated.h"

class UCanvasPanel;
class UCanvasPanelSlot;
class UBorder;
class UButton;
class UImage;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UHorizontalBox;
class UScrollBox;
class UTextureRenderTarget2D;
class USceneCaptureComponent2D;
class UStaticMeshComponent;
class AActor;
class UTowerStoreWidget;

/** Tiny UObject binder so each card button can map events → card index. */
UCLASS()
class TD_API UTowerStoreCardClickBinder : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UTowerStoreWidget> Store = nullptr;

	int32 CardIndex = INDEX_NONE;

	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();

	UFUNCTION()
	void HandleUnhovered();
};

/** One store entry: identity, stats, and the select function on BuildManager. */
USTRUCT(BlueprintType)
struct FTowerStoreEntryDef
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store")
	FString DisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store")
	FString Role;

	/** Blueprint function on BP_BuildManager (e.g. SelectArrowTower). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store")
	FName SelectFunctionName;

	/** Soft path to the tower Blueprint class for CDO stats / mesh. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store")
	FString TowerClassPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store")
	FString MeshPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store")
	FString ColorMaterialPath;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store")
	int32 Cost = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store")
	float BuildTime = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store")
	float AttackSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store")
	float Damage = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store")
	float Range = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store")
	FString ExtraNote;
};

/** Runtime UI handles for one compact tower tile in the horizontal strip. */
USTRUCT()
struct FTowerStoreCardUI
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<UBorder> CardFrame = nullptr;

	UPROPERTY()
	TObjectPtr<UButton> CardButton = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> NameText = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> CostText = nullptr;

	FTowerStoreEntryDef Def;
	int32 CardIndex = INDEX_NONE;
};

/**
 * Tower store HUD docked above the ability bar:
 * - Toggle via the ability bar "+" slot (first button before Q)
 * - Horizontal tower strip
 * - Hover projects a translucent tower mesh + stats above the strip
 */
UCLASS()
class TD_API UTowerStoreWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UTowerStoreWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store")
	bool bStartOpen = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store", meta = (ClampMin = "64"))
	int32 PreviewRenderSize = 220;

	/** Matches AbilityBarWidget: bottom offset of the QWER chrome. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store|Layout")
	float AbilityBarBottomPad = 40.f;

	/** Ability bar chrome height (slot + padding). Includes a little slack so the store never covers it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store|Layout")
	float AbilityBarHeight = 118.f;

	/** Clear air between ability bar top and store strip bottom. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store|Layout")
	float StoreBottomGap = 28.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store|Layout")
	float StoreStripHeight = 86.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store|Layout")
	float StoreHeaderHeight = 28.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store|Layout")
	float StoreMaxWidth = 780.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store|Layout")
	float HoverPanelHeight = 180.f;

	/** Extra lift of the floating hover panel above the store strip. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store|Layout")
	float HoverFloatGap = 18.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store|Preview")
	FVector PreviewOrigin = FVector(0.f, 0.f, -80000.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store|Preview")
	float CaptureInterval = 0.06f;

	/** Soft path to translucent silhouette material (matches construction holo). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store|Preview")
	FString HoloMaterialPath = TEXT("/Game/TD/Materials/M_TowerHolo.M_TowerHolo");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store|Preview")
	FString FallbackMeshPath = TEXT("/Game/TD/Assets/Towers/TowerBase.TowerBase");

	/** Extra UI alpha on the hover projection (0–1). Lower = more see-through. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tower Store|Preview", meta = (ClampMin = "0.15", ClampMax = "1.0"))
	float HoverProjectionOpacity = 0.72f;

	UFUNCTION(BlueprintCallable, Category = "Tower Store")
	void SetStoreOpen(bool bOpen);

	UFUNCTION(BlueprintCallable, Category = "Tower Store")
	void ToggleStore();

	UFUNCTION(BlueprintPure, Category = "Tower Store")
	bool IsStoreOpen() const { return bStoreOpen; }

	void OnCardClicked(int32 CardIndex);
	void OnCardHovered(int32 CardIndex);
	void OnCardUnhovered(int32 CardIndex);

protected:
	void EnsureBuilt();
	void BuildDefaultUI();
	void ApplyHitTestPolicy();
	void ApplyDockLayout();
	float GetStoreStripBottomPad() const;
	float GetHoverFloatBottomPad() const;
	void BuildDefaultCatalog();
	void BuildCards();
	FTowerStoreCardUI BuildCard(const FTowerStoreEntryDef& Def, int32 Index);

	void EnsureHoverPreview();
	void DestroyHoverPreview();
	void UpdateHoverPreview(float DeltaTime);
	void CaptureHoverPreview();
	void SetHoveredCard(int32 CardIndex);
	void ApplyHoverMesh(const FTowerStoreEntryDef& Def);
	void ShowHoverPanel(bool bShow);

	UStaticMesh* ResolveMeshForEntry(const FTowerStoreEntryDef& Def) const;
	UMaterialInterface* ResolveHoloMaterial() const;
	void ApplyTranslucentPreviewMaterial(UStaticMeshComponent* MeshComp, const FTowerStoreEntryDef& Def) const;

	AActor* FindBuildManager() const;
	float ReadResource(AActor* BuildManager) const;
	bool CallBuildManagerSelect(const FName& FunctionName);
	void RefreshResourceLabel();
	void RefreshCardAffordability(float Resource);

	UFUNCTION()
	void OnCloseClicked();

	FString FormatStats(const FTowerStoreEntryDef& Def) const;
	FString FormatDetailedStats(const FTowerStoreEntryDef& Def) const;

	UPROPERTY()
	TArray<TObjectPtr<UObject>> CardClickBinders;

	UPROPERTY()
	TObjectPtr<UCanvasPanel> RootCanvas = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> StorePanel = nullptr;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> StorePanelSlot = nullptr;

	/** Floating mesh + stats — not inside store chrome; drawn over the world/UI. */
	UPROPERTY()
	TObjectPtr<USizeBox> HoverPanel = nullptr;

	UPROPERTY()
	TObjectPtr<UCanvasPanelSlot> HoverPanelSlot = nullptr;

	UPROPERTY()
	TObjectPtr<UImage> HoverMeshImage = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> HoverNameText = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> HoverCostText = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> HoverStatsText = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleText = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> ResourceText = nullptr;

	UPROPERTY()
	TObjectPtr<UButton> CloseButton = nullptr;

	UPROPERTY()
	TObjectPtr<UScrollBox> CardScroll = nullptr;

	UPROPERTY()
	TObjectPtr<UHorizontalBox> CardRow = nullptr;

	UPROPERTY()
	TArray<FTowerStoreCardUI> Cards;

	UPROPERTY()
	TArray<FTowerStoreEntryDef> Catalog;

	/** Single off-screen stage used for hover mesh projection. */
	UPROPERTY()
	TObjectPtr<AActor> HoverPreviewActor = nullptr;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> HoverPreviewMesh = nullptr;

	UPROPERTY()
	TObjectPtr<USceneCaptureComponent2D> HoverCapture = nullptr;

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> HoverRenderTarget = nullptr;

	bool bBuilt = false;
	bool bStoreOpen = false;
	bool bHoverPreviewReady = false;
	int32 HoveredCardIndex = INDEX_NONE;
	float CaptureTimer = 0.f;
	float PreviewYaw = 25.f;
};
