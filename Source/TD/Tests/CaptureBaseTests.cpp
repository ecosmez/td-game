#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../TDCaptureBaseLogic.h"

namespace CaptureBaseTestPrivate
{
	FTDCaptureBaseInput MakeIdleInput()
	{
		FTDCaptureBaseInput In;
		In.ChannelDuration = 5.f;
		In.DeltaSeconds = 1.f;
		return In;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCaptureBaseFillsChannelWhenChampionAloneTest,
	"TD.Capture.Base.FillsChannelWhenChampionAlone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaptureBaseFillsChannelWhenChampionAloneTest::RunTest(const FString& Parameters)
{
	FTDCaptureBaseInput In = CaptureBaseTestPrivate::MakeIdleInput();
	In.bChampionInRadius = true;
	In.DeltaSeconds = 2.5f;

	const FTDCaptureBaseOutput Out = FTDCaptureBaseLogic::Step(In);
	TestEqual(TEXT("Half a 5s channel after 2.5s"), Out.ChannelProgress, 0.5f);
	TestTrue(TEXT("Champion in radius with no enemies fills"),
		FTDCaptureBaseLogic::ShouldFillChannel(true, 0, 0, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCaptureBasePausesChannelWhenEnemyContestsTest,
	"TD.Capture.Base.PausesChannelWhenEnemyContests",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaptureBasePausesChannelWhenEnemyContestsTest::RunTest(const FString& Parameters)
{
	FTDCaptureBaseInput In = CaptureBaseTestPrivate::MakeIdleInput();
	In.bChampionInRadius = true;
	In.EnemiesInRadius = 1;
	In.ChannelProgress = 0.4f;
	In.DeltaSeconds = 10.f;

	const FTDCaptureBaseOutput Out = FTDCaptureBaseLogic::Step(In);
	TestEqual(TEXT("Contested channel keeps the same progress"), Out.ChannelProgress, 0.4f);
	TestFalse(TEXT("An enemy in radius blocks fill"),
		FTDCaptureBaseLogic::ShouldFillChannel(true, 1, 0, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCaptureBasePausesWhenChampionLeavesTest,
	"TD.Capture.Base.PausesWhenChampionLeaves",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaptureBasePausesWhenChampionLeavesTest::RunTest(const FString& Parameters)
{
	FTDCaptureBaseInput In = CaptureBaseTestPrivate::MakeIdleInput();
	In.bChampionInRadius = false;
	In.ChannelProgress = 0.7f;
	In.DeltaSeconds = 10.f;

	const FTDCaptureBaseOutput Out = FTDCaptureBaseLogic::Step(In);
	TestEqual(TEXT("Leaving the radius pauses the bar"), Out.ChannelProgress, 0.7f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCaptureBaseChannelCompleteShowsStarterPadsTest,
	"TD.Capture.Base.ChannelCompleteShowsStarterPads",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaptureBaseChannelCompleteShowsStarterPadsTest::RunTest(const FString& Parameters)
{
	FTDCaptureBaseInput In = CaptureBaseTestPrivate::MakeIdleInput();
	In.bChampionInRadius = true;
	In.ChannelProgress = 0.9f;
	In.DeltaSeconds = 1.f;

	const FTDCaptureBaseOutput Out = FTDCaptureBaseLogic::Step(In);
	TestTrue(TEXT("Channel completes"), Out.bChannelCompletedThisLife);
	TestTrue(TEXT("Starter pads become buildable"), Out.bStarterPadsBuildable);
	TestTrue(TEXT("Starter pads become visible"), Out.bStarterPadsVisible);
	TestFalse(TEXT("Vision stays off before 3 towers"), Out.bVisionOn);
	TestFalse(TEXT("Extra pads stay hidden before first full capture"), Out.bExtraPadsVisible);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCaptureBasePartialStartersGrantNothingTest,
	"TD.Capture.Base.PartialStartersGrantNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaptureBasePartialStartersGrantNothingTest::RunTest(const FString& Parameters)
{
	FTDCaptureBaseInput In = CaptureBaseTestPrivate::MakeIdleInput();
	In.bChannelCompletedThisLife = true;
	In.StarterTowersAlive = 2;
	In.PreviousStarterTowersAlive = 1;

	const FTDCaptureBaseOutput Out = FTDCaptureBaseLogic::Step(In);
	TestFalse(TEXT("1-2 starters before a full capture do not hold the base"), Out.bHeld);
	TestFalse(TEXT("No vision yet"), Out.bVisionOn);
	TestFalse(TEXT("Extra pads still hidden"), Out.bExtraPadsVisible);
	TestFalse(TEXT("Extra towers unpowered"), Out.bExtraTowersPowered);
	TestTrue(TEXT("The empty starter pad is still buildable"), Out.bStarterPadsBuildable);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCaptureBaseThirdStarterFullyCapturesTest,
	"TD.Capture.Base.ThirdStarterFullyCaptures",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaptureBaseThirdStarterFullyCapturesTest::RunTest(const FString& Parameters)
{
	FTDCaptureBaseInput In = CaptureBaseTestPrivate::MakeIdleInput();
	In.bChannelCompletedThisLife = true;
	In.PreviousStarterTowersAlive = 2;
	In.StarterTowersAlive = 3;

	const FTDCaptureBaseOutput Out = FTDCaptureBaseLogic::Step(In);
	TestTrue(TEXT("Three starters hold the base"), Out.bHeld);
	TestTrue(TEXT("Extra pads unlock"), Out.bExtraPadsUnlocked);
	TestTrue(TEXT("Extra pads visible"), Out.bExtraPadsVisible);
	TestTrue(TEXT("Vision on"), Out.bVisionOn);
	TestTrue(TEXT("Extra towers powered"), Out.bExtraTowersPowered);
	TestTrue(TEXT("Vision should register"), Out.bShouldRegisterVision);
	TestTrue(TEXT("Extra pads are buildable while held"), Out.bExtraPadsBuildable);
	TestFalse(TEXT("Occupied starters are not offered as empty pads"), Out.bStarterPadsBuildable);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCaptureBaseHoldSurvivesPartialLossTest,
	"TD.Capture.Base.HoldSurvivesPartialLoss",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaptureBaseHoldSurvivesPartialLossTest::RunTest(const FString& Parameters)
{
	FTDCaptureBaseInput In = CaptureBaseTestPrivate::MakeIdleInput();
	In.bHeld = true;
	In.bExtraPadsUnlocked = true;
	In.bChannelCompletedThisLife = true;
	In.PreviousStarterTowersAlive = 3;
	In.StarterTowersAlive = 2;

	const FTDCaptureBaseOutput Out = FTDCaptureBaseLogic::Step(In);
	TestTrue(TEXT("Still held with 2 starters"), Out.bHeld);
	TestTrue(TEXT("Vision stays on"), Out.bVisionOn);
	TestTrue(TEXT("Extra towers keep firing"), Out.bExtraTowersPowered);
	TestTrue(TEXT("Extra pads stay visible"), Out.bExtraPadsVisible);
	TestTrue(TEXT("The destroyed starter pad can be rebuilt without a new channel"), Out.bStarterPadsBuildable);
	TestFalse(TEXT("Vision is not re-registered"), Out.bShouldRegisterVision);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCaptureBaseLossOnLastStarterDestroyedTest,
	"TD.Capture.Base.LossOnLastStarterDestroyed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaptureBaseLossOnLastStarterDestroyedTest::RunTest(const FString& Parameters)
{
	FTDCaptureBaseInput In = CaptureBaseTestPrivate::MakeIdleInput();
	In.bHeld = true;
	In.bExtraPadsUnlocked = true;
	In.bChannelCompletedThisLife = true;
	In.ChannelProgress = 1.f;
	In.PreviousStarterTowersAlive = 1;
	In.StarterTowersAlive = 0;

	const FTDCaptureBaseOutput Out = FTDCaptureBaseLogic::Step(In);
	TestFalse(TEXT("Hold clears"), Out.bHeld);
	TestFalse(TEXT("Vision off"), Out.bVisionOn);
	TestFalse(TEXT("Extra towers unpowered"), Out.bExtraTowersPowered);
	TestTrue(TEXT("Extra pads stay unlocked/visible"), Out.bExtraPadsVisible);
	TestFalse(TEXT("Extra pads reject new builds"), Out.bExtraPadsBuildable);
	TestFalse(TEXT("Starter pads hide until a new channel"), Out.bStarterPadsVisible);
	TestFalse(TEXT("Channel completion is cleared"), Out.bChannelCompletedThisLife);
	TestEqual(TEXT("Channel progress resets"), Out.ChannelProgress, 0.f);
	TestTrue(TEXT("Vision should unregister"), Out.bShouldUnregisterVision);
	TestFalse(TEXT("Empty extra pad is not buildable while not held"),
		FTDCaptureBaseLogic::IsPadBuildable(false, false, Out));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCaptureBaseRecaptureRestoresHoldTest,
	"TD.Capture.Base.RecaptureRestoresHold",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaptureBaseRecaptureRestoresHoldTest::RunTest(const FString& Parameters)
{
	FTDCaptureBaseInput Channel = CaptureBaseTestPrivate::MakeIdleInput();
	Channel.bExtraPadsUnlocked = true;
	Channel.bChampionInRadius = true;
	Channel.ChannelProgress = 1.f;
	Channel.DeltaSeconds = 0.f;
	const FTDCaptureBaseOutput AfterChannel = FTDCaptureBaseLogic::Step(Channel);
	TestTrue(TEXT("Recapture channel completes"), AfterChannel.bChannelCompletedThisLife);
	TestFalse(TEXT("Vision still off until 3 starters"), AfterChannel.bVisionOn);
	TestTrue(TEXT("Extra pads remain visible after unlock"), AfterChannel.bExtraPadsVisible);

	FTDCaptureBaseInput Rebuild = CaptureBaseTestPrivate::MakeIdleInput();
	Rebuild.bChannelCompletedThisLife = AfterChannel.bChannelCompletedThisLife;
	Rebuild.bExtraPadsUnlocked = true;
	Rebuild.bHeld = false;
	Rebuild.PreviousStarterTowersAlive = 2;
	Rebuild.StarterTowersAlive = 3;
	const FTDCaptureBaseOutput Restored = FTDCaptureBaseLogic::Step(Rebuild);
	TestTrue(TEXT("Rebuilding 3 starters restores hold"), Restored.bHeld);
	TestTrue(TEXT("Vision returns"), Restored.bVisionOn);
	TestTrue(TEXT("Extra towers power on"), Restored.bExtraTowersPowered);
	TestTrue(TEXT("Vision should register again"), Restored.bShouldRegisterVision);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCaptureBasePadAssignmentValidationTest,
	"TD.Capture.Base.PadAssignmentValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaptureBasePadAssignmentValidationTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Three unique starters and extras are valid"),
		FTDCaptureBaseLogic::ArePadAssignmentsValid(3, 2, 5, false));
	TestFalse(TEXT("Missing a starter slot is invalid"),
		FTDCaptureBaseLogic::ArePadAssignmentsValid(2, 2, 4, false));
	TestFalse(TEXT("A null pad is invalid"),
		FTDCaptureBaseLogic::ArePadAssignmentsValid(3, 1, 4, true));
	TestFalse(TEXT("A duplicated pad is invalid"),
		FTDCaptureBaseLogic::ArePadAssignmentsValid(3, 1, 3, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCaptureBaseInsideRadiusUsesXYTest,
	"TD.Capture.Base.InsideRadiusUsesXY",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaptureBaseInsideRadiusUsesXYTest::RunTest(const FString& Parameters)
{
	const FVector Origin(0.f, 0.f, 0.f);
	TestTrue(TEXT("A point inside the XY radius counts"),
		FTDCaptureBaseLogic::IsInsideRadius(Origin, FVector(500.f, 0.f, 800.f), 1200.f));
	TestFalse(TEXT("A point outside the XY radius does not count"),
		FTDCaptureBaseLogic::IsInsideRadius(Origin, FVector(1201.f, 0.f, 0.f), 1200.f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCaptureBaseOccupiedPadNeverBuildableTest,
	"TD.Capture.Base.OccupiedPadNeverBuildable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCaptureBaseOccupiedPadNeverBuildableTest::RunTest(const FString& Parameters)
{
	FTDCaptureBaseOutput Held;
	Held.bStarterPadsBuildable = true;
	Held.bExtraPadsBuildable = true;
	TestFalse(TEXT("An occupied starter pad is not buildable"),
		FTDCaptureBaseLogic::IsPadBuildable(true, true, Held));
	TestFalse(TEXT("An occupied extra pad is not buildable"),
		FTDCaptureBaseLogic::IsPadBuildable(false, true, Held));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FVisionSourceListUnregisterLeavesOthersTest,
	"TD.Fog.Vision.UnregisterLeavesOtherSources",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVisionSourceListUnregisterLeavesOthersTest::RunTest(const FString& Parameters)
{
	TArray<uint64> Keys = { 11ull, 22ull, 33ull };
	TestEqual(TEXT("One matching key is removed"),
		FTDVisionSourceList::Unregister(Keys, 22ull), 1);
	TestEqual(TEXT("Two sources remain"), Keys.Num(), 2);
	TestTrue(TEXT("Crystal-like key 11 remains"), Keys.Contains(11ull));
	TestTrue(TEXT("Other key 33 remains"), Keys.Contains(33ull));
	TestEqual(TEXT("Missing keys remove nothing"),
		FTDVisionSourceList::Unregister(Keys, 99ull), 0);
	return true;
}

#endif
