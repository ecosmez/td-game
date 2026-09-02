#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../TDMinimapProjection.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMinimapProjectionCenterStaysCenteredTest,
	"TD.Minimap.Projection.CenterStaysCentered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinimapProjectionCenterStaysCenteredTest::RunTest(const FString& Parameters)
{
	const FVector2D UV = FTDMinimapProjection::WorldToNormalized(
		FVector(100.f, -50.f, 0.f), 100.f, -50.f, 1000.f, 45.f);
	TestEqual(TEXT("Map center projects to the minimap middle"), UV, FVector2D(0.5f, 0.5f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMinimapProjectionZeroYawMatchesLegacyAxesTest,
	"TD.Minimap.Projection.ZeroYawMatchesLegacyAxes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinimapProjectionZeroYawMatchesLegacyAxesTest::RunTest(const FString& Parameters)
{
	const FVector2D UV = FTDMinimapProjection::WorldToNormalized(
		FVector(100.f, 0.f, 0.f), 0.f, 0.f, 400.f, 0.f);
	TestEqual(TEXT("World +X still maps left on the unrotated capture"), UV, FVector2D(0.25f, 0.5f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMinimapProjectionFortyFiveYawRotatesPlusXTest,
	"TD.Minimap.Projection.FortyFiveYawRotatesPlusX",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinimapProjectionFortyFiveYawRotatesPlusXTest::RunTest(const FString& Parameters)
{
	const FVector2D UV = FTDMinimapProjection::WorldToNormalized(
		FVector(100.f, 0.f, 0.f), 0.f, 0.f, 400.f, 45.f);
	const float Expected = 0.5f - (100.f * UE_INV_SQRT_2) / 400.f;
	TestEqual(TEXT("A +X world point sits on the 45-degree diagonal"), UV, FVector2D(Expected, Expected));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMinimapProjectionRoundTripsThroughFortyFiveYawTest,
	"TD.Minimap.Projection.RoundTripsThroughFortyFiveYaw",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinimapProjectionRoundTripsThroughFortyFiveYawTest::RunTest(const FString& Parameters)
{
	const FVector World(1250.f, -800.f, 0.f);
	const FVector2D UV = FTDMinimapProjection::WorldToNormalized(World, 0.f, 0.f, 4000.f, 45.f);
	const FVector2D Back = FTDMinimapProjection::NormalizedToWorldXY(UV, 0.f, 0.f, 4000.f, 45.f);
	TestEqual(TEXT("Minimap clicks reverse the 45-degree capture"), Back, FVector2D(World.X, World.Y));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMinimapProjectionFortyFiveYawFitsRotatedLandscapeTest,
	"TD.Minimap.Projection.FortyFiveYawFitsRotatedLandscape",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinimapProjectionFortyFiveYawFitsRotatedLandscapeTest::RunTest(const FString& Parameters)
{
	float CenterX = 0.f;
	float CenterY = 0.f;
	float Ortho = 0.f;
	FTDMinimapProjection::GetOrthoWorldRect(
		FVector2D(-5000.f, -5000.f), FVector2D(5000.f, 5000.f), 1.f, 45.f, CenterX, CenterY, Ortho);

	TestEqual(TEXT("Rotated square landscape is centered"), FVector2D(CenterX, CenterY), FVector2D(0.f, 0.f));
	TestEqual(TEXT("Ortho grows by sqrt(2) so the diamond fits the frame"), Ortho, 10000.f * UE_SQRT_2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMinimapProjectionFitsLandscapeBoundsWithPaddingTest,
	"TD.Minimap.Projection.FitsLandscapeBoundsWithPadding",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinimapProjectionFitsLandscapeBoundsWithPaddingTest::RunTest(const FString& Parameters)
{
	const FBox Landscape(
		FVector(-15724.f, -10198.f, -3258.f),
		FVector(9476.f, 15002.f, -1514.f));
	const FBox2D Fitted = FTDMinimapProjection::FitLandscapeBounds(Landscape, 800.f);

	TestTrue(TEXT("Landscape XY bounds are valid"), Fitted.bIsValid);
	TestEqual(TEXT("Padding expands min XY"), Fitted.Min, FVector2D(-16524.f, -10998.f));
	TestEqual(TEXT("Padding expands max XY"), Fitted.Max, FVector2D(10276.f, 15802.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMinimapProjectionNinetyYawRotatesPlusXTest,
	"TD.Minimap.Projection.NinetyYawRotatesPlusX",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinimapProjectionNinetyYawRotatesPlusXTest::RunTest(const FString& Parameters)
{
	const FVector2D UV = FTDMinimapProjection::WorldToNormalized(
		FVector(100.f, 0.f, 0.f), 0.f, 0.f, 400.f, 90.f);
	TestEqual(TEXT("A +X world point maps to the top of a 90-degree capture"), UV, FVector2D(0.5f, 0.25f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FMinimapProjectionCaptureYawAddsMapOffsetTest,
	"TD.Minimap.Projection.CaptureYawAddsMapOffset",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMinimapProjectionCaptureYawAddsMapOffsetTest::RunTest(const FString& Parameters)
{
	const FRotator Rot45 = FTDMinimapProjection::MakeCaptureRotation(45.f);
	TestEqual(TEXT("Capture still looks straight down"), Rot45.Pitch, -90.0);
	TestEqual(TEXT("Capture yaw is the legacy -90 plus the 45-degree map offset"), Rot45.Yaw, -45.0);
	const FRotator Rot90 = FTDMinimapProjection::MakeCaptureRotation(90.f);
	TestEqual(TEXT("A further 45 degrees lands at yaw 0"), Rot90.Yaw, 0.0);
	return true;
}

#endif
