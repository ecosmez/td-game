#include "AbilityBarWidget.h"

#include "MinimapWidget.h"
#include "TDUIInputLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

namespace AbilityBarPrivate
{
	static FLinearColor SlotReadyBg(0.10f, 0.14f, 0.20f, 0.92f);
	static FLinearColor SlotReadyFrame(0.55f, 0.62f, 0.72f, 1.f);
	static FLinearColor SlotAimFrame(0.95f, 0.82f, 0.25f, 1.f);
	static FLinearColor SlotLockedBg(0.05f, 0.05f, 0.07f, 0.95f);
	static FLinearColor CdOverlay(0.02f, 0.02f, 0.04f, 0.78f);
	static FLinearColor KeyColor(0.92f, 0.95f, 1.f, 1.f);
	static FLinearColor CdTextColor(1.f, 0.88f, 0.35f, 1.f);
	static FLinearColor LockTextColor(0.75f, 0.75f, 0.8f, 1.f);
}

UAbilityBarWidget::UAbilityBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
}

void UAbilityBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureBuilt();
}

void UAbilityBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureBuilt();
	// Visible but ignore mouse so world click-to-move / cast still works.
	SetVisibility(ESlateVisibility::HitTestInvisible);
	UE_LOG(LogTemp, Display, TEXT("AbilityBarWidget constructed. Built=%d Root=%s"),
		bBuilt ? 1 : 0,
		WidgetTree && WidgetTree->RootWidget ? *WidgetTree->RootWidget->GetName() : TEXT("None"));

	// Ensure a bottom-right minimap exists for non-MOBA controllers as well.
	if (APlayerController* PC = GetOwningPlayer())
	{
		bool bAlready = false;
		for (TObjectIterator<UMinimapWidget> It; It; ++It)
		{
			if (It->GetOwningPlayer() == PC && It->IsInViewport())
			{
				bAlready = true;
				break;
			}
		}
		if (!bAlready)
		{
			if (UMinimapWidget* Mini = CreateWidget<UMinimapWidget>(PC, UMinimapWidget::StaticClass()))
			{
				Mini->AddToViewport(20);
			}
		}
	}
}

void UAbilityBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bBuilt)
	{
		EnsureBuilt();
	}

	if (APawn* Pawn = ResolveChampionPawn())
	{
		RefreshFromPawn(Pawn);
	}
}

bool UAbilityBarWidget::ShouldBlockWorldClickInput(const UObject* WorldContextObject)
{
	return UTDUIInputLibrary::ShouldBlockWorldClickInput(WorldContextObject, true);
}

void UAbilityBarWidget::EnsureBuilt()
{
	if (bBuilt)
	{
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	if (!WidgetTree->RootWidget)
	{
		BuildDefaultUI();
	}
	else if (!SlotRow)
	{
		// Blueprint may already own a tree; still build a bottom-centered row if missing.
		BuildDefaultUI();
	}

	bBuilt = SlotQ.Button != nullptr && SlotR.Button != nullptr;
}

void UAbilityBarWidget::BuildDefaultUI()
{
	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Root)
	{
		Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("AbilityBarRoot"));
		WidgetTree->RootWidget = Root;
	}

	UBorder* BarChrome = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AbilityBarChrome"));
	BarChrome->SetPadding(FMargin(16.f, 12.f));
	// High-contrast chrome so the bar is obvious against the greybox.
	BarChrome->SetBrushColor(FLinearColor(0.05f, 0.08f, 0.12f, 0.92f));
	if (UCanvasPanelSlot* ChromeSlot = Root->AddChildToCanvas(BarChrome))
	{
		ChromeSlot->SetAnchors(FAnchors(0.5f, 1.f, 0.5f, 1.f));
		ChromeSlot->SetAlignment(FVector2D(0.5f, 1.f));
		ChromeSlot->SetAutoSize(true);
		ChromeSlot->SetOffsets(FMargin(0.f, 0.f, 0.f, 40.f));
		ChromeSlot->SetZOrder(10);
	}

	SlotRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AbilitySlotRow"));
	BarChrome->SetContent(SlotRow);

	SlotQ = BuildSlot(SlotRow, TEXT('Q'), 1);
	SlotW = BuildSlot(SlotRow, TEXT('W'), 2);
	SlotE = BuildSlot(SlotRow, TEXT('E'), 3);
	SlotR = BuildSlot(SlotRow, TEXT('R'), 4);
}

