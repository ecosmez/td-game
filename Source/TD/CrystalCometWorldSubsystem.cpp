#include "CrystalCometWorldSubsystem.h"

#include "CrystalCometActor.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "NavigationSystem.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UnrealType.h"

namespace CrystalCometPrivate
{
	const FSoftClassPath SpawnerClassPath(TEXT("/Game/TD/BP_EnemySpawner.BP_EnemySpawner_C"));
	const FSoftClassPath CrystalClassPath(TEXT("/Game/TD/BP_Crystal.BP_Crystal_C"));

	bool ReadBool(const UObject* Object, const FName Name, bool& OutValue)
	{
		if (!Object)
		{
			return false;
		}
		if (const FBoolProperty* Property = FindFProperty<FBoolProperty>(Object->GetClass(), Name))
		{
			OutValue = Property->GetPropertyValue_InContainer(Object);
			return true;
		}
		return false;
	}

	bool ReadInt(const UObject* Object, const FName Name, int32& OutValue)
	{
		if (!Object)
		{
			return false;
		}
		if (const FIntProperty* Property = FindFProperty<FIntProperty>(Object->GetClass(), Name))
		{
			OutValue = Property->GetPropertyValue_InContainer(Object);
			return true;
		}
		return false;
	}
}

void UCrystalCometWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	bWaveStateInitialized = false;
	bWasWaitingForClear = false;
	PollElapsed = 0.f;
}

bool UCrystalCometWorldSubsystem::DoesSupportWorldType(EWorldType::Type WorldType) const
{
	return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UCrystalCometWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UCrystalCometWorldSubsystem, STATGROUP_Tickables);
}

bool UCrystalCometWorldSubsystem::ShouldSpawnComet(
	float Roll, float Chance, int32 ActiveExtras, int32 MaxExtras)
{
	return MaxExtras > 0 && ActiveExtras < MaxExtras
		&& Roll < FMath::Clamp(Chance, 0.f, 1.f);
}

bool UCrystalCometWorldSubsystem::DidWaveJustClear(
	bool bWasWaitingForClearValue, bool bIsWaitingForClear)
{
	return bWasWaitingForClearValue && !bIsWaitingForClear;
}

FSoftClassPath UCrystalCometWorldSubsystem::GetLandingMarkerClassPath()
{
	return FSoftClassPath(TEXT("/Game/TD/BP_CrystalCometMarker.BP_CrystalCometMarker_C"));
}

bool UCrystalCometWorldSubsystem::IsLandingLocationClear(const FVector& Candidate,
	const TArray<FVector>& OccupiedLocations, float ExclusionRadius)
{
	const float RadiusSquared = FMath::Square(FMath::Max(0.f, ExclusionRadius));
	return !OccupiedLocations.ContainsByPredicate([&](const FVector& Occupied)
	{
		return FVector::DistSquared2D(Candidate, Occupied) < RadiusSquared;
	});
}

void UCrystalCometWorldSubsystem::Tick(float DeltaTime)
{
	PollElapsed += DeltaTime;
	if (PollElapsed < 0.2f)
	{
		return;
	}
	PollElapsed = 0.f;

	AActor* Spawner = CachedSpawner.Get();
	if (!Spawner)
	{
		Spawner = FindPrimarySpawner();
		CachedSpawner = Spawner;
		bWaveStateInitialized = false;
	}
	if (!Spawner)
	{
		return;
	}

	bool bWaitingForClear = false;
	if (!ReadWaitingForClear(Spawner, bWaitingForClear))
	{
		return;
	}
	if (!bWaveStateInitialized)
	{
		bWasWaitingForClear = bWaitingForClear;
		bWaveStateInitialized = true;
		return;
	}

	if (DidWaveJustClear(bWasWaitingForClear, bWaitingForClear))
	{
		TryLaunchComet();
	}
	bWasWaitingForClear = bWaitingForClear;
}

