#include "TDEnemyPathSubsystem.h"
#include "TDEnemyPathLibrary.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FTDEnemyPathAvoidanceTest,
	"TD.EnemyPath.AvoidanceChoosesOpenSide",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTDEnemyPathAvoidanceTest::RunTest(const FString& Parameters)
{
	TestEqual(TEXT("Chooses left when left is clearer"),
		UTDEnemyPathSubsystem::ChooseAvoidanceSide(2, 0.f, 2.f), -1);
	TestEqual(TEXT("Chooses right when right is clearer"),
		UTDEnemyPathSubsystem::ChooseAvoidanceSide(2, 3.f, 0.f), 1);
	TestEqual(TEXT("Even id breaks an equal tie to the right"),
		UTDEnemyPathSubsystem::ChooseAvoidanceSide(2, 1.f, 1.f), 1);
	TestEqual(TEXT("Odd id breaks an equal tie to the left"),
		UTDEnemyPathSubsystem::ChooseAvoidanceSide(3, 1.f, 1.f), -1);
	TestEqual(TEXT("Keeps a stable attack slot while it remains free"),
		UTDEnemyPathSubsystem::ChooseStableAttackSlot(3, 1u << 1, 8), 3);
	TestEqual(TEXT("Chooses the nearest free attack slot instead of jumping across the ring"),
		UTDEnemyPathSubsystem::ChooseStableAttackSlot(3, (1u << 3) | (1u << 4), 8), 2);
	TestEqual(TEXT("Reports no attack slot when the ring is full"),
		UTDEnemyPathSubsystem::ChooseStableAttackSlot(0, 0xffu, 8), INDEX_NONE);
	return true;
}
#endif

void UTDEnemyPathSubsystem::Tick(float DeltaTime)
{
	TArray<TWeakObjectPtr<AActor>> Enemies;
	States.GenerateKeyArray(Enemies);
	for (const TWeakObjectPtr<AActor>& Enemy : Enemies)
	{
		if (AActor* Actor = Enemy.Get())
		{
			UTDEnemyPathLibrary::ApplyChampionEngagementSeparation(Actor);
		}
	}
}

TStatId UTDEnemyPathSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UTDEnemyPathSubsystem, STATGROUP_Tickables);
}

int32 UTDEnemyPathSubsystem::ChooseAvoidanceSide(uint32 EnemyId, float LeftOccupancy, float RightOccupancy)
{
	if (!FMath::IsNearlyEqual(LeftOccupancy, RightOccupancy))
	{
		return LeftOccupancy < RightOccupancy ? -1 : 1;
	}

	return (EnemyId & 1u) == 0u ? 1 : -1;
}

int32 UTDEnemyPathSubsystem::ChooseStableAttackSlot(int32 PreferredSlot, uint32 OccupiedMask, int32 SlotCount)
{
	if (SlotCount <= 0 || SlotCount > 32)
	{
		return INDEX_NONE;
	}

	PreferredSlot = ((PreferredSlot % SlotCount) + SlotCount) % SlotCount;
	if ((OccupiedMask & (1u << PreferredSlot)) == 0u)
	{
		return PreferredSlot;
	}

	for (int32 Step = 1; Step < SlotCount; ++Step)
	{
		const int32 CounterClockwise = (PreferredSlot - Step + SlotCount) % SlotCount;
		if ((OccupiedMask & (1u << CounterClockwise)) == 0u)
		{
			return CounterClockwise;
		}
		const int32 Clockwise = (PreferredSlot + Step) % SlotCount;
		if ((OccupiedMask & (1u << Clockwise)) == 0u)
		{
			return Clockwise;
		}
	}

	return INDEX_NONE;
}

