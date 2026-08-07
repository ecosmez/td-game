#include "CrystalHealthBarWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

namespace CrystalHealthBarPrivate
{
	static FLinearColor ChromeBg(0.05f, 0.08f, 0.12f, 0.92f);
	static FLinearColor FrameColor(0.55f, 0.62f, 0.72f, 1.f);
	static FLinearColor TrackBg(0.08f, 0.10f, 0.14f, 0.95f);
	static FLinearColor FillHealthy(0.20f, 0.85f, 0.45f, 1.f);
	static FLinearColor FillHurt(0.95f, 0.72f, 0.22f, 1.f);
	static FLinearColor FillCritical(0.92f, 0.28f, 0.28f, 1.f);
	static FLinearColor TitleColor(0.92f, 0.95f, 1.f, 1.f);
	static FLinearColor ValueColor(1.f, 0.88f, 0.35f, 1.f);
}

UCrystalHealthBarWidget::UCrystalHealthBarWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
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

	if (!bBuilt)
	{
		EnsureBuilt();
	}

	RefreshFromCrystal();
}

void UCrystalHealthBarWidget::EnsureBuilt()
{
	if (bBuilt)
	{
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	if (!WidgetTree->RootWidget || !HealthBar)
	{
		BuildDefaultUI();
	}

	bBuilt = HealthBar != nullptr && ValueLabel != nullptr;
	if (bBuilt)
	{
		ApplyHitTestPolicy();
	}
}

void UCrystalHealthBarWidget::ApplyHitTestPolicy()
{
	// Full-screen root must not eat world clicks; chrome is display-only.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (UWidget* Root = WidgetTree ? WidgetTree->RootWidget : nullptr)
	{
		Root->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (BarChrome)
	{
		BarChrome->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
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

	if (!BarChrome)
	{
		BarChrome = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CrystalHealthChrome"));
		BarChrome->SetPadding(FMargin(16.f, 10.f));
		BarChrome->SetBrushColor(CrystalHealthBarPrivate::ChromeBg);
		if (UCanvasPanelSlot* ChromeSlot = Root->AddChildToCanvas(BarChrome))
		{
			ChromeSlot->SetAnchors(FAnchors(0.5f, 0.f, 0.5f, 0.f));
			ChromeSlot->SetAlignment(FVector2D(0.5f, 0.f));
			ChromeSlot->SetAutoSize(true);
			ChromeSlot->SetOffsets(FMargin(0.f, TopPad, 0.f, 0.f));
			ChromeSlot->SetZOrder(10);
		}
	}
	else if (UCanvasPanelSlot* ChromeSlot = Cast<UCanvasPanelSlot>(BarChrome->Slot))
	{
		ChromeSlot->SetAnchors(FAnchors(0.5f, 0.f, 0.5f, 0.f));
		ChromeSlot->SetAlignment(FVector2D(0.5f, 0.f));
		ChromeSlot->SetAutoSize(true);
		ChromeSlot->SetOffsets(FMargin(0.f, TopPad, 0.f, 0.f));
	}

	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CrystalHealthColumn"));
	BarChrome->SetContent(Column);

	TitleLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CrystalTitle"));
	TitleLabel->SetText(FText::FromString(TEXT("CRYSTAL")));
	TitleLabel->SetJustification(ETextJustify::Center);
	TitleLabel->SetColorAndOpacity(FSlateColor(CrystalHealthBarPrivate::TitleColor));
	TitleLabel->SetShadowOffset(FVector2D(1.f, 1.f));
	TitleLabel->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
	{
		FSlateFontInfo Font = TitleLabel->GetFont();
		Font.Size = 14.f;
		Font.TypefaceFontName = TEXT("Bold");
		TitleLabel->SetFont(Font);
	}
	if (UVerticalBoxSlot* TitleSlot = Column->AddChildToVerticalBox(TitleLabel))
	{
		TitleSlot->SetHorizontalAlignment(HAlign_Center);
		TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	UBorder* Frame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CrystalHealthFrame"));
	Frame->SetPadding(FMargin(2.f));
	Frame->SetBrushColor(CrystalHealthBarPrivate::FrameColor);

	BarSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CrystalHealthSize"));
	BarSizeBox->SetWidthOverride(BarWidth);
	BarSizeBox->SetHeightOverride(BarHeight);
	Frame->SetContent(BarSizeBox);

	UOverlay* TrackOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("CrystalHealthOverlay"));
	BarSizeBox->SetContent(TrackOverlay);

	UBorder* Track = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CrystalHealthTrack"));
	Track->SetBrushColor(CrystalHealthBarPrivate::TrackBg);
	Track->SetPadding(FMargin(0.f));
	if (UOverlaySlot* TrackSlot = TrackOverlay->AddChildToOverlay(Track))
	{
		TrackSlot->SetHorizontalAlignment(HAlign_Fill);
		TrackSlot->SetVerticalAlignment(VAlign_Fill);
	}

	HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("CrystalHealthBar"));
	HealthBar->SetPercent(1.f);
	HealthBar->SetFillColorAndOpacity(CrystalHealthBarPrivate::FillHealthy);
	HealthBar->SetBarFillType(EProgressBarFillType::LeftToRight);
	HealthBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UOverlaySlot* BarSlot = TrackOverlay->AddChildToOverlay(HealthBar))
	{
		BarSlot->SetHorizontalAlignment(HAlign_Fill);
		BarSlot->SetVerticalAlignment(VAlign_Fill);
	}

	ValueLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CrystalHealthValue"));
	ValueLabel->SetText(FText::FromString(TEXT("100 / 100")));
	ValueLabel->SetJustification(ETextJustify::Center);
	ValueLabel->SetColorAndOpacity(FSlateColor(CrystalHealthBarPrivate::ValueColor));
	ValueLabel->SetShadowOffset(FVector2D(1.f, 1.f));
	ValueLabel->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.9f));
	ValueLabel->SetVisibility(ESlateVisibility::HitTestInvisible);
	{
		FSlateFontInfo Font = ValueLabel->GetFont();
		Font.Size = 12.f;
		Font.TypefaceFontName = TEXT("Bold");
		ValueLabel->SetFont(Font);
	}
	if (UOverlaySlot* ValueSlot = TrackOverlay->AddChildToOverlay(ValueLabel))
	{
		ValueSlot->SetHorizontalAlignment(HAlign_Fill);
		ValueSlot->SetVerticalAlignment(VAlign_Center);
	}

	if (UVerticalBoxSlot* FrameSlot = Column->AddChildToVerticalBox(Frame))
	{
		FrameSlot->SetHorizontalAlignment(HAlign_Center);
	}

	ApplyHitTestPolicy();
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
