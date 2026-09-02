#include "CaptureChannelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
#include "Styling/CoreStyle.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateTypes.h"
#include "UObject/UObjectIterator.h"
#include "UObject/UnrealType.h"

namespace CaptureChannelPrivate
{
	const FLinearColor Track(0.05f, 0.07f, 0.10f, 0.92f);

	FSlateBrush MakeSolid(const FLinearColor& Color)
	{
		FSlateBrush Brush;
		if (const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("GenericWhiteBox")))
		{
			Brush = *White;
		}
		Brush.TintColor = FSlateColor(Color);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.Margin = FMargin(0.f);
		return Brush;
	}
}

namespace
{
	const FLinearColor PlayerFill(0.22f, 0.78f, 0.95f, 1.f);
	const FLinearColor EnemyFill(0.95f, 0.18f, 0.16f, 1.f);

	bool ReadFloatProp(const AActor* Actor, FName Name, float& OutValue)
	{
		if (!Actor)
		{
			return false;
		}
		if (const FFloatProperty* Prop = FindFProperty<FFloatProperty>(Actor->GetClass(), Name))
		{
			OutValue = Prop->GetPropertyValue_InContainer(Actor);
			return true;
		}
		return false;
	}

	bool ReadIntProp(const AActor* Actor, FName Name, int32& OutValue)
	{
		if (!Actor)
		{
			return false;
		}
		if (const FIntProperty* Prop = FindFProperty<FIntProperty>(Actor->GetClass(), Name))
		{
			OutValue = Prop->GetPropertyValue_InContainer(Actor);
			return true;
		}
		return false;
	}
}

UCaptureChannelWidget::UCaptureChannelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
	bHasScriptImplementedTick = true;
}

void UCaptureChannelWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureBuilt();
}

void UCaptureChannelWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureBuilt();
}

void UCaptureChannelWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	SyncFromHostActor();
}

AActor* UCaptureChannelWidget::ResolveHostActor()
{
	if (HostActor.IsValid())
	{
		return HostActor.Get();
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TObjectIterator<UWidgetComponent> It; It; ++It)
	{
		UWidgetComponent* Comp = *It;
		if (!Comp || Comp->GetWorld() != World)
		{
			continue;
		}
		if (Comp->GetWidget() == this)
		{
			HostActor = Comp->GetOwner();
			break;
		}
	}
	return HostActor.Get();
}

void UCaptureChannelWidget::SyncFromHostActor()
{
	AActor* OwnerActor = ResolveHostActor();
	if (!OwnerActor)
	{
		return;
	}

	float ContestProgress = 0.f;
	if (!ReadFloatProp(OwnerActor, FName(TEXT("ContestProgress")), ContestProgress))
	{
		return;
	}

	int32 OwnerState = 0;
	ReadIntProp(OwnerActor, FName(TEXT("OwnerState")), OwnerState);

	const float Magnitude = FMath::Abs(ContestProgress);
	const bool bShow = OwnerState == 0 && Magnitude > KINDA_SMALL_NUMBER;
	SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (!bShow)
	{
		return;
	}

	SetProgress(Magnitude);
	SetFillColor(ContestProgress >= 0.f ? PlayerFill : EnemyFill);
}

void UCaptureChannelWidget::EnsureBuilt()
{
	if (bBuilt || !WidgetTree)
	{
		return;
	}

	Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ChannelBar"));
	Bar->SetPercent(0.f);
	Bar->SetBarFillType(EProgressBarFillType::LeftToRight);
	ApplyFillStyle();
	WidgetTree->RootWidget = Bar;
	bBuilt = true;
}

void UCaptureChannelWidget::ApplyFillStyle()
{
	if (!Bar)
	{
		return;
	}

	FProgressBarStyle Style = Bar->GetWidgetStyle();
	Style.BackgroundImage = CaptureChannelPrivate::MakeSolid(CaptureChannelPrivate::Track);
	Style.FillImage = CaptureChannelPrivate::MakeSolid(FillColor);
	Bar->SetWidgetStyle(Style);
	Bar->SetFillColorAndOpacity(FillColor);
}

void UCaptureChannelWidget::SetProgress(float In01)
{
	EnsureBuilt();
	if (Bar)
	{
		Bar->SetPercent(FMath::Clamp(In01, 0.f, 1.f));
	}
}

void UCaptureChannelWidget::SetFillColor(FLinearColor Color)
{
	EnsureBuilt();
	FillColor = Color;
	ApplyFillStyle();
}
