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

/** One minion spawn assignment (route + lane + Index 0 location). */
struct FTDWaveSpawnSlot
{
	int32 RouteId = 0;
	bool bOverLane = true;
	FVector Location = FVector::ZeroVector;
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

	/** Spawner that owns the current wave countdown / spawn timer. */
	TWeakObjectPtr<AActor> ActiveWaveSpawner;
	int32 PreparedWaveNumber = INDEX_NONE;

	/** Shuffled per-minion spawn assignments for the active wave. */
	TArray<FTDWaveSpawnSlot> WaveSpawnQueue;
	int32 WaveSpawnQueueIndex = 0;

private:
	TMap<TWeakObjectPtr<AActor>, FTDEnemyPathState> States;
};
