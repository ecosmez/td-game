#if WITH_DEV_AUTOMATION_TESTS

#include "../TDEnemyPathLibrary.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWavePerSpawnCountGrowsEachWaveTest,
	"TD.EnemyPath.WaveScaling.PerSpawnCountGrowsEachWave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWavePerSpawnCountGrowsEachWaveTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Wave 1 is 20 per spawn"), UTDEnemyPathLibrary::ComputeWavePerSpawnCount(1), 20);
	TestEqual(TEXT("Wave 2 is 28 per spawn"), UTDEnemyPathLibrary::ComputeWavePerSpawnCount(2), 28);
	TestEqual(TEXT("Wave 3 is 36 per spawn"), UTDEnemyPathLibrary::ComputeWavePerSpawnCount(3), 36);
	TestEqual(TEXT("Wave 4 is 44 per spawn"), UTDEnemyPathLibrary::ComputeWavePerSpawnCount(4), 44);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWaveEnemyCountMultipliesBySpawnSlotsTest,
	"TD.EnemyPath.WaveScaling.TotalCountMultipliesBySpawnSlots",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWaveEnemyCountMultipliesBySpawnSlotsTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Wave 1 uses one lane"), UTDEnemyPathLibrary::ComputeWaveSpawnSlotCount(1, 3), 1);
	TestEqual(TEXT("Wave 2 adds a lane"), UTDEnemyPathLibrary::ComputeWaveSpawnSlotCount(2, 3), 2);
	TestEqual(TEXT("Wave 5 cannot exceed available lanes"), UTDEnemyPathLibrary::ComputeWaveSpawnSlotCount(5, 3), 3);

	TestEqual(TEXT("Wave 1 total is 20"), UTDEnemyPathLibrary::ComputeWaveEnemyCount(1, 1, false), 20);
	TestEqual(TEXT("Wave 2 total is 56"), UTDEnemyPathLibrary::ComputeWaveEnemyCount(2, 2, false), 56);
	TestEqual(TEXT("Wave 3 total is 108"), UTDEnemyPathLibrary::ComputeWaveEnemyCount(3, 3, false), 108);
	TestEqual(TEXT("Boss wave adds one extra"), UTDEnemyPathLibrary::ComputeWaveEnemyCount(3, 3, true), 109);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWaveSpawnIntervalTightensEachWaveTest,
	"TD.EnemyPath.WaveScaling.SpawnIntervalTightensEachWave",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWaveSpawnIntervalTightensEachWaveTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Wave 1 interval is 0.55s"),
		FMath::IsNearlyEqual(UTDEnemyPathLibrary::ComputeWaveSpawnInterval(1), 0.55f));
	TestTrue(TEXT("Wave 2 interval is 0.50s"),
		FMath::IsNearlyEqual(UTDEnemyPathLibrary::ComputeWaveSpawnInterval(2), 0.50f));
	TestTrue(TEXT("Wave 6 hits the 0.30s floor"),
		FMath::IsNearlyEqual(UTDEnemyPathLibrary::ComputeWaveSpawnInterval(6), 0.30f));
	TestTrue(TEXT("Later waves stay on the 0.30s floor"),
		FMath::IsNearlyEqual(UTDEnemyPathLibrary::ComputeWaveSpawnInterval(12), 0.30f));
	return true;
}

#endif
