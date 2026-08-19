#include "CrystalHealthBarWidget.h"

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
#include "TDEnemyPathLibrary.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Styling/SlateBrush.h"
#include "UObject/UnrealType.h"

namespace CrystalHealthBarPrivate
{
	static FLinearColor ChromeBg(0.03f, 0.04f, 0.06f, 0.94f);
	static FLinearColor ChromeOutline(0.18f, 0.55f, 0.85f, 0.75f);
	static FLinearColor TrackBg(0.05f, 0.07f, 0.10f, 0.98f);
	static FLinearColor FillHealthy(0.18f, 0.72f, 1.0f, 1.f);
	static FLinearColor FillHurt(0.95f, 0.62f, 0.18f, 1.f);
	static FLinearColor FillCritical(0.92f, 0.22f, 0.22f, 1.f);
	static FLinearColor TitleColor(0.95f, 0.97f, 1.f, 1.f);
	static FLinearColor ValueColor(1.f, 1.f, 1.f, 1.f);
	static FLinearColor Neon(0.20f, 0.78f, 1.0f, 1.f);
	static FLinearColor DotEmptyFill(0.04f, 0.06f, 0.09f, 0.95f);
	static FLinearColor DotEmptyOutline(0.22f, 0.55f, 0.78f, 0.85f);
	static FLinearColor BossFill(0.12f, 0.03f, 0.03f, 0.96f);
	static FLinearColor BossOutline(0.95f, 0.18f, 0.16f, 1.f);
	static FLinearColor BossIcon(1.f, 0.32f, 0.28f, 1.f);
	static FLinearColor PlayFill(0.05f, 0.08f, 0.12f, 0.98f);
	static FLinearColor PlayFillBusy(0.06f, 0.07f, 0.09f, 0.95f);
	static FLinearColor PlayIcon(0.95f, 0.98f, 1.f, 1.f);
	static FLinearColor TimerDim(0.40f, 0.50f, 0.58f, 1.f);
	static FLinearColor EnemyCountHot(1.0f, 0.55f, 0.18f, 1.f);
	static FLinearColor EnemyCountIdle(0.40f, 0.50f, 0.58f, 1.f);
}

UCrystalHealthBarWidget::UCrystalHealthBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
	bHasScriptImplementedTick = true;
}

void UCrystalHealthBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureBuilt();
}

void UCrystalHealthBarWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureBuilt();
	ApplyHitTestPolicy();
	UE_LOG(LogTemp, Display, TEXT("CrystalHealthBarWidget constructed. Built=%d"), bBuilt ? 1 : 0);
}

void UCrystalHealthBarWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bBuilt || !NextWaveButton)
	{
		EnsureBuilt();
	}

	RefreshFromCrystal();
	RefreshWaveHud();
}

