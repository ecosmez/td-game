#include "MapFogWorldSubsystem.h"

#include "MapDiscoveryComponent.h"
#include "WorldFogOfWarComponent.h"
#include "MobaPlayerController.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UMapFogWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	RetryTimer = 0.f;
}

void UMapFogWorldSubsystem::Deinitialize()
{
	BoundDiscovery = nullptr;
	BoundFog = nullptr;
	BoundPC = nullptr;
	Super::Deinitialize();
}

TStatId UMapFogWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMapFogWorldSubsystem, STATGROUP_Tickables);
}

void UMapFogWorldSubsystem::Tick(float DeltaTime)
{
	if (!bAutoEnable)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return;
	}

	// Retry until a player controller + pawn exist (post-sky-drop, streaming, etc.).
	RetryTimer -= DeltaTime;
	if (RetryTimer <= 0.f)
	{
		RetryTimer = 0.25f;
		EnsureOnLocalPlayer();
	}

	if (BoundDiscovery && BoundPC.IsValid())
	{
		if (AActor* Explorer = ResolveExplorer(BoundPC.Get()))
		{
			BoundDiscovery->SetExplorer(Explorer);
		}
	}
}

AActor* UMapFogWorldSubsystem::ResolveExplorer(APlayerController* PC) const
{
	if (!PC)
	{
		return nullptr;
	}
	if (const AMobaPlayerController* MPC = Cast<AMobaPlayerController>(PC))
	{
		if (APawn* Champ = MPC->GetControlledChampion())
		{
			return Champ;
		}
	}
	return PC->GetPawn();
}

void UMapFogWorldSubsystem::EnsureOnLocalPlayer()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (!PC || !PC->IsLocalController())
	{
		return;
	}

	// New PC (travel / restart) — rebinding required.
	if (BoundPC.Get() != PC)
	{
		BoundPC = PC;
		BoundDiscovery = nullptr;
		BoundFog = nullptr;
	}

	// Reuse components when already on MobaPlayerController CDO path.
	if (!BoundDiscovery)
	{
		BoundDiscovery = PC->FindComponentByClass<UMapDiscoveryComponent>();
		if (!BoundDiscovery)
		{
			BoundDiscovery = NewObject<UMapDiscoveryComponent>(PC, TEXT("MapDiscovery_Runtime"));
			BoundDiscovery->RegisterComponent();
			PC->AddInstanceComponent(BoundDiscovery);
			BoundDiscovery->SetEnabled(true);
		}
		else
		{
			BoundDiscovery->SetEnabled(true);
		}
	}

	if (!BoundFog)
	{
		BoundFog = PC->FindComponentByClass<UWorldFogOfWarComponent>();
		if (!BoundFog)
		{
			BoundFog = NewObject<UWorldFogOfWarComponent>(PC, TEXT("WorldFogOfWar_Runtime"));
			BoundFog->RegisterComponent();
			PC->AddInstanceComponent(BoundFog);
			// Runtime-added FOW defaults on; Moba PC keeps its own preference.
			BoundFog->SetEnabled(true);
		}
		BoundFog->SetDiscoverySource(BoundDiscovery);

		// Respect AMobaPlayerController flags / hotkey toggle — never force re-enable.
		if (const AMobaPlayerController* MPC = Cast<AMobaPlayerController>(PC))
		{
			BoundFog->SetEnabled(MPC->bEnableWorldFogOfWar && MPC->bEnableMapDiscovery);
		}
	}

	if (AActor* Explorer = ResolveExplorer(PC))
	{
		BoundDiscovery->SetExplorer(Explorer);
	}
}
