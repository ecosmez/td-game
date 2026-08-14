#include "ChampionFrameWidget.h"

#include "MobaPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/Char.h"
#include "Styling/SlateBrush.h"
#include "UObject/UnrealType.h"

namespace ChampionFramePrivate
{
	static FLinearColor ChromeBg(0.03f, 0.04f, 0.06f, 0.94f);
	static FLinearColor ChromeOutline(0.45f, 0.78f, 0.92f, 0.90f);
	static FLinearColor AvatarFill(0.08f, 0.14f, 0.20f, 1.f);
	static FLinearColor TrackBg(0.05f, 0.07f, 0.10f, 0.98f);
	static FLinearColor FillHealthy(0.22f, 0.82f, 0.38f, 1.f);
	static FLinearColor FillHurt(0.95f, 0.62f, 0.18f, 1.f);
	static FLinearColor FillCritical(0.92f, 0.22f, 0.22f, 1.f);
	static FLinearColor NameColor(0.95f, 0.97f, 1.f, 1.f);
	static FLinearColor ValueColor(1.f, 1.f, 1.f, 1.f);
	static FLinearColor LetterColor(0.55f, 0.88f, 1.f, 1.f);
	static FLinearColor LevelFill(0.06f, 0.10f, 0.16f, 0.96f);
	static FLinearColor LevelOutline(0.95f, 0.82f, 0.28f, 1.f);
	static FLinearColor LevelText(1.f, 0.92f, 0.45f, 1.f);
}

UChampionFrameWidget::UChampionFrameWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
	bHasScriptImplementedTick = true;
}

void UChampionFrameWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureBuilt();
}

void UChampionFrameWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureBuilt();
	ApplyHitTestPolicy();
}

void UChampionFrameWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bBuilt)
	{
		EnsureBuilt();
	}

	RefreshFromChampion();
}

void UChampionFrameWidget::EnsureBuilt()
{
	if (bBuilt && FrameChrome && HealthBar && AvatarFrame)
	{
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	if (!WidgetTree->RootWidget || !FrameChrome || !HealthBar)
	{
		BuildDefaultUI();
	}

	bBuilt = FrameChrome != nullptr && HealthBar != nullptr && AvatarFrame != nullptr && HealthValue != nullptr;
	if (bBuilt)
	{
		ApplyHitTestPolicy();
	}
}

void UChampionFrameWidget::ApplyHitTestPolicy()
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (RootCanvas)
	{
		RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (FrameChrome)
	{
		FrameChrome->SetVisibility(ESlateVisibility::Visible);
	}
}

void UChampionFrameWidget::ApplyRoundedBrush(UBorder* Border, const FLinearColor& Fill, const FLinearColor& Outline,
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
		Brush.OutlineSettings.CornerRadii = FVector4(10.f, 10.f, 10.f, 10.f);
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
	}
	Border->SetBrush(Brush);
}

