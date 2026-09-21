#include "WavePowerUpWidget.h"

#include "TDEnemyPathLibrary.h"
#include "TDEnemyPathSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Styling/SlateBrush.h"
#include "UObject/UObjectIterator.h"

namespace WavePowerUpWidgetPrivate
{
	static const FLinearColor DimmerColor(0.02f, 0.03f, 0.05f, 0.78f);
	static const FLinearColor CardFill(0.07f, 0.10f, 0.14f, 0.98f);
	static const FLinearColor CardOutline(0.45f, 0.72f, 0.88f, 1.f);
	static const FLinearColor PlayerFill(0.07f, 0.22f, 0.14f, 0.96f);
	static const FLinearColor PlayerTitle(0.55f, 0.95f, 0.62f, 1.f);
	static const FLinearColor PlayerBody(0.86f, 0.96f, 0.88f, 1.f);
	static const FLinearColor EnemyFill(0.24f, 0.07f, 0.07f, 0.96f);
	static const FLinearColor EnemyTitle(1.f, 0.42f, 0.36f, 1.f);
	static const FLinearColor EnemyBody(1.f, 0.82f, 0.78f, 1.f);
	static const FLinearColor TitleColor(0.94f, 0.97f, 1.f, 1.f);
	static const FLinearColor SubtitleColor(0.72f, 0.80f, 0.88f, 1.f);
	static const FLinearColor RerollColor(1.f, 0.86f, 0.35f, 1.f);
	static const FLinearColor RerollDisabled(0.55f, 0.50f, 0.40f, 1.f);

	static void ApplyRoundedBrush(UBorder* Border, const FLinearColor& Fill, const FLinearColor& Outline, float OutlineWidth)
	{
		if (!Border)
		{
			return;
		}
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Fill);
		Brush.OutlineSettings.Color = Outline;
		Brush.OutlineSettings.Width = OutlineWidth;
		Brush.OutlineSettings.CornerRadii = FVector4(12.f, 12.f, 12.f, 12.f);
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Border->SetBrush(Brush);
	}
}

void UWavePowerUpCardBinder::HandleSelectClicked()
{
	if (Owner)
	{
		Owner->OnCardSelected(CardIndex);
	}
}

void UWavePowerUpCardBinder::HandleRerollClicked()
{
	if (Owner)
	{
		Owner->OnCardRerolled(CardIndex);
	}
}

UWavePowerUpWidget::UWavePowerUpWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
	bHasScriptImplementedTick = true;
}

void UWavePowerUpWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureBuilt();
}

void UWavePowerUpWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureBuilt();
	if (!bDraftVisible)
	{
		HideDraft();
	}
	ApplyHitTestPolicy();
}

void UWavePowerUpWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!bBuilt)
	{
		EnsureBuilt();
	}
	if (bDraftVisible)
	{
		RefreshRerollAffordability();
	}
}

const TCHAR* UWavePowerUpWidget::GetWidgetBlueprintPath()
{
	return TEXT("/Game/TD/UI/WBP_WavePowerUp.WBP_WavePowerUp_C");
}

TSubclassOf<UWavePowerUpWidget> UWavePowerUpWidget::ResolveWidgetClass()
{
	if (UClass* Loaded = LoadClass<UWavePowerUpWidget>(nullptr, GetWidgetBlueprintPath()))
	{
		return Loaded;
	}
	return UWavePowerUpWidget::StaticClass();
}

bool UWavePowerUpWidget::OpenDraft(AActor* Spawner)
{
	if (!IsValid(Spawner))
	{
		return false;
	}

	UWorld* World = Spawner->GetWorld();
	APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
	if (!PC)
	{
		return false;
	}

	UWavePowerUpWidget* Widget = nullptr;
	for (TObjectIterator<UWavePowerUpWidget> It; It; ++It)
	{
		UWavePowerUpWidget* Existing = *It;
		if (!IsValid(Existing) || Existing->HasAnyFlags(RF_ClassDefaultObject))
		{
			continue;
		}
		if (Existing->GetOwningPlayer() == PC && Existing->GetWorld() == World)
		{
			Widget = Existing;
			break;
		}
	}

	if (!Widget)
	{
		TSubclassOf<UWavePowerUpWidget> Class = ResolveWidgetClass();
		Widget = CreateWidget<UWavePowerUpWidget>(PC, Class);
	}
	if (!Widget)
	{
		return false;
	}

	if (!Widget->IsInViewport())
	{
		Widget->AddToViewport(180);
	}
	Widget->ShowDraft();
	return true;
}

