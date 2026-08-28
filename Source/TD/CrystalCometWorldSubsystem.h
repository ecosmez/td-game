#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CrystalCometWorldSubsystem.generated.h"

UCLASS(Config=Game, DefaultConfig)
class TD_API UCrystalCometWorldSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;

	static bool ShouldSpawnComet(float Roll, float Chance, int32 ActiveExtras, int32 MaxExtras);
	static bool DidWaveJustClear(bool bWasWaitingForClear, bool bIsWaitingForClear);
	static bool IsLandingLocationClear(const FVector& Candidate,
		const TArray<FVector>& OccupiedLocations, float ExclusionRadius);

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Crystal Comets", meta=(ClampMin="0.0", ClampMax="1.0"))
	float SpawnChancePerClearedWave = 0.10f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Crystal Comets", meta=(ClampMin="0"))
	int32 MaxActiveExtraCrystals = 2;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Crystal Comets", meta=(ClampMin="500.0"))
	float CometHeight = 6500.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Crystal Comets", meta=(ClampMin="0.25"))
	float CometFallDuration = 2.5f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Crystal Comets", meta=(ClampMin="0.0"))
	float MarkerExclusionRadius = 1200.f;

	UPROPERTY(Config, EditAnywhere, BlueprintReadWrite, Category="Crystal Comets", meta=(ClampMin="1000.0"))
	float RandomSpawnRadius = 18000.f;

private:
	TWeakObjectPtr<AActor> CachedSpawner;
	TArray<TWeakObjectPtr<AActor>> ExtraCrystals;
	TArray<FVector> PendingLandingLocations;
	bool bWaveStateInitialized = false;
	bool bWasWaitingForClear = false;
	float PollElapsed = 0.f;

	AActor* FindPrimarySpawner();
	bool ReadWaitingForClear(const AActor* Spawner, bool& OutValue) const;
	void TryLaunchComet();
	void PruneDestroyedCrystals();
};
