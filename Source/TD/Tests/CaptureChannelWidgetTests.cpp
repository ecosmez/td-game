#if WITH_DEV_AUTOMATION_TESTS

#include "../CaptureChannelWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCaptureChannelWidgetClampsProgressAndAppliesFillColorTest,
	"TD.UI.CaptureChannel.ClampsProgressAndAppliesFillColor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaptureChannelWidgetClampsProgressAndAppliesFillColorTest::RunTest(const FString& Parameters)
{
	UCaptureChannelWidget* Widget = NewObject<UCaptureChannelWidget>();
	TestNotNull(TEXT("channel widget can be created"), Widget);
	if (!Widget || !Widget->WidgetTree)
	{
		return false;
	}

	USizeBox* Size = Widget->WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("BarSize"));
	Size->SetWidthOverride(80.f);
	Size->SetHeightOverride(10.f);
	UProgressBar* Bar = Widget->WidgetTree->ConstructWidget<UProgressBar>(
		UProgressBar::StaticClass(), TEXT("ChannelBar"));
	Size->SetContent(Bar);
	Widget->WidgetTree->RootWidget = Size;

	TestTrue(TEXT("widget initializes"), Widget->Initialize());
	Widget->TakeWidget();

	UProgressBar* Bound = Cast<UProgressBar>(Widget->GetWidgetFromName(TEXT("ChannelBar")));
	TestNotNull(TEXT("channel widget binds the designer progress bar"), Bound);
	if (!Bound)
	{
		return false;
	}

	Widget->SetProgress(0.4f);
	TestEqual(TEXT("progress is applied"), Bound->GetPercent(), 0.4f);

	Widget->SetProgress(1.7f);
	TestEqual(TEXT("progress clamps to 1"), Bound->GetPercent(), 1.f);

	Widget->SetProgress(-0.2f);
	TestEqual(TEXT("progress clamps to 0"), Bound->GetPercent(), 0.f);

	const FVector2D Designed = Widget->GetDesignedDrawSize();
	TestTrue(TEXT("designed size has a usable width"), Designed.X > 1.f);
	TestTrue(TEXT("designed size has a usable height"), Designed.Y > 1.f);

	const FLinearColor ExpectedEnemyFill(0.95f, 0.18f, 0.16f, 1.f);
	Widget->SetFillColor(ExpectedEnemyFill);
	TestEqual(TEXT("fill color is applied"), Bound->GetFillColorAndOpacity(), ExpectedEnemyFill);

	return true;
}

#endif
