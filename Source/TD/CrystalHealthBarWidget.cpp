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
	static FLinearColor EnemySideOutline(0.95f, 0.18f, 0.16f, 1.f);
	static FLinearColor ThreatLow(0.96f, 0.78f, 0.20f, 1.f);
	static FLinearColor ThreatMedium(1.0f, 0.48f, 0.12f, 1.f);
	static FLinearColor ThreatHigh(0.95f, 0.16f, 0.12f, 1.f);
}

FCrystalWaveThreat UCrystalHealthBarWidget::CalculateCrystalWaveThreat(float EnemyResourcePool)
{
	const float Pool = FMath::Max(0.f, EnemyResourcePool);
	FCrystalWaveThreat Result;
	Result.ExtraEnemies = FMath::Min(FMath::FloorToInt(Pool / 15.f), 5);
	Result.EmpowermentPercent = FMath::RoundToInt(FMath::Clamp(Pool * 0.03f, 0.f, 1.f) * 100.f);
	if (Result.ExtraEnemies >= 5)
	{
		Result.ThreatLevel = ECrystalThreatLevel::High;
	}
	else if (Result.ExtraEnemies >= 3)
	{
		Result.ThreatLevel = ECrystalThreatLevel::Medium;
	}
	else if (Pool > KINDA_SMALL_NUMBER)
	{
		Result.ThreatLevel = ECrystalThreatLevel::Low;
	}
	return Result;
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

const TCHAR* UCrystalHealthBarWidget::GetWidgetBlueprintPath()
{
	return TEXT("/Game/TD/UI/WBP_CrystalHealthBar.WBP_CrystalHealthBar_C");
}

TSubclassOf<UCrystalHealthBarWidget> UCrystalHealthBarWidget::ResolveWidgetClass()
{
	return LoadClass<UCrystalHealthBarWidget>(nullptr, GetWidgetBlueprintPath());
}

void UCrystalHealthBarWidget::EnsureBuilt()
{
	if (bBuilt && HealthBar && NextWaveButton && EnemiesCountLabel && ThreatImpactLabel && ThreatSourceLabel)
	{
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	BindDesignerWidgets();
	BindNextWaveClick();

	bBuilt = HealthBar != nullptr && ValueLabel != nullptr && NextWaveButton != nullptr
		&& WaveChrome != nullptr && EnemiesCountLabel != nullptr
		&& ThreatImpactLabel != nullptr && ThreatSourceLabel != nullptr
		&& WaveDotsBox != nullptr;
	if (bBuilt)
	{
		ApplyHitTestPolicy();
	}
}

void UCrystalHealthBarWidget::BindDesignerWidgets()
{
	BarChrome = Cast<UBorder>(GetWidgetFromName(TEXT("BaseHealthChrome")));
	WaveChrome = Cast<UBorder>(GetWidgetFromName(TEXT("WaveStripChrome")));
	EnemiesChrome = Cast<UBorder>(GetWidgetFromName(TEXT("EnemiesCountChrome")));
	ThreatChrome = Cast<UBorder>(GetWidgetFromName(TEXT("CrystalThreatChrome")));
	BarSizeBox = Cast<USizeBox>(GetWidgetFromName(TEXT("BaseHealthSize")));
	HealthBar = Cast<UProgressBar>(GetWidgetFromName(TEXT("BaseHealthBar")));
	TitleLabel = Cast<UTextBlock>(GetWidgetFromName(TEXT("BaseHealthTitle")));
	ValueLabel = Cast<UTextBlock>(GetWidgetFromName(TEXT("BaseHealthValue")));
	WaveLabel = Cast<UTextBlock>(GetWidgetFromName(TEXT("WaveLabel")));
	if (!WaveLabel)
	{
		WaveLabel = Cast<UTextBlock>(GetWidgetFromName(TEXT("WaveTitle")));
	}
	WaveDotsBox = Cast<UHorizontalBox>(GetWidgetFromName(TEXT("WaveDotsBox")));
	if (!WaveDotsBox)
	{
		WaveDotsBox = Cast<UHorizontalBox>(GetWidgetFromName(TEXT("WaveDotsHost")));
	}
	NextWaveSizeBox = Cast<USizeBox>(GetWidgetFromName(TEXT("NextWaveSize")));
	NextWaveFrame = Cast<UBorder>(GetWidgetFromName(TEXT("NextWaveFrame")));
	if (!NextWaveFrame)
	{
		NextWaveFrame = Cast<UBorder>(GetWidgetFromName(TEXT("PlayNextWaveFrame")));
	}
	NextWaveButton = Cast<UButton>(GetWidgetFromName(TEXT("NextWaveButton")));
	if (!NextWaveButton)
	{
		NextWaveButton = Cast<UButton>(GetWidgetFromName(TEXT("PlayNextWave")));
	}
	NextWaveLabel = Cast<UTextBlock>(GetWidgetFromName(TEXT("NextWavePlayIcon")));
	EnemiesCountLabel = Cast<UTextBlock>(GetWidgetFromName(TEXT("WaveEnemiesCount")));
	ThreatImpactLabel = Cast<UTextBlock>(GetWidgetFromName(TEXT("NextWaveCrystalImpact")));
	ThreatSourceLabel = Cast<UTextBlock>(GetWidgetFromName(TEXT("EnemyCrystalAccumulation")));
	TimerLabel = Cast<UTextBlock>(GetWidgetFromName(TEXT("WaveTimer")));
}

void UCrystalHealthBarWidget::BindNextWaveClick()
{
	if (NextWaveButton && !NextWaveButton->OnClicked.IsBound())
	{
		NextWaveButton->OnClicked.AddDynamic(this, &UCrystalHealthBarWidget::OnNextWaveClicked);
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
	if (ThreatChrome)
	{
		ThreatChrome->SetVisibility(ESlateVisibility::HitTestInvisible);
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
		ApplyRoundedBrush(Dot, CrystalHealthBarPrivate::DotEmptyFill, CrystalHealthBarPrivate::EnemySideOutline, 1.6f, true);
		DotSize->SetContent(Dot);
		WaveDots.Add(Dot);

		// Skull glyph shown on top of this dot only when it is the upcoming boss wave.
		UTextBlock* Icon = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(),
			*FString::Printf(TEXT("WaveDotBossIcon_%d"), Index));
		Icon->SetText(FText::FromString(TEXT("â˜ ")));
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
	float EnemyResourcePool = 0.f;
	float EnemyBonusPerSecond = 0.f;

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
		ReadFloatProp(Spawner, FName(TEXT("EnemyResourcePool")), EnemyResourcePool);
		ReadFloatProp(Spawner, FName(TEXT("EnemyBonusPerSecond")), EnemyBonusPerSecond);
	}

	if (ThreatImpactLabel && ThreatSourceLabel && ThreatChrome)
	{
		const FCrystalWaveThreat Threat = CalculateCrystalWaveThreat(EnemyResourcePool);
		FLinearColor ThreatColor = CrystalHealthBarPrivate::EnemyCountIdle;
		const TCHAR* ThreatName = TEXT("NONE");
		switch (Threat.ThreatLevel)
		{
		case ECrystalThreatLevel::Low:
			ThreatColor = CrystalHealthBarPrivate::ThreatLow;
			ThreatName = TEXT("LOW");
			break;
		case ECrystalThreatLevel::Medium:
			ThreatColor = CrystalHealthBarPrivate::ThreatMedium;
			ThreatName = TEXT("MEDIUM");
			break;
		case ECrystalThreatLevel::High:
			ThreatColor = CrystalHealthBarPrivate::ThreatHigh;
			ThreatName = TEXT("HIGH");
			break;
		default:
			break;
		}

		ThreatImpactLabel->SetText(FText::FromString(FString::Printf(
			TEXT("NEXT WAVE  +%d ENEMIES  |  +%d%% HP & DAMAGE"),
			Threat.ExtraEnemies, Threat.EmpowermentPercent)));
		ThreatImpactLabel->SetColorAndOpacity(FSlateColor(ThreatColor));
		ThreatSourceLabel->SetText(FText::FromString(
			EnemyBonusPerSecond > KINDA_SMALL_NUMBER
				? FString::Printf(TEXT("ENEMY CRYSTALS +%.1f/s  â€¢  THREAT %s"), EnemyBonusPerSecond, ThreatName)
				: FString::Printf(TEXT("ACCUMULATION STOPPED  â€¢  THREAT %s"), ThreatName)));
		ThreatSourceLabel->SetColorAndOpacity(FSlateColor(ThreatColor.CopyWithNewOpacity(0.82f)));
		ApplyRoundedBrush(ThreatChrome, CrystalHealthBarPrivate::ChromeBg, ThreatColor, 1.6f, false);
	}

	if (WaveLabel)
	{
		WaveLabel->SetText(FText::FromString(FString::Printf(
			TEXT("WAVE  %d / %d"), FMath::Max(0, WaveNumber), DesiredDots)));
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
			ApplyRoundedBrush(Dot, CrystalHealthBarPrivate::EnemySideOutline, CrystalHealthBarPrivate::EnemySideOutline, 1.2f, true);
		}
		else
		{
			ApplyRoundedBrush(Dot, CrystalHealthBarPrivate::DotEmptyFill, CrystalHealthBarPrivate::EnemySideOutline, 1.6f, true);
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
			bBusy ? CrystalHealthBarPrivate::DotEmptyOutline : CrystalHealthBarPrivate::EnemySideOutline,
			2.f, false);
	}
	if (NextWaveSizeBox)
	{
		NextWaveSizeBox->SetRenderOpacity(bBusy ? 0.45f : 1.f);
	}
	if (NextWaveLabel)
	{
		NextWaveLabel->SetText(FText::FromString(bBusy ? TEXT("âšâš") : TEXT("â–¶")));
		NextWaveLabel->SetColorAndOpacity(FSlateColor(CrystalHealthBarPrivate::PlayIcon));
	}

	if (EnemiesCountLabel)
	{
		const int32 Remaining = UTDEnemyPathLibrary::CountWaveEnemiesRemaining(this);
		EnemiesCountLabel->SetText(FText::FromString(FString::Printf(TEXT("ENEMIES\n%d"), Remaining)));
		EnemiesCountLabel->SetColorAndOpacity(FSlateColor(
			Remaining > 0 ? CrystalHealthBarPrivate::EnemyCountHot : CrystalHealthBarPrivate::EnemyCountIdle));
	}

	if (TimerLabel)
	{
		const int32 TotalSeconds = FMath::Max(0, FMath::CeilToInt(Countdown));
		const int32 Minutes = TotalSeconds / 60;
		const int32 Seconds = TotalSeconds % 60;
		TimerLabel->SetText(FText::FromString(FString::Printf(TEXT("â—·  %d:%02d"), Minutes, Seconds)));
		TimerLabel->SetColorAndOpacity(FSlateColor(
			TotalSeconds > 0 ? CrystalHealthBarPrivate::EnemySideOutline : CrystalHealthBarPrivate::TimerDim));
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