FAbilityBarSlotWidgets UAbilityBarWidget::BuildSlot(UHorizontalBox* Parent, TCHAR KeyChar, int32 AbilityId)
{
	FAbilityBarSlotWidgets Out;
	if (!Parent || !WidgetTree)
	{
		return Out;
	}

	const FString KeyStr = FString(1, &KeyChar);

	Out.SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("Size_%s"), *KeyStr));
	Out.SizeBox->SetWidthOverride(SlotSize);
	Out.SizeBox->SetHeightOverride(SlotSize);

	Out.Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("Frame_%s"), *KeyStr));
	Out.Frame->SetPadding(FMargin(2.f));
	Out.Frame->SetBrushColor(AbilityBarPrivate::SlotReadyFrame);
	Out.SizeBox->SetContent(Out.Frame);

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), *FString::Printf(TEXT("Slot_%s"), *KeyStr));
	Out.Frame->SetContent(Overlay);

	Out.Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *FString::Printf(TEXT("Btn_%s"), *KeyStr));
	Out.Button->SetBackgroundColor(AbilityBarPrivate::SlotReadyBg);
	Out.Button->SetIsEnabled(false); // Visual only; casting stays on character keybinds.
	if (UOverlaySlot* BtnSlot = Overlay->AddChildToOverlay(Out.Button))
	{
		BtnSlot->SetHorizontalAlignment(HAlign_Fill);
		BtnSlot->SetVerticalAlignment(VAlign_Fill);
	}

	Out.CooldownBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), *FString::Printf(TEXT("CDBar_%s"), *KeyStr));
	Out.CooldownBar->SetPercent(0.f);
	Out.CooldownBar->SetFillColorAndOpacity(AbilityBarPrivate::CdOverlay);
	Out.CooldownBar->SetBarFillType(EProgressBarFillType::TopToBottom);
	Out.CooldownBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UOverlaySlot* CdSlot = Overlay->AddChildToOverlay(Out.CooldownBar))
	{
		CdSlot->SetHorizontalAlignment(HAlign_Fill);
		CdSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UVerticalBox* Labels = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *FString::Printf(TEXT("Labels_%s"), *KeyStr));
	Labels->SetVisibility(ESlateVisibility::HitTestInvisible);

	Out.KeyLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("Key_%s"), *KeyStr));
	Out.KeyLabel->SetText(FText::FromString(KeyStr));
	Out.KeyLabel->SetJustification(ETextJustify::Center);
	Out.KeyLabel->SetColorAndOpacity(FSlateColor(AbilityBarPrivate::KeyColor));
	Out.KeyLabel->SetShadowOffset(FVector2D(1.f, 1.f));
	Out.KeyLabel->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
	if (UVerticalBoxSlot* KeySlot = Labels->AddChildToVerticalBox(Out.KeyLabel))
	{
		KeySlot->SetHorizontalAlignment(HAlign_Center);
		KeySlot->SetPadding(FMargin(0.f, 12.f, 0.f, 0.f));
	}

	Out.CooldownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("CDText_%s"), *KeyStr));
	Out.CooldownText->SetText(FText::GetEmpty());
	Out.CooldownText->SetJustification(ETextJustify::Center);
	Out.CooldownText->SetColorAndOpacity(FSlateColor(AbilityBarPrivate::CdTextColor));
	Out.CooldownText->SetShadowOffset(FVector2D(1.f, 1.f));
	Out.CooldownText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f));
	if (UVerticalBoxSlot* CdTextSlot = Labels->AddChildToVerticalBox(Out.CooldownText))
	{
		CdTextSlot->SetHorizontalAlignment(HAlign_Center);
	}

	Out.LockText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("Lock_%s"), *KeyStr));
	Out.LockText->SetText(FText::FromString(TEXT("LOCKED")));
	Out.LockText->SetJustification(ETextJustify::Center);
	Out.LockText->SetColorAndOpacity(FSlateColor(AbilityBarPrivate::LockTextColor));
	Out.LockText->SetVisibility(ESlateVisibility::Collapsed);
	if (UVerticalBoxSlot* LockSlot = Labels->AddChildToVerticalBox(Out.LockText))
	{
		LockSlot->SetHorizontalAlignment(HAlign_Center);
		LockSlot->SetPadding(FMargin(0.f, 2.f, 0.f, 0.f));
	}

	if (UOverlaySlot* LabelSlot = Overlay->AddChildToOverlay(Labels))
	{
		LabelSlot->SetHorizontalAlignment(HAlign_Fill);
		LabelSlot->SetVerticalAlignment(VAlign_Fill);
	}

	if (UHorizontalBoxSlot* RowSlot = Parent->AddChildToHorizontalBox(Out.SizeBox))
	{
		RowSlot->SetPadding(FMargin(8.f, 0.f));
		RowSlot->SetVerticalAlignment(VAlign_Center);
	}

	return Out;
}

