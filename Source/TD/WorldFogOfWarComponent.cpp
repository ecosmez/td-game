#include "WorldFogOfWarComponent.h"

#include "MapDiscoveryComponent.h"
#include "MobaPlayerController.h"

#include "Components/PostProcessComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

UWorldFogOfWarComponent::UWorldFogOfWarComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	// Plane is the reliable top-down look; PP is an optional extra when material is good.
	bUseFallbackPlane = true;
	bAlwaysDrawPlane = true;
}

void UWorldFogOfWarComponent::BeginPlay()
{
	Super::BeginPlay();
	if (bEnabled)
	{
		EnsureResources();
	}
}

void UWorldFogOfWarComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DestroyResources();
	Super::EndPlay(EndPlayReason);
}

void UWorldFogOfWarComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bEnabled)
	{
		// Keep explorer wired even if BeginPlay ran before the champion existed.
		if (UMapDiscoveryComponent* Discovery = ResolveDiscovery())
		{
			if (const AMobaPlayerController* MPC = Cast<AMobaPlayerController>(GetOwner()))
			{
				if (APawn* Champ = MPC->GetControlledChampion())
				{
					Discovery->SetExplorer(Champ);
				}
			}
			else if (const APlayerController* PC = Cast<APlayerController>(GetOwner()))
			{
				if (APawn* P = PC->GetPawn())
				{
					Discovery->SetExplorer(P);
				}
			}
		}
		EnsureResources();
		UpdateFogPresentation();
	}
}

void UWorldFogOfWarComponent::SetEnabled(bool bInEnabled)
{
	bEnabled = bInEnabled;
	if (!bEnabled)
	{
		DestroyResources();
		return;
	}
	EnsureResources();
	UpdateFogPresentation();
}

void UWorldFogOfWarComponent::SetDiscoverySource(UMapDiscoveryComponent* InDiscovery)
{
	DiscoverySource = InDiscovery;
}

UMapDiscoveryComponent* UWorldFogOfWarComponent::ResolveDiscovery() const
{
	if (DiscoverySource)
	{
		return DiscoverySource;
	}
	if (const AActor* Owner = GetOwner())
	{
		if (UMapDiscoveryComponent* Comp = Owner->FindComponentByClass<UMapDiscoveryComponent>())
		{
			return Comp;
		}
	}
	return nullptr;
}

