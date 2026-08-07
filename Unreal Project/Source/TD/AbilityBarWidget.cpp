#include "AbilityBarWidget.h"

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
	static FLinearColor NextWaveBg(0.10f, 0.48f, 0.28f, 0.96f);
	static FLinearColor NextWaveBgReady(0.12f, 0.58f, 0.34f, 0.98f);
	static FLinearColor NextWaveBgBusy(0.12f, 0.16f, 0.14f, 0.95f);
	static FLinearColor NextWaveFrame(0.45f, 0.95f, 0.65f, 1.f);
	static FLinearColor NextWaveFrameBusy(0.30f, 0.38f, 0.32f, 1.f);
	static FLinearColor NextWaveIcon(0.95f, 1.f, 0.97f, 1.f);
	static FLinearColor NextWaveCaption(0.82f, 0.98f, 0.88f, 1.f);
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
	// Root ignores empty space; bar chrome + store plus receive clicks.
	ApplyHitTestPolicy();
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
	}
}

void UAbilityBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bBuilt)
	{
		EnsureBuilt();
	}

	RefreshStorePlusVisual();
	RefreshNextWaveVisual();

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

	if (!WidgetTree->RootWidget || !SlotRow)
	{
		BuildDefaultUI();
	}
	else if (!StorePlusButton || !NextWaveButton || !SlotQ.CooldownClip)
	{
		// Hot-reload / older runtime tree missing store opener, next-wave, or CD wipe.
		if (SlotRow)
		{
			SlotRow->ClearChildren();
			BuildStorePlusSlot(SlotRow);
			SlotQ = BuildSlot(SlotRow, TEXT('Q'), 1);
			SlotW = BuildSlot(SlotRow, TEXT('W'), 2);
			SlotE = BuildSlot(SlotRow, TEXT('E'), 3);
			SlotR = BuildSlot(SlotRow, TEXT('R'), 4);
			BuildNextWaveSlot(SlotRow);
		}
		else
		{
			BuildDefaultUI();
		}
	}

	bBuilt = SlotQ.Button != nullptr && SlotR.Button != nullptr
		&& SlotQ.CooldownClip != nullptr
		&& StorePlusButton != nullptr && NextWaveButton != nullptr;
	if (bBuilt)
	{
		ApplyHitTestPolicy();
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
	if (NextWaveButton)
	{
		NextWaveButton->SetVisibility(ESlateVisibility::Visible);
	}
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
		BarChrome->SetPadding(FMargin(16.f, 12.f));
		BarChrome->SetBrushColor(FLinearColor(0.05f, 0.08f, 0.12f, 0.92f));
		if (UCanvasPanelSlot* ChromeSlot = Root->AddChildToCanvas(BarChrome))
		{
			ChromeSlot->SetAnchors(FAnchors(0.5f, 1.f, 0.5f, 1.f));
			ChromeSlot->SetAlignment(FVector2D(0.5f, 1.f));
			ChromeSlot->SetAutoSize(true);
			// Non-stretch canvas: Offset.Top lifts from bottom (Alignment Y=1). Bottom is ignored.
			ChromeSlot->SetOffsets(FMargin(0.f, -40.f, 0.f, 0.f));
			ChromeSlot->SetZOrder(10);
		}
	}
	else if (UCanvasPanelSlot* ChromeSlot = Cast<UCanvasPanelSlot>(BarChrome->Slot))
	{
		ChromeSlot->SetAnchors(FAnchors(0.5f, 1.f, 0.5f, 1.f));
		ChromeSlot->SetAlignment(FVector2D(0.5f, 1.f));
		ChromeSlot->SetAutoSize(true);
		ChromeSlot->SetOffsets(FMargin(0.f, -40.f, 0.f, 0.f));
	}

	if (!SlotRow)
	{
		SlotRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("AbilitySlotRow"));
		BarChrome->SetContent(SlotRow);
	}
	else
	{
		SlotRow->ClearChildren();
	}

	// Tower store opener is the first button, left of Q.
	BuildStorePlusSlot(SlotRow);

	SlotQ = BuildSlot(SlotRow, TEXT('Q'), 1);
	SlotW = BuildSlot(SlotRow, TEXT('W'), 2);
	SlotE = BuildSlot(SlotRow, TEXT('E'), 3);
	SlotR = BuildSlot(SlotRow, TEXT('R'), 4);

	// Next-wave force-start sits after R (same as Enter hotkey).
	BuildNextWaveSlot(SlotRow);

	ApplyHitTestPolicy();
}

