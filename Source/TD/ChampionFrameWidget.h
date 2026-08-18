#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UObject/SoftObjectPath.h"
#include "ChampionFrameWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UImage;
class UProgressBar;
class USizeBox;
class UTextBlock;
class UTexture2D;

/**
 * LoL-style champion unit frame: circular avatar + HP bar, bottom-left.
 * Polls the controlled champion's CurrentHealth / MaxHealth (and ChampionLevel)
 * each tick. Optional portrait texture, otherwise a monogram fallback.
 */
UCLASS()
class TD_API UChampionFrameWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UChampionFrameWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Distance from the bottom-left corner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Champion Frame")
	FVector2D ScreenMargin = FVector2D(24.0f, 24.0f);

	/** Circular portrait diameter (slate units). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Champion Frame", meta = (ClampMin = "48.0"))
	float AvatarSize = 92.0f;

	/** HP bar width (slate units). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Champion Frame", meta = (ClampMin = "80.0"))
	float BarWidth = 196.0f;

	/** HP track height (slate units). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Champion Frame", meta = (ClampMin = "8.0"))
	float BarHeight = 22.0f;

	/** Optional portrait; pawn Portrait / Avatar / ChampionPortrait overrides when set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Champion Frame")
	FSoftObjectPath PortraitTexturePath;

	/** True when screen pos is over the unit-frame chrome (world-click blocking). */
	UFUNCTION(BlueprintPure, Category = "Champion Frame")
	bool IsScreenPosOverFrame(FVector2D ScreenPos) const;

	/** Bottom-left margin used to dock companion HUD (ability bar). */
	UFUNCTION(BlueprintPure, Category = "Champion Frame")
	FVector2D GetChromeScreenMargin() const { return ScreenMargin; }

	/** Size of the unit-frame chrome (live geometry when available, else estimated). */
	UFUNCTION(BlueprintPure, Category = "Champion Frame")
	FVector2D GetChromeScreenSize() const;

protected:
	void EnsureBuilt();
	void BuildDefaultUI();
	void ApplyHitTestPolicy();
	void RefreshFromChampion();
	void ApplyPortrait(UTexture2D* Texture);
	APawn* ResolveChampionPawn() const;
	bool TryReadHealth(const UObject* Obj, float& OutCurrent, float& OutMax) const;
	UTexture2D* ResolvePortraitTexture(const UObject* Obj) const;
	FString ResolveChampionName(const APawn* Pawn) const;

	static void ApplyRoundedBrush(UBorder* Border, const FLinearColor& Fill, const FLinearColor& Outline,
		float OutlineWidth, bool bCircle);
	static bool ReadFloatProp(const UObject* Obj, FName Name, float& OutValue);
	static bool ReadIntProp(const UObject* Obj, FName Name, int32& OutValue);
	static bool ReadStringProp(const UObject* Obj, FName Name, FString& OutValue);
	static UTexture2D* ReadTextureProp(const UObject* Obj, FName Name);

	UPROPERTY()
	TObjectPtr<UCanvasPanel> RootCanvas = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> FrameChrome = nullptr;

	UPROPERTY()
	TObjectPtr<USizeBox> AvatarSizeBox = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> AvatarFrame = nullptr;

	UPROPERTY()
	TObjectPtr<UImage> AvatarImage = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> AvatarLetter = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> LevelFrame = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> LevelLabel = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> NameLabel = nullptr;

	UPROPERTY()
	TObjectPtr<USizeBox> BarSizeBox = nullptr;

	UPROPERTY()
	TObjectPtr<UProgressBar> HealthBar = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> HealthValue = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> CachedPortrait = nullptr;

	bool bBuilt = false;
};