void UWavePowerUpWidget::ShowDraft()
{
	EnsureBuilt();
	bDraftVisible = true;
	SetVisibility(ESlateVisibility::Visible);
	ApplyHitTestPolicy();
	RefreshCards();
	RefreshRerollAffordability();

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->bShowMouseCursor = true;
		PC->bEnableClickEvents = true;
	}
}

void UWavePowerUpWidget::HideDraft()
{
	bDraftVisible = false;
	SetVisibility(ESlateVisibility::Collapsed);
	ApplyHitTestPolicy();
}

void UWavePowerUpWidget::OnCardSelected(int32 CardIndex)
{
	UWorld* World = GetWorld();
	UTDEnemyPathSubsystem* Sys = World ? World->GetSubsystem<UTDEnemyPathSubsystem>() : nullptr;
	if (!Sys || !Sys->DraftOffers.IsValidIndex(CardIndex))
	{
		return;
	}

	const FTDWavePowerUpOffer Offer = Sys->DraftOffers[CardIndex];
	Sys->ActiveWavePowerUp = FTDWavePowerUp::MakeActive(Offer);
	Sys->bAwaitingPowerUpPick = false;

	if (const int32 Bonus = FTDWavePowerUp::ResolvePaydayBonus(Sys->ActiveWavePowerUp))
	{
		FTDWavePowerUp::AddResource(this, Bonus);
	}

	HideDraft();

	AActor* Spawner = Sys->DraftSpawner.Get();
	if (!Spawner)
	{
		Spawner = Sys->ActiveWaveSpawner.Get();
	}
	UTDEnemyPathLibrary::ContinueAfterWavePowerUp(Spawner);
}

void UWavePowerUpWidget::OnCardRerolled(int32 CardIndex)
{
	UWorld* World = GetWorld();
	UTDEnemyPathSubsystem* Sys = World ? World->GetSubsystem<UTDEnemyPathSubsystem>() : nullptr;
	if (!Sys || !Sys->DraftOffers.IsValidIndex(CardIndex))
	{
		return;
	}

	if (!FTDWavePowerUp::TrySpendResource(this, FTDWavePowerUp::RerollCost))
	{
		return;
	}

	FTDWavePowerUp::RerollCard(Sys->DraftOffers, CardIndex);
	RefreshCards();
	RefreshRerollAffordability();
}

void UWavePowerUpWidget::EnsureBuilt()
{
	if (bBuilt && CardRow && Cards.Num() == FTDWavePowerUp::CardCount)
	{
		return;
	}
	if (!WidgetTree)
	{
		return;
	}

	BindDesignerWidgets();
	if (!RootCanvas || !CardRow)
	{
		BuildFallbackTree();
	}
	BuildCards();
	bBuilt = RootCanvas != nullptr && CardRow != nullptr && Cards.Num() == FTDWavePowerUp::CardCount;
	if (bBuilt)
	{
		ApplyHitTestPolicy();
	}
}

void UWavePowerUpWidget::BindDesignerWidgets()
{
	RootCanvas = Cast<UCanvasPanel>(GetWidgetFromName(TEXT("WavePowerUpRoot")));
	if (!RootCanvas)
	{
		RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	}
	Dimmer = Cast<UBorder>(GetWidgetFromName(TEXT("WavePowerUpDimmer")));
	PanelBox = Cast<UVerticalBox>(GetWidgetFromName(TEXT("WavePowerUpPanel")));
	TitleLabel = Cast<UTextBlock>(GetWidgetFromName(TEXT("WavePowerUpTitle")));
	SubtitleLabel = Cast<UTextBlock>(GetWidgetFromName(TEXT("WavePowerUpSubtitle")));
	CardRow = Cast<UHorizontalBox>(GetWidgetFromName(TEXT("WavePowerUpCardRow")));
}