void UChampionFrameWidget::BuildDefaultUI()
{
	if (!WidgetTree)
	{
		return;
	}

	RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("ChampionFrameRoot"));
		WidgetTree->RootWidget = RootCanvas;
	}
	else
	{
		RootCanvas->ClearChildren();
	}

	FrameChrome = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ChampionFrameChrome"));
	FrameChrome->SetPadding(FMargin(10.f, 8.f, 14.f, 8.f));
	ApplyRoundedBrush(FrameChrome, ChampionFramePrivate::ChromeBg, ChampionFramePrivate::ChromeOutline, 1.8f, false);
	if (UCanvasPanelSlot* ChromeSlot = RootCanvas->AddChildToCanvas(FrameChrome))
	{
		ChromeSlot->SetAnchors(FAnchors(0.f, 1.f, 0.f, 1.f));
		ChromeSlot->SetAlignment(FVector2D(0.f, 1.f));
		ChromeSlot->SetAutoSize(true);
		ChromeSlot->SetOffsets(FMargin(ScreenMargin.X, 0.f, 0.f, ScreenMargin.Y));
		ChromeSlot->SetZOrder(10);
	}

	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ChampionFrameRow"));
	FrameChrome->SetContent(Row);

	AvatarSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ChampionAvatarSize"));
	AvatarSizeBox->SetWidthOverride(AvatarSize);
	AvatarSizeBox->SetHeightOverride(AvatarSize);
	if (UHorizontalBoxSlot* AvatarSlot = Row->AddChildToHorizontalBox(AvatarSizeBox))
	{
		AvatarSlot->SetVerticalAlignment(VAlign_Center);
		AvatarSlot->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
	}

	UOverlay* AvatarOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ChampionAvatarOverlay"));
	AvatarSizeBox->SetContent(AvatarOverlay);

	AvatarFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ChampionAvatarFrame"));
	AvatarFrame->SetPadding(FMargin(4.f));
	ApplyRoundedBrush(AvatarFrame, ChampionFramePrivate::AvatarFill, ChampionFramePrivate::ChromeOutline, 2.4f, true);
	if (UOverlaySlot* FrameSlot = AvatarOverlay->AddChildToOverlay(AvatarFrame))
	{
		FrameSlot->SetHorizontalAlignment(HAlign_Fill);
		FrameSlot->SetVerticalAlignment(VAlign_Fill);
	}

	UOverlay* PortraitOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ChampionPortraitOverlay"));
	AvatarFrame->SetContent(PortraitOverlay);

	AvatarImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("ChampionAvatarImage"));
	AvatarImage->SetVisibility(ESlateVisibility::Collapsed);
	if (UOverlaySlot* ImageSlot = PortraitOverlay->AddChildToOverlay(AvatarImage))
	{
		ImageSlot->SetHorizontalAlignment(HAlign_Fill);
		ImageSlot->SetVerticalAlignment(VAlign_Fill);
	}

	AvatarLetter = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ChampionAvatarLetter"));
	AvatarLetter->SetText(FText::FromString(TEXT("C")));
	AvatarLetter->SetJustification(ETextJustify::Center);
	AvatarLetter->SetColorAndOpacity(FSlateColor(ChampionFramePrivate::LetterColor));
	AvatarLetter->SetShadowOffset(FVector2D(1.f, 1.f));
	AvatarLetter->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
	{
		FSlateFontInfo Font = AvatarLetter->GetFont();
		Font.Size = 36.f;
		Font.TypefaceFontName = TEXT("Bold");
		AvatarLetter->SetFont(Font);
	}
	if (UOverlaySlot* LetterSlot = PortraitOverlay->AddChildToOverlay(AvatarLetter))
	{
		LetterSlot->SetHorizontalAlignment(HAlign_Center);
		LetterSlot->SetVerticalAlignment(VAlign_Center);
	}

	const float LevelSize = 26.f;
	USizeBox* LevelSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ChampionLevelSize"));
	LevelSizeBox->SetWidthOverride(LevelSize);
	LevelSizeBox->SetHeightOverride(LevelSize);
	if (UOverlaySlot* LevelOuterSlot = AvatarOverlay->AddChildToOverlay(LevelSizeBox))
	{
		LevelOuterSlot->SetHorizontalAlignment(HAlign_Right);
		LevelOuterSlot->SetVerticalAlignment(VAlign_Bottom);
		LevelOuterSlot->SetPadding(FMargin(0.f, 0.f, -2.f, -2.f));
	}

	LevelFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ChampionLevelFrame"));
	LevelFrame->SetPadding(FMargin(0.f));
	ApplyRoundedBrush(LevelFrame, ChampionFramePrivate::LevelFill, ChampionFramePrivate::LevelOutline, 1.6f, true);
	LevelSizeBox->SetContent(LevelFrame);

	UOverlay* LevelOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ChampionLevelOverlay"));
	LevelFrame->SetContent(LevelOverlay);

	LevelLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ChampionLevelLabel"));
	LevelLabel->SetText(FText::FromString(TEXT("1")));
	LevelLabel->SetJustification(ETextJustify::Center);
	LevelLabel->SetColorAndOpacity(FSlateColor(ChampionFramePrivate::LevelText));
	{
		FSlateFontInfo Font = LevelLabel->GetFont();
		Font.Size = 12.f;
		Font.TypefaceFontName = TEXT("Bold");
		LevelLabel->SetFont(Font);
	}
	if (UOverlaySlot* LevelTextSlot = LevelOverlay->AddChildToOverlay(LevelLabel))
	{
		LevelTextSlot->SetHorizontalAlignment(HAlign_Center);
		LevelTextSlot->SetVerticalAlignment(VAlign_Center);
	}

	UVerticalBox* Stats = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ChampionStats"));
	if (UHorizontalBoxSlot* StatsSlot = Row->AddChildToHorizontalBox(Stats))
	{
		StatsSlot->SetVerticalAlignment(VAlign_Center);
		StatsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	}

	NameLabel = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ChampionName"));
	NameLabel->SetText(FText::FromString(TEXT("CHAMPION")));
	NameLabel->SetColorAndOpacity(FSlateColor(ChampionFramePrivate::NameColor));
	NameLabel->SetShadowOffset(FVector2D(1.f, 1.f));
	NameLabel->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.85f));
	{
		FSlateFontInfo Font = NameLabel->GetFont();
		Font.Size = 14.f;
		Font.TypefaceFontName = TEXT("Bold");
		NameLabel->SetFont(Font);
	}
	if (UVerticalBoxSlot* NameSlot = Stats->AddChildToVerticalBox(NameLabel))
	{
		NameSlot->SetHorizontalAlignment(HAlign_Left);
		NameSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
	}

	UBorder* HpFrame = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ChampionHpFrame"));
	HpFrame->SetPadding(FMargin(2.f));
	ApplyRoundedBrush(HpFrame, ChampionFramePrivate::TrackBg, ChampionFramePrivate::ChromeOutline, 1.2f, false);
	if (UVerticalBoxSlot* HpSlot = Stats->AddChildToVerticalBox(HpFrame))
	{
		HpSlot->SetHorizontalAlignment(HAlign_Left);
	}

	BarSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ChampionHpSize"));
	BarSizeBox->SetWidthOverride(BarWidth);
	BarSizeBox->SetHeightOverride(BarHeight);
	HpFrame->SetContent(BarSizeBox);

	UOverlay* TrackOverlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ChampionHpOverlay"));
	BarSizeBox->SetContent(TrackOverlay);

	UBorder* Track = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ChampionHpTrack"));
	Track->SetPadding(FMargin(0.f));
	ApplyRoundedBrush(Track, ChampionFramePrivate::TrackBg, FLinearColor(0.f, 0.f, 0.f, 0.f), 0.f, false);
	if (UOverlaySlot* TrackSlot = TrackOverlay->AddChildToOverlay(Track))
	{
		TrackSlot->SetHorizontalAlignment(HAlign_Fill);
		TrackSlot->SetVerticalAlignment(VAlign_Fill);
	}

	HealthBar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ChampionHpBar"));
	HealthBar->SetPercent(1.f);
	HealthBar->SetFillColorAndOpacity(ChampionFramePrivate::FillHealthy);
	HealthBar->SetBarFillType(EProgressBarFillType::LeftToRight);
	HealthBar->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UOverlaySlot* BarSlot = TrackOverlay->AddChildToOverlay(HealthBar))
	{
		BarSlot->SetHorizontalAlignment(HAlign_Fill);
		BarSlot->SetVerticalAlignment(VAlign_Fill);
	}

	HealthValue = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ChampionHpValue"));
	HealthValue->SetText(FText::FromString(TEXT("— / —")));
	HealthValue->SetJustification(ETextJustify::Center);
	HealthValue->SetColorAndOpacity(FSlateColor(ChampionFramePrivate::ValueColor));
	HealthValue->SetShadowOffset(FVector2D(1.f, 1.f));
	HealthValue->SetShadowColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, 0.80f));
	{
		FSlateFontInfo Font = HealthValue->GetFont();
		Font.Size = 12.f;
		Font.TypefaceFontName = TEXT("Bold");
		HealthValue->SetFont(Font);
	}
	if (UOverlaySlot* ValueSlot = TrackOverlay->AddChildToOverlay(HealthValue))
	{
		ValueSlot->SetHorizontalAlignment(HAlign_Center);
		ValueSlot->SetVerticalAlignment(VAlign_Center);
	}

	ApplyHitTestPolicy();
}

