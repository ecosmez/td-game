#include "AbilityBarWidget.h"

#include "ChampionFrameWidget.h"
#include "CrystalHealthBarWidget.h"
#include "MinimapWidget.h"
#include "MobaPlayerController.h"
#include "TowerStoreWidget.h"
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
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

namespace AbilityBarPrivate
{
	static constexpr int32 NumBarSlots = 5; // + Q W E R
	static constexpr float ChromePadX = 6.f;
	static constexpr float ChromePadY = 4.f;
	static constexpr float SlotPadX = 3.f;

	static FLinearColor SlotReadyBg(0.12f, 0.22f, 0.32f, 0.96f);
	static FLinearColor SlotReadyFrame(0.45f, 0.85f, 0.95f, 1.f);
	static FLinearColor SlotCdBg(0.06f, 0.08f, 0.12f, 0.96f);
	static FLinearColor SlotCdFrame(0.35f, 0.40f, 0.48f, 1.f);
	static FLinearColor SlotAimFrame(0.98f, 0.82f, 0.20f, 1.f);
	static FLinearColor SlotLockedBg(0.04f, 0.04f, 0.06f, 0.98f);
	static FLinearColor SlotLockedFrame(0.28f, 0.28f, 0.32f, 1.f);
	static FLinearColor CdOverlay(0.02f, 0.03f, 0.06f, 0.82f);
	static FLinearColor KeyColor(0.95f, 0.98f, 1.f, 1.f);
	static FLinearColor KeyColorDim(0.55f, 0.58f, 0.65f, 1.f);
	static FLinearColor CdTextColor(1.f, 0.90f, 0.30f, 1.f);
	static FLinearColor LockTextColor(0.72f, 0.74f, 0.80f, 1.f);
	static FLinearColor StorePlusBg(0.12f, 0.42f, 0.55f, 0.95f);
	static FLinearColor StorePlusBgOpen(0.55f, 0.28f, 0.18f, 0.95f);
	static FLinearColor StorePlusFrame(0.45f, 0.78f, 0.88f, 1.f);

	static void SetBoldFont(UTextBlock* Text, float Size)
	{
		if (!Text)
		{
			return;
		}
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = Size;
		Font.TypefaceFontName = TEXT("Bold");
		Text->SetFont(Font);
	}
}

UAbilityBarWidget::UAbilityBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
	// Ensure NativeTick runs even when the Widget BP has no Event Tick node.
	bHasScriptImplementedTick = true;
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
	// Root ignores empty space; bar chrome + store plus + ability slots receive clicks.
	ApplyHitTestPolicy();
	ApplyDockLayout();
	UE_LOG(LogTemp, Display, TEXT("AbilityBarWidget constructed. Built=%d Root=%s"),
		bBuilt ? 1 : 0,
		WidgetTree && WidgetTree->RootWidget ? *WidgetTree->RootWidget->GetName() : TEXT("None"));

	// Ensure companion HUD pieces exist for non-MOBA controllers as well.
	if (APlayerController* PC = GetOwningPlayer())
	{
		bool bMinimapAlready = false;
		for (TObjectIterator<UMinimapWidget> It; It; ++It)
		{
			if (It->GetOwningPlayer() == PC && It->IsInViewport())
			{
				bMinimapAlready = true;
				break;
			}
		}
		if (!bMinimapAlready)
		{
			if (UMinimapWidget* Mini = CreateWidget<UMinimapWidget>(PC, UMinimapWidget::StaticClass()))
			{
				Mini->AddToViewport(20);
			}
		}

		bool bCrystalBarAlready = false;
		for (TObjectIterator<UCrystalHealthBarWidget> It; It; ++It)
		{
			if (It->GetOwningPlayer() == PC && It->IsInViewport())
			{
				bCrystalBarAlready = true;
				break;
			}
		}
		if (!bCrystalBarAlready)
		{
			if (UCrystalHealthBarWidget* CrystalBar =
				CreateWidget<UCrystalHealthBarWidget>(PC, UCrystalHealthBarWidget::StaticClass()))
			{
				// Below ability bar Z=100; top-center so it does not overlap the bar.
				CrystalBar->AddToViewport(90);
			}
		}

		bool bChampionFrameAlready = false;
		for (TObjectIterator<UChampionFrameWidget> It; It; ++It)
		{
			if (It->GetOwningPlayer() == PC && It->IsInViewport())
			{
				bChampionFrameAlready = true;
				break;
			}
		}
		if (!bChampionFrameAlready)
		{
			if (UChampionFrameWidget* Frame =
				CreateWidget<UChampionFrameWidget>(PC, UChampionFrameWidget::StaticClass()))
			{
				// Below ability bar Z=100; bottom-left unit frame (abilities dock over it).
				Frame->AddToViewport(92);
			}
		}
	}

	ApplyDockLayout();
}

void UAbilityBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bBuilt || !BarSizeBox || (SlotRow && SlotRow->GetChildrenCount() != AbilityBarPrivate::NumBarSlots))
	{
		EnsureBuilt();
	}

	ApplyDockLayout();
	RefreshStorePlusVisual();

	if (APawn* Pawn = ResolveChampionPawn())
	{
		RefreshFromPawn(Pawn);
	}
}

bool UAbilityBarWidget::ShouldBlockWorldClickInput(const UObject* WorldContextObject)
{
	return UTDUIInputLibrary::ShouldBlockChampionClickToMove(WorldContextObject, true);
}

void UAbilityBarWidget::EnsureBuilt()
{
	if (bBuilt && StorePlusButton && BarSizeBox && SlotQ.CooldownClip
		&& (!SlotRow || SlotRow->GetChildrenCount() == AbilityBarPrivate::NumBarSlots))
	{
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	if (!WidgetTree->RootWidget || !SlotRow || !BarSizeBox)
	{
		BuildDefaultUI();
	}
	else if (!StorePlusButton || !SlotQ.CooldownClip
		|| (SlotRow && SlotRow->GetChildrenCount() != AbilityBarPrivate::NumBarSlots))
	{
		// Hot-reload / older runtime tree missing store opener, CD wipe, or leftover next-wave slot.
		if (SlotRow)
		{
			SlotRow->ClearChildren();
			BuildStorePlusSlot(SlotRow);
			SlotQ = BuildSlot(SlotRow, TEXT('Q'), 1);
			SlotW = BuildSlot(SlotRow, TEXT('W'), 2);
			SlotE = BuildSlot(SlotRow, TEXT('E'), 3);
			SlotR = BuildSlot(SlotRow, TEXT('R'), 4);
		}
		else
		{
			BuildDefaultUI();
		}
	}

	bBuilt = SlotQ.Button != nullptr && SlotR.Button != nullptr
		&& SlotQ.CooldownClip != nullptr
		&& StorePlusButton != nullptr
		&& BarSizeBox != nullptr;
	if (bBuilt)
	{
		ApplyHitTestPolicy();
		ApplyDockLayout();
	}
}

void UAbilityBarWidget::ApplyHitTestPolicy()
{
	// Full-screen root does not eat world clicks; chrome + interactive plus do.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UWidget* Root = WidgetTree ? WidgetTree->RootWidget : nullptr)
	{
		Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (BarChrome)
	{
		BarChrome->SetVisibility(ESlateVisibility::Visible);
	}
	if (SlotRow)
	{
		SlotRow->SetVisibility(ESlateVisibility::Visible);
	}
	if (StorePlusButton)
	{
		StorePlusButton->SetVisibility(ESlateVisibility::Visible);
	}
	auto ShowSlot = [](FAbilityBarSlotWidgets& SlotUI)
	{
		if (SlotUI.Button)
		{
			SlotUI.Button->SetVisibility(ESlateVisibility::Visible);
			SlotUI.Button->SetIsEnabled(true);
		}
	};
	ShowSlot(SlotQ);
	ShowSlot(SlotW);
	ShowSlot(SlotE);
	ShowSlot(SlotR);
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

	if (!BarChrome)
	{
		BarChrome = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("AbilityBarChrome"));
		BarChrome->SetPadding(FMargin(AbilityBarPrivate::ChromePadX, AbilityBarPrivate::ChromePadY));
		BarChrome->SetBrushColor(FLinearColor(0.05f, 0.08f, 0.12f, 0.72f));
		if (UCanvasPanelSlot* ChromeSlot = Root->AddChildToCanvas(BarChrome))
		{
			ChromeSlot->SetZOrder(10);
		}
	}
	else
	{
		BarChrome->SetPadding(FMargin(AbilityBarPrivate::ChromePadX, AbilityBarPrivate::ChromePadY));
	}

	if (!BarSizeBox)
	{
		BarSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AbilityBarSize"));
		BarChrome->SetContent(BarSizeBox);
	}

	if (!SlotRow)
	{
		SlotRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AbilitySlotRow"));
	}
	else
	{
		SlotRow->ClearChildren();
	}
	BarSizeBox->SetContent(SlotRow);

	// Tower store opener is the first button, left of Q.
	BuildStorePlusSlot(SlotRow);

	SlotQ = BuildSlot(SlotRow, TEXT('Q'), 1);
	SlotW = BuildSlot(SlotRow, TEXT('W'), 2);
	SlotE = BuildSlot(SlotRow, TEXT('E'), 3);
	SlotR = BuildSlot(SlotRow, TEXT('R'), 4);

	ApplyHitTestPolicy();
	ApplyDockLayout();
}

void UAbilityBarWidget::BuildStorePlusSlot(UHorizontalBox* Parent)
{
	if (!Parent || !WidgetTree)
	{
		return;
	}

	StorePlusSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("StorePlusSize"));
	StorePlusSizeBox->SetWidthOverride(EffectiveSlotSize);
	StorePlusSizeBox->SetHeightOverride(EffectiveSlotSize);

	StorePlusFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StorePlusFrame"));
	StorePlusFrame->SetPadding(FMargin(2.f));
	StorePlusFrame->SetBrushColor(AbilityBarPrivate::StorePlusFrame);
	StorePlusSizeBox->SetContent(StorePlusFrame);

	StorePlusButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("StorePlusButton"));
	StorePlusButton->SetBackgroundColor(AbilityBarPrivate::StorePlusBg);
	StorePlusButton->SetIsEnabled(true);
	StorePlusButton->OnClicked.AddDynamic(this, &UAbilityBarWidget::OnStorePlusClicked);
	StorePlusFrame->SetContent(StorePlusButton);

	StorePlusLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StorePlusLabel"));
	StorePlusLabel->SetText(FText::FromString(TEXT("+")));
	StorePlusLabel->SetJustification(ETextJustify::Center);
	StorePlusLabel->SetColorAndOpacity(FSlateColor(AbilityBarPrivate::KeyColor));
	StorePlusLabel->SetShadowOffset(FVector2D(1.f, 1.f));
	StorePlusLabel->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
	AbilityBarPrivate::SetBoldFont(StorePlusLabel, 22.f);
	StorePlusButton->SetContent(StorePlusLabel);

	if (UHorizontalBoxSlot* RowSlot = Parent->AddChildToHorizontalBox(StorePlusSizeBox))
	{
		RowSlot->SetPadding(FMargin(AbilityBarPrivate::SlotPadX, 0.f));
		RowSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UAbilityBarWidget::OnStorePlusClicked()
{
	ToggleStore();
}

void UAbilityBarWidget::ToggleStore()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Prefer the store widget already shown by BuildManager.
	for (TObjectIterator<UTowerStoreWidget> It; It; ++It)
	{
		UTowerStoreWidget* Store = *It;
		if (!IsValid(Store) || Store->GetWorld() != World)
		{
			continue;
		}
		Store->ToggleStore();
		RefreshStorePlusVisual();
		return;
	}

	// Fallback: spawn store if BuildManager has not created one yet.
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (UTowerStoreWidget* Store = CreateWidget<UTowerStoreWidget>(PC, UTowerStoreWidget::StaticClass()))
		{
			// Above ability bar (Z=100 in ShowAbilityHUD) so the bottom-center strip draws on top.
			Store->AddToViewport(120);
			Store->SetStoreOpen(true);
			RefreshStorePlusVisual();
		}
	}
}

