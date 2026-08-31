#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../TDFogVision.h"
#include "../TDChampionClickMove.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFogVisionInsideChampionCircleIsVisibleTest,
	"TD.Fog.Vision.InsideChampionCircleIsVisible",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFogVisionInsideChampionCircleIsVisibleTest::RunTest(const FString& Parameters)
{
	const FTDFogVisionSource Champion{FVector(0.f, 0.f, 100.f), 2500.f};
	const FTDFogVisionSource Sources[] = {Champion};

	TestTrue(TEXT("A point under the champion is in vision"),
		FTDFogVision::IsLocationVisible(FVector(0.f, 0.f, 0.f), Sources));
	TestTrue(TEXT("A point just inside the champion radius is in vision"),
		FTDFogVision::IsLocationVisible(FVector(2499.f, 0.f, 0.f), Sources));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFogVisionOutsideAllCirclesIsHiddenTest,
	"TD.Fog.Vision.OutsideAllCirclesIsHidden",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFogVisionOutsideAllCirclesIsHiddenTest::RunTest(const FString& Parameters)
{
	const FTDFogVisionSource Champion{FVector(0.f, 0.f, 0.f), 2500.f};
	const FTDFogVisionSource Crystal{FVector(0.f, 0.f, 0.f), 8000.f};
	const FTDFogVisionSource Sources[] = {Champion, Crystal};

	TestTrue(TEXT("A point beyond the crystal radius is not in vision"),
		!FTDFogVision::IsLocationVisible(FVector(8001.f, 0.f, 0.f), Sources));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFogVisionEnvironmentBlocksChampionVisionTest,
	"TD.Fog.Vision.EnvironmentBlocksChampionVision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFogVisionEnvironmentBlocksChampionVisionTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("An enemy inside the radius stays hidden behind blocking environment"),
		FTDFogVision::IsChampionLocationVisible(true, true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFogVisionClearLineRevealsChampionTargetTest,
	"TD.Fog.Vision.ClearLineRevealsChampionTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFogVisionClearLineRevealsChampionTargetTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("An enemy inside the radius is visible through a clear line"),
		FTDFogVision::IsChampionLocationVisible(true, false));
	TestFalse(TEXT("A clear line never reveals an enemy beyond the vision radius"),
		FTDFogVision::IsChampionLocationVisible(false, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFogVisionCrystalCoversFarFromChampionTest,
	"TD.Fog.Vision.CrystalCoversFarFromChampion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFogVisionCrystalCoversFarFromChampionTest::RunTest(const FString& Parameters)
{
	const FTDFogVisionSource Champion{FVector(0.f, 0.f, 0.f), 2500.f};
	const FTDFogVisionSource Crystal{FVector(0.f, 0.f, 0.f), 8000.f};
	const FTDFogVisionSource Sources[] = {Champion, Crystal};

	TestTrue(TEXT("A point outside champion radius but inside the crystal radius is visible"),
		FTDFogVision::IsLocationVisible(FVector(6000.f, 0.f, 0.f), Sources));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFogVisionCompositeAlphaClearAndDimTest,
	"TD.Fog.Vision.CompositeAlphaClearAndDim",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFogVisionCompositeAlphaClearAndDimTest::RunTest(const FString& Parameters)
{
	constexpr uint8 DimAlpha = 140;
	TestEqual(TEXT("Inside the hard vision radius the fog is fully clear"),
		FTDFogVision::CompositeFogAlpha(0.f, 1000.f, 0.35f, DimAlpha), static_cast<uint8>(0));
	TestEqual(TEXT("Outside the vision radius the fog is the dim overlay"),
		FTDFogVision::CompositeFogAlpha(2000.f, 1000.f, 0.35f, DimAlpha), DimAlpha);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFogVisionHidesMinionsOutsideVisionTest,
	"TD.Fog.Vision.HidesMinionsOutsideVision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFogVisionHidesMinionsOutsideVisionTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Minions in vision stay visible"),
		!FTDFogVision::ShouldHideEnemy(true));
	TestTrue(TEXT("Minions outside vision are hidden"),
		FTDFogVision::ShouldHideEnemy(false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFogVisionClickSkipsFoggedMinionTest,
	"TD.Fog.Vision.ClickSkipsFoggedMinion",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFogVisionClickSkipsFoggedMinionTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A fogged minion is skipped so the ground behind can be ordered"),
		FTDFogVision::ShouldSkipClickThroughFoggedEnemy(true, false));
	TestTrue(TEXT("A visible minion is not skipped"),
		!FTDFogVision::ShouldSkipClickThroughFoggedEnemy(true, true));
	TestTrue(TEXT("Non-enemies are not treated as fogged minion skips"),
		!FTDFogVision::ShouldSkipClickThroughFoggedEnemy(false, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChampionClickContinuesThroughFoggedEnemyTest,
	"TD.Champion.ClickMove.ContinuesThroughFoggedEnemy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChampionClickContinuesThroughFoggedEnemyTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A fogged minion does not consume the click"),
		FTDChampionClickMove::ClassifyHit(false, true, true, false, false)
			== ETDChampionClickIntent::ContinueTrace);
	TestTrue(TEXT("A visible minion is still an attack"),
		FTDChampionClickMove::ClassifyHit(false, true, true, false, true)
			== ETDChampionClickIntent::Attack);
	return true;
}

#endif
