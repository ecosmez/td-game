#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../TDTowerCombat.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTowerCombatRecognizesTowerBlockersTest,
	"TD.Tower.Combat.RecognizesTowerBlockers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTowerCombatRecognizesTowerBlockersTest::RunTest(const FString& Parameters)
{
	TestTrue(TEXT("Base tower class blocks shots"),
		FTDTowerCombat::IsTowerShotBlockerClassName(TEXT("BP_Tower_C")));
	TestTrue(TEXT("Arrow towers block shots"),
		FTDTowerCombat::IsTowerShotBlockerClassName(TEXT("BP_Tower_Arrow_C")));
	TestTrue(TEXT("Wall towers block shots"),
		FTDTowerCombat::IsTowerShotBlockerClassName(TEXT("BP_Tower_Wall_C")));
	TestFalse(TEXT("Tower pads do not block shots"),
		FTDTowerCombat::IsTowerShotBlockerClassName(TEXT("BP_TowerPad_C")));
	TestFalse(TEXT("Hex pads do not block shots"),
		FTDTowerCombat::IsTowerShotBlockerClassName(TEXT("BP_HexPad_C")));
	TestFalse(TEXT("Enemies do not count as tower blockers"),
		FTDTowerCombat::IsTowerShotBlockerClassName(TEXT("BP_Enemy_C")));
	TestFalse(TEXT("Terrain does not count as a tower blocker"),
		FTDTowerCombat::IsTowerShotBlockerClassName(TEXT("Landscape")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTowerCombatHitRulesIgnoreSelfGhostAndUnbuiltTest,
	"TD.Tower.Combat.HitRulesIgnoreSelfGhostAndUnbuilt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTowerCombatHitRulesIgnoreSelfGhostAndUnbuiltTest::RunTest(const FString& Parameters)
{
	TestFalse(TEXT("A tower never blocks its own shot"),
		FTDTowerCombat::DoesHitBlockTowerShot(TEXT("BP_Tower_Arrow_C"), true, false, true, true));
	TestFalse(TEXT("Ghost previews do not block shots"),
		FTDTowerCombat::DoesHitBlockTowerShot(TEXT("BP_Tower_Arrow_C"), false, true, true, true));
	TestFalse(TEXT("Unfinished construction does not block shots"),
		FTDTowerCombat::DoesHitBlockTowerShot(TEXT("BP_Tower_Cannon_C"), false, false, true, false));
	TestTrue(TEXT("A finished other tower blocks the shot"),
		FTDTowerCombat::DoesHitBlockTowerShot(TEXT("BP_Tower_Cannon_C"), false, false, true, true));
	TestFalse(TEXT("Rocky pads and terrain do not block shots"),
		FTDTowerCombat::DoesHitBlockTowerShot(TEXT("Landscape"), false, false, false, false));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTowerCombatTerrainDoesNotHideTowerBehindItTest,
	"TD.Tower.Combat.TerrainDoesNotHideTowerBehindIt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTowerCombatTerrainDoesNotHideTowerBehindItTest::RunTest(const FString& Parameters)
{
	const FTDTowerShotHitInfo Terrain{TEXT("Landscape"), false, false, false, false};
	const FTDTowerShotHitInfo OtherTower{TEXT("BP_Tower_Arrow_C"), false, false, true, true};
	const FTDTowerShotHitInfo Hits[] = {Terrain, OtherTower};

	TestTrue(TEXT("Terrain in front of another tower still lets that tower block the shot"),
		FTDTowerCombat::IsShotBlockedByHits(Hits));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTowerCombatClearLineThroughTerrainStaysOpenTest,
	"TD.Tower.Combat.ClearLineThroughTerrainStaysOpen",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTowerCombatClearLineThroughTerrainStaysOpenTest::RunTest(const FString& Parameters)
{
	const FTDTowerShotHitInfo Pad{TEXT("BP_TowerPad_C"), false, false, false, false};
	const FTDTowerShotHitInfo Terrain{TEXT("Landscape"), false, false, false, false};
	const FTDTowerShotHitInfo Hits[] = {Pad, Terrain};

	TestFalse(TEXT("Pads and terrain alone never block a tower shot"),
		FTDTowerCombat::IsShotBlockedByHits(Hits));
	return true;
}

#endif