void UCrystalHealthBarWidget::EnsureBuilt()
{
	if (bBuilt && HealthBar && NextWaveButton && EnemiesCountLabel)
	{
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	if (!WidgetTree->RootWidget || !HealthBar || !NextWaveButton || !EnemiesCountLabel)
	{
		BuildDefaultUI();
	}

	bBuilt = HealthBar != nullptr && ValueLabel != nullptr && NextWaveButton != nullptr
		&& WaveChrome != nullptr && EnemiesCountLabel != nullptr;
	if (bBuilt)
	{
		ApplyHitTestPolicy();
	}
}

void UCrystalHealthBarWidget::ApplyHitTestPolicy()
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UWidget* Root = WidgetTree ? WidgetTree->RootWidget : nullptr)
	{
		Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (BarChrome)
	{
		BarChrome->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (WaveChrome)
	{
		WaveChrome->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (EnemiesChrome)
	{
		EnemiesChrome->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (EnemiesCountLabel)
	{
		EnemiesCountLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (NextWaveButton)
	{
		NextWaveButton->SetVisibility(ESlateVisibility::Visible);
	}
}

void UCrystalHealthBarWidget::ApplyRoundedBrush(UBorder* Border, const FLinearColor& Fill, const FLinearColor& Outline,
	float OutlineWidth, bool bCircle)
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
	if (bCircle)
	{
		Brush.OutlineSettings.CornerRadii = FVector4(1.f, 1.f, 1.f, 1.f);
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
	}
	else
	{
		Brush.OutlineSettings.CornerRadii = FVector4(8.f, 8.f, 8.f, 8.f);
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	}
	Border->SetBrush(Brush);
}

void UCrystalHealthBarWidget::BuildDefaultUI()
{
	if (!WidgetTree)
	{
		return;
	}

	UCanvasPanel* Root = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!Root)
	{
		Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("CrystalHealthRoot"));
		WidgetTree->RootWidget = Root;
	}
	else
	{
		Root->ClearChildren();
	}

	BarChrome = nullptr;
	WaveChrome = nullptr;
	EnemiesChrome = nullptr;
	BarSizeBox = nullptr;
	HealthBar = nullptr;
	TitleLabel = nullptr;
	ValueLabel = nullptr;
	WaveDotsBox = nullptr;
	WaveDots.Reset();
	WaveDotIcons.Reset();
	NextWaveSizeBox = nullptr;
	NextWaveFrame = nullptr;
	NextWaveButton = nullptr;
	NextWaveLabel = nullptr;
	EnemiesCountLabel = nullptr;
	TimerLabel = nullptr;
	BuiltDotCount = 0;

	UVerticalBox* HudColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BaseHudColumn"));
	HudColumn->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UCanvasPanelSlot* ColumnSlot = Root->AddChildToCanvas(HudColumn))
	{
		ColumnSlot->SetAnchors(FAnchors(0.5f, 0.f, 0.5f, 0.f));
		ColumnSlot->SetAlignment(FVector2D(0.5f, 0.f));
		ColumnSlot->SetAutoSize(true);
		ColumnSlot->SetOffsets(FMargin(0.f, TopPad, 0.f, 0.f));
		ColumnSlot->SetZOrder(10);
	}

	BarChrome = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BaseHealthChrome"));
	BarChrome->SetPadding(FMargin(16.f, 10.f, 16.f, 12.f));
	ApplyRoundedBrush(BarChrome, CrystalHealthBarPrivate::ChromeBg, CrystalHealthBarPrivate::ChromeOutline, 1.6f, false);
	if (UVerticalBoxSlot* HealthSlot = HudColumn->AddChildToVerticalBox(BarChrome))
	{
		HealthSlot->SetHorizontalAlignment(HAlign_Fill);
		HealthSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}

	UVerticalBox* HealthColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("BaseHealthColumn"));
	BarChrome->SetContent(HealthColumn);

	TitleLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BaseHealthTitle"));
	TitleLabel->SetText(FText::FromString(TEXT("BASE HEALTH")));
	TitleLabel->SetJustification(ETextJustify::Center);
	TitleLabel->SetColorAndOpacity(FSlateColor(CrystalHealthBarPrivate::TitleColor));
	TitleLabel->SetShadowOffset(FVector2D(1.f, 1.f));
	TitleLabel->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
	{
		FSlateFontInfo Font = TitleLabel->GetFont();
		Font.Size = 13.f;
		Font.TypefaceFontName = TEXT("Bold");
		TitleLabel->SetFont(Font);
	}
	if (UVerticalBoxSlot* TitleSlot = HealthColumn->AddChildToVerticalBox(TitleLabel))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BaseHealthFrame"));
	Frame->SetPadding(FMargin(2.f));
	ApplyRoundedBrush(Frame, CrystalHealthBarPrivate::TrackBg, CrystalHealthBarPrivate::ChromeOutline, 1.2f, false);

	BarSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BaseHealthSize"));
	BarSizeBox->SetWidthOverride(BarWidth);
	BarSizeBox->SetHeightOverride(BarHeight);
	Frame->SetContent(BarSizeBox);

	UOverlay* TrackOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("BaseHealthOverlay"));
	BarSizeBox->SetContent(TrackOverlay);

	UBorder* Track = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("BaseHealthTrack"));
	ApplyRoundedBrush(Track, CrystalHealthBarPrivate::TrackBg, FLinearColor(0.f, 0.f, 0.f, 0.f), 0.f, false);
	if (UOverlaySlot* TrackSlot = TrackOverlay->AddChildToOverlay(Track))
	{
		TrackSlot->SetHorizontalAlignment(HAlign_Fill);
		TrackSlot->SetVerticalAlignment(VAlign_Fill);
	}

	HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("BaseHealthBar"));
	HealthBar->SetPercent(1.f);
	HealthBar->SetFillColorAndOpacity(CrystalHealthBarPrivate::FillHealthy);
	HealthBar->SetBarFillType(EProgressBarFillType::LeftToRight);
	HealthBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UOverlaySlot* BarSlot = TrackOverlay->AddChildToOverlay(HealthBar))
	{
		BarSlot->SetHorizontalAlignment(HAlign_Fill);
		BarSlot->SetVerticalAlignment(VAlign_Fill);
	}

	ValueLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("BaseHealthValue"));
	ValueLabel->SetText(FText::FromString(TEXT("100 / 100")));
	ValueLabel->SetJustification(ETextJustify::Center);
	ValueLabel->SetColorAndOpacity(FSlateColor(CrystalHealthBarPrivate::ValueColor));
	ValueLabel->SetShadowOffset(FVector2D(1.f, 1.f));
	ValueLabel->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f));
	ValueLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
	{
		FSlateFontInfo Font = ValueLabel->GetFont();
		Font.Size = 13.f;
		Font.TypefaceFontName = TEXT("Bold");
		ValueLabel->SetFont(Font);
	}
	if (UOverlaySlot* ValueSlot = TrackOverlay->AddChildToOverlay(ValueLabel))
	{
		ValueSlot->SetHorizontalAlignment(HAlign_Fill);
		ValueSlot->SetVerticalAlignment(VAlign_Center);
	}

	if (UVerticalBoxSlot* FrameSlot = HealthColumn->AddChildToVerticalBox(Frame))
	{
		FrameSlot->SetHorizontalAlignment(HAlign_Center);
	}

	WaveChrome = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("WaveStripChrome"));
	WaveChrome->SetPadding(FMargin(14.f, 8.f));
	ApplyRoundedBrush(WaveChrome, CrystalHealthBarPrivate::ChromeBg, CrystalHealthBarPrivate::ChromeOutline, 1.6f, false);
	if (UVerticalBoxSlot* WaveSlot = HudColumn->AddChildToVerticalBox(WaveChrome))
	{
		WaveSlot->SetHorizontalAlignment(HAlign_Fill);
	}

	UHorizontalBox* WaveRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("WaveStripRow"));
	WaveChrome->SetContent(WaveRow);
	BuildWaveRow(WaveRow);

	EnemiesChrome = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EnemiesCountChrome"));
	EnemiesChrome->SetPadding(FMargin(16.f, 8.f));
	ApplyRoundedBrush(EnemiesChrome, CrystalHealthBarPrivate::ChromeBg, CrystalHealthBarPrivate::EnemyCountHot, 1.8f, false);
	if (UVerticalBoxSlot* EnemiesSlot = HudColumn->AddChildToVerticalBox(EnemiesChrome))
	{
		EnemiesSlot->SetHorizontalAlignment(HAlign_Fill);
		EnemiesSlot->SetPadding(FMargin(0.f, 8.f, 0.f, 0.f));
	}

	EnemiesCountLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WaveEnemiesCount"));
	EnemiesCountLabel->SetText(FText::FromString(TEXT("ENEMIES LEFT  0")));
	EnemiesCountLabel->SetJustification(ETextJustify::Center);
	EnemiesCountLabel->SetColorAndOpacity(FSlateColor(CrystalHealthBarPrivate::EnemyCountIdle));
	EnemiesCountLabel->SetShadowOffset(FVector2D(1.f, 1.f));
	EnemiesCountLabel->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f));
	EnemiesCountLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
	{
		FSlateFontInfo Font = EnemiesCountLabel->GetFont();
		Font.Size = 22.f;
		Font.TypefaceFontName = TEXT("Bold");
		EnemiesCountLabel->SetFont(Font);
	}
	EnemiesChrome->SetContent(EnemiesCountLabel);

	ApplyHitTestPolicy();
}