APawn* UAbilityBarWidget::ResolveChampionPawn() const
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		return PC->GetPawn();
	}
	return GetOwningPlayerPawn();
}

void UAbilityBarWidget::RefreshFromPawn(APawn* Pawn)
{
	if (!Pawn || !bBuilt)
	{
		return;
	}

	float CdQ = 0.f, CdW = 0.f, CdE = 0.f, CdR = 0.f;
	float MaxQ = 1.f, MaxW = 1.f, MaxE = 1.f, MaxR = 1.f;
	bool bDropping = false;
	int32 Level = 1;
	int32 Pending = 0;

	ReadFloatProp(Pawn, FName(TEXT("CD_Q")), CdQ);
	ReadFloatProp(Pawn, FName(TEXT("CD_W")), CdW);
	ReadFloatProp(Pawn, FName(TEXT("CD_E")), CdE);
	ReadFloatProp(Pawn, FName(TEXT("CD_R")), CdR);
	ReadFloatProp(Pawn, FName(TEXT("MaxCD_Q")), MaxQ);
	ReadFloatProp(Pawn, FName(TEXT("MaxCD_W")), MaxW);
	ReadFloatProp(Pawn, FName(TEXT("MaxCD_E")), MaxE);
	ReadFloatProp(Pawn, FName(TEXT("MaxCD_R")), MaxR);
	if (!ReadBoolProp(Pawn, FName(TEXT("bIsDropping")), bDropping))
	{
		ReadBoolProp(Pawn, FName(TEXT("IsDropping")), bDropping);
	}
	ReadIntProp(Pawn, FName(TEXT("ChampionLevel")), Level);
	ReadIntProp(Pawn, FName(TEXT("PendingAbility")), Pending);

	ApplySlotState(SlotQ, 1, CdQ, MaxQ, bDropping, Level, Pending);
	ApplySlotState(SlotW, 2, CdW, MaxW, bDropping, Level, Pending);
	ApplySlotState(SlotE, 3, CdE, MaxE, bDropping, Level, Pending);
	ApplySlotState(SlotR, 4, CdR, MaxR, bDropping, Level, Pending);
}

