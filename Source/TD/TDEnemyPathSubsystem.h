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
	float LateralOffset = 0.f;
	bool bValid = false;
	bool bReachedNotified = false;

	/** True while something (e.g. the champion's melee lock) should freeze path movement this tick. */
	bool bHeld = false;

	/** Stable place on the melee ring while this enemy is engaging the champion. */
	TWeakObjectPtr<AActor> EngagementTarget;
	int32 EngagementSlot = INDEX_NONE;
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
class TD_API UTDEnemyPathSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickableInEditor() const override { return false; }

	static int32 ChooseAvoidanceSide(uint32 EnemyId, float LeftOccupancy, float RightOccupancy);
	static int32 ChooseStableAttackSlot(int32 PreferredSlot, uint32 OccupiedMask, int32 SlotCount);
	static bool ShouldHoldEnemyPath(bool bExplicitlyHeld, bool bHasEngagementTarget);
	static bool ShouldMoveToEngagementSlot(float DistanceToSlot, float StopTolerance);
	static float MinimumEngagementRingRadius(float EnemyRadius, int32 SlotCount, float Gap);
	float ComputeAvoidanceOffset(AActor* Enemy, const FTDEnemyPathState& State, const FVector& PathLocation,
		const FVector& PathTangent, float AvoidanceRadius, float SideStepDistance) const;
	int32 FindOrAssignEngagementSlot(AActor* Enemy, AActor* Target, int32 SlotCount);

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
