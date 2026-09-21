#include "ChampionFrameWidget.h"

#include "MobaPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/Image.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/UnrealType.h"

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

	if (IsDesignTime())
	{
		return;
	}

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
	if (IsDesignTime())
	{
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void UChampionFrameWidget::RefreshFromChampion()
{
	if (!bBuilt || !HealthBar || !HealthValue)
	{
		return;
	}

	APawn* Champion = ResolveChampionPawn();
	if (!Champion)
	{
		return;
	}

	int32 Level = 1;
	ReadIntProp(Champion, FName(TEXT("ChampionLevel")), Level);
	if (LevelLabel)
	{
		LevelLabel->SetText(FText::AsNumber(FMath::Max(Level, 1)));
	}

	float Current = 0.f;
	float Max = 0.f;
	if (!TryReadHealth(Champion, Current, Max) || Max <= 0.f)
	{
		return;
	}

	Current = FMath::Clamp(Current, 0.f, Max);
	HealthBar->SetPercent(Current / Max);
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