void UWavePowerUpWidget::BuildFallbackTree()
{
	if (!WidgetTree)
	{
		return;
	}

	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("WavePowerUpRoot"));
		WidgetTree->RootWidget = RootCanvas;
	}

	if (!Dimmer)
	{
		Dimmer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WavePowerUpDimmer"));
		WavePowerUpWidgetPrivate::ApplyRoundedBrush(Dimmer, WavePowerUpWidgetPrivate::DimmerColor,
			FLinearColor(0.f, 0.f, 0.f, 0.f), 0.f);
		Dimmer->SetPadding(FMargin(0.f));
		if (UCanvasPanelSlot* DimSlot = RootCanvas->AddChildToCanvas(Dimmer))
		{
			DimSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			DimSlot->SetOffsets(FMargin(0.f));
			DimSlot->SetZOrder(0);
		}
	}

	if (!PanelBox)
	{
		PanelBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("WavePowerUpPanel"));
		if (UCanvasPanelSlot* PanelSlot = RootCanvas->AddChildToCanvas(PanelBox))
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetAutoSize(true);
			PanelSlot->SetZOrder(1);
		}
	}

	if (!TitleLabel)
	{
		TitleLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WavePowerUpTitle"));
		TitleLabel->SetText(FText::FromString(TEXT("CHOOSE A POWER-UP")));
		TitleLabel->SetJustification(ETextJustify::Center);
		SetTextStyle(TitleLabel, WavePowerUpWidgetPrivate::TitleColor, 28.f, true);
		if (UVerticalBoxSlot* TitleSlot = PanelBox->AddChildToVerticalBox(TitleLabel))
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Center);
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}
	}

	if (!SubtitleLabel)
	{
		SubtitleLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WavePowerUpSubtitle"));
		SubtitleLabel->SetText(FText::FromString(TEXT("One card. Both sides apply to the next wave.")));
		SubtitleLabel->SetJustification(ETextJustify::Center);
		SetTextStyle(SubtitleLabel, WavePowerUpWidgetPrivate::SubtitleColor, 14.f);
		if (UVerticalBoxSlot* SubSlot = PanelBox->AddChildToVerticalBox(SubtitleLabel))
		{
			SubSlot->SetHorizontalAlignment(HAlign_Center);
			SubSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
		}
	}

	if (!CardRow)
	{
		CardRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("WavePowerUpCardRow"));
		if (UVerticalBoxSlot* RowSlot = PanelBox->AddChildToVerticalBox(CardRow))
		{
			RowSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}
}

void UWavePowerUpWidget::BuildCards()
{
	if (!CardRow || !WidgetTree)
	{
		return;
	}

	Cards.Reset();
	Binders.Reset();
	CardRow->ClearChildren();
	for (int32 i = 0; i < FTDWavePowerUp::CardCount; ++i)
	{
		Cards.Add(BuildCard(i));
	}
}