void UCrystalHealthBarWidget::BuildWaveRow(UHorizontalBox* Parent)
{
	if (!Parent || !WidgetTree)
	{
		return;
	}

	WaveDotsBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("WaveDotsBox"));
	if (UHorizontalBoxSlot* DotsSlot = Parent->AddChildToHorizontalBox(WaveDotsBox))
	{
		DotsSlot->SetVerticalAlignment(VAlign_Center);
		DotsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		DotsSlot->SetPadding(FMargin(4.f, 0.f, 8.f, 0.f));
	}
	RebuildWaveDots();

	NextWaveSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("NextWaveSize"));
	NextWaveSizeBox->SetWidthOverride(PlayButtonSize);
	NextWaveSizeBox->SetHeightOverride(PlayButtonSize);
	if (UHorizontalBoxSlot* PlaySlot = Parent->AddChildToHorizontalBox(NextWaveSizeBox))
	{
		PlaySlot->SetVerticalAlignment(VAlign_Center);
		PlaySlot->SetPadding(FMargin(6.f, 0.f, 10.f, 0.f));
	}

	NextWaveFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NextWaveFrame"));
	ApplyRoundedBrush(NextWaveFrame, CrystalHealthBarPrivate::PlayFill, CrystalHealthBarPrivate::Neon, 2.f, true);
	NextWaveSizeBox->SetContent(NextWaveFrame);

	NextWaveButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("NextWaveButton"));
	NextWaveButton->SetBackgroundColor(FLinearColor(0.04f, 0.08f, 0.12f, 0.2f));
	NextWaveButton->SetIsEnabled(true);
	NextWaveButton->OnClicked.AddDynamic(this, &UCrystalHealthBarWidget::OnNextWaveClicked);
	NextWaveFrame->SetContent(NextWaveButton);

	NextWaveLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NextWavePlayIcon"));
	NextWaveLabel->SetText(FText::FromString(TEXT("▶")));
	NextWaveLabel->SetJustification(ETextJustify::Center);
	NextWaveLabel->SetColorAndOpacity(FSlateColor(CrystalHealthBarPrivate::PlayIcon));
	NextWaveLabel->SetShadowOffset(FVector2D(1.f, 1.f));
	NextWaveLabel->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.7f));
	{
		FSlateFontInfo Font = NextWaveLabel->GetFont();
		Font.Size = 16.f;
		Font.TypefaceFontName = TEXT("Bold");
		NextWaveLabel->SetFont(Font);
	}
	NextWaveButton->SetContent(NextWaveLabel);

	TimerLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("WaveTimer"));
	TimerLabel->SetText(FText::FromString(TEXT("0:00")));
	TimerLabel->SetJustification(ETextJustify::Right);
	TimerLabel->SetColorAndOpacity(FSlateColor(CrystalHealthBarPrivate::Neon));
	TimerLabel->SetShadowOffset(FVector2D(1.f, 1.f));
	TimerLabel->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
	TimerLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
	{
		FSlateFontInfo Font = TimerLabel->GetFont();
		Font.Size = 20.f;
		Font.TypefaceFontName = TEXT("Bold");
		TimerLabel->SetFont(Font);
	}
	if (UHorizontalBoxSlot* TimerSlot = Parent->AddChildToHorizontalBox(TimerLabel))
	{
		TimerSlot->SetVerticalAlignment(VAlign_Center);
		TimerSlot->SetPadding(FMargin(4.f, 0.f, 4.f, 0.f));
	}
}

