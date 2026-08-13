#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbilityBarWidget.generated.h"

class UHorizontalBox;
class UButton;
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

	/** Clips the dark remaining-CD wipe (height = SlotSize * Remaining/Max). */
	UPROPERTY()
	TObjectPtr<USizeBox> CooldownClip = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> CooldownFill = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> KeyLabel = nullptr;

	/** Large centered countdown while on cooldown (LoL-style). */
	UPROPERTY()
	TObjectPtr<UTextBlock> CooldownText = nullptr;

	/** Locked / unavailable badge (e.g. "Lv3", "—"). */
	UPROPERTY()
	TObjectPtr<UTextBlock> LockText = nullptr;

	/** Hotkey letter for this slot (Q/W/E/R). Not reflected. */
	TCHAR KeyChar = TEXT('\0');
};

/**
 * League-style champion ability bar (+ store | Q W E R).
 * Leading "+" opens the tower store. Next-wave play lives on the top Base Health HUD.
 * Ability slots read CD_* from the champion pawn.
 *
 * States per ability slot (LoL-style):
 * - Available: bright cyan frame, full opacity, key letter
 * - On cooldown: dark remaining wipe + large gold seconds
 * - Unavailable (dropping / locked ult): dimmed + Lv# / —
 * - Aiming: gold frame when PendingAbility matches the slot
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
	void BuildStorePlusSlot(UHorizontalBox* Parent);
	void ApplyHitTestPolicy();
	void RefreshStorePlusVisual();

	UFUNCTION()
	void OnStorePlusClicked();

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
	TObjectPtr<UBorder> BarChrome = nullptr;

	/** Tower store toggle — first button in the bar, before Q. */
	UPROPERTY()
	TObjectPtr<USizeBox> StorePlusSizeBox = nullptr;

	UPROPERTY()
	TObjectPtr<UBorder> StorePlusFrame = nullptr;

	UPROPERTY()
	TObjectPtr<UButton> StorePlusButton = nullptr;

	UPROPERTY()
	TObjectPtr<UTextBlock> StorePlusLabel = nullptr;

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
