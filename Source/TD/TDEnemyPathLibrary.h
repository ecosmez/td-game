#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"
#include "TDEnemyPathLibrary.generated.h"

class UUserWidget;

/** World HP bar delayed-damage chunk (LoL-style lag). */
struct FTDEnemyHealthBarLagState
{
	float LagPercent = 1.f;
	float PreviousPercent = 1.f;
	float HoldRemaining = 0.f;
	bool bInitialized = false;
};

/**
 * Waypoint-only enemy lanes. Chooses Over/Under branches, then moves along a
 * Catmull-Rom curve through those points (smooth corners, no BP_EnemyPath).
 */
UCLASS()
class TD_API UTDEnemyPathLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Distance from a world location to the nearest enemy lane curve. */
	UFUNCTION(BlueprintPure, Category = "TD|EnemyPath", meta = (WorldContext = "WorldContextObject"))
	static float GetDistanceToNearestPath(const UObject* WorldContextObject, FVector Location);

	/** Pure geometry helper used by GetDistanceToNearestPath and automation tests. */
	static float DistanceToPolyline2D(FVector Location, const TArray<FVector>& Points);
	static float DistanceAlongPolyline2D(FVector Location, const TArray<FVector>& Points);
	static bool IsRouteInsideCorridor2D(
		const TArray<FVector>& Route, const TArray<FVector>& Guide, float CorridorRadius);
	static bool ShouldRefreshNavigationRoute(
		float RepathRemaining, bool bHasRoute, bool bRouteFinished, float GoalDelta, float GoalMoveThreshold);
	static int32 AdvanceNavigationRouteIndex(
		FVector Location, const TArray<FVector>& Route, int32 RouteIndex, float AcceptanceRadius);
	static bool IsRouteReturningToCorridor2D(
		const TArray<FVector>& Route, const TArray<FVector>& Guide, float CorridorRadius);
	/** Pure-pursuit sample along a Nav route; empty/invalid route returns Location. */
	static FVector SampleNavigationRouteLookAhead(
		FVector Location, const TArray<FVector>& Route, int32 RouteIndex, float LookAheadDistance);
	/** Drop Recast stair-steps: colinear cull + corridor-aware string-pull. */
	static TArray<FVector> SimplifyNavigationRoute2D(
		const TArray<FVector>& Route, const TArray<FVector>& Guide, float CorridorRadius,
		float ColinearTolerance = 8.f);
	static FVector ResolveNavigationSteeringTarget(
		FVector CurrentLocation, FVector GuideLocation, const TArray<FVector>& Route, int32 RouteIndex,
		float LookAheadDistance = 0.f);
	static bool DoesNavigationGoalAdvance(FVector From, FVector Goal, float MinDistance);
	/** True when wall contact leaves no lateral skirt — attack/break instead of pathing. */
	static bool ShouldAttackBlockingWall(bool bWallContact, bool bCanSkirt);
	/** Grow/decay lateral bypass while a physical blocker sits on the straight-line guide. */
	static float ResolveBlockBypassOffset(
		bool bPathBlocked, float CurrentOffset, float DeltaSeconds, float GrowPerSecond, float DecayPerSecond,
		float MaxOffset);
	/** Keep a locked bypass side; otherwise take PreferredSide (must be +/-1). */
	static int32 ResolveTerrainSteerSide(int32 LockedSide, int32 PreferredSide);
	static FVector ResolveUnwalkableStep(bool bFoundWalkable, FVector Walkable, FVector Previous);
	static bool IsGroundTraceIgnoredClassName(const FString& ClassName);
	static bool IsWithinObjectiveReach2D(
		FVector EnemyLocation, FVector ObjectiveLocation, FVector ObjectiveBoundsExtent, float ReachDistance);
	static FVector ResolveGroundCorrectionAfterSweep(FVector SweptLocation, FVector GroundSnappedLocation);
	static FVector ResolveEngagementGroundLocation(FVector PlanarLocation, FVector GroundSnappedLocation);

	/** True until this minion has already rolled its personal speed and lane slot. */
	static bool ShouldChooseWalkStyle(bool bAlreadyChosen);
	/** Map 0..1 into [1-Variance, 1+Variance]. Variance <= 0 keeps the authored speed. */
	static float ResolveMoveSpeedScale(float Variance, float RandomUnit);
	/** BaseSpeed * SpeedScale * SlowFactor, never negative. */
	static float ResolveEnemyMoveSpeed(float BaseSpeed, float SpeedScale, float SlowFactor);
	/** Map -1..1 into [-Range, Range]. Range <= 0 stays on the centerline. */
	static float ResolveLaneOffset(float Range, float RandomSignedUnit);
	/** Personal lane slot plus a momentary avoidance sidestep. */
	static float ResolveLaneGuideOffset(float PersonalOffset, float AvoidanceOffset);
	/** Shift a guide point onto a parallel lane using the path tangent. */
	static FVector OffsetAlongPathRight(FVector PathLocation, FVector PathTangent, float LateralOffset);

	/** Capture bases, resource crystals, and pads must not cut the minion lane. */
	static bool IsLaneDecorationClassName(const FString& ClassName);

	/** Keep decorations queryable for capture/click, but do not carve NavMesh or block pawns. */
	static void ApplyLaneDecorationCollision(AActor* Actor);

	/** Champion slow-area (and similar) volumes must never count as terrain. */
	static bool IsAbilityVolumeClassName(const FString& ClassName);

	/** Visual mesh is non-blocking; overlap sphere stays query-only and does not carve NavMesh. */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path")
	static void ApplyAbilityVolumeCollision(AActor* Actor);

	/** True once a champion pursuit has carried an enemy beyond its lane leash. */
	static bool ShouldAbandonChampionPursuit(float DistanceToOwnPath, float MaxPathLeashRange);

	/** Build / rebuild the enemy's curve from BP_Waypoint actors matching RouteId + lane. */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path")
	static void ChooseEnemyPath(AActor* Enemy);

	/**
	 * Move along the curved waypoint path. Handles stun / wall / crystal attack
	 * by calling the enemy's existing Blueprint functions when present.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path")
	static void AdvanceEnemyAlongPath(AActor* Enemy, float DeltaSeconds);

	/** Spawn location for a route (Index 0 waypoint, matching lane when possible). */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path", meta = (WorldContext = "WorldContextObject"))
	static FTransform GetEnemySpawnTransform(const UObject* WorldContextObject, int32 RouteId, bool bPreferOverLane);

	/**
	 * Replaces BP_EnemySpawner.SpawnEnemyInner: spawn trash / ranged / boss from
	 * the current wave's shuffled spawn queue (split across routes), then ChooseEnemyPath.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path")
	static AActor* SpawnNextWaveEnemy(AActor* Spawner);

	/**
	 * Split this wave's minions across spawn points and start the SpawnEnemy timer.
	 * Only the primary spawner (lowest RouteId) runs waves; others no-op.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path")
	static void BeginWaveSpawning(AActor* Spawner);

	/** Minions sent down each chosen spawn: 12 + WaveNumber * 8. */
	static int32 ComputeWavePerSpawnCount(int32 WaveNumber);
	/** How many spawn slots wave N uses, capped by available points. */
	static int32 ComputeWaveSpawnSlotCount(int32 WaveNumber, int32 AvailablePoints);
	/** Per-spawn count * slots, plus one on a boss wave. */
	static int32 ComputeWaveEnemyCount(int32 WaveNumber, int32 SpawnSlots, bool bBossWave);
	/** 0.55s on wave 1, -0.05s each wave, floor 0.30s. */
	static float ComputeWaveSpawnInterval(int32 WaveNumber);

	/** True if this is the wave director (lowest RouteId among same-class spawners). */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path")
	static bool IsPrimaryWaveSpawner(AActor* Spawner);

	/** Lowest-RouteId enemy spawner of the given class (defaults to the caller's class). */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path", meta = (WorldContext = "WorldContextObject"))
	static AActor* GetPrimaryWaveSpawner(const UObject* WorldContextObject, TSubclassOf<AActor> SpawnerClass);

	/** Call AnnounceWave only on the primary spawner so extra level spawners stay idle. */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path")
	static void AnnounceWaveIfPrimary(AActor* Spawner);

	/** Replaces BP_EnemySpawner.ForceStartNextWave; ignored on non-primary spawners. */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path")
	static void ForceStartNextWave(AActor* Spawner);

	/** After the player picks a between-wave power-up, resume OnWaveCleared / countdown. */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path")
	static void ContinueAfterWavePowerUp(AActor* Spawner);

	/** True if any trash / ranged / boss enemy is still alive. */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path", meta = (WorldContext = "WorldContextObject"))
	static bool AreWaveEnemiesAlive(const UObject* WorldContextObject);

	/** Living trash / ranged / boss enemies (skips CurrentHealth <= 0). */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path", meta = (WorldContext = "WorldContextObject"))
	static int32 CountWaveEnemiesAlive(const UObject* WorldContextObject);

	/** Alive enemies plus minions still waiting in the current wave spawn queue. */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path", meta = (WorldContext = "WorldContextObject"))
	static int32 CountWaveEnemiesRemaining(const UObject* WorldContextObject);

	/** Replaces BP_EnemySpawner.CheckWaveClear (counts BP_Boss too). */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path")
	static void CheckWaveEnemiesCleared(AActor* Spawner);

	/**
	 * True if Actor is a living enemy the champion can attack: exposes an
	 * ApplyEnemyDamage(Amount) function (BP_Enemy + subclasses) and CurrentHealth > 0.
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path")
	static bool IsAttackableEnemy(AActor* Actor);

	/** Reduce Enemy's health via its own ApplyEnemyDamage(Amount) function (health bar + death handled there). */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path")
	static void ApplyDamageToEnemy(AActor* Enemy, float Amount);

	/** Same screen-space channel widget capture bases use. */
	static TSubclassOf<UUserWidget> GetEnemyHealthBarWidgetClass();

	/** Pixel size matching ACaptureBase::ChannelBar. */
	static FVector2D GetEnemyHealthBarWidgetDrawSize();

	/** Ghost placement previews skip the world HP bar; live enemies and towers do not. */
	static bool ShouldShowWorldHealthBar(bool bIsGhost);

	/** Fill scale/location matching BP_Enemy.UpdateHealthBar (left-anchored cube). */
	static void ComputeEnemyHealthBarFill(float CurrentHealth, float MaxHealth, FVector& OutScale, FVector& OutRelativeLocation);

	/** Dark track around the fill — slightly thicker so hits read against a frame. */
	static void ComputeEnemyHealthBarTrack(FVector& OutScale);

	/** Hold the lost chunk, then drain it toward current HP so damage is readable. */
	static void TickEnemyHealthBarLag(FTDEnemyHealthBarLagState& State, float CurrentPercent, float DeltaTime);

	/** Hide mesh HP cubes and drive a screen-space capture-style channel bar on enemies and towers. */
	static void UpdateEnemyHealthBar(AActor* Enemy, float DeltaTime = 0.f);

	/**
	 * Freeze/unfreeze an enemy's waypoint-path movement (used while it's locked as the
	 * champion's melee target, so it doesn't keep teleporting into the champion's collision
	 * every tick and fighting the physics de-penetration).
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path")
	static void SetEnemyPathHeld(AActor* Enemy, bool bHeld);

	/** Smoothly moves a melee enemy toward a stable, reserved place around its champion target. */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path")
	static void ApplyChampionEngagementSeparation(AActor* Enemy);

	/** Tessellate Catmull-Rom through points. Used by enemies and waypoint previews. */
	static void TessellateCatmullRom(const TArray<FVector>& Points, TArray<FVector>& OutSamples, int32 SamplesPerSegment = 12);

	static FVector CatmullRom(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T);
};