AActor* UCrystalCometWorldSubsystem::FindPrimarySpawner()
{
	UClass* SpawnerClass = CrystalCometPrivate::SpawnerClassPath.TryLoadClass<AActor>();
	UWorld* World = GetWorld();
	if (!SpawnerClass || !World)
	{
		return nullptr;
	}

	AActor* Best = nullptr;
	int32 BestRoute = MAX_int32;
	for (TActorIterator<AActor> It(World, SpawnerClass); It; ++It)
	{
		int32 Route = 0;
		CrystalCometPrivate::ReadInt(*It, TEXT("RouteId"), Route);
		if (!Best || Route < BestRoute)
		{
			Best = *It;
			BestRoute = Route;
		}
	}
	return Best;
}

bool UCrystalCometWorldSubsystem::ReadWaitingForClear(const AActor* Spawner, bool& OutValue) const
{
	return CrystalCometPrivate::ReadBool(Spawner, TEXT("WaitingforClear"), OutValue)
		|| CrystalCometPrivate::ReadBool(Spawner, TEXT("WaitingForClear"), OutValue);
}

void UCrystalCometWorldSubsystem::PruneDestroyedCrystals()
{
	ExtraCrystals.RemoveAll([](const TWeakObjectPtr<AActor>& Crystal)
	{
		return !Crystal.IsValid();
	});
}

void UCrystalCometWorldSubsystem::TryLaunchComet()
{
	PruneDestroyedCrystals();
	if (!ShouldSpawnComet(FMath::FRand(), SpawnChancePerClearedWave,
		ExtraCrystals.Num() + PendingLandingLocations.Num(), MaxActiveExtraCrystals))
	{
		return;
	}

	UWorld* World = GetWorld();
	UClass* MarkerClass = GetLandingMarkerClassPath().TryLoadClass<AActor>();
	UClass* CrystalClass = CrystalCometPrivate::CrystalClassPath.TryLoadClass<AActor>();
	if (!World || !MarkerClass || !CrystalClass)
	{
		return;
	}

	TArray<FVector> Candidates;
	TArray<FVector> OccupiedLocations = PendingLandingLocations;
	for (TActorIterator<AActor> It(World, CrystalClass); It; ++It)
	{
		if (IsValid(*It))
		{
			OccupiedLocations.Add(It->GetActorLocation());
		}
	}
	for (TActorIterator<AActor> It(World, MarkerClass); It; ++It)
	{
		const FVector Location = It->GetActorLocation();
		if (IsLandingLocationClear(Location, OccupiedLocations, MarkerExclusionRadius))
		{
			Candidates.Add(Location);
		}
	}

	// Existing levels may have no explicit markers. In that case, sample reachable
	// terrain around the initial crystal so the feature works without map edits.
	if (Candidates.IsEmpty() && !OccupiedLocations.IsEmpty())
	{
		if (UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World))
		{
			const FVector Origin = OccupiedLocations[0];
			for (int32 Attempt = 0; Attempt < 24; ++Attempt)
			{
				FNavLocation NavLocation;
				if (NavSystem->GetRandomReachablePointInRadius(Origin, RandomSpawnRadius, NavLocation)
					&& IsLandingLocationClear(NavLocation.Location, OccupiedLocations,
						FMath::Max(MarkerExclusionRadius, 2500.f)))
				{
					Candidates.Add(NavLocation.Location);
				}
			}
		}
	}
	if (Candidates.IsEmpty())
	{
		return;
	}

	const FVector LandingLocation = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
	PendingLandingLocations.Add(LandingLocation);
	ACrystalCometActor* Comet = World->SpawnActor<ACrystalCometActor>();
	if (!Comet)
	{
		PendingLandingLocations.RemoveSingle(LandingLocation);
		return;
	}

	TWeakObjectPtr<UCrystalCometWorldSubsystem> WeakThis(this);
	Comet->BeginFall(LandingLocation, CometHeight, CometFallDuration, CrystalClass,
		[WeakThis, LandingLocation](AActor* SpawnedCrystal)
		{
			if (UCrystalCometWorldSubsystem* Self = WeakThis.Get())
			{
				Self->PendingLandingLocations.RemoveSingle(LandingLocation);
				if (SpawnedCrystal)
				{
					Self->ExtraCrystals.Add(SpawnedCrystal);
				}
			}
		});
}
