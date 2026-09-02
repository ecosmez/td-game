#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../MobaCameraPawn.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMobaCameraEdgeScrollDirectionTest,
	"TD.Camera.EdgeScroll.BlendsTowardCorners",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMobaCameraEdgeScrollDirectionTest::RunTest(const FString& Parameters)
{
	const FVector2D ViewportSize(1000.f, 800.f);
	constexpr float Threshold = 20.f;

	const FVector2D BottomCenter = AMobaCameraPawn::CalculateEdgeScrollDirection(
		FVector2D(500.f, 800.f), ViewportSize, Threshold);
	TestTrue(TEXT("The middle of the bottom edge pans straight down"),
		BottomCenter.Equals(FVector2D(0.f, -1.f), KINDA_SMALL_NUMBER));

	const FVector2D BottomThreeQuarters = AMobaCameraPawn::CalculateEdgeScrollDirection(
		FVector2D(750.f, 800.f), ViewportSize, Threshold);
	TestTrue(TEXT("Moving halfway from bottom-center to bottom-right adds half-strength right pan"),
		BottomThreeQuarters.Equals(FVector2D(0.5f, -1.f).GetSafeNormal(), KINDA_SMALL_NUMBER));

	const FVector2D BottomRight = AMobaCameraPawn::CalculateEdgeScrollDirection(
		FVector2D(1000.f, 800.f), ViewportSize, Threshold);
	TestTrue(TEXT("The bottom-right corner pans diagonally at normalized speed"),
		BottomRight.Equals(FVector2D(1.f, -1.f).GetSafeNormal(), KINDA_SMALL_NUMBER));

	const FVector2D RightCenter = AMobaCameraPawn::CalculateEdgeScrollDirection(
		FVector2D(1000.f, 400.f), ViewportSize, Threshold);
	TestTrue(TEXT("The middle of the right edge pans straight right"),
		RightCenter.Equals(FVector2D(1.f, 0.f), KINDA_SMALL_NUMBER));

	const FVector2D AwayFromEdges = AMobaCameraPawn::CalculateEdgeScrollDirection(
		FVector2D(500.f, 400.f), ViewportSize, Threshold);
	TestTrue(TEXT("The viewport interior does not edge-scroll"), AwayFromEdges.IsNearlyZero());

	return true;
}

#endif
