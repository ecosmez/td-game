#include "TDChampionClickMove.h"
#include "TDFogVision.h"

#include "AI/NavigationSystemBase.h"
#include "Components/PrimitiveComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "GameFramework/Actor.h"
#include "PhysicsEngine/BodySetup.h"

bool FTDChampionClickMove::IsClickThroughActorName(const FString& ActorName, const FString& ClassName)
{
	if (ActorName.Contains(TEXT("WorldFOW")) || ClassName.Contains(TEXT("WorldFOW")))
	{
		return true;
	}
	if (ClassName.Contains(TEXT("AbilityAimPreview")))
	{
		return true;
	}
	// BP_Crystal / CrystalComet visual hulls are oversized spheres above Landscape.
	if (ClassName.Contains(TEXT("Crystal")))
	{
		return true;
	}
	return false;
}

bool FTDChampionClickMove::IsKitEnvironmentActorName(const FString& ActorName)
{
	return !ActorName.Contains(TEXT("Sky"));
}

bool FTDChampionClickMove::ShouldConfigureEnvironmentCollisionAtRuntime()
{
	return false;
}

void FTDChampionClickMove::StripActorTraceCollision(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	TInlineComponentArray<UPrimitiveComponent*> Primitives(Actor);
	for (UPrimitiveComponent* Prim : Primitives)
	{
		if (!Prim)
		{
			continue;
		}
		Prim->SetCollisionProfileName(TEXT("NoCollision"));
		Prim->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Prim->SetCollisionResponseToAllChannels(ECR_Ignore);
		Prim->SetGenerateOverlapEvents(false);
		Prim->CanCharacterStepUpOn = ECB_No;
	}
}

void FTDChampionClickMove::UseComplexCollisionOnEnvironmentMesh(AActor* Actor)
{
	if (!Actor)
	{
		return;
	}

	TInlineComponentArray<UStaticMeshComponent*> Meshes(Actor);
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		if (!Mesh)
		{
			continue;
		}

		if (UBodySetup* BodySetup = Mesh->GetBodySetup())
		{
			if (BodySetup->CollisionTraceFlag != CTF_UseComplexAsSimple)
			{
				BodySetup->CollisionTraceFlag = CTF_UseComplexAsSimple;
				BodySetup->InvalidatePhysicsData();
				BodySetup->CreatePhysicsMeshes();
			}
		}

		Mesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Mesh->SetCollisionObjectType(ECC_WorldStatic);
		Mesh->SetCollisionResponseToAllChannels(ECR_Block);
		Mesh->SetGenerateOverlapEvents(false);
		Mesh->SetCanEverAffectNavigation(true);
		Mesh->RecreatePhysicsState();
		FNavigationSystem::UpdateComponentData(*Mesh);
	}
}

ETDChampionClickIntent FTDChampionClickMove::ClassifyHit(
	bool bHitActorIsChampion,
	bool bEnableAttack,
	bool bHitActorIsAttackableEnemy,
	bool bHitActorIsLandscape,
	bool bHitEnemyIsVisible,
	bool bHitActorIsClickThrough,
	bool bHitActorIsEnvironmentOccluder)
{
	if (bHitActorIsChampion || bHitActorIsClickThrough || bHitActorIsEnvironmentOccluder)
	{
		return ETDChampionClickIntent::ContinueTrace;
	}
	if (FTDFogVision::ShouldSkipClickThroughFoggedEnemy(bHitActorIsAttackableEnemy, bHitEnemyIsVisible))
	{
		return ETDChampionClickIntent::ContinueTrace;
	}
	if (bHitActorIsAttackableEnemy)
	{
		return bEnableAttack ? ETDChampionClickIntent::Attack : ETDChampionClickIntent::IgnoreClick;
	}
	// Landscape, hex pads, and kit meshes are all valid move impacts.
	(void)bHitActorIsLandscape;
	return ETDChampionClickIntent::MoveToHit;
}

ETDChampionGroundMoveMode FTDChampionClickMove::ChooseMoveMode(
	bool bHasCompleteNavPath,
	bool bHasNavProjection,
	float ChampionZ,
	float DestinationZ,
	float CliffDropFallbackZ)
{
	if (bHasCompleteNavPath)
	{
		return ETDChampionGroundMoveMode::NavMesh;
	}
	if ((ChampionZ - DestinationZ) >= CliffDropFallbackZ)
	{
		return ETDChampionGroundMoveMode::DirectXY;
	}
	if (bHasNavProjection)
	{
		return ETDChampionGroundMoveMode::NavMesh;
	}
	return ETDChampionGroundMoveMode::DirectXY;
}

FVector FTDChampionClickMove::ResolveMoveDestination(
	const FVector& ClickLocation,
	ETDChampionGroundMoveMode Mode,
	bool bHasNavProjection,
	const FVector& ProjectedNavLocation)
{
	if (Mode == ETDChampionGroundMoveMode::DirectXY)
	{
		return ClickLocation;
	}
	if (bHasNavProjection)
	{
		return ProjectedNavLocation;
	}
	return ClickLocation;
}

bool FTDChampionClickMove::CalculateMoveIndicatorFrame(
	float Elapsed,
	float Duration,
	float& OutScale,
	float& OutIntensity)
{
	if (Duration <= 0.0f || Elapsed < 0.0f || Elapsed >= Duration)
	{
		OutScale = 1.0f;
		OutIntensity = 0.0f;
		return false;
	}

	const float Alpha = FMath::Clamp(Elapsed / Duration, 0.0f, 1.0f);
	OutScale = 1.0f + (0.35f * FMath::Sin(Alpha * PI));
	OutIntensity = 1.0f - Alpha;
	return true;
}
