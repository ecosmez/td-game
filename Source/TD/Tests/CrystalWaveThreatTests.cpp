#if WITH_DEV_AUTOMATION_TESTS

#include "../CrystalHealthBarWidget.h"
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FCrystalWaveThreatCalculationTest,
	"TD.UI.CrystalWaveThreat.CalculatesNextWaveImpact",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrystalWaveThreatCalculationTest::RunTest(const FString& Parameters)
{
	struct FCase
	{
		float ResourcePool;
		int32 ExtraEnemies;
		int32 EmpowermentPercent;
		ECrystalThreatLevel ThreatLevel;
	};

	const FCase Cases[] =
	{
		{ 0.f, 0, 0, ECrystalThreatLevel::None },
		{ 14.9f, 0, 45, ECrystalThreatLevel::Low },
		{ 15.f, 1, 45, ECrystalThreatLevel::Low },
		{ 45.f, 3, 100, ECrystalThreatLevel::Medium },
		{ 75.f, 5, 100, ECrystalThreatLevel::High },
		{ 150.f, 5, 100, ECrystalThreatLevel::High },
	};

	for (const FCase& TestCase : Cases)
	{
		const FCrystalWaveThreat Actual = UCrystalHealthBarWidget::CalculateCrystalWaveThreat(TestCase.ResourcePool);
		TestEqual(TEXT("extra enemy count"), Actual.ExtraEnemies, TestCase.ExtraEnemies);
		TestEqual(TEXT("empowerment percentage"), Actual.EmpowermentPercent, TestCase.EmpowermentPercent);
		TestEqual(TEXT("threat level"), Actual.ThreatLevel, TestCase.ThreatLevel);
	}

	return true;
}

#endif
