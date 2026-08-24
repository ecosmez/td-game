#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"
#include "TDEnemyPathLibrary.generated.h"

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

	/**
	 * Freeze/unfreeze an enemy's waypoint-path movement (used while it's locked as the
	 * champion's melee target, so it doesn't keep teleporting into the champion's collision
	 * every tick and fighting the physics de-penetration).
	 */
	UFUNCTION(BlueprintCallable, Category = "TD|Enemy Path")
	static void SetEnemyPathHeld(AActor* Enemy, bool bHeld);

	/** Tessellate Catmull-Rom through points. Used by enemies and waypoint previews. */
	static void TessellateCatmullRom(const TArray<FVector>& Points, TArray<FVector>& OutSamples, int32 SamplesPerSegment = 12);

	static FVector CatmullRom(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T);
};
