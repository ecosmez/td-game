#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../TDChampionDropLand.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChampionDropLandsOppositeSpawnerTest,
	"TD.Champion.DropLand.LandsCloseOppositeSpawner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChampionDropLandsOppositeSpawnerTest::RunTest(const FString& Parameters)
{
	const FVector Crystal(0.f, 0.f, 100.f);
	const FVector Spawner(1000.f, 0.f, 0.f);
	const FVector Land = FTDChampionDropLand::ComputeLocationNearCrystal(
		Crystal, true, Spawner, 500.f);

	TestTrue(TEXT("The champion lands 500cm from the crystal, away from the spawner"),
		Land.Equals(FVector(-500.f, 0.f, 100.f), 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChampionDropFallsBackToPositiveXTest,
	"TD.Champion.DropLand.UsesPositiveXWithoutHint",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChampionDropFallsBackToPositiveXTest::RunTest(const FString& Parameters)
{
	const FVector Crystal(10.f, 20.f, 30.f);
	const FVector Land = FTDChampionDropLand::ComputeLocationNearCrystal(
		Crystal, false, FVector::ZeroVector, 400.f);

	TestTrue(TEXT("With no spawner hint the drop offsets along +X"),
		Land.Equals(FVector(410.f, 20.f, 30.f), 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChampionDropCoincidentHintUsesPositiveXTest,
	"TD.Champion.DropLand.CoincidentHintUsesPositiveX",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChampionDropCoincidentHintUsesPositiveXTest::RunTest(const FString& Parameters)
{
	const FVector Crystal(100.f, 100.f, 50.f);
	const FVector Land = FTDChampionDropLand::ComputeLocationNearCrystal(
		Crystal, true, Crystal, 600.f);

	TestTrue(TEXT("A coincident spawner still lands close on +X instead of on the crystal"),
		Land.Equals(FVector(700.f, 100.f, 50.f), 0.01f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FChampionDropZeroOffsetStaysAtCrystalTest,
	"TD.Champion.DropLand.ZeroOffsetStaysAtCrystal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChampionDropZeroOffsetStaysAtCrystalTest::RunTest(const FString& Parameters)
{
	const FVector Crystal(250.f, -80.f, 12.f);
	const FVector Land = FTDChampionDropLand::ComputeLocationNearCrystal(
		Crystal, true, FVector(1000.f, 0.f, 0.f), 0.f);

	TestTrue(TEXT("A zero offset keeps the crystal location"),
		Land.Equals(Crystal, 0.01f));
	return true;
}

#endif
