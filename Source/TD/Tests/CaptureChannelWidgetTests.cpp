#if WITH_DEV_AUTOMATION_TESTS

#include "../CaptureChannelWidget.h"
#include "Components/ProgressBar.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCaptureChannelWidgetClampsProgressAndAppliesFillColorTest,
	"TD.UI.CaptureChannel.ClampsProgressAndAppliesFillColor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaptureChannelWidgetClampsProgressAndAppliesFillColorTest::RunTest(const FString& Parameters)
{
	UCaptureChannelWidget* Widget = NewObject<UCaptureChannelWidget>();
	TestNotNull(TEXT("channel widget can be created"), Widget);
	if (!Widget)
	{
		return false;
	}

	TestTrue(TEXT("widget initializes"), Widget->Initialize());
	Widget->TakeWidget();

	UProgressBar* Bar = Cast<UProgressBar>(Widget->GetWidgetFromName(TEXT("ChannelBar")));
	TestNotNull(TEXT("channel widget builds a progress bar"), Bar);
	if (!Bar)
	{
		return false;
	}

	Widget->SetProgress(0.4f);
	TestEqual(TEXT("progress is applied"), Bar->GetPercent(), 0.4f);

	Widget->SetProgress(1.7f);
	TestEqual(TEXT("progress clamps to 1"), Bar->GetPercent(), 1.f);

	Widget->SetProgress(-0.2f);
	TestEqual(TEXT("progress clamps to 0"), Bar->GetPercent(), 0.f);

	const FLinearColor EnemyFill(0.95f, 0.18f, 0.16f, 1.f);
	Widget->SetFillColor(EnemyFill);
	TestEqual(TEXT("fill color is applied"), Bar->GetFillColorAndOpacity(), EnemyFill);

	return true;
}

#endif
