#if WITH_DEV_AUTOMATION_TESTS

#include "../CrystalHealthBarWidget.h"
#include "Components/Border.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/VerticalBox.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCrystalHudComparisonLayoutTest,
	"TD.UI.CrystalHealthBar.BuildsCenteredTopBarWithThreatBelow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrystalHudComparisonLayoutTest::RunTest(const FString& Parameters)
{
	UCrystalHealthBarWidget* Widget = NewObject<UCrystalHealthBarWidget>();
	TestNotNull(TEXT("widget can be created"), Widget);
	if (!Widget)
	{
		return false;
	}

	TestTrue(TEXT("widget initializes"), Widget->Initialize());
	Widget->TakeWidget();

	UVerticalBox* TopHudStack = Cast<UVerticalBox>(Widget->GetWidgetFromName(TEXT("TopHudStack")));
	TestNotNull(TEXT("HUD uses one top-centered vertical stack"), TopHudStack);
	if (!TopHudStack)
	{
		return false;
	}

	UCanvasPanelSlot* StackSlot = Cast<UCanvasPanelSlot>(TopHudStack->Slot);
	TestNotNull(TEXT("top HUD stack is placed on the root canvas"), StackSlot);
	if (StackSlot)
	{
		TestEqual(TEXT("top HUD is horizontally centered"), StackSlot->GetAnchors().Minimum.X, 0.5);
		TestEqual(TEXT("top HUD is anchored to the top edge"), StackSlot->GetAnchors().Minimum.Y, 0.0);
		TestEqual(TEXT("top HUD aligns around its center"), StackSlot->GetAlignment().X, 0.5);
	}

	TestEqual(TEXT("stack contains the main bar and threat panel"), TopHudStack->GetChildrenCount(), 2);
	TestTrue(TEXT("main bar is first"),
		TopHudStack->GetChildAt(0) && TopHudStack->GetChildAt(0)->GetFName() == TEXT("TopBarChrome"));
	TestTrue(TEXT("threat panel is directly below"),
		TopHudStack->GetChildAt(1) && TopHudStack->GetChildAt(1)->GetFName() == TEXT("CrystalThreatChrome"));

	UHorizontalBox* TopBarRow = Cast<UHorizontalBox>(Widget->GetWidgetFromName(TEXT("TopBarRow")));
	TestNotNull(TEXT("main HUD content uses one horizontal bar"), TopBarRow);
	if (TopBarRow)
	{
		TestEqual(TEXT("top bar exposes health, wave, enemies, timer, and action sections"),
			TopBarRow->GetChildrenCount(), 5);
	}

	return true;
}

#endif
