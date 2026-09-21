#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDEnemyPathLibrary.h"
#include "TDWavePowerUp.h"
#include "TDEnemyPathSubsystem.generated.h"

struct FTDEnemyPathState
{
	TArray<FVector> Waypoints;
	TArray<FVector> Samples;
	TArray<float> CumLength;
	float Distance = 0.f;
	float TotalLength = 0.f;
	float LateralOffset = 0.f;
	float SpeedScale = 1.f;
	float LaneOffset = 0.f;
	bool bWalkStyleChosen = false;
	TArray<FVector> NavigationRoute;
	int32 NavigationRouteIndex = 0;
	float RepathRemaining = 0.f;
	FVector NavigationGoal = FVector::ZeroVector;
	/** Locked lateral sign (+1/-1) while skirting physical blockers NavMesh no longer carves. */
	int32 TerrainSteerSide = 0;
	/** Extra cm of lateral guide offset while a physical blocker sits on the lane. */
	float BlockBypassOffset = 0.f;
	bool bValid = false;
	bool bReachedNotified = false;

	/** True while something (e.g. the champion's melee lock) should freeze path movement this tick. */
	bool bHeld = false;

	/** Stable place on the melee ring while this enemy is engaging the champion. */
	TWeakObjectPtr<AActor> EngagementTarget;
	int32 EngagementSlot = INDEX_NONE;

	/** Delayed yellow chunk so incoming hits are visible on the world HP bar. */
	FTDEnemyHealthBarLagState HealthBarLag;
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
	static FVector ComputePlanarEngagementStep(
		const FVector& Current, const FVector& Desired, float DeltaSeconds, float MoveSpeed);
	float ComputeAvoidanceOffset(AActor* Enemy, const FTDEnemyPathState& State, const FVector& PathLocation,
		const FVector& PathTangent, float AvoidanceRadius, float SideStepDistance) const;
	int32 FindOrAssignEngagementSlot(AActor* Enemy, AActor* Target, int32 SlotCount);

	FTDEnemyPathState& FindOrAdd(AActor* Enemy);
	FTDEnemyPathState* Find(AActor* Enemy);
	void Remove(AActor* Enemy);
	void Prune();
	void PrepareLaneDecorations();

	/** Spawner that owns the current wave countdown / spawn timer. */
	TWeakObjectPtr<AActor> ActiveWaveSpawner;
	int32 PreparedWaveNumber = INDEX_NONE;

	/** Shuffled per-minion spawn assignments for the active wave. */
	TArray<FTDWaveSpawnSlot> WaveSpawnQueue;
	int32 WaveSpawnQueueIndex = 0;

	/** Chosen player+enemy pair for the incoming / current wave. */
	FTDActiveWavePowerUp ActiveWavePowerUp;
	/** Three double-sided offers while the pick overlay is up. */
	TArray<FTDWavePowerUpOffer> DraftOffers;
	bool bAwaitingPowerUpPick = false;
	TWeakObjectPtr<AActor> DraftSpawner;

private:
	TMap<TWeakObjectPtr<AActor>, FTDEnemyPathState> States;
	bool bLaneDecorationsPrepared = false;
};
