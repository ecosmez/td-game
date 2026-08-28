#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../CrystalCometWorldSubsystem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCrystalCometFixedChanceTest,
	"TD.Crystals.Comets.UsesFixedLowChance",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrystalCometFixedChanceTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("A roll below ten percent spawns"),
		UCrystalCometWorldSubsystem::ShouldSpawnComet(0.099f, 0.10f, 0, 2));
	TestFalse(TEXT("The ten-percent boundary does not spawn"),
		UCrystalCometWorldSubsystem::ShouldSpawnComet(0.10f, 0.10f, 0, 2));
	TestFalse(TEXT("A later roll keeps the same low chance"),
		UCrystalCometWorldSubsystem::ShouldSpawnComet(0.40f, 0.10f, 0, 2));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCrystalCometLimitTest,
	"TD.Crystals.Comets.RespectsActiveLimit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrystalCometLimitTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("One active extra crystal still leaves room"),
		UCrystalCometWorldSubsystem::ShouldSpawnComet(0.01f, 0.10f, 1, 2));
	TestFalse(TEXT("Two active extra crystals block another comet"),
		UCrystalCometWorldSubsystem::ShouldSpawnComet(0.01f, 0.10f, 2, 2));
	TestFalse(TEXT("A disabled limit blocks spawning"),
		UCrystalCometWorldSubsystem::ShouldSpawnComet(0.01f, 0.10f, 0, 0));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCrystalCometWaveTransitionTest,
	"TD.Crystals.Comets.RollsOnceWhenWaveClears",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrystalCometWaveTransitionTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("Entering the waiting-for-clear state is not a clear"),
		UCrystalCometWorldSubsystem::DidWaveJustClear(false, true));
	TestTrue(TEXT("Leaving the waiting-for-clear state is one clear"),
		UCrystalCometWorldSubsystem::DidWaveJustClear(true, false));
	TestFalse(TEXT("Remaining idle does not roll repeatedly"),
		UCrystalCometWorldSubsystem::DidWaveJustClear(false, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCrystalCometLandingClearanceTest,
	"TD.Crystals.Comets.RejectsOccupiedLandingLocations",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrystalCometLandingClearanceTest::RunTest(const FString& Parameters)
{
	const TArray<FVector> OccupiedLocations = { FVector(1000.f, 1000.f, 0.f) };
	TestFalse(TEXT("A nearby crystal blocks the candidate"),
		UCrystalCometWorldSubsystem::IsLandingLocationClear(
			FVector(1500.f, 1000.f, 500.f), OccupiedLocations, 1200.f));
	TestTrue(TEXT("Vertical separation does not hide an overlapping map location"),
		UCrystalCometWorldSubsystem::IsLandingLocationClear(
			FVector(2300.f, 1000.f, 500.f), OccupiedLocations, 1200.f));
	return true;
}

#endif
