#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UObject/SoftObjectPath.h"
#include "CrystalHealthBarWidget.generated.h"

class UBorder;
class UProgressBar;
class UTextBlock;
class USizeBox;

/**
 * Screen-space crystal HP bar — top-center chrome, same visual language as the ability bar.
 * Polls BP_Crystal CurrentHealth / MaxHealth each tick (no crystal health dispatcher yet).
 */
UCLASS()
class TD_API UCrystalHealthBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UCrystalHealthBarWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Soft class path for BP_Crystal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crystal Health")
	FSoftClassPath CrystalActorClass =
		FSoftClassPath(TEXT("/Game/TD/BP_Crystal.BP_Crystal_C"));

	/** Bar width in slate units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crystal Health", meta = (ClampMin = "120.0"))
	float BarWidth = 420.f;

	/** Progress track height in slate units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crystal Health", meta = (ClampMin = "8.0"))
	float BarHeight = 22.f;

	/** Distance from the top edge of the screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crystal Health")
	float TopPad = 40.f;

protected:
	void EnsureBuilt();
	void BuildDefaultUI();
	void ApplyHitTestPolicy();
	void RefreshFromCrystal();

	AActor* FindCrystal() const;

	static bool ReadFloatProp(const UObject* Obj, FName Name, float& OutValue);

	UPROPERTY()
	TObjectPtr<UBorder> BarChrome = nullptr;

	UPROPERTY()
	TObjectPtr<USizeBox> BarSizeBox = nullptr;

	UPROPERTY()
	TObjectPtr<UProgressBar> HealthBar = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleLabel = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> ValueLabel = nullptr;

	UPROPERTY()
	TWeakObjectPtr<AActor> CachedCrystal;

	bool bBuilt = false;
};