void UAbilityBarWidget::BuildStorePlusSlot(UHorizontalBox* Parent)
{
	if (!Parent || !WidgetTree)
	{
		return;
	}

	StorePlusSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("StorePlusSize"));
	StorePlusSizeBox->SetWidthOverride(SlotSize);
	StorePlusSizeBox->SetHeightOverride(SlotSize);

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
	{
		FSlateFontInfo Font = StorePlusLabel->GetFont();
		Font.Size = 32.f;
		Font.TypefaceFontName = TEXT("Bold");
		StorePlusLabel->SetFont(Font);
	}
	StorePlusButton->SetContent(StorePlusLabel);

	if (UHorizontalBoxSlot* RowSlot = Parent->AddChildToHorizontalBox(StorePlusSizeBox))
	{
		RowSlot->SetPadding(FMargin(8.f, 0.f, 12.f, 0.f));
		RowSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UAbilityBarWidget::OnStorePlusClicked()
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
			// Above ability bar (Z=100 in ShowAbilityHUD) so strip draws on top.
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

void UAbilityBarWidget::BuildNextWaveSlot(UHorizontalBox* Parent)
{
	if (!Parent || !WidgetTree)
	{
		return;
	}

	// Slightly wider than ability slots so it reads as a play/next control, not another keybind.
	const float PlayWidth = SlotSize + 10.f;

	NextWaveSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("NextWaveSize"));
	NextWaveSizeBox->SetWidthOverride(PlayWidth);
	NextWaveSizeBox->SetHeightOverride(SlotSize);

	NextWaveFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NextWaveFrame"));
	NextWaveFrame->SetPadding(FMargin(2.f));
	NextWaveFrame->SetBrushColor(AbilityBarPrivate::NextWaveFrame);
	NextWaveSizeBox->SetContent(NextWaveFrame);

	NextWaveButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("NextWaveButton"));
	NextWaveButton->SetBackgroundColor(AbilityBarPrivate::NextWaveBgReady);
	NextWaveButton->SetIsEnabled(true);
	NextWaveButton->OnClicked.AddDynamic(this, &UAbilityBarWidget::OnNextWaveClicked);
	NextWaveFrame->SetContent(NextWaveButton);

	UVerticalBox* Labels = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("NextWaveLabels"));

	// Large play triangle — primary affordance.
	NextWaveLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NextWavePlayIcon"));
	NextWaveLabel->SetText(FText::FromString(TEXT("▶")));
	NextWaveLabel->SetJustification(ETextJustify::Center);
	NextWaveLabel->SetColorAndOpacity(FSlateColor(AbilityBarPrivate::NextWaveIcon));
	NextWaveLabel->SetShadowOffset(FVector2D(1.f, 1.f));
	NextWaveLabel->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f));
	{
		FSlateFontInfo Font = NextWaveLabel->GetFont();
		Font.Size = 30.f;
		Font.TypefaceFontName = TEXT("Bold");
		NextWaveLabel->SetFont(Font);
	}
	if (UVerticalBoxSlot* IconSlot = Labels->AddChildToVerticalBox(NextWaveLabel))
	{
		IconSlot->SetHorizontalAlignment(HAlign_Center);
		IconSlot->SetPadding(FMargin(2.f, 8.f, 0.f, 0.f));
	}

	// "NEXT" caption under the play icon (media-control style).
	NextWaveKeyLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NextWaveCaption"));
	NextWaveKeyLabel->SetText(FText::FromString(TEXT("NEXT")));
	NextWaveKeyLabel->SetJustification(ETextJustify::Center);
	NextWaveKeyLabel->SetColorAndOpacity(FSlateColor(AbilityBarPrivate::NextWaveCaption));
	NextWaveKeyLabel->SetShadowOffset(FVector2D(1.f, 1.f));
	NextWaveKeyLabel->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.75f));
	{
		FSlateFontInfo Font = NextWaveKeyLabel->GetFont();
		Font.Size = 12.f;
		Font.TypefaceFontName = TEXT("Bold");
		NextWaveKeyLabel->SetFont(Font);
	}
	if (UVerticalBoxSlot* CaptionSlot = Labels->AddChildToVerticalBox(NextWaveKeyLabel))
	{
		CaptionSlot->SetHorizontalAlignment(HAlign_Center);
		CaptionSlot->SetPadding(FMargin(0.f, -2.f, 0.f, 6.f));
	}

	NextWaveButton->SetContent(Labels);

	if (UHorizontalBoxSlot* RowSlot = Parent->AddChildToHorizontalBox(NextWaveSizeBox))
	{
		RowSlot->SetPadding(FMargin(14.f, 0.f, 8.f, 0.f));
		RowSlot->SetVerticalAlignment(VAlign_Center);
	}
}

void UAbilityBarWidget::OnNextWaveClicked()
{
	AActor* Spawner = FindEnemySpawner();
	if (!Spawner)
	{
		UE_LOG(LogTemp, Warning, TEXT("AbilityBar NextWave: no BP_EnemySpawner found"));
		return;
	}

	if (UFunction* Fn = Spawner->FindFunction(FName(TEXT("ForceStartNextWave"))))
	{
		Spawner->ProcessEvent(Fn, nullptr);
		RefreshNextWaveVisual();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AbilityBar NextWave: ForceStartNextWave missing on %s"),
			*Spawner->GetName());
	}
}