void UChampionFrameWidget::ApplyPortrait(UTexture2D* Texture)
{
	if (!AvatarImage || !AvatarLetter)
	{
		return;
	}

	if (!Texture)
	{
		AvatarImage->SetVisibility(ESlateVisibility::Collapsed);
		AvatarLetter->SetVisibility(ESlateVisibility::HitTestInvisible);
		return;
	}

	FSlateBrush Brush;
	Brush.SetResourceObject(Texture);
	Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
	Brush.ImageSize = FVector2D(AvatarSize, AvatarSize);
	Brush.TintColor = FSlateColor(FLinearColor::White);
	Brush.OutlineSettings.CornerRadii = FVector4(1.f, 1.f, 1.f, 1.f);
	Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::HalfHeightRadius;
	Brush.OutlineSettings.Width = 0.f;
	AvatarImage->SetBrush(Brush);
	AvatarImage->SetColorAndOpacity(FLinearColor::White);
	AvatarImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	AvatarLetter->SetVisibility(ESlateVisibility::Collapsed);
}

void UChampionFrameWidget::RefreshFromChampion()
{
	if (!bBuilt || !HealthBar || !HealthValue)
	{
		return;
	}

	if (AvatarSizeBox)
	{
		AvatarSizeBox->SetWidthOverride(AvatarSize);
		AvatarSizeBox->SetHeightOverride(AvatarSize);
	}
	if (BarSizeBox)
	{
		BarSizeBox->SetWidthOverride(BarWidth);
		BarSizeBox->SetHeightOverride(BarHeight);
	}

	APawn* Champion = ResolveChampionPawn();
	if (!Champion)
	{
		HealthBar->SetPercent(0.f);
		HealthBar->SetFillColorAndOpacity(ChampionFramePrivate::FillCritical);
		HealthValue->SetText(FText::FromString(TEXT("— / —")));
		return;
	}

	if (NameLabel)
	{
		NameLabel->SetText(FText::FromString(ResolveChampionName(Champion).ToUpper()));
	}

	const FString DisplayName = ResolveChampionName(Champion);
	if (AvatarLetter)
	{
		TCHAR Letter = TEXT('C');
		for (int32 i = 0; i < DisplayName.Len(); ++i)
		{
			if (FChar::IsAlpha(DisplayName[i]))
			{
				Letter = FChar::ToUpper(DisplayName[i]);
				break;
			}
		}
		AvatarLetter->SetText(FText::FromString(FString(1, &Letter)));
	}

	int32 Level = 1;
	ReadIntProp(Champion, FName(TEXT("ChampionLevel")), Level);
	if (LevelLabel)
	{
		LevelLabel->SetText(FText::AsNumber(FMath::Max(Level, 1)));
	}

	UTexture2D* Portrait = ResolvePortraitTexture(Champion);
	if (Portrait != CachedPortrait.Get())
	{
		CachedPortrait = Portrait;
		ApplyPortrait(Portrait);
	}

	float Current = 0.f;
	float Max = 0.f;
	if (!TryReadHealth(Champion, Current, Max) || Max <= 0.f)
	{
		HealthBar->SetPercent(0.f);
		HealthBar->SetFillColorAndOpacity(ChampionFramePrivate::FillCritical);
		HealthValue->SetText(FText::FromString(TEXT("— / —")));
		return;
	}

	Current = FMath::Clamp(Current, 0.f, Max);
	const float Percent = Current / Max;
	HealthBar->SetPercent(Percent);

	FLinearColor Fill = ChampionFramePrivate::FillHealthy;
	if (Percent <= 0.25f)
	{
		Fill = ChampionFramePrivate::FillCritical;
	}
	else if (Percent <= 0.55f)
	{
		Fill = ChampionFramePrivate::FillHurt;
	}
	HealthBar->SetFillColorAndOpacity(Fill);

	HealthValue->SetText(FText::FromString(FString::Printf(TEXT("%d / %d"),
		FMath::RoundToInt(Current), FMath::RoundToInt(Max))));
}

