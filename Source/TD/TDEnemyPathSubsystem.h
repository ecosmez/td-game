#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDEnemyPathSubsystem.generated.h"

struct FTDEnemyPathState
{
	TArray<FVector> Waypoints;
	TArray<FVector> Samples;
	TArray<float> CumLength;
	float Distance = 0.f;
	float TotalLength = 0.f;
	bool bValid = false;
	bool bReachedNotified = false;
};

/** Per-world runtime path state for enemies following waypoint curves. */
UCLASS()
class TD_API UTDEnemyPathSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	FTDEnemyPathState& FindOrAdd(AActor* Enemy);
	FTDEnemyPathState* Find(AActor* Enemy);
	void Remove(AActor* Enemy);
	void Prune();

	/** One randomly chosen spawn (route + lane + location) owns the current wave. */
	TWeakObjectPtr<AActor> ActiveWaveSpawner;
	int32 PreparedWaveNumber = INDEX_NONE;
	int32 WaveRouteId = 0;
	bool bWavePreferOverLane = true;
	FVector WaveSpawnLocation = FVector::ZeroVector;
	bool bWaveSpawnPicked = false;

private:
	TMap<TWeakObjectPtr<AActor>, FTDEnemyPathState> States;
};
