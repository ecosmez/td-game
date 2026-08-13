#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UObject/SoftObjectPath.h"
#include "CrystalHealthBarWidget.generated.h"

class UBorder;
class UButton;
class UHorizontalBox;
class UProgressBar;
class USizeBox;
class UTextBlock;

/**
 * Top-center Base Health + wave HUD (replaces the old crystal-only bar).
 * Polls BP_Crystal CurrentHealth / MaxHealth and BP_EnemySpawner wave state each tick.
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
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Health")
	FSoftClassPath CrystalActorClass =
		FSoftClassPath(TEXT("/Game/TD/BP_Crystal.BP_Crystal_C"));

	/** Soft class path for BP_EnemySpawner (wave dots, countdown, next-wave). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Health")
	FSoftClassPath EnemySpawnerActorClass =
		FSoftClassPath(TEXT("/Game/TD/BP_EnemySpawner.BP_EnemySpawner_C"));

	/** Bar width in slate units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Health", meta = (ClampMin = "120.0"))
	float BarWidth = 460.f;

	/** Progress track height in slate units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Health", meta = (ClampMin = "8.0"))
	float BarHeight = 26.f;

	/** Distance from the top edge of the screen. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Health")
	float TopPad = 28.f;

	/** Wave dots shown in the strip (spawner TotalWaves / MaxWaves overrides when set). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Health", meta = (ClampMin = "1", ClampMax = "20"))
	int32 TotalWaves = 7;

	/** Dot diameter in slate units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Health", meta = (ClampMin = "8.0"))
	float CircleSize = 16.f;

	/** Play / next-wave button diameter. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Base Health", meta = (ClampMin = "20.0"))
	float PlayButtonSize = 36.f;

protected:
	void EnsureBuilt();
	void BuildDefaultUI();
	void BuildWaveRow(UHorizontalBox* Parent);
	void RebuildWaveDots();
	void ApplyHitTestPolicy();
	void RefreshFromCrystal();
	void RefreshWaveHud();

	UFUNCTION()
	void OnNextWaveClicked();

	AActor* FindCrystal() const;
	AActor* FindEnemySpawner() const;
	int32 ResolveTotalWaves(const AActor* Spawner) const;

	static void ApplyRoundedBrush(UBorder* Border, const FLinearColor& Fill, const FLinearColor& Outline,
		float OutlineWidth, bool bCircle);
	static bool ReadFloatProp(const UObject* Obj, FName Name, float& OutValue);
	static bool ReadBoolProp(const UObject* Obj, FName Name, bool& OutValue);
	static bool ReadIntProp(const UObject* Obj, FName Name, int32& OutValue);

	UPROPERTY()
	TObjectPtr<UBorder> BarChrome = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> WaveChrome = nullptr;

	UPROPERTY()
	TObjectPtr<USizeBox> BarSizeBox = nullptr;

	UPROPERTY()
	TObjectPtr<UProgressBar> HealthBar = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> TitleLabel = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> ValueLabel = nullptr;

	UPROPERTY()
	TObjectPtr<UHorizontalBox> WaveDotsBox = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UBorder>> WaveDots;

	UPROPERTY()
	TObjectPtr<USizeBox> BossSizeBox = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> BossFrame = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> BossLabel = nullptr;

	UPROPERTY()
	TObjectPtr<USizeBox> NextWaveSizeBox = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> NextWaveFrame = nullptr;

	UPROPERTY()
	TObjectPtr<UButton> NextWaveButton = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> NextWaveLabel = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> TimerLabel = nullptr;

	UPROPERTY()
	TWeakObjectPtr<AActor> CachedCrystal;

	UPROPERTY()
	TWeakObjectPtr<AActor> CachedSpawner;

	int32 BuiltDotCount = 0;
	bool bBuilt = false;
};