void UAbilityBarWidget::RefreshStorePlusVisual()
{
	if (!StorePlusButton || !StorePlusLabel)
	{
		return;
	}

	bool bOpen = false;
	UWorld* World = GetWorld();
	if (World)
	{
		for (TObjectIterator<UTowerStoreWidget> It; It; ++It)
		{
			UTowerStoreWidget* Store = *It;
			if (IsValid(Store) && Store->GetWorld() == World && Store->IsInViewport())
			{
				bOpen = Store->IsStoreOpen();
				break;
			}
		}
	}

	StorePlusButton->SetBackgroundColor(bOpen ? AbilityBarPrivate::StorePlusBgOpen : AbilityBarPrivate::StorePlusBg);
	StorePlusLabel->SetText(FText::FromString(bOpen ? TEXT("−") : TEXT("+")));
	if (StorePlusFrame)
	{
		StorePlusFrame->SetBrushColor(bOpen
			? FLinearColor(0.9f, 0.55f, 0.35f, 1.f)
			: AbilityBarPrivate::StorePlusFrame);
	}
}

FAbilityBarSlotWidgets UAbilityBarWidget::BuildSlot(UHorizontalBox* Parent, TCHAR KeyChar, int32 AbilityId)
{
	FAbilityBarSlotWidgets Out;
	if (!Parent || !WidgetTree)
	{
		return Out;
	}

	Out.KeyChar = KeyChar;
	Out.AbilityId = AbilityId;
	const FString KeyStr = FString(1, &KeyChar);

	Out.SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("Size_%s"), *KeyStr));
	Out.SizeBox->SetWidthOverride(EffectiveSlotSize);
	Out.SizeBox->SetHeightOverride(EffectiveSlotSize);

	Out.Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("Frame_%s"), *KeyStr));
	Out.Frame->SetPadding(FMargin(3.f));
	Out.Frame->SetBrushColor(AbilityBarPrivate::SlotReadyFrame);
	Out.SizeBox->SetContent(Out.Frame);

	UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), *FString::Printf(TEXT("Slot_%s"), *KeyStr));
	Out.Frame->SetContent(Overlay);

	Out.Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), *FString::Printf(TEXT("Btn_%s"), *KeyStr));
	Out.Button->SetBackgroundColor(AbilityBarPrivate::SlotReadyBg);
	Out.Button->SetIsEnabled(true);
	if (AbilityId == 1)
	{
		Out.Button->OnClicked.AddDynamic(this, &UAbilityBarWidget::OnSlotQClicked);
	}
	else if (AbilityId == 2)
	{
		Out.Button->OnClicked.AddDynamic(this, &UAbilityBarWidget::OnSlotWClicked);
	}
	else if (AbilityId == 3)
	{
		Out.Button->OnClicked.AddDynamic(this, &UAbilityBarWidget::OnSlotEClicked);
	}
	else if (AbilityId == 4)
	{
		Out.Button->OnClicked.AddDynamic(this, &UAbilityBarWidget::OnSlotRClicked);
	}
	if (UOverlaySlot* BtnSlot = Overlay->AddChildToOverlay(Out.Button))
	{
		BtnSlot->SetHorizontalAlignment(HAlign_Fill);
		BtnSlot->SetVerticalAlignment(VAlign_Fill);
	}

	// LoL-style remaining-CD wipe: SizeBox height shrinks as cooldown ticks down.
	Out.CooldownClip = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("CDClip_%s"), *KeyStr));
	Out.CooldownClip->SetWidthOverride(EffectiveSlotSize);
	Out.CooldownClip->SetHeightOverride(0.f);
	Out.CooldownClip->SetVisibility(ESlateVisibility::Collapsed);

	Out.CooldownFill = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("CDFill_%s"), *KeyStr));
	{
		// Explicit solid brush — BorderColor alone can be invisible without a drawable brush.
		FSlateBrush FillBrush;
		FillBrush.DrawAs = ESlateBrushDrawType::Image;
		if (const FSlateBrush* White = FCoreStyle::Get().GetBrush("GenericWhiteBox"))
		{
			FillBrush = *White;
		}
		FillBrush.TintColor = FSlateColor(AbilityBarPrivate::CdOverlay);
		Out.CooldownFill->SetBrush(FillBrush);
	}
	Out.CooldownFill->SetBrushColor(AbilityBarPrivate::CdOverlay);
	Out.CooldownFill->SetPadding(FMargin(0.f));
	Out.CooldownClip->SetContent(Out.CooldownFill);

	if (UOverlaySlot* CdSlot = Overlay->AddChildToOverlay(Out.CooldownClip))
	{
		CdSlot->SetHorizontalAlignment(HAlign_Fill);
		CdSlot->SetVerticalAlignment(VAlign_Top);
	}

	UVerticalBox* Labels = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), *FString::Printf(TEXT("Labels_%s"), *KeyStr));
	Labels->SetVisibility(ESlateVisibility::HitTestInvisible);

	Out.KeyLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("Key_%s"), *KeyStr));
	Out.KeyLabel->SetText(FText::FromString(KeyStr));
	Out.KeyLabel->SetJustification(ETextJustify::Center);
	Out.KeyLabel->SetColorAndOpacity(FSlateColor(AbilityBarPrivate::KeyColor));
	Out.KeyLabel->SetShadowOffset(FVector2D(1.f, 1.f));
	Out.KeyLabel->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
	AbilityBarPrivate::SetBoldFont(Out.KeyLabel, 18.f);
	if (UVerticalBoxSlot* KeySlot = Labels->AddChildToVerticalBox(Out.KeyLabel))
	{
		KeySlot->SetHorizontalAlignment(HAlign_Center);
		KeySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		KeySlot->SetVerticalAlignment(VAlign_Center);
	}

	Out.CooldownText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("CDText_%s"), *KeyStr));
	Out.CooldownText->SetText(FText::GetEmpty());
	Out.CooldownText->SetJustification(ETextJustify::Center);
	Out.CooldownText->SetColorAndOpacity(FSlateColor(AbilityBarPrivate::CdTextColor));
	Out.CooldownText->SetShadowOffset(FVector2D(2.f, 2.f));
	Out.CooldownText->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.95f));
	Out.CooldownText->SetVisibility(ESlateVisibility::Collapsed);
	AbilityBarPrivate::SetBoldFont(Out.CooldownText, 16.f);
	if (UVerticalBoxSlot* CdTextSlot = Labels->AddChildToVerticalBox(Out.CooldownText))
	{
		CdTextSlot->SetHorizontalAlignment(HAlign_Center);
		CdTextSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		CdTextSlot->SetVerticalAlignment(VAlign_Center);
	}

	Out.LockText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), *FString::Printf(TEXT("Lock_%s"), *KeyStr));
	Out.LockText->SetText(FText::FromString(TEXT("LOCKED")));
	Out.LockText->SetJustification(ETextJustify::Center);
	Out.LockText->SetColorAndOpacity(FSlateColor(AbilityBarPrivate::LockTextColor));
	Out.LockText->SetVisibility(ESlateVisibility::Collapsed);
	AbilityBarPrivate::SetBoldFont(Out.LockText, 10.f);
	if (UVerticalBoxSlot* LockSlot = Labels->AddChildToVerticalBox(Out.LockText))
	{
		LockSlot->SetHorizontalAlignment(HAlign_Center);
		LockSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	if (UOverlaySlot* LabelSlot = Overlay->AddChildToOverlay(Labels))
	{
		LabelSlot->SetHorizontalAlignment(HAlign_Fill);
		LabelSlot->SetVerticalAlignment(VAlign_Fill);
	}

	if (UHorizontalBoxSlot* RowSlot = Parent->AddChildToHorizontalBox(Out.SizeBox))
	{
		RowSlot->SetPadding(FMargin(AbilityBarPrivate::SlotPadX, 0.f));
		RowSlot->SetVerticalAlignment(VAlign_Center);
	}

	return Out;
}