FWavePowerUpCardUI UWavePowerUpWidget::BuildCard(int32 Index)
{
	FWavePowerUpCardUI Card;
	Card.CardIndex = Index;
	if (!CardRow || !WidgetTree)
	{
		return Card;
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), *FString::Printf(TEXT("PowerCardCol_%d"), Index));
	if (UHorizontalBoxSlot* ColSlot = CardRow->AddChildToHorizontalBox(Column))
	{
		ColSlot->SetPadding(FMargin(12.f, 0.f));
		ColSlot->SetVerticalAlignment(VAlign_Top);
	}

	USizeBox* Tile = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(), *FString::Printf(TEXT("PowerCardSize_%d"), Index));
	Tile->SetWidthOverride(240.f);
	Tile->SetHeightOverride(320.f);
	if (UVerticalBoxSlot* TileSlot = Column->AddChildToVerticalBox(Tile))
	{
		TileSlot->SetHorizontalAlignment(HAlign_Center);
	}

	Card.CardFrame = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), *FString::Printf(TEXT("PowerCardFrame_%d"), Index));
	Card.CardFrame->SetPadding(FMargin(8.f));
	WavePowerUpWidgetPrivate::ApplyRoundedBrush(
		Card.CardFrame, WavePowerUpWidgetPrivate::CardFill, WavePowerUpWidgetPrivate::CardOutline, 2.f);
	Tile->SetContent(Card.CardFrame);

	Card.SelectButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(), *FString::Printf(TEXT("PowerCardSelect_%d"), Index));
	Card.SelectButton->SetBackgroundColor(FLinearColor(0.05f, 0.07f, 0.10f, 0.35f));
	{
		UWavePowerUpCardBinder* Binder = NewObject<UWavePowerUpCardBinder>(this);
		Binder->Owner = this;
		Binder->CardIndex = Index;
		Card.SelectButton->OnClicked.AddDynamic(Binder, &UWavePowerUpCardBinder::HandleSelectClicked);
		Binders.Add(Binder);
	}
	Card.CardFrame->SetContent(Card.SelectButton);

	UVerticalBox* Faces = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), *FString::Printf(TEXT("PowerCardFaces_%d"), Index));
	Card.SelectButton->SetContent(Faces);

	UBorder* PlayerFace = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), *FString::Printf(TEXT("PowerCardPlayer_%d"), Index));
	PlayerFace->SetPadding(FMargin(12.f, 14.f));
	PlayerFace->SetVisibility(ESlateVisibility::HitTestInvisible);
	WavePowerUpWidgetPrivate::ApplyRoundedBrush(
		PlayerFace, WavePowerUpWidgetPrivate::PlayerFill, FLinearColor(0.2f, 0.55f, 0.28f, 1.f), 1.4f);
	if (UVerticalBoxSlot* PlayerSlot = Faces->AddChildToVerticalBox(PlayerFace))
	{
		PlayerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		PlayerSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}

	UVerticalBox* PlayerCol = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), *FString::Printf(TEXT("PowerCardPlayerCol_%d"), Index));
	PlayerFace->SetContent(PlayerCol);

	UTextBlock* PlayerTag = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), *FString::Printf(TEXT("PowerCardPlayerTag_%d"), Index));
	PlayerTag->SetText(FText::FromString(TEXT("YOU")));
	PlayerTag->SetJustification(ETextJustify::Center);
	SetTextStyle(PlayerTag, WavePowerUpWidgetPrivate::PlayerTitle, 11.f);
	PlayerCol->AddChildToVerticalBox(PlayerTag);

	Card.PlayerTitle = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), *FString::Printf(TEXT("PowerCardPlayerTitle_%d"), Index));
	Card.PlayerTitle->SetJustification(ETextJustify::Center);
	Card.PlayerTitle->SetAutoWrapText(true);
	SetTextStyle(Card.PlayerTitle, WavePowerUpWidgetPrivate::PlayerTitle, 20.f, true);
	if (UVerticalBoxSlot* PTSlot = PlayerCol->AddChildToVerticalBox(Card.PlayerTitle))
	{
		PTSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 8.f));
		PTSlot->SetHorizontalAlignment(HAlign_Center);
	}

	Card.PlayerDesc = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), *FString::Printf(TEXT("PowerCardPlayerDesc_%d"), Index));
	Card.PlayerDesc->SetJustification(ETextJustify::Center);
	Card.PlayerDesc->SetAutoWrapText(true);
	SetTextStyle(Card.PlayerDesc, WavePowerUpWidgetPrivate::PlayerBody, 14.f);
	PlayerCol->AddChildToVerticalBox(Card.PlayerDesc);

	UBorder* EnemyFace = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), *FString::Printf(TEXT("PowerCardEnemy_%d"), Index));
	EnemyFace->SetPadding(FMargin(12.f, 12.f));
	EnemyFace->SetVisibility(ESlateVisibility::HitTestInvisible);
	WavePowerUpWidgetPrivate::ApplyRoundedBrush(
		EnemyFace, WavePowerUpWidgetPrivate::EnemyFill, FLinearColor(0.7f, 0.18f, 0.14f, 1.f), 1.4f);
	if (UVerticalBoxSlot* EnemySlot = Faces->AddChildToVerticalBox(EnemyFace))
	{
		EnemySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	UVerticalBox* EnemyCol = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), *FString::Printf(TEXT("PowerCardEnemyCol_%d"), Index));
	EnemyFace->SetContent(EnemyCol);

	UTextBlock* EnemyTag = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), *FString::Printf(TEXT("PowerCardEnemyTag_%d"), Index));
	EnemyTag->SetText(FText::FromString(TEXT("ENEMY — BACK")));
	EnemyTag->SetJustification(ETextJustify::Center);
	SetTextStyle(EnemyTag, WavePowerUpWidgetPrivate::EnemyTitle, 11.f);
	EnemyCol->AddChildToVerticalBox(EnemyTag);

	Card.EnemyTitle = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), *FString::Printf(TEXT("PowerCardEnemyTitle_%d"), Index));
	Card.EnemyTitle->SetJustification(ETextJustify::Center);
	Card.EnemyTitle->SetAutoWrapText(true);
	SetTextStyle(Card.EnemyTitle, WavePowerUpWidgetPrivate::EnemyTitle, 18.f, true);
	if (UVerticalBoxSlot* ETSlot = EnemyCol->AddChildToVerticalBox(Card.EnemyTitle))
	{
		ETSlot->SetPadding(FMargin(0.f, 6.f, 0.f, 6.f));
		ETSlot->SetHorizontalAlignment(HAlign_Center);
	}

	Card.EnemyDesc = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), *FString::Printf(TEXT("PowerCardEnemyDesc_%d"), Index));
	Card.EnemyDesc->SetJustification(ETextJustify::Center);
	Card.EnemyDesc->SetAutoWrapText(true);
	SetTextStyle(Card.EnemyDesc, WavePowerUpWidgetPrivate::EnemyBody, 13.f);
	EnemyCol->AddChildToVerticalBox(Card.EnemyDesc);

	Card.RerollButton = WidgetTree->ConstructWidget<UButton>(
		UButton::StaticClass(), *FString::Printf(TEXT("PowerCardReroll_%d"), Index));
	Card.RerollButton->SetBackgroundColor(FLinearColor(0.12f, 0.10f, 0.06f, 0.96f));
	{
		UWavePowerUpCardBinder* Binder = NewObject<UWavePowerUpCardBinder>(this);
		Binder->Owner = this;
		Binder->CardIndex = Index;
		Card.RerollButton->OnClicked.AddDynamic(Binder, &UWavePowerUpCardBinder::HandleRerollClicked);
		Binders.Add(Binder);
	}
	if (UVerticalBoxSlot* RerollSlot = Column->AddChildToVerticalBox(Card.RerollButton))
	{
		RerollSlot->SetPadding(FMargin(0.f, 10.f, 0.f, 0.f));
		RerollSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	Card.RerollLabel = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), *FString::Printf(TEXT("PowerCardRerollLabel_%d"), Index));
	Card.RerollLabel->SetText(FText::FromString(FString::Printf(TEXT("Reroll  (%d)"), FTDWavePowerUp::RerollCost)));
	Card.RerollLabel->SetJustification(ETextJustify::Center);
	SetTextStyle(Card.RerollLabel, WavePowerUpWidgetPrivate::RerollColor, 13.f, true);
	Card.RerollButton->SetContent(Card.RerollLabel);

	return Card;
}

