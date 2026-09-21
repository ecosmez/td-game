#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TDWavePowerUp.h"
#include "WavePowerUpWidget.generated.h"

class AActor;
class UBorder;
class UButton;
class UCanvasPanel;
class UHorizontalBox;
class UTextBlock;
class UVerticalBox;
class UWavePowerUpWidget;

UCLASS()
class TD_API UWavePowerUpCardBinder : public UObject
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UWavePowerUpWidget> Owner = nullptr;

	int32 CardIndex = INDEX_NONE;

	UFUNCTION()
	void HandleSelectClicked();

	UFUNCTION()
	void HandleRerollClicked();
};

struct FWavePowerUpCardUI
{
	int32 CardIndex = INDEX_NONE;
	TObjectPtr<UBorder> CardFrame = nullptr;
	TObjectPtr<UButton> SelectButton = nullptr;
	TObjectPtr<UTextBlock> PlayerTitle = nullptr;
	TObjectPtr<UTextBlock> PlayerDesc = nullptr;
	TObjectPtr<UTextBlock> EnemyTitle = nullptr;
	TObjectPtr<UTextBlock> EnemyDesc = nullptr;
	TObjectPtr<UButton> RerollButton = nullptr;
	TObjectPtr<UTextBlock> RerollLabel = nullptr;
};

/**
 * Blocking overlay: 3 double-sided power-up cards between waves.
 * Player buff on the face, enemy buff on the back. Pick one; reroll costs Resource.
 */
UCLASS(Blueprintable)
class TD_API UWavePowerUpWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWavePowerUpWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	static const TCHAR* GetWidgetBlueprintPath();
	static TSubclassOf<UWavePowerUpWidget> ResolveWidgetClass();

	/** Show the draft for this spawner, creating the overlay if needed. */
	static bool OpenDraft(AActor* Spawner);

	void ShowDraft();
	void HideDraft();
	bool IsDraftVisible() const { return bDraftVisible; }

	void OnCardSelected(int32 CardIndex);
	void OnCardRerolled(int32 CardIndex);

protected:
	void EnsureBuilt();
	void BindDesignerWidgets();
	void BuildFallbackTree();
	void BuildCards();
	FWavePowerUpCardUI BuildCard(int32 Index);
	void RefreshCards();
	void RefreshRerollAffordability();
	void ApplyHitTestPolicy();

	static void SetTextStyle(UTextBlock* Text, const FLinearColor& Color, float Size, bool bBold = false);

	UPROPERTY()
	TObjectPtr<UCanvasPanel> RootCanvas = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> Dimmer = nullptr;

	UPROPERTY()
	TObjectPtr<UVerticalBox> PanelBox = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleLabel = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> SubtitleLabel = nullptr;

	UPROPERTY()
	TObjectPtr<UHorizontalBox> CardRow = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UWavePowerUpCardBinder>> Binders;

	TArray<FWavePowerUpCardUI> Cards;
	bool bBuilt = false;
	bool bDraftVisible = false;
};