int32 UTDEnemyPathSubsystem::FindOrAssignEngagementSlot(AActor* Enemy, AActor* Target, int32 SlotCount)
{
	if (!Enemy || !Target || SlotCount <= 0 || SlotCount > 32)
	{
		return INDEX_NONE;
	}

	FTDEnemyPathState& State = FindOrAdd(Enemy);
	if (State.EngagementTarget.Get() == Target && State.EngagementSlot >= 0 && State.EngagementSlot < SlotCount)
	{
		return State.EngagementSlot;
	}

	uint32 OccupiedMask = 0u;
	for (const TPair<TWeakObjectPtr<AActor>, FTDEnemyPathState>& Pair : States)
	{
		if (Pair.Key.Get() != Enemy && Pair.Value.EngagementTarget.Get() == Target
			&& Pair.Value.EngagementSlot >= 0 && Pair.Value.EngagementSlot < SlotCount)
		{
			OccupiedMask |= 1u << Pair.Value.EngagementSlot;
		}
	}

	const int32 Preferred = static_cast<int32>(Enemy->GetUniqueID() % static_cast<uint32>(SlotCount));
	State.EngagementTarget = Target;
	State.EngagementSlot = ChooseStableAttackSlot(Preferred, OccupiedMask, SlotCount);
	return State.EngagementSlot;
}

float UTDEnemyPathSubsystem::ComputeAvoidanceOffset(
	AActor* Enemy, const FTDEnemyPathState& State, const FVector& PathLocation,
	const FVector& PathTangent, float AvoidanceRadius, float SideStepDistance) const
{
	if (!Enemy || State.Waypoints.Num() == 0 || AvoidanceRadius <= 0.f)
	{
		return 0.f;
	}

	const FVector Forward = PathTangent.GetSafeNormal2D();
	const FVector Right(-Forward.Y, Forward.X, 0.f);
	const FVector& Start = State.Waypoints[0];
	const FVector& End = State.Waypoints.Last();
	bool bBlocked = false;
	float LeftOccupancy = 0.f;
	float RightOccupancy = 0.f;
	const float RadiusSq = FMath::Square(AvoidanceRadius * 1.5f);

	for (const TPair<TWeakObjectPtr<AActor>, FTDEnemyPathState>& Pair : States)
	{
		AActor* Other = Pair.Key.Get();
		const FTDEnemyPathState& OtherState = Pair.Value;
		if (!Other || Other == Enemy || Other->IsActorBeingDestroyed() || !OtherState.bValid || OtherState.Waypoints.Num() == 0
			|| !Start.Equals(OtherState.Waypoints[0], 1.f) || !End.Equals(OtherState.Waypoints.Last(), 1.f))
		{
			continue;
		}

		const FVector ToOther = Other->GetActorLocation() - PathLocation;
		const float ForwardDistance = FVector::DotProduct(ToOther, Forward);
		const float SideDistance = FVector::DotProduct(ToOther, Right);
		const bool bExactTieWinner = FMath::Abs(ForwardDistance) <= 5.f && Enemy->GetUniqueID() < Other->GetUniqueID();
		if (!bExactTieWinner && ForwardDistance >= -5.f && ForwardDistance <= AvoidanceRadius * 1.5f
			&& FMath::Abs(SideDistance) < AvoidanceRadius)
		{
			bBlocked = true;
		}

		const FVector OtherLocation = Other->GetActorLocation();
		const float LeftDistSq = FVector::DistSquared2D(PathLocation - Right * SideStepDistance, OtherLocation);
		const float RightDistSq = FVector::DistSquared2D(PathLocation + Right * SideStepDistance, OtherLocation);
		if (LeftDistSq < RadiusSq)
		{
			LeftOccupancy += 1.f - LeftDistSq / RadiusSq;
		}
		if (RightDistSq < RadiusSq)
		{
			RightOccupancy += 1.f - RightDistSq / RadiusSq;
		}
	}

	return bBlocked
		? static_cast<float>(ChooseAvoidanceSide(Enemy->GetUniqueID(), LeftOccupancy, RightOccupancy)) * SideStepDistance
		: 0.f;
}

FTDEnemyPathState& UTDEnemyPathSubsystem::FindOrAdd(AActor* Enemy)
{
	Prune();
	return States.FindOrAdd(Enemy);
}

FTDEnemyPathState* UTDEnemyPathSubsystem::Find(AActor* Enemy)
{
	Prune();
	return States.Find(Enemy);
}

void UTDEnemyPathSubsystem::Remove(AActor* Enemy)
{
	States.Remove(Enemy);
}

void UTDEnemyPathSubsystem::Prune()
{
	for (auto It = States.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
}