void UWorldFogOfWarComponent::EnsureResources()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Dedicated world actor — PP on PlayerController has no usable scene root and is ignored.
	if (!HostActor || !IsValid(HostActor))
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		Params.Owner = GetOwner();
		Params.Name = MakeUniqueObjectName(World, AActor::StaticClass(), TEXT("WorldFOW_Host"));
		HostActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Params);
		if (HostActor)
		{
		#if WITH_EDITOR
			HostActor->SetActorLabel(TEXT("WorldFOW_Host"));
		#endif
			USceneComponent* Root = NewObject<USceneComponent>(HostActor, TEXT("Root"));
			HostActor->SetRootComponent(Root);
			Root->RegisterComponent();
			Root->SetMobility(EComponentMobility::Movable);
			HostActor->SetActorEnableCollision(false);
		}
	}

	if (HostActor && !PostProcess)
	{
		PostProcess = NewObject<UPostProcessComponent>(HostActor, TEXT("WorldFOW_PostProcess"));
		if (PostProcess)
		{
			PostProcess->SetupAttachment(HostActor->GetRootComponent());
			PostProcess->RegisterComponent();
			PostProcess->bUnbound = true;
			PostProcess->bEnabled = false;
			PostProcess->Priority = 50.f;
			PostProcess->BlendWeight = 1.f;
		}
	}

	if (!bTriedLoadPPMaterial)
	{
		bTriedLoadPPMaterial = true;
		UMaterialInterface* SourceMat = PostProcessMaterialOverride.Get();
		if (!SourceMat)
		{
			SourceMat = PostProcessMaterial.LoadSynchronous();
		}
		if (SourceMat && PostProcess)
		{
			PostProcessMID = UMaterialInstanceDynamic::Create(SourceMat, this);
			if (PostProcessMID)
			{
				FWeightedBlendable Blendable;
				Blendable.Object = PostProcessMID;
				Blendable.Weight = 1.f;
				PostProcess->Settings.WeightedBlendables.Array.Reset();
				PostProcess->Settings.WeightedBlendables.Array.Add(Blendable);
				bHasPPMaterial = true;
			}
		}
	}

	const bool bWantPlane = bUseFallbackPlane && (bAlwaysDrawPlane || !bHasPPMaterial);
	if (bWantPlane && HostActor && !FallbackMesh)
	{
		FallbackMesh = NewObject<UStaticMeshComponent>(HostActor, TEXT("FOWPlane"));
		FallbackMesh->SetupAttachment(HostActor->GetRootComponent());
		FallbackMesh->SetGenerateOverlapEvents(false);
		FallbackMesh->SetCastShadow(false);
		FallbackMesh->bReceivesDecals = false;
		FallbackMesh->SetMobility(EComponentMobility::Movable);
		// Draw over terrain; translucent + no depth test keeps it as screen sheet for top-down.
		FallbackMesh->SetTranslucentSortPriority(100);
		FallbackMesh->SetRenderCustomDepth(false);
		FallbackMesh->SetBoundsScale(2.f);
		FallbackMesh->RegisterComponent();

		if (UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")))
		{
			FallbackMesh->SetStaticMesh(PlaneMesh);
		}

		// SetStaticMesh copies the engine plane's blocking collision — strip it after.
		FallbackMesh->SetCollisionProfileName(TEXT("NoCollision"));
		FallbackMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		FallbackMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
		FallbackMesh->SetGenerateOverlapEvents(false);
		HostActor->SetActorEnableCollision(false);

		// Prefer project plane material; fall back to Widget3D translucent.
		UMaterialInterface* PlaneMat = PlaneMaterialOverride.Get();
		if (!PlaneMat)
		{
			PlaneMat = PlaneMaterial.LoadSynchronous();
		}
		if (!PlaneMat)
		{
			PlaneMat = LoadObject<UMaterialInterface>(
				nullptr, TEXT("/Engine/EngineMaterials/Widget3DPassThrough_Translucent.Widget3DPassThrough_Translucent"));
		}
		if (PlaneMat)
		{
			FallbackMID = UMaterialInstanceDynamic::Create(PlaneMat, this);
			if (FallbackMID)
			{
				FallbackMesh->SetMaterial(0, FallbackMID);
			}
		}
	}

	if (PostProcess)
	{
		PostProcess->bEnabled = bEnabled && bHasPPMaterial;
	}
	if (FallbackMesh)
	{
		FallbackMesh->SetHiddenInGame(!(bEnabled && bWantPlane));
	}
}

void UWorldFogOfWarComponent::DestroyResources()
{
	PostProcess = nullptr;
	PostProcessMID = nullptr;
	FallbackMesh = nullptr;
	FallbackMID = nullptr;
	bTriedLoadPPMaterial = false;
	bHasPPMaterial = false;

	if (HostActor && IsValid(HostActor))
	{
		HostActor->Destroy();
	}
	HostActor = nullptr;
	FallbackPlaneActor = nullptr;
}

void UWorldFogOfWarComponent::UpdateFogPresentation()
{
	UMapDiscoveryComponent* Discovery = ResolveDiscovery();
	if (!Discovery || !Discovery->IsDiscoveryEnabled())
	{
		if (PostProcess)
		{
			PostProcess->bEnabled = false;
		}
		if (FallbackMesh)
		{
			FallbackMesh->SetHiddenInGame(true);
		}
		return;
	}

	// Force texture creation even before first explorer stamp.
	UTexture2D* FogTex = Discovery->GetFogTexture();
	if (!FogTex)
	{
		Discovery->SetEnabled(true);
		FogTex = Discovery->GetFogTexture();
	}
	if (!FogTex)
	{
		return;
	}

	float CenterX = 0.f;
	float CenterY = 0.f;
	float Ortho = 1.f;
	Discovery->GetOrthoWorldRect(CenterX, CenterY, Ortho);
	const FLinearColor FogColor = Discovery->UndiscoveredColor;

	if (bHasPPMaterial && PostProcessMID && PostProcess)
	{
		PostProcess->bEnabled = bEnabled;
		ApplyPostProcessParams(FogTex, CenterX, CenterY, Ortho, FogColor);
	}

	const bool bWantPlane = bUseFallbackPlane && (bAlwaysDrawPlane || !bHasPPMaterial);
	if (bWantPlane)
	{
		UpdateFallbackPlane(FogTex, CenterX, CenterY, Ortho);
	}
}