void UWavePowerUpWidget::RefreshCards()
{
	UWorld* World = GetWorld();
	UTDEnemyPathSubsystem* Sys = World ? World->GetSubsystem<UTDEnemyPathSubsystem>() : nullptr;
	if (!Sys)
	{
		return;
	}

	for (int32 i = 0; i < Cards.Num(); ++i)
	{
		FWavePowerUpCardUI& Card = Cards[i];
		if (!Sys->DraftOffers.IsValidIndex(i))
		{
			continue;
		}
		const FTDWavePowerUpOffer& Offer = Sys->DraftOffers[i];
		const FTDWavePowerUpInfo Player = FTDWavePowerUp::DescribePlayer(Offer.Player);
		const FTDWavePowerUpInfo Enemy = FTDWavePowerUp::DescribeEnemy(Offer.Enemy);
		if (Card.PlayerTitle)
		{
			Card.PlayerTitle->SetText(FText::FromString(Player.Title));
		}
		if (Card.PlayerDesc)
		{
			Card.PlayerDesc->SetText(FText::FromString(Player.Description));
		}
		if (Card.EnemyTitle)
		{
			Card.EnemyTitle->SetText(FText::FromString(Enemy.Title));
		}
		if (Card.EnemyDesc)
		{
			Card.EnemyDesc->SetText(FText::FromString(Enemy.Description));
		}
	}
}

void UWavePowerUpWidget::RefreshRerollAffordability()
{
	const float Resource = FTDWavePowerUp::ReadResource(FTDWavePowerUp::FindBuildManager(this));
	const bool bCan = Resource + 0.01f >= static_cast<float>(FTDWavePowerUp::RerollCost);
	for (FWavePowerUpCardUI& Card : Cards)
	{
		if (Card.RerollButton)
		{
			Card.RerollButton->SetIsEnabled(bCan);
			Card.RerollButton->SetRenderOpacity(bCan ? 1.f : 0.45f);
		}
		if (Card.RerollLabel)
		{
			SetTextStyle(Card.RerollLabel,
				bCan ? WavePowerUpWidgetPrivate::RerollColor : WavePowerUpWidgetPrivate::RerollDisabled,
				13.f, true);
		}
	}
}

void UWavePowerUpWidget::ApplyHitTestPolicy()
{
	const ESlateVisibility Vis = bDraftVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	SetVisibility(Vis);
	if (RootCanvas)
	{
		RootCanvas->SetVisibility(Vis);
	}
	if (Dimmer)
	{
		Dimmer->SetVisibility(bDraftVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UWavePowerUpWidget::SetTextStyle(UTextBlock* Text, const FLinearColor& Color, float Size, bool bBold)
{
	if (!Text)
	{
		return;
	}
	Text->SetColorAndOpacity(FSlateColor(Color));
	FSlateFontInfo Font = Text->GetFont();
	Font.Size = Size;
	if (bBold)
	{
		Font.TypefaceFontName = TEXT("Bold");
	}
	Text->SetFont(Font);
	Text->SetShadowOffset(FVector2D(1.f, 1.f));
	Text->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
}
