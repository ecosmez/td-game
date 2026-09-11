#include "MapFogWorldSubsystem.h"

#include "CaptureBase.h"
#include "MapDiscoveryComponent.h"
#include "WorldFogOfWarComponent.h"
#include "MobaPlayerController.h"
#include "TDFogVision.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/SoftObjectPath.h"

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
	CachedCrystal = nullptr;
	Super::Deinitialize();
}

TStatId UMapFogWorldSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UMapFogWorldSubsystem, STATGROUP_Tickables);
}

void UMapFogWorldSubsystem::Tick(float DeltaTime)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	DrawVisionPreviews(DeltaTime);

	if (!bAutoEnable || !World->IsGameWorld())
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

	// Live vision around the main crystal (minimap path also registers this).
	RegisterLandmarkReveals(BoundDiscovery);
}

void UMapFogWorldSubsystem::RegisterLandmarkReveals(UMapDiscoveryComponent* Discovery)
{
	if (!Discovery)
	{
		return;
	}

	if (AActor* Crystal = FindCrystal())
	{
		Discovery->RegisterVisionSource(Crystal, Discovery->CrystalVisionRadius);
	}
}

AActor* UMapFogWorldSubsystem::FindCrystal()
{
	if (AActor* Cached = CachedCrystal.Get())
	{
		return Cached;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const FSoftClassPath Path(TEXT("/Game/TD/BP_Crystal.BP_Crystal_C"));
	UClass* Cls = Path.TryLoadClass<AActor>();
	if (!Cls)
	{
		return nullptr;
	}

	AActor* First = nullptr;
	for (TActorIterator<AActor> It(World, Cls); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor))
		{
			continue;
		}
		if (!First || Actor->GetName() < First->GetName())
		{
			First = Actor;
		}
	}
	CachedCrystal = First;
	return First;
}

const UMapDiscoveryComponent* UMapFogWorldSubsystem::ResolveDiscoveryForPreview() const
{
	if (BoundDiscovery)
	{
		return BoundDiscovery;
	}
	if (const AMobaPlayerController* PCCdo = GetDefault<AMobaPlayerController>())
	{
		if (const UMapDiscoveryComponent* Comp = PCCdo->FindComponentByClass<UMapDiscoveryComponent>())
		{
			return Comp;
		}
	}
	return GetDefault<UMapDiscoveryComponent>();
}

void UMapFogWorldSubsystem::DrawVisionPreviews(float DeltaTime)
{
	UWorld* World = GetWorld();
	const UMapDiscoveryComponent* Discovery = ResolveDiscoveryForPreview();
	if (!World || !Discovery)
	{
		return;
	}

	const bool bGameWorld = World->IsGameWorld();
	if (FTDVisionPreview::ShouldDraw(
			bGameWorld,
			Discovery->bDrawCrystalVisionPreview,
			Discovery->bDrawCrystalVisionPreviewInPlay))
	{
		AActor* Crystal = CachedCrystal.Get();
		CrystalFindRetry -= DeltaTime;
		if (!Crystal && CrystalFindRetry <= 0.f)
		{
			CrystalFindRetry = 0.5f;
			Crystal = FindCrystal();
		}
		if (Crystal)
		{
			FTDVisionPreview::DrawCircle(
				World,
				Crystal->GetActorLocation(),
				Discovery->CrystalVisionRadius,
				FTDVisionPreview::CrystalColor());
		}
	}

	for (TActorIterator<ACaptureBase> It(World); It; ++It)
	{
		ACaptureBase* Base = *It;
		if (!IsValid(Base))
		{
			continue;
		}
		if (!FTDVisionPreview::ShouldDraw(
				bGameWorld,
				Base->bDrawVisionPreview,
				Base->bDrawVisionPreviewInPlay))
		{
			continue;
		}
		FTDVisionPreview::DrawCircle(
			World,
			Base->GetActorLocation(),
			Base->VisionRadius,
			FTDVisionPreview::CaptureBaseColor());
	}
}