void UWorldFogOfWarComponent::ApplyPostProcessParams(
	UTexture2D* FogTex,
	float CenterX,
	float CenterY,
	float Ortho,
	const FLinearColor& FogColor)
{
	if (!PostProcessMID)
	{
		return;
	}
	PostProcessMID->SetTextureParameterValue(TEXT("FogMask"), FogTex);
	PostProcessMID->SetVectorParameterValue(TEXT("MapCenterOrtho"), FLinearColor(CenterX, CenterY, Ortho, 1.f));
	PostProcessMID->SetVectorParameterValue(TEXT("FogColor"), FogColor);
	PostProcessMID->SetScalarParameterValue(TEXT("FogIntensity"), FogIntensity);
}

void UWorldFogOfWarComponent::UpdateFallbackPlane(UTexture2D* FogTex, float CenterX, float CenterY, float Ortho)
{
	if (!HostActor || !FallbackMesh)
	{
		return;
	}

	float PlaneZ = FallbackPlaneHeight;
	if (bAutoFitPlaneHeightToExplorer)
	{
		AActor* Explorer = nullptr;
		if (const AMobaPlayerController* MPC = Cast<AMobaPlayerController>(GetOwner()))
		{
			Explorer = MPC->GetControlledChampion();
		}
		if (!Explorer)
		{
			if (const APlayerController* PC = Cast<APlayerController>(GetOwner()))
			{
				Explorer = PC->GetPawn();
			}
		}
		if (Explorer)
		{
			// Sit just above playable ground so the top-down camera looks through fog.
			PlaneZ = Explorer->GetActorLocation().Z + FallbackPlaneHeightOffset;
		}
	}

	HostActor->SetActorHiddenInGame(false);
	HostActor->SetActorEnableCollision(false);
	FallbackMesh->SetHiddenInGame(false);
	FallbackMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HostActor->SetActorLocation(FVector(CenterX, CenterY, PlaneZ));
	HostActor->SetActorRotation(FRotator::ZeroRotator);

	// Engine plane is 100x100 → scale so full extent matches discovery ortho square.
	const float Scale = FMath::Max(1.f, Ortho) / 100.f;
	FallbackMesh->SetRelativeScale3D(FVector(Scale, Scale, 1.f));
	FallbackMesh->SetRelativeLocation(FVector::ZeroVector);
	FallbackMesh->SetRelativeRotation(FRotator::ZeroRotator);

	if (FallbackMID && FogTex)
	{
		// Project material uses FogMask; Widget3D fallback uses SlateUI.
		// MapCenterOrtho drives world-space UVs (matches discovery + PP).
		FallbackMID->SetTextureParameterValue(TEXT("FogMask"), FogTex);
		FallbackMID->SetTextureParameterValue(TEXT("SlateUI"), FogTex);
		FallbackMID->SetVectorParameterValue(TEXT("MapCenterOrtho"), FLinearColor(CenterX, CenterY, Ortho, 1.f));
		const UMapDiscoveryComponent* Discovery = ResolveDiscovery();
		const FLinearColor FogColor = Discovery
			? Discovery->UndiscoveredColor
			: FLinearColor(0.f, 0.f, 0.f, 0.32f);
		FallbackMID->SetVectorParameterValue(TEXT("FogColor"), FogColor);
		FallbackMID->SetScalarParameterValue(TEXT("FogIntensity"), FogIntensity);
		FallbackMesh->SetMaterial(0, FallbackMID);
	}
}