void UCrystalHealthBarWidget::RebuildWaveDots()
{
	if (!WaveDotsBox || !WidgetTree)
	{
		return;
	}

	const int32 Count = FMath::Clamp(TotalWaves, 1, 20);
	WaveDotsBox->ClearChildren();
	WaveDots.Reset();
	WaveDots.Reserve(Count);
	WaveDotIcons.Reset();
	WaveDotIcons.Reserve(Count);

	for (int32 Index = 0; Index < Count; ++Index)
	{
		USizeBox* DotSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(),
			*FString::Printf(TEXT("WaveDotSize_%d"), Index));
		DotSize->SetWidthOverride(CircleSize);
		DotSize->SetHeightOverride(CircleSize);
		DotSize->SetVisibility(ESlateVisibility::HitTestInvisible);

		UBorder* Dot = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(),
			*FString::Printf(TEXT("WaveDot_%d"), Index));
		ApplyRoundedBrush(Dot, CrystalHealthBarPrivate::DotEmptyFill, CrystalHealthBarPrivate::DotEmptyOutline, 1.6f, true);
		DotSize->SetContent(Dot);
		WaveDots.Add(Dot);

		// Skull glyph shown on top of this dot only when it is the upcoming boss wave.
		UTextBlock* Icon = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
			*FString::Printf(TEXT("WaveDotBossIcon_%d"), Index));
		Icon->SetText(FText::FromString(TEXT("☠")));
		Icon->SetJustification(ETextJustify::Center);
		Icon->SetColorAndOpacity(FSlateColor(CrystalHealthBarPrivate::BossIcon));
		Icon->SetShadowOffset(FVector2D(1.f, 1.f));
		Icon->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
		Icon->SetVisibility(ESlateVisibility::Collapsed);
		{
			FSlateFontInfo Font = Icon->GetFont();
			Font.Size = FMath::Max(8.f, CircleSize * 0.6f);
			Font.TypefaceFontName = TEXT("Bold");
			Icon->SetFont(Font);
		}
		Dot->SetContent(Icon);
		WaveDotIcons.Add(Icon);

		if (UHorizontalBoxSlot* DotSlot = WaveDotsBox->AddChildToHorizontalBox(DotSize))
		{
			DotSlot->SetVerticalAlignment(VAlign_Center);
			DotSlot->SetPadding(FMargin(4.f, 0.f));
		}
	}

	BuiltDotCount = Count;
}

