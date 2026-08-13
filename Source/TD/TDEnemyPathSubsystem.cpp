#include "TDEnemyPathSubsystem.h"

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