void UAbilityBarWidget::OnAbilitySlotClicked(int32 AbilityId)
{
	TryBeginAbilityAim(AbilityId);
}

void UAbilityBarWidget::OnSlotQClicked()
{
	OnAbilitySlotClicked(1);
}

void UAbilityBarWidget::OnSlotWClicked()
{
	OnAbilitySlotClicked(2);
}

void UAbilityBarWidget::OnSlotEClicked()
{
	OnAbilitySlotClicked(3);
}

void UAbilityBarWidget::OnSlotRClicked()
{
	OnAbilitySlotClicked(4);
}

void UAbilityBarWidget::TryBeginAbilityAim(int32 AbilityId)
{
	APawn* Pawn = ResolveChampionPawn();
	if (!Pawn)
	{
		return;
	}

	UFunction* Fn = Pawn->FindFunction(FName(TEXT("BeginAbilityAim")));
	if (!Fn)
	{
		Fn = Pawn->FindFunction(FName(TEXT("beginAbilityAim")));
	}
	if (!Fn)
	{
		UE_LOG(LogTemp, Warning, TEXT("AbilityBar: BeginAbilityAim not found on %s"), *Pawn->GetName());
		return;
	}

	const int32 ParamsSize = Fn->ParmsSize;
	TArray<uint8> ParamBuffer;
	void* Params = nullptr;
	if (ParamsSize > 0)
	{
		ParamBuffer.SetNumZeroed(ParamsSize);
		Params = ParamBuffer.GetData();
		for (TFieldIterator<FProperty> It(Fn); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
		{
			if (It->HasAnyPropertyFlags(CPF_OutParm | CPF_ReturnParm))
			{
				continue;
			}
			if (FIntProperty* IntProp = CastField<FIntProperty>(*It))
			{
				IntProp->SetPropertyValue_InContainer(Params, AbilityId);
				break;
			}
			if (FByteProperty* ByteProp = CastField<FByteProperty>(*It))
			{
				ByteProp->SetPropertyValue_InContainer(Params, static_cast<uint8>(AbilityId));
				break;
			}
		}
	}

	Pawn->ProcessEvent(Fn, Params);
}

UChampionFrameWidget* UAbilityBarWidget::ResolveChampionFrame() const
{
	APlayerController* PC = GetOwningPlayer();
	UWorld* World = GetWorld();
	for (TObjectIterator<UChampionFrameWidget> It; It; ++It)
	{
		UChampionFrameWidget* Frame = *It;
		if (!IsValid(Frame) || !Frame->IsInViewport())
		{
			continue;
		}
		if (PC && Frame->GetOwningPlayer() != PC)
		{
			continue;
		}
		if (World && Frame->GetWorld() != World)
		{
			continue;
		}
		return Frame;
	}
	return nullptr;
}

void UAbilityBarWidget::ApplyDockLayout()
{
	if (!BarChrome)
	{
		return;
	}

	FVector2D Margin(24.f, 24.f);
	FVector2D FrameSize(324.f, 108.f);
	if (UChampionFrameWidget* Frame = ResolveChampionFrame())
	{
		Margin = Frame->GetChromeScreenMargin();
		FrameSize = Frame->GetChromeScreenSize();
	}

	const float TargetWidth = FMath::Max(FrameSize.X, 160.f);
	const float Inner = FMath::Max(TargetWidth - AbilityBarPrivate::ChromePadX * 2.f, 80.f);
	const float TotalPad = AbilityBarPrivate::SlotPadX * 2.f * static_cast<float>(AbilityBarPrivate::NumBarSlots);
	const float Fitted = (Inner - TotalPad) / static_cast<float>(AbilityBarPrivate::NumBarSlots);
	EffectiveSlotSize = FMath::Clamp(Fitted, 32.f, SlotSize);
	const float Lift = Margin.Y + FrameSize.Y + AbilityOverFrameGap;

	const bool bLayoutDirty =
		!FMath::IsNearlyEqual(EffectiveSlotSize, LastAppliedSlotSize, 0.25f)
		|| !FMath::IsNearlyEqual(TargetWidth, LastAppliedWidth, 0.25f)
		|| !FMath::IsNearlyEqual(Lift, LastAppliedLift, 0.25f)
		|| !FMath::IsNearlyEqual(Margin.X, LastAppliedLeft, 0.25f);
	if (!bLayoutDirty)
	{
		return;
	}

	LastAppliedSlotSize = EffectiveSlotSize;
	LastAppliedWidth = TargetWidth;
	LastAppliedLift = Lift;
	LastAppliedLeft = Margin.X;

	if (BarSizeBox)
	{
		BarSizeBox->SetWidthOverride(TargetWidth - AbilityBarPrivate::ChromePadX * 2.f);
		BarSizeBox->SetHeightOverride(EffectiveSlotSize);
	}

	if (UCanvasPanelSlot* ChromeSlot = Cast<UCanvasPanelSlot>(BarChrome->Slot))
	{
		ChromeSlot->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
		ChromeSlot->SetAlignment(FVector2D(0.f, 1.f));
		ChromeSlot->SetAutoSize(true);
		ChromeSlot->SetOffsets(FMargin(Margin.X, -Lift, 0.f, 0.f));
		ChromeSlot->SetZOrder(10);
	}

	ApplySlotMetrics();
}

void UAbilityBarWidget::ApplySlotMetrics()
{
	auto ApplyBox = [this](USizeBox* Box)
	{
		if (Box)
		{
			Box->SetWidthOverride(EffectiveSlotSize);
			Box->SetHeightOverride(EffectiveSlotSize);
		}
	};
	ApplyBox(StorePlusSizeBox);
	ApplyBox(SlotQ.SizeBox);
	ApplyBox(SlotW.SizeBox);
	ApplyBox(SlotE.SizeBox);
	ApplyBox(SlotR.SizeBox);

	auto ApplyClipWidth = [this](USizeBox* Clip)
	{
		if (Clip)
		{
			Clip->SetWidthOverride(EffectiveSlotSize);
		}
	};
	ApplyClipWidth(SlotQ.CooldownClip);
	ApplyClipWidth(SlotW.CooldownClip);
	ApplyClipWidth(SlotE.CooldownClip);
	ApplyClipWidth(SlotR.CooldownClip);

	auto ApplyPad = [](UWidget* W)
	{
		if (W)
		{
			if (UHorizontalBoxSlot* HS = Cast<UHorizontalBoxSlot>(W->Slot))
			{
				HS->SetPadding(FMargin(AbilityBarPrivate::SlotPadX, 0.f));
			}
		}
	};
	ApplyPad(StorePlusSizeBox);
	ApplyPad(SlotQ.SizeBox);
	ApplyPad(SlotW.SizeBox);
	ApplyPad(SlotE.SizeBox);
	ApplyPad(SlotR.SizeBox);

	const float FontScale = FMath::Clamp(EffectiveSlotSize / 56.f, 0.65f, 1.15f);
	AbilityBarPrivate::SetBoldFont(StorePlusLabel, 22.f * FontScale);
	auto ApplySlotFonts = [FontScale](FAbilityBarSlotWidgets& SlotUI)
	{
		AbilityBarPrivate::SetBoldFont(SlotUI.KeyLabel, 18.f * FontScale);
		AbilityBarPrivate::SetBoldFont(SlotUI.CooldownText, 16.f * FontScale);
		AbilityBarPrivate::SetBoldFont(SlotUI.LockText, 10.f * FontScale);
	};
	ApplySlotFonts(SlotQ);
	ApplySlotFonts(SlotW);
	ApplySlotFonts(SlotE);
	ApplySlotFonts(SlotR);
}

APawn* UAbilityBarWidget::ResolveChampionPawn() const
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return GetOwningPlayerPawn();
	}

	// MOBA mode possesses the free camera; ability CDs live on ControlledChampion.
	if (const AMobaPlayerController* MobaPC = Cast<AMobaPlayerController>(PC))
	{
		if (APawn* Champion = MobaPC->GetControlledChampion())
		{
			return Champion;
		}
	}

	if (APawn* Possessed = PC->GetPawn())
	{
		// Prefer a pawn that actually exposes ability CD props.
		float UnusedCd = 0.f;
		if (ReadFloatProp(Possessed, FName(TEXT("CD_Q")), UnusedCd))
		{
			return Possessed;
		}
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<APawn> It(World); It; ++It)
		{
			APawn* Candidate = *It;
			if (!IsValid(Candidate))
			{
				continue;
			}
			float UnusedCd = 0.f;
			if (ReadFloatProp(Candidate, FName(TEXT("CD_Q")), UnusedCd))
			{
				return Candidate;
			}
		}
	}

	return PC->GetPawn();
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

	static TWeakObjectPtr<APawn> LastLoggedChampion;
	if (LastLoggedChampion.Get() != Pawn)
	{
		LastLoggedChampion = Pawn;
		UE_LOG(LogTemp, Display,
			TEXT("AbilityBar bound to champion=%s CD_Q=%.2f Level=%d Dropping=%d Pending=%d"),
			*Pawn->GetName(), CdQ, Level, bDropping ? 1 : 0, Pending);
	}

	ApplySlotState(SlotQ, 1, CdQ, MaxQ, bDropping, Level, Pending);
	ApplySlotState(SlotW, 2, CdW, MaxW, bDropping, Level, Pending);
	ApplySlotState(SlotE, 3, CdE, MaxE, bDropping, Level, Pending);
	ApplySlotState(SlotR, 4, CdR, MaxR, bDropping, Level, Pending);
}