APawn* UChampionFrameWidget::ResolveChampionPawn() const
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return GetOwningPlayerPawn();
	}

	if (const AMobaPlayerController* MobaPC = Cast<AMobaPlayerController>(PC))
	{
		if (APawn* Champion = MobaPC->GetControlledChampion())
		{
			return Champion;
		}
	}

	if (APawn* Possessed = PC->GetPawn())
	{
		float Current = 0.f;
		float Max = 0.f;
		float CdQ = 0.f;
		int32 Level = 0;
		if (TryReadHealth(Possessed, Current, Max)
			|| ReadIntProp(Possessed, FName(TEXT("ChampionLevel")), Level)
			|| ReadFloatProp(Possessed, FName(TEXT("CD_Q")), CdQ))
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
			float CdQ = 0.f;
			if (ReadFloatProp(Candidate, FName(TEXT("CD_Q")), CdQ))
			{
				return Candidate;
			}
		}
	}

	return PC->GetPawn();
}

bool UChampionFrameWidget::TryReadHealth(const UObject* Obj, float& OutCurrent, float& OutMax) const
{
	if (!Obj)
	{
		return false;
	}

	static const FName CurrentNames[] = {
		TEXT("CurrentHealth"), TEXT("Health"), TEXT("CurrentHP"), TEXT("HP")
	};
	static const FName MaxNames[] = {
		TEXT("MaxHealth"), TEXT("MaximumHealth"), TEXT("MaxHP")
	};

	for (const FName CurrentName : CurrentNames)
	{
		for (const FName MaxName : MaxNames)
		{
			float Current = 0.f;
			float Max = 0.f;
			const bool bFloat = ReadFloatProp(Obj, CurrentName, Current) && ReadFloatProp(Obj, MaxName, Max);
			if (bFloat && Max > 0.f)
			{
				OutCurrent = Current;
				OutMax = Max;
				return true;
			}

			int32 CurrentInt = 0;
			int32 MaxInt = 0;
			if (ReadIntProp(Obj, CurrentName, CurrentInt) && ReadIntProp(Obj, MaxName, MaxInt) && MaxInt > 0)
			{
				OutCurrent = static_cast<float>(CurrentInt);
				OutMax = static_cast<float>(MaxInt);
				return true;
			}
		}
	}
	return false;
}

