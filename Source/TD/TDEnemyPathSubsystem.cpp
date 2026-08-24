#include "TDEnemyPathSubsystem.h"

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
	return true;
}
#endif

int32 UTDEnemyPathSubsystem::ChooseAvoidanceSide(uint32 EnemyId, float LeftOccupancy, float RightOccupancy)
{
	if (!FMath::IsNearlyEqual(LeftOccupancy, RightOccupancy))
	{
		return LeftOccupancy < RightOccupancy ? -1 : 1;
	}

	return (EnemyId & 1u) == 0u ? 1 : -1;
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