void UAbilityBarWidget::RefreshNextWaveVisual()
{
	if (!NextWaveButton || !NextWaveLabel)
	{
		return;
	}

	AActor* Spawner = FindEnemySpawner();
	bool bBusy = false;
	float Countdown = 0.f;
	if (Spawner)
	{
		bool bSpawning = false;
		bool bWaitingClear = false;
		ReadBoolProp(Spawner, FName(TEXT("IsSpawningWave")), bSpawning);
		if (!ReadBoolProp(Spawner, FName(TEXT("WaitingforClear")), bWaitingClear))
		{
			ReadBoolProp(Spawner, FName(TEXT("WaitingForClear")), bWaitingClear);
		}
		if (!ReadFloatProp(Spawner, FName(TEXT("CountdownRemaining")), Countdown))
		{
			int32 CountdownInt = 0;
			if (ReadIntProp(Spawner, FName(TEXT("CountdownRemaining")), CountdownInt))
			{
				Countdown = static_cast<float>(CountdownInt);
			}
		}
		bBusy = bSpawning || bWaitingClear;
	}

	NextWaveButton->SetIsEnabled(!bBusy && Spawner != nullptr);
	NextWaveButton->SetBackgroundColor(bBusy ? AbilityBarPrivate::NextWaveBgBusy : AbilityBarPrivate::NextWaveBgReady);
	if (NextWaveFrame)
	{
		NextWaveFrame->SetBrushColor(bBusy ? AbilityBarPrivate::NextWaveFrameBusy : AbilityBarPrivate::NextWaveFrame);
	}
	if (NextWaveSizeBox)
	{
		NextWaveSizeBox->SetRenderOpacity(bBusy ? 0.5f : 1.f);
	}

	NextWaveLabel->SetColorAndOpacity(FSlateColor(AbilityBarPrivate::NextWaveIcon));
	if (NextWaveKeyLabel)
	{
		NextWaveKeyLabel->SetColorAndOpacity(FSlateColor(AbilityBarPrivate::NextWaveCaption));
	}

	if (bBusy)
	{
		// Pause-style while a wave is active.
		NextWaveLabel->SetText(FText::FromString(TEXT("❚❚")));
		if (NextWaveKeyLabel)
		{
			NextWaveKeyLabel->SetText(FText::FromString(TEXT("WAIT")));
		}
	}
	else if (Countdown > 0.05f)
	{
		// Still a play button — countdown shows you can skip ahead.
		NextWaveLabel->SetText(FText::FromString(TEXT("▶")));
		if (NextWaveKeyLabel)
		{
			NextWaveKeyLabel->SetText(FText::FromString(
				FString::Printf(TEXT("%d"), FMath::CeilToInt(Countdown))));
		}
	}
	else
	{
		NextWaveLabel->SetText(FText::FromString(TEXT("▶")));
		if (NextWaveKeyLabel)
		{
			NextWaveKeyLabel->SetText(FText::FromString(TEXT("NEXT")));
		}
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
	const FString KeyStr = FString(1, &KeyChar);

	Out.SizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("Size_%s"), *KeyStr));
	Out.SizeBox->SetWidthOverride(SlotSize);
	Out.SizeBox->SetHeightOverride(SlotSize);

	Out.Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), *FString::Printf(TEXT("Frame_%s"), *KeyStr));
	Out.Frame->SetPadding(FMargin(3.f));
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

	// LoL-style remaining-CD wipe: SizeBox height shrinks as cooldown ticks down.
	Out.CooldownClip = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), *FString::Printf(TEXT("CDClip_%s"), *KeyStr));
	Out.CooldownClip->SetWidthOverride(SlotSize);
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
	{
		FSlateFontInfo Font = Out.KeyLabel->GetFont();
		Font.Size = 28.f;
		Font.TypefaceFontName = TEXT("Bold");
		Out.KeyLabel->SetFont(Font);
	}
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
	{
		FSlateFontInfo Font = Out.CooldownText->GetFont();
		Font.Size = 26.f;
		Font.TypefaceFontName = TEXT("Bold");
		Out.CooldownText->SetFont(Font);
	}
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
	{
		FSlateFontInfo Font = Out.LockText->GetFont();
		Font.Size = 14.f;
		Font.TypefaceFontName = TEXT("Bold");
		Out.LockText->SetFont(Font);
	}
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
		RowSlot->SetPadding(FMargin(8.f, 0.f));
		RowSlot->SetVerticalAlignment(VAlign_Center);
	}

	return Out;
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

AActor* UAbilityBarWidget::FindEnemySpawner() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UClass* SpawnerClass = EnemySpawnerActorClass.TryLoadClass<AActor>();
	if (!SpawnerClass)
	{
		return nullptr;
	}

	for (TActorIterator<AActor> It(World, SpawnerClass); It; ++It)
	{
		if (IsValid(*It))
		{
			return *It;
		}
	}
	return nullptr;
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
			SlotUI.CooldownClip->SetHeightOverride(SlotSize * CdPercent);
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
