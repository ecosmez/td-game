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

const TCHAR* UChampionFrameWidget::GetWidgetBlueprintPath()
{
	return TEXT("/Game/TD/UI/WBP_ChampionFrame.WBP_ChampionFrame_C");
}

TSubclassOf<UChampionFrameWidget> UChampionFrameWidget::ResolveWidgetClass()
{
	return LoadClass<UChampionFrameWidget>(nullptr, GetWidgetBlueprintPath());
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

	BindDesignerWidgets();

	bBuilt = FrameChrome != nullptr && HealthBar != nullptr && AvatarFrame != nullptr && HealthValue != nullptr;
	if (bBuilt)
	{
		ApplyHitTestPolicy();
	}
}

void UChampionFrameWidget::BindDesignerWidgets()
{
	RootCanvas = Cast<UCanvasPanel>(GetWidgetFromName(TEXT("ChampionFrameRoot")));
	if (!RootCanvas)
	{
		RootCanvas = Cast<UCanvasPanel>(WidgetTree ? WidgetTree->RootWidget : nullptr);
	}
	FrameChrome = Cast<UBorder>(GetWidgetFromName(TEXT("ChampionFrameChrome")));
	AvatarSizeBox = Cast<USizeBox>(GetWidgetFromName(TEXT("ChampionAvatarSize")));
	AvatarFrame = Cast<UBorder>(GetWidgetFromName(TEXT("ChampionAvatarFrame")));
	AvatarImage = Cast<UImage>(GetWidgetFromName(TEXT("ChampionAvatarImage")));
	AvatarLetter = Cast<UTextBlock>(GetWidgetFromName(TEXT("ChampionAvatarLetter")));
	LevelFrame = Cast<UBorder>(GetWidgetFromName(TEXT("ChampionLevelFrame")));
	LevelLabel = Cast<UTextBlock>(GetWidgetFromName(TEXT("ChampionLevelLabel")));
	NameLabel = Cast<UTextBlock>(GetWidgetFromName(TEXT("ChampionName")));
	BarSizeBox = Cast<USizeBox>(GetWidgetFromName(TEXT("ChampionHpSize")));
	HealthBar = Cast<UProgressBar>(GetWidgetFromName(TEXT("ChampionHpBar")));
	HealthValue = Cast<UTextBlock>(GetWidgetFromName(TEXT("ChampionHpValue")));
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
		HealthValue->SetText(FText::FromString(TEXT("â€” / â€”")));
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
		HealthValue->SetText(FText::FromString(TEXT("â€” / â€”")));
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

FVector2D UChampionFrameWidget::GetChromeScreenSize() const
{
	if (FrameChrome)
	{
		const FVector2D Size = FrameChrome->GetCachedGeometry().GetLocalSize();
		if (Size.X > 1.f && Size.Y > 1.f)
		{
			return Size;
		}
	}

	// Matches BuildDefaultUI: chrome padding (10,8,14,8) + avatar + gap + HP bar.
	const float Width = 10.f + AvatarSize + 12.f + BarWidth + 14.f;
	const float Height = 8.f + FMath::Max(AvatarSize, BarHeight + 40.f) + 8.f;
	return FVector2D(Width, Height);
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