UTexture2D* UChampionFrameWidget::ResolvePortraitTexture(const UObject* Obj) const
{
	if (Obj)
	{
		static const FName PortraitNames[] = {
			TEXT("Portrait"), TEXT("Avatar"), TEXT("ChampionPortrait"), TEXT("ChampionIcon")
		};
		for (const FName Name : PortraitNames)
		{
			if (UTexture2D* Tex = ReadTextureProp(Obj, Name))
			{
				return Tex;
			}
		}
	}

	if (PortraitTexturePath.IsValid())
	{
		return Cast<UTexture2D>(PortraitTexturePath.TryLoad());
	}
	return nullptr;
}

FString UChampionFrameWidget::ResolveChampionName(const APawn* Pawn) const
{
	if (!Pawn)
	{
		return TEXT("Champion");
	}

	FString Named;
	if (ReadStringProp(Pawn, FName(TEXT("ChampionName")), Named) && !Named.IsEmpty())
	{
		return Named;
	}
	if (ReadStringProp(Pawn, FName(TEXT("DisplayName")), Named) && !Named.IsEmpty())
	{
		return Named;
	}

	return TEXT("Champion");
}

bool UChampionFrameWidget::IsScreenPosOverFrame(FVector2D ScreenPos) const
{
	if (!FrameChrome || !IsInViewport())
	{
		return false;
	}

	const FGeometry& Geo = FrameChrome->GetCachedGeometry();
	const FVector2D Size = Geo.GetLocalSize();
	if (Size.X <= KINDA_SMALL_NUMBER || Size.Y <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector2D Local = Geo.AbsoluteToLocal(ScreenPos);
	return Local.X >= 0.f && Local.Y >= 0.f && Local.X <= Size.X && Local.Y <= Size.Y;
}

bool UChampionFrameWidget::ReadFloatProp(const UObject* Obj, FName Name, float& OutValue)
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

bool UChampionFrameWidget::ReadIntProp(const UObject* Obj, FName Name, int32& OutValue)
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

bool UChampionFrameWidget::ReadStringProp(const UObject* Obj, FName Name, FString& OutValue)
{
	if (!Obj)
	{
		return false;
	}
	if (const FStrProperty* Prop = FindFProperty<FStrProperty>(Obj->GetClass(), Name))
	{
		OutValue = Prop->GetPropertyValue_InContainer(Obj);
		return true;
	}
	if (const FNameProperty* NameProp = FindFProperty<FNameProperty>(Obj->GetClass(), Name))
	{
		OutValue = NameProp->GetPropertyValue_InContainer(Obj).ToString();
		return true;
	}
	if (const FTextProperty* TextProp = FindFProperty<FTextProperty>(Obj->GetClass(), Name))
	{
		OutValue = TextProp->GetPropertyValue_InContainer(Obj).ToString();
		return true;
	}
	return false;
}

UTexture2D* UChampionFrameWidget::ReadTextureProp(const UObject* Obj, FName Name)
{
	if (!Obj)
	{
		return nullptr;
	}
	if (const FObjectProperty* Prop = FindFProperty<FObjectProperty>(Obj->GetClass(), Name))
	{
		if (Prop->PropertyClass && Prop->PropertyClass->IsChildOf(UTexture2D::StaticClass()))
		{
			return Cast<UTexture2D>(Prop->GetObjectPropertyValue_InContainer(Obj));
		}
	}
	if (const FSoftObjectProperty* SoftProp = FindFProperty<FSoftObjectProperty>(Obj->GetClass(), Name))
	{
		if (SoftProp->PropertyClass && SoftProp->PropertyClass->IsChildOf(UTexture2D::StaticClass()))
		{
			const FSoftObjectPtr SoftPtr = SoftProp->GetPropertyValue_InContainer(Obj);
			return Cast<UTexture2D>(SoftPtr.LoadSynchronous());
		}
	}
	return nullptr;
}
