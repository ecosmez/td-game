#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../TDChampionClickMove.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChampionClickSkipsSelfHitTest,
	"TD.Champion.ClickMove.SkipsChampionMeshHits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChampionClickSkipsSelfHitTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A click on the champion is skipped so the ground behind them can be used"),
		FTDChampionClickMove::ClassifyHit(true, true, false) == ETDChampionClickIntent::SkipHit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChampionClickAttacksEnemyTest,
	"TD.Champion.ClickMove.AttacksEnemyHits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChampionClickAttacksEnemyTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A right-click on an attackable enemy issues an attack"),
		FTDChampionClickMove::ClassifyHit(false, true, true) == ETDChampionClickIntent::Attack);
	TestTrue(TEXT("The same enemy hit is a ground move when attacks are disabled"),
		FTDChampionClickMove::ClassifyHit(false, false, true) == ETDChampionClickIntent::MoveToHit);
	TestTrue(TEXT("A non-enemy world hit is a ground move"),
		FTDChampionClickMove::ClassifyHit(false, true, false) == ETDChampionClickIntent::MoveToHit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChampionClickMoveModeTest,
	"TD.Champion.ClickMove.ChoosesReachableOrDirectMove",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChampionClickMoveModeTest::RunTest(const FString& Parameters)
{
	constexpr float CliffZ = 80.f;
	TestTrue(TEXT("A complete NavMesh path uses path following even toward lower ground"),
		FTDChampionClickMove::ChooseMoveMode(true, true, 200.f, 0.f, CliffZ) == ETDChampionGroundMoveMode::NavMesh);
	TestTrue(TEXT("An unreachable lower click walks off the ledge in XY"),
		FTDChampionClickMove::ChooseMoveMode(false, false, 200.f, 0.f, CliffZ) == ETDChampionGroundMoveMode::DirectXY);
	TestTrue(TEXT("An unreachable same-height click that projected onto NavMesh uses that point"),
		FTDChampionClickMove::ChooseMoveMode(false, true, 100.f, 100.f, CliffZ) == ETDChampionGroundMoveMode::NavMesh);
	TestTrue(TEXT("An unreachable click with no NavMesh projection still steers in XY instead of stalling"),
		FTDChampionClickMove::ChooseMoveMode(false, false, 100.f, 100.f, CliffZ) == ETDChampionGroundMoveMode::DirectXY);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChampionClickDestinationTest,
	"TD.Champion.ClickMove.NavMovesUseProjectedPoint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChampionClickDestinationTest::RunTest(const FString& Parameters)
{
	const FVector Click(100.f, 200.f, 50.f);
	const FVector Projected(110.f, 190.f, 0.f);
	TestEqual(TEXT("Direct cliff/fallback moves keep the raw click location"),
		FTDChampionClickMove::ResolveMoveDestination(
			Click, ETDChampionGroundMoveMode::DirectXY, true, Projected),
		Click);
	TestEqual(TEXT("NavMesh moves walk to the projected walkable point"),
		FTDChampionClickMove::ResolveMoveDestination(
			Click, ETDChampionGroundMoveMode::NavMesh, true, Projected),
		Projected);
	TestEqual(TEXT("NavMesh moves without a projection keep the click location"),
		FTDChampionClickMove::ResolveMoveDestination(
			Click, ETDChampionGroundMoveMode::NavMesh, false, Projected),
		Click);
	return true;
}

#endif
