#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbilityBarWidget.generated.h"

class UHorizontalBox;
class UButton;
class UProgressBar;
class UTextBlock;
class UBorder;
class USizeBox;

/** Per-slot runtime handles for the LoL-style ability HUD. */
USTRUCT()
struct FAbilityBarSlotWidgets
{
	GENERATED_BODY()

	UPROPERTY()
	TObjectPtr<USizeBox> SizeBox = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> Frame = nullptr;

	UPROPERTY()
	TObjectPtr<UButton> Button = nullptr;

	UPROPERTY()
	TObjectPtr<UProgressBar> CooldownBar = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> KeyLabel = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> CooldownText = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> LockText = nullptr;
};

/**
 * League-style champion ability bar (Q W E R).
 * Reads CD_*, MaxCD_*, bIsDropping, ChampionLevel, PendingAbility from the owning pawn
 * (Blueprint variables on BP_TopDownCharacter) via reflection.
 *
 * States per slot:
 * - Available: full opacity, no CD overlay
 * - On cooldown: dark fill + remaining seconds
 * - Unavailable (dropping / locked ult): dimmed + disabled
 * - Aiming: brighter frame when PendingAbility matches the slot
 */
UCLASS()
class TD_API UAbilityBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UAbilityBarWidget(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/**
	 * True when world click-to-move / destination pathing should ignore the cursor
	 * (tower store UI, HUD under cursor, or active tower drag/selection).
	 */
	UFUNCTION(BlueprintPure, Category = "TD|Input", meta = (WorldContext = "WorldContextObject"))
	static bool ShouldBlockWorldClickInput(const UObject* WorldContextObject);

	/** Minimum champion level required before R is available. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Bar")
	int32 UltimateUnlockLevel = 3;

	/** Slot edge length in slate units. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ability Bar", meta = (ClampMin = "32.0"))
	float SlotSize = 78.f;

protected:
	void EnsureBuilt();
	void BuildDefaultUI();
	FAbilityBarSlotWidgets BuildSlot(UHorizontalBox* Parent, TCHAR KeyChar, int32 AbilityId);

	APawn* ResolveChampionPawn() const;
	void RefreshFromPawn(APawn* Pawn);
	void ApplySlotState(FAbilityBarSlotWidgets& SlotUI, int32 AbilityId, float RemainingCD, float MaxCD,
		bool bDropping, int32 ChampionLevel, int32 PendingAbility);

	static bool ReadBoolProp(const UObject* Obj, FName Name, bool& OutValue);
	static bool ReadFloatProp(const UObject* Obj, FName Name, float& OutValue);
	static bool ReadIntProp(const UObject* Obj, FName Name, int32& OutValue);

	UPROPERTY()
	TObjectPtr<UHorizontalBox> SlotRow = nullptr;

	UPROPERTY()
	FAbilityBarSlotWidgets SlotQ;

	UPROPERTY()
	FAbilityBarSlotWidgets SlotW;

	UPROPERTY()
	FAbilityBarSlotWidgets SlotE;

	UPROPERTY()
	FAbilityBarSlotWidgets SlotR;

	bool bBuilt = false;
};