void UAbilityBarWidget::ApplySlotState(FAbilityBarSlotWidgets& SlotUI, int32 AbilityId, float RemainingCD, float MaxCD,
	bool bDropping, int32 ChampionLevel, int32 PendingAbility)
{
	if (!SlotUI.Button || !SlotUI.CooldownText)
	{
		return;
	}

	const bool bUltLocked = (AbilityId == 4) && (ChampionLevel < UltimateUnlockLevel);
	const bool bOnCooldown = RemainingCD > 0.05f;
	const bool bUnavailable = bUltLocked || bDropping;
	const bool bAvailable = !bUnavailable && !bOnCooldown;
	const bool bAiming = PendingAbility == AbilityId;

	const float SafeMax = FMath::Max(MaxCD, 0.01f);
	const float CdPercent = bOnCooldown ? FMath::Clamp(RemainingCD / SafeMax, 0.f, 1.f) : 0.f;

	// Dark wipe covers remaining CD (shrinks top→bottom as it cools, LoL-style).
	if (SlotUI.CooldownClip && SlotUI.CooldownFill)
	{
		if (bOnCooldown && !bUltLocked)
		{
			SlotUI.CooldownClip->SetHeightOverride(EffectiveSlotSize * CdPercent);
			SlotUI.CooldownClip->SetVisibility(ESlateVisibility::HitTestInvisible);
			SlotUI.CooldownFill->SetBrushColor(AbilityBarPrivate::CdOverlay);
		}
		else
		{
			SlotUI.CooldownClip->SetHeightOverride(0.f);
			SlotUI.CooldownClip->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (bOnCooldown && !bUltLocked)
	{
		const FString CdStr = RemainingCD >= 10.f
			? FString::Printf(TEXT("%d"), FMath::CeilToInt(RemainingCD))
			: FString::Printf(TEXT("%.1f"), RemainingCD);
		SlotUI.CooldownText->SetText(FText::FromString(CdStr));
		SlotUI.CooldownText->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (SlotUI.KeyLabel)
		{
			SlotUI.KeyLabel->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else
	{
		SlotUI.CooldownText->SetText(FText::GetEmpty());
		SlotUI.CooldownText->SetVisibility(ESlateVisibility::Collapsed);
		if (SlotUI.KeyLabel)
		{
			SlotUI.KeyLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
			SlotUI.KeyLabel->SetColorAndOpacity(FSlateColor(
				bUnavailable ? AbilityBarPrivate::KeyColorDim : AbilityBarPrivate::KeyColor));
		}
	}

	if (SlotUI.LockText)
	{
		if (bUltLocked)
		{
			SlotUI.LockText->SetText(FText::FromString(FString::Printf(TEXT("Lv%d"), UltimateUnlockLevel)));
			SlotUI.LockText->SetVisibility(ESlateVisibility::HitTestInvisible);
			if (SlotUI.KeyLabel)
			{
				SlotUI.KeyLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
		else if (bDropping)
		{
			SlotUI.LockText->SetText(FText::FromString(TEXT("—")));
			SlotUI.LockText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			SlotUI.LockText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (SlotUI.Frame)
	{
		if (bAiming && bAvailable)
		{
			SlotUI.Frame->SetBrushColor(AbilityBarPrivate::SlotAimFrame);
		}
		else if (bUnavailable)
		{
			SlotUI.Frame->SetBrushColor(AbilityBarPrivate::SlotLockedFrame);
		}
		else if (bOnCooldown)
		{
			SlotUI.Frame->SetBrushColor(AbilityBarPrivate::SlotCdFrame);
		}
		else
		{
			SlotUI.Frame->SetBrushColor(AbilityBarPrivate::SlotReadyFrame);
		}
	}

	if (SlotUI.SizeBox)
	{
		if (bUltLocked || bDropping)
		{
			SlotUI.Button->SetBackgroundColor(AbilityBarPrivate::SlotLockedBg);
			SlotUI.SizeBox->SetRenderOpacity(0.42f);
		}
		else if (bOnCooldown)
		{
			SlotUI.Button->SetBackgroundColor(AbilityBarPrivate::SlotCdBg);
			SlotUI.SizeBox->SetRenderOpacity(0.92f);
		}
		else
		{
			SlotUI.Button->SetBackgroundColor(AbilityBarPrivate::SlotReadyBg);
			SlotUI.SizeBox->SetRenderOpacity(1.f);
		}
	}

	SlotUI.Button->SetIsEnabled(true);
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
