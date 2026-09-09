#include "CaptureChannelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/Actor.h"
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
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.TintColor = FSlateColor(Color);
		Brush.Margin = FMargin(0.f);
		Brush.ImageSize = FVector2D(32.f, 32.f);
		return Brush;
	}
}

namespace
{
	const FLinearColor PlayerFill(0.22f, 0.78f, 0.95f, 1.f);
	const FLinearColor EnemyFill(0.95f, 0.18f, 0.16f, 1.f);
	constexpr float DefaultBarWidth = 80.f;
	constexpr float DefaultBarHeight = 10.f;

	FVector2D SizeFromBox(const USizeBox* Box)
	{
		if (!Box)
		{
			return FVector2D::ZeroVector;
		}
		const float W = Box->IsWidthOverride() ? Box->GetWidthOverride() : 0.f;
		const float H = Box->IsHeightOverride() ? Box->GetHeightOverride() : 0.f;
		if (W > 1.f && H > 1.f)
		{
			return FVector2D(W, H);
		}
		return FVector2D::ZeroVector;
	}

	USizeBox* FindBarSizeBox(UWidgetTree* Tree)
	{
		if (!Tree)
		{
			return nullptr;
		}
		USizeBox* Named = Cast<USizeBox>(Tree->FindWidget(FName(TEXT("BarSize"))));
		if (Named)
		{
			return Named;
		}
		USizeBox* First = nullptr;
		Tree->ForEachWidget([&First](UWidget* Widget)
		{
			if (!First)
			{
				First = Cast<USizeBox>(Widget);
			}
		});
		return First;
	}

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
		// Blueprint floats are FDoubleProperty in UE5.
		if (const FDoubleProperty* DProp = FindFProperty<FDoubleProperty>(Actor->GetClass(), Name))
		{
			OutValue = static_cast<float>(DProp->GetPropertyValue_InContainer(Actor));
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
		if (const FByteProperty* ByteProp = FindFProperty<FByteProperty>(Actor->GetClass(), Name))
		{
			OutValue = ByteProp->GetPropertyValue_InContainer(Actor);
			return true;
		}
		if (const FInt64Property* Int64Prop = FindFProperty<FInt64Property>(Actor->GetClass(), Name))
		{
			OutValue = static_cast<int32>(Int64Prop->GetPropertyValue_InContainer(Actor));
			return true;
		}
		return false;
	}

	bool ReadBoolProp(const AActor* Actor, FName Name, bool& OutValue)
	{
		if (!Actor)
		{
			return false;
		}
		if (const FBoolProperty* Prop = FindFProperty<FBoolProperty>(Actor->GetClass(), Name))
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

TSharedRef<SWidget> UCaptureChannelWidget::RebuildWidget()
{
	TSharedRef<SWidget> Result = Super::RebuildWidget();
	EnsureBuilt();
	return Result;
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
	bool bChampionInRange = false;
	bool bEnemyInRange = false;
	if (!ReadBoolProp(OwnerActor, FName(TEXT("bChampionInRange")), bChampionInRange))
	{
		ReadBoolProp(OwnerActor, FName(TEXT("ChampionInRange")), bChampionInRange);
	}
	if (!ReadBoolProp(OwnerActor, FName(TEXT("bEnemyInRange")), bEnemyInRange))
	{
		ReadBoolProp(OwnerActor, FName(TEXT("EnemyInRange")), bEnemyInRange);
	}
	const bool bShow = bChampionInRange || bEnemyInRange;
	SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	SetProgress(Magnitude);
	SetFillColor(ContestProgress >= 0.f ? PlayerFill : EnemyFill);
}

void UCaptureChannelWidget::EnsureBuilt()
{
	if (bBuilt && Bar)
	{
		return;
	}

	if (!WidgetTree)
	{
		return;
	}

	Bar = Cast<UProgressBar>(GetWidgetFromName(TEXT("ChannelBar")));
	if (!Bar)
	{
		Bar = Cast<UProgressBar>(WidgetTree->RootWidget);
	}
	if (!Bar)
	{
		bBuilt = false;
		return;
	}

	bDesignerBar = true;
	ApplyFillStyle();
	bBuilt = true;
}

void UCaptureChannelWidget::ApplyFillStyle()
{
	if (!Bar)
	{
		return;
	}

	Bar->SetFillColorAndOpacity(FillColor);
	if (bDesignerBar)
	{
		return;
	}

	FProgressBarStyle Style = Bar->GetWidgetStyle();
	Style.BackgroundImage = CaptureChannelPrivate::MakeSolid(CaptureChannelPrivate::Track);
	Style.FillImage = CaptureChannelPrivate::MakeSolid(FillColor);
	Bar->SetWidgetStyle(Style);
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

const TCHAR* UCaptureChannelWidget::GetWidgetBlueprintPath()
{
	return TEXT("/Game/TD/UI/WBP_CaptureChannel.WBP_CaptureChannel_C");
}

TSubclassOf<UCaptureChannelWidget> UCaptureChannelWidget::ResolveWidgetClass()
{
	if (UClass* Loaded = LoadClass<UCaptureChannelWidget>(nullptr, GetWidgetBlueprintPath()))
	{
		return Loaded;
	}
	return nullptr;
}

FVector2D UCaptureChannelWidget::GetDesignedDrawSize() const
{
	USizeBox* Box = Cast<USizeBox>(GetWidgetFromName(TEXT("BarSize")));
	if (!Box)
	{
		Box = FindBarSizeBox(WidgetTree);
	}
	const FVector2D FromBox = SizeFromBox(Box);
	if (FromBox.X > 1.f && FromBox.Y > 1.f)
	{
		return FromBox;
	}
	return FVector2D(DefaultBarWidth, DefaultBarHeight);
}

FVector2D UCaptureChannelWidget::GetDefaultDesignedDrawSize()
{
	if (UClass* Class = ResolveWidgetClass())
	{
		if (const UCaptureChannelWidget* CDO = Cast<UCaptureChannelWidget>(Class->GetDefaultObject()))
		{
			const FVector2D FromCDO = CDO->GetDesignedDrawSize();
			if (FromCDO.X > 1.f && FromCDO.Y > 1.f)
			{
				return FromCDO;
			}
		}
	}
	return FVector2D(DefaultBarWidth, DefaultBarHeight);
}

void UCaptureChannelWidget::ApplyDrawSizeToComponent(UWidgetComponent* Comp, UCaptureChannelWidget* Widget)
{
	if (!Comp)
	{
		return;
	}

	// Screen + explicit SizeBox pixels. DrawAtDesiredSize uses the 1280x720
	// designer canvas and turns enemy HP into huge green slabs.
	Comp->SetWidgetSpace(EWidgetSpace::Screen);
	Comp->SetDrawAtDesiredSize(false);
	Comp->SetPivot(FVector2D(0.5f, 1.f));

	FVector2D Size = Widget ? Widget->GetDesignedDrawSize() : GetDefaultDesignedDrawSize();
	if (Size.X >= 640.f || Size.Y >= 180.f)
	{
		Size = FVector2D(DefaultBarWidth, DefaultBarHeight);
	}
	Size.X = FMath::Clamp(Size.X, 8.f, 400.f);
	Size.Y = FMath::Clamp(Size.Y, 4.f, 48.f);
	Comp->SetDrawSize(Size);
}
