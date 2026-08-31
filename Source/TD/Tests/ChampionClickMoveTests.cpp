#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../TDChampionClickMove.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChampionClickContinuesThroughOverlaysTest,
	"TD.Champion.ClickMove.ContinuesThroughOverlays",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChampionClickContinuesThroughOverlaysTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A click on the champion continues to the ground behind it"),
		FTDChampionClickMove::ClassifyHit(true, true, false, false) == ETDChampionClickIntent::ContinueTrace);
	TestTrue(TEXT("A click-through overlay (crystal / fog plane) continues to Landscape"),
		FTDChampionClickMove::ClassifyHit(false, true, false, false, true, true)
			== ETDChampionClickIntent::ContinueTrace);
	TestTrue(TEXT("A mountain occluding the map continues to the walkable ground behind it"),
		FTDChampionClickMove::ClassifyHit(false, true, false, false, true, false, true)
			== ETDChampionClickIntent::ContinueTrace);
	TestTrue(TEXT("Crystal class names are treated as click-through overlays"),
		FTDChampionClickMove::IsClickThroughActorName(TEXT("BP_Crystal_C_0"), TEXT("BP_Crystal_C")));
	TestTrue(TEXT("World fog host actors are treated as click-through overlays"),
		FTDChampionClickMove::IsClickThroughActorName(TEXT("WorldFOW_Host"), TEXT("Actor")));
	TestTrue(TEXT("Ability aim preview is treated as a click-through overlay"),
		FTDChampionClickMove::IsClickThroughActorName(TEXT("BP_AbilityAimPreview_C_0"), TEXT("BP_AbilityAimPreview_C")));
	TestFalse(TEXT("3DKit rocks are solid environment, not click-through overlays"),
		FTDChampionClickMove::IsClickThroughActorName(TEXT("StaticMeshActor_2"), TEXT("StaticMeshActor")));
	TestTrue(TEXT("Placed kit meshes are treated as environment collision"),
		FTDChampionClickMove::IsKitEnvironmentActorName(TEXT("StaticMeshActor_2")));
	TestFalse(TEXT("Sky spheres are not treated as kit rocks"),
		FTDChampionClickMove::IsKitEnvironmentActorName(TEXT("SkySphere")));
	TestFalse(TEXT("Persistent terrain collision must not rebuild navigation when Play begins"),
		FTDChampionClickMove::ShouldConfigureEnvironmentCollisionAtRuntime());
	FTDChampionClickMove::StripActorTraceCollision(nullptr);
	FTDChampionClickMove::UseComplexCollisionOnEnvironmentMesh(nullptr);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChampionClickMovesToWorldGeometryTest,
	"TD.Champion.ClickMove.MovesToWorldGeometry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChampionClickMovesToWorldGeometryTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A Landscape hit issues a ground move"),
		FTDChampionClickMove::ClassifyHit(false, true, false, true) == ETDChampionClickIntent::MoveToHit);
	TestTrue(TEXT("A hex-pad hit issues a move to that impact"),
		FTDChampionClickMove::ClassifyHit(false, true, false, false) == ETDChampionClickIntent::MoveToHit);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChampionClickAttacksEnemyTest,
	"TD.Champion.ClickMove.AttacksEnemyHits",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChampionClickAttacksEnemyTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A right-click on an attackable enemy issues an attack"),
		FTDChampionClickMove::ClassifyHit(false, true, true, false) == ETDChampionClickIntent::Attack);
	TestTrue(TEXT("An enemy is ignored rather than treated as terrain when attacks are disabled"),
		FTDChampionClickMove::ClassifyHit(false, false, true, false) == ETDChampionClickIntent::IgnoreClick);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChampionMoveIndicatorAnimationTest,
	"TD.Champion.ClickMove.IndicatorPulsesAndExpires",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChampionMoveIndicatorAnimationTest::RunTest(const FString& Parameters)
{
	float Scale = 0.0f;
	float Intensity = 0.0f;
	TestTrue(TEXT("A fresh move order shows the destination indicator"),
		FTDChampionClickMove::CalculateMoveIndicatorFrame(0.0f, 0.6f, Scale, Intensity));
	TestEqual(TEXT("The indicator starts at its base size"), Scale, 1.0f);
	TestEqual(TEXT("The indicator starts fully bright"), Intensity, 1.0f);

	TestTrue(TEXT("The indicator remains visible halfway through its pulse"),
		FTDChampionClickMove::CalculateMoveIndicatorFrame(0.3f, 0.6f, Scale, Intensity));
	TestTrue(TEXT("The pulse expands the ring before it disappears"), Scale > 1.0f);
	TestTrue(TEXT("The ring fades as it ages"), Intensity < 1.0f && Intensity > 0.0f);

	TestFalse(TEXT("The destination indicator expires at the configured duration"),
		FTDChampionClickMove::CalculateMoveIndicatorFrame(0.6f, 0.6f, Scale, Intensity));
	return true;
}

#endif