AActor* UCrystalHealthBarWidget::FindCrystal() const
{
	if (AActor* Cached = CachedCrystal.Get())
	{
		return Cached;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UClass* CrystalClass = CrystalActorClass.TryLoadClass<AActor>();
	if (!CrystalClass)
	{
		return nullptr;
	}

	for (TActorIterator<AActor> It(World, CrystalClass); It; ++It)
	{
		if (IsValid(*It))
		{
			return *It;
		}
	}
	return nullptr;
}

AActor* UCrystalHealthBarWidget::FindEnemySpawner() const
{
	if (AActor* Cached = CachedSpawner.Get())
	{
		return Cached;
	}

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

	AActor* Best = nullptr;
	int32 BestRoute = MAX_int32;
	for (TActorIterator<AActor> It(World, SpawnerClass); It; ++It)
	{
		AActor* Candidate = *It;
		if (!IsValid(Candidate))
		{
			continue;
		}
		int32 Route = 0;
		if (!ReadIntProp(Candidate, FName(TEXT("routeId")), Route))
		{
			ReadIntProp(Candidate, FName(TEXT("RouteId")), Route);
		}
		if (!Best || Route < BestRoute)
		{
			Best = Candidate;
			BestRoute = Route;
		}
	}
	return Best;
}

int32 UCrystalHealthBarWidget::ResolveTotalWaves(const AActor* Spawner) const
{
	int32 Count = TotalWaves;
	if (Spawner)
	{
		int32 FromSpawner = 0;
		if (ReadIntProp(Spawner, FName(TEXT("TotalWaves")), FromSpawner)
			|| ReadIntProp(Spawner, FName(TEXT("MaxWaves")), FromSpawner)
			|| ReadIntProp(Spawner, FName(TEXT("WaveCount")), FromSpawner))
		{
			if (FromSpawner > 0)
			{
				Count = FromSpawner;
			}
		}

		int32 BossWave = 0;
		if (ReadIntProp(Spawner, FName(TEXT("BossWaveNumber")), BossWave) && BossWave > Count)
		{
			Count = BossWave;
		}
	}
	return FMath::Clamp(Count, 1, 20);
}

void UCrystalHealthBarWidget::RefreshFromCrystal()
{
	if (!bBuilt || !HealthBar || !ValueLabel)
	{
		return;
	}

	AActor* Crystal = FindCrystal();
	CachedCrystal = Crystal;

	float Current = 0.f;
	float Max = 0.f;
	bool bHasHealth = false;

	if (Crystal)
	{
		bHasHealth = ReadFloatProp(Crystal, FName(TEXT("CurrentHealth")), Current)
			&& ReadFloatProp(Crystal, FName(TEXT("MaxHealth")), Max);
		if (!bHasHealth)
		{
			int32 CurrentInt = 0;
			int32 MaxInt = 0;
			if (ReadIntProp(Crystal, FName(TEXT("CurrentHealth")), CurrentInt)
				&& ReadIntProp(Crystal, FName(TEXT("MaxHealth")), MaxInt))
			{
				Current = static_cast<float>(CurrentInt);
				Max = static_cast<float>(MaxInt);
				bHasHealth = true;
			}
		}
	}

	if (!bHasHealth || Max <= 0.f)
	{
		HealthBar->SetPercent(0.f);
		ValueLabel->SetText(FText::FromString(TEXT("-- / --")));
		HealthBar->SetFillColorAndOpacity(CrystalHealthBarPrivate::FillCritical);
		if (BarChrome)
		{
			BarChrome->SetRenderOpacity(0.55f);
		}
		return;
	}

	const float Percent = FMath::Clamp(Current / Max, 0.f, 1.f);
	HealthBar->SetPercent(Percent);

	FLinearColor Fill = CrystalHealthBarPrivate::FillHealthy;
	if (Percent <= 0.25f)
	{
		Fill = CrystalHealthBarPrivate::FillCritical;
	}
	else if (Percent <= 0.5f)
	{
		Fill = CrystalHealthBarPrivate::FillHurt;
	}
	HealthBar->SetFillColorAndOpacity(Fill);

	const int32 CurInt = FMath::Max(0, FMath::CeilToInt(Current));
	const int32 MaxInt = FMath::Max(1, FMath::CeilToInt(Max));
	ValueLabel->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"), CurInt, MaxInt)));

	if (BarChrome)
	{
		BarChrome->SetRenderOpacity(1.f);
	}
}

void UCrystalHealthBarWidget::RefreshWaveHud()
{
	if (!bBuilt)
	{
		return;
	}

	AActor* Spawner = FindEnemySpawner();
	CachedSpawner = Spawner;

	const int32 DesiredDots = ResolveTotalWaves(Spawner);
	if (DesiredDots != BuiltDotCount)
	{
		TotalWaves = DesiredDots;
		RebuildWaveDots();
	}

	int32 WaveNumber = 0;
	int32 BossWaveNumber = 0;
	bool bIsBossWave = false;
	bool bSpawning = false;
	bool bWaitingClear = false;
	float Countdown = 0.f;

	if (Spawner)
	{
		ReadIntProp(Spawner, FName(TEXT("WaveNumber")), WaveNumber);
		ReadIntProp(Spawner, FName(TEXT("BossWaveNumber")), BossWaveNumber);
		ReadBoolProp(Spawner, FName(TEXT("IsBossWave")), bIsBossWave);
		if (!bIsBossWave)
		{
			ReadBoolProp(Spawner, FName(TEXT("bIsBossWave")), bIsBossWave);
		}
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
	}

	// The boss dot is whichever wave-dot index matches BossWaveNumber (1-based); it stays marked
	// red with the skull glyph for as long as the boss wave hasn't been reached/cleared yet
	// (same condition the old floating icon used), so the icon always sits on the wave it
	// actually spawns from instead of floating in a fixed spot next to the dot row.
	const int32 BossIndex = BossWaveNumber > 0 ? (BossWaveNumber - 1) : (bIsBossWave ? WaveNumber - 1 : INDEX_NONE);
	const bool bBossPending = BossIndex != INDEX_NONE
		&& (bIsBossWave || (BossWaveNumber > 0 && WaveNumber <= BossWaveNumber));
	const int32 Filled = FMath::Clamp(WaveNumber, 0, WaveDots.Num());
	for (int32 Index = 0; Index < WaveDots.Num(); ++Index)
	{
		UBorder* Dot = WaveDots[Index];
		if (!Dot)
		{
			continue;
		}
		const bool bFilled = Index < Filled;
		const bool bIsBossDot = Index == BossIndex && bBossPending;
		if (bIsBossDot)
		{
			ApplyRoundedBrush(Dot, CrystalHealthBarPrivate::BossFill, CrystalHealthBarPrivate::BossOutline, 1.6f, true);
		}
		else if (bFilled)
		{
			ApplyRoundedBrush(Dot, CrystalHealthBarPrivate::Neon, CrystalHealthBarPrivate::Neon, 1.2f, true);
		}
		else
		{
			ApplyRoundedBrush(Dot, CrystalHealthBarPrivate::DotEmptyFill, CrystalHealthBarPrivate::DotEmptyOutline, 1.6f, true);
		}

		if (WaveDotIcons.IsValidIndex(Index) && WaveDotIcons[Index])
		{
			WaveDotIcons[Index]->SetVisibility(bIsBossDot ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
		}
	}

	const bool bBusy = bSpawning || bWaitingClear;
	if (NextWaveButton)
	{
		NextWaveButton->SetIsEnabled(!bBusy && Spawner != nullptr);
	}
	if (NextWaveFrame)
	{
		ApplyRoundedBrush(NextWaveFrame,
			bBusy ? CrystalHealthBarPrivate::PlayFillBusy : CrystalHealthBarPrivate::PlayFill,
			bBusy ? CrystalHealthBarPrivate::DotEmptyOutline : CrystalHealthBarPrivate::Neon,
			2.f, true);
	}
	if (NextWaveSizeBox)
	{
		NextWaveSizeBox->SetRenderOpacity(bBusy ? 0.45f : 1.f);
	}
	if (NextWaveLabel)
	{
		NextWaveLabel->SetText(FText::FromString(bBusy ? TEXT("❚❚") : TEXT("▶")));
		NextWaveLabel->SetColorAndOpacity(FSlateColor(CrystalHealthBarPrivate::PlayIcon));
	}

	if (EnemiesCountLabel)
	{
		const int32 Remaining = UTDEnemyPathLibrary::CountWaveEnemiesRemaining(this);
		EnemiesCountLabel->SetText(FText::FromString(FString::Printf(TEXT("ENEMIES %d"), Remaining)));
		EnemiesCountLabel->SetColorAndOpacity(FSlateColor(
			Remaining > 0 ? CrystalHealthBarPrivate::EnemyCountHot : CrystalHealthBarPrivate::EnemyCountIdle));
	}

	if (TimerLabel)
	{
		const int32 TotalSeconds = FMath::Max(0, FMath::CeilToInt(Countdown));
		const int32 Minutes = TotalSeconds / 60;
		const int32 Seconds = TotalSeconds % 60;
		TimerLabel->SetText(FText::FromString(FString::Printf(TEXT("%d:%02d"), Minutes, Seconds)));
		TimerLabel->SetColorAndOpacity(FSlateColor(
			TotalSeconds > 0 ? CrystalHealthBarPrivate::Neon : CrystalHealthBarPrivate::TimerDim));
	}
}

void UCrystalHealthBarWidget::OnNextWaveClicked()
{
	AActor* Spawner = FindEnemySpawner();
	if (!Spawner)
	{
		UE_LOG(LogTemp, Warning, TEXT("BaseHealth NextWave: no BP_EnemySpawner found"));
		return;
	}

	UTDEnemyPathLibrary::ForceStartNextWave(Spawner);
	RefreshWaveHud();
}

bool UCrystalHealthBarWidget::ReadFloatProp(const UObject* Obj, FName Name, float& OutValue)
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

bool UCrystalHealthBarWidget::ReadBoolProp(const UObject* Obj, FName Name, bool& OutValue)
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

bool UCrystalHealthBarWidget::ReadIntProp(const UObject* Obj, FName Name, int32& OutValue)
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
