#include "CaptureChannelWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Styling/SlateTypes.h"

UCaptureChannelWidget::UCaptureChannelWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
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

void UCaptureChannelWidget::EnsureBuilt()
{
	if (bBuilt || !WidgetTree)
	{
		return;
	}

	Bar = WidgetTree->ConstructWidget<UProgressBar>(UProgressBar::StaticClass(), TEXT("ChannelBar"));
	Bar->SetPercent(0.f);
	FProgressBarStyle Style = Bar->GetWidgetStyle();
	Style.BackgroundImage.TintColor = FSlateColor(FLinearColor(0.05f, 0.07f, 0.10f, 0.92f));
	Style.FillImage.TintColor = FSlateColor(FLinearColor(0.22f, 0.78f, 0.95f, 1.f));
	Bar->SetWidgetStyle(Style);
	WidgetTree->RootWidget = Bar;
	bBuilt = true;
}

void UCaptureChannelWidget::SetProgress(float In01)
{
	EnsureBuilt();
	if (Bar)
	{
		Bar->SetPercent(FMath::Clamp(In01, 0.f, 1.f));
	}
}