void UAbilityBarWidget::ApplySlotState(FAbilityBarSlotWidgets& SlotUI, int32 AbilityId, float RemainingCD, float MaxCD,
	bool bDropping, int32 ChampionLevel, int32 PendingAbility)
{
	if (!SlotUI.Button || !SlotUI.CooldownBar || !SlotUI.CooldownText)
	{
		return;
	}

	const bool bUltLocked = (AbilityId == 4) && (ChampionLevel < UltimateUnlockLevel);
	const bool bOnCooldown = RemainingCD > 0.05f;
	const bool bAiming = PendingAbility == AbilityId;

	const float SafeMax = FMath::Max(MaxCD, 0.01f);
	const float CdPercent = bOnCooldown ? FMath::Clamp(RemainingCD / SafeMax, 0.f, 1.f) : 0.f;
	SlotUI.CooldownBar->SetPercent(CdPercent);
	SlotUI.CooldownBar->SetVisibility(bOnCooldown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);

	if (bOnCooldown)
	{
		const FString CdStr = RemainingCD >= 10.f
			? FString::Printf(TEXT("%d"), FMath::CeilToInt(RemainingCD))
			: FString::Printf(TEXT("%.1f"), RemainingCD);
		SlotUI.CooldownText->SetText(FText::FromString(CdStr));
		SlotUI.CooldownText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		SlotUI.CooldownText->SetText(FText::GetEmpty());
		SlotUI.CooldownText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (SlotUI.LockText)
	{
		SlotUI.LockText->SetVisibility(bUltLocked ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		if (bUltLocked)
		{
			SlotUI.LockText->SetText(FText::FromString(FString::Printf(TEXT("Lv%d"), UltimateUnlockLevel)));
		}
	}

	if (SlotUI.Frame)
	{
		SlotUI.Frame->SetBrushColor(bAiming ? AbilityBarPrivate::SlotAimFrame : AbilityBarPrivate::SlotReadyFrame);
	}

	if (SlotUI.SizeBox)
	{
		if (bUltLocked)
		{
			SlotUI.Button->SetBackgroundColor(AbilityBarPrivate::SlotLockedBg);
			SlotUI.SizeBox->SetRenderOpacity(0.45f);
		}
		else if (bDropping)
		{
			SlotUI.Button->SetBackgroundColor(AbilityBarPrivate::SlotLockedBg);
			SlotUI.SizeBox->SetRenderOpacity(0.4f);
		}
		else if (bOnCooldown)
		{
			SlotUI.Button->SetBackgroundColor(AbilityBarPrivate::SlotReadyBg);
			SlotUI.SizeBox->SetRenderOpacity(0.85f);
		}
		else
		{
			SlotUI.Button->SetBackgroundColor(AbilityBarPrivate::SlotReadyBg);
			SlotUI.SizeBox->SetRenderOpacity(bAiming ? 1.f : 0.98f);
		}
	}

	// Keep buttons non-interactive; hotkeys live on the character.
	SlotUI.Button->SetIsEnabled(false);
}

bool UAbilityBarWidget::ReadBoolProp(const UObject* Obj, FName Name, bool& OutValue)
{
	if (!Obj)
	{
		return false;
	}
	if (const FBoolProperty* Prop = FindFProperty<FBoolProperty>(Obj->GetClass(), Name))
	{
		OutValue = Prop->GetPropertyValue_InContainer(Obj);
		return true;
	}
	return false;
}

bool UAbilityBarWidget::ReadFloatProp(const UObject* Obj, FName Name, float& OutValue)
{
	if (!Obj)
	{
		return false;
	}
	if (const FFloatProperty* Prop = FindFProperty<FFloatProperty>(Obj->GetClass(), Name))
	{
		OutValue = Prop->GetPropertyValue_InContainer(Obj);
		return true;
	}
	if (const FDoubleProperty* DProp = FindFProperty<FDoubleProperty>(Obj->GetClass(), Name))
	{
		OutValue = static_cast<float>(DProp->GetPropertyValue_InContainer(Obj));
		return true;
	}
	return false;
}

bool UAbilityBarWidget::ReadIntProp(const UObject* Obj, FName Name, int32& OutValue)
{
	if (!Obj)
	{
		return false;
	}
	if (const FIntProperty* Prop = FindFProperty<FIntProperty>(Obj->GetClass(), Name))
	{
		OutValue = Prop->GetPropertyValue_InContainer(Obj);
		return true;
	}
	if (const FByteProperty* ByteProp = FindFProperty<FByteProperty>(Obj->GetClass(), Name))
	{
		OutValue = ByteProp->GetPropertyValue_InContainer(Obj);
		return true;
	}
	return false;
}
