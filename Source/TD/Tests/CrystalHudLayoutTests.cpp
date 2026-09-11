#if WITH_DEV_AUTOMATION_TESTS

#include "../CrystalHealthBarWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Layout/Geometry.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCrystalHudComparisonLayoutTest,
	"TD.UI.CrystalHealthBar.BuildsCenteredTopBarWithThreatBelow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrystalHudComparisonLayoutTest::RunTest(const FString& Parameters)
{
	UClass* DesignerClass = UCrystalHealthBarWidget::ResolveWidgetClass();
	TestNotNull(TEXT("crystal HUD loads WBP_CrystalHealthBar"), DesignerClass);
	if (!DesignerClass)
	{
		return false;
	}

	UCrystalHealthBarWidget* Widget = NewObject<UCrystalHealthBarWidget>(GetTransientPackage(), DesignerClass);
	TestNotNull(TEXT("widget can be created from the designer class"), Widget);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCrystalHudPreservesDesignerColorsOnTickTest,
	"TD.UI.CrystalHealthBar.PreservesDesignerColorsOnTick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrystalHudPreservesDesignerColorsOnTickTest::RunTest(const FString& Parameters)
{
	UCrystalHealthBarWidget* Widget = NewObject<UCrystalHealthBarWidget>();
	TestNotNull(TEXT("crystal HUD can be created"), Widget);
	if (!Widget || !Widget->WidgetTree)
	{
		return false;
	}

	UVerticalBox* Root = Widget->WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("CrystalHealthRoot"));
	Widget->WidgetTree->RootWidget = Root;

	auto AddNamed = [Widget, Root](UClass* Class, FName Name) -> UWidget*
	{
		UWidget* Child = Widget->WidgetTree->ConstructWidget<UWidget>(Class, Name);
		Root->AddChild(Child);
		return Child;
	};

	UProgressBar* HealthBar = Cast<UProgressBar>(
		AddNamed(UProgressBar::StaticClass(), TEXT("BaseHealthBar")));
	AddNamed(UTextBlock::StaticClass(), TEXT("BaseHealthValue"));
	AddNamed(UButton::StaticClass(), TEXT("NextWaveButton"));
	AddNamed(UBorder::StaticClass(), TEXT("WaveStripChrome"));
	UBorder* ThreatChrome = Cast<UBorder>(
		AddNamed(UBorder::StaticClass(), TEXT("CrystalThreatChrome")));
	UBorder* NextWaveFrame = Cast<UBorder>(
		AddNamed(UBorder::StaticClass(), TEXT("NextWaveFrame")));
	UTextBlock* Enemies = Cast<UTextBlock>(
		AddNamed(UTextBlock::StaticClass(), TEXT("WaveEnemiesCount")));
	UTextBlock* ThreatImpact = Cast<UTextBlock>(
		AddNamed(UTextBlock::StaticClass(), TEXT("NextWaveCrystalImpact")));
	UTextBlock* ThreatSource = Cast<UTextBlock>(
		AddNamed(UTextBlock::StaticClass(), TEXT("EnemyCrystalAccumulation")));
	UTextBlock* Timer = Cast<UTextBlock>(
		AddNamed(UTextBlock::StaticClass(), TEXT("WaveTimer")));
	AddNamed(UHorizontalBox::StaticClass(), TEXT("WaveDotsBox"));

	TestNotNull(TEXT("health bar exists"), HealthBar);
	TestNotNull(TEXT("threat chrome exists"), ThreatChrome);
	TestNotNull(TEXT("next-wave frame exists"), NextWaveFrame);
	TestNotNull(TEXT("enemies label exists"), Enemies);
	TestNotNull(TEXT("threat impact exists"), ThreatImpact);
	TestNotNull(TEXT("threat source exists"), ThreatSource);
	TestNotNull(TEXT("timer exists"), Timer);
	if (!HealthBar || !ThreatChrome || !NextWaveFrame || !Enemies || !ThreatImpact || !ThreatSource || !Timer)
	{
		return false;
	}

	TestTrue(TEXT("widget initializes"), Widget->Initialize());
	Widget->TakeWidget();

	const FLinearColor Sentinel(0.12f, 0.84f, 0.33f, 1.f);
	HealthBar->SetFillColorAndOpacity(Sentinel);
	ThreatChrome->SetBrushColor(Sentinel);
	NextWaveFrame->SetBrushColor(Sentinel);
	Enemies->SetColorAndOpacity(FSlateColor(Sentinel));
	ThreatImpact->SetColorAndOpacity(FSlateColor(Sentinel));
	ThreatSource->SetColorAndOpacity(FSlateColor(Sentinel));
	Timer->SetColorAndOpacity(FSlateColor(Sentinel));

	Widget->NativeTick(FGeometry(), 0.f);

	TestEqual(TEXT("health fill keeps the designer color"),
		HealthBar->GetFillColorAndOpacity(), Sentinel);
	TestEqual(TEXT("threat chrome keeps the designer tint"),
		ThreatChrome->GetBrushColor(), Sentinel);
	TestEqual(TEXT("next-wave frame keeps the designer tint"),
		NextWaveFrame->GetBrushColor(), Sentinel);
	TestEqual(TEXT("enemies label keeps the designer color"),
		Enemies->GetColorAndOpacity().GetSpecifiedColor(), Sentinel);
	TestEqual(TEXT("threat impact keeps the designer color"),
		ThreatImpact->GetColorAndOpacity().GetSpecifiedColor(), Sentinel);
	TestEqual(TEXT("threat source keeps the designer color"),
		ThreatSource->GetColorAndOpacity().GetSpecifiedColor(), Sentinel);
	TestEqual(TEXT("timer keeps the designer color"),
		Timer->GetColorAndOpacity().GetSpecifiedColor(), Sentinel);

	return true;
}

#endif
