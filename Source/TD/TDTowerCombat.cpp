#include "TDTowerCombat.h"

#include "CollisionQueryParams.h"
#include "Components/ActorComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TDCaptureBaseLogic.h"
#include "UObject/UnrealType.h"

bool FTDTowerCombat::IsTowerShotBlockerClassName(const FString& ClassName)
{
	if (ClassName.IsEmpty() || FTDCaptureBaseLogic::IsPadLikeClassName(ClassName))
	{
		return false;
	}
	if (ClassName.Contains(TEXT("Pad"))
		|| ClassName.Contains(TEXT("Store"))
		|| ClassName.Contains(TEXT("Widget")))
	{
		return false;
	}
	return ClassName.Contains(TEXT("BP_Tower"));
}

bool FTDTowerCombat::DoesHitBlockTowerShot(
	const FString& ClassName,
	bool bIsSelf,
	bool bIsGhost,
	bool bHasBuiltFlag,
	bool bIsBuilt)
{
	if (bIsSelf || bIsGhost)
	{
		return false;
	}
	if (!IsTowerShotBlockerClassName(ClassName))
	{
		return false;
	}
	if (bHasBuiltFlag && !bIsBuilt)
	{
		return false;
	}
	return true;
}

bool FTDTowerCombat::IsShotBlockedByHits(TArrayView<const FTDTowerShotHitInfo> Hits)
{
	for (const FTDTowerShotHitInfo& Hit : Hits)
	{
		if (DoesHitBlockTowerShot(
			Hit.ClassName,
			Hit.bIsSelf,
			Hit.bIsGhost,
			Hit.bHasBuiltFlag,
			Hit.bIsBuilt))
		{
			return true;
		}
	}
	return false;
}

namespace TDTowerCombatPrivate
{
	static AActor* ResolveFiringTower(const UObject* WorldContextObject)
	{
		if (AActor* Actor = Cast<AActor>(const_cast<UObject*>(WorldContextObject)))
		{
			return Actor;
		}
		if (const UActorComponent* Comp = Cast<UActorComponent>(WorldContextObject))
		{
			return Comp->GetOwner();
		}
		return nullptr;
	}

	static bool ReadBoolProperty(const AActor* Actor, const TCHAR* Name, bool& bOutFound)
	{
		bOutFound = false;
		if (!IsValid(Actor))
		{
			return false;
		}
		if (const FBoolProperty* Prop = FindFProperty<FBoolProperty>(Actor->GetClass(), Name))
		{
			bOutFound = true;
			return Prop->GetPropertyValue_InContainer(Actor);
		}
		return false;
	}

	static FTDTowerShotHitInfo MakeHitInfo(const AActor* HitActor, const AActor* FiringTower)
	{
		FTDTowerShotHitInfo Info;
		if (!IsValid(HitActor))
		{
			return Info;
		}
		Info.ClassName = HitActor->GetClass()->GetName();
		Info.bIsSelf = HitActor == FiringTower;
		bool bFoundGhost = false;
		Info.bIsGhost = ReadBoolProperty(HitActor, TEXT("IsGhost"), bFoundGhost);
		(void)bFoundGhost;
		bool bFoundBuilt = false;
		Info.bIsBuilt = ReadBoolProperty(HitActor, TEXT("IsBuilt"), bFoundBuilt);
		Info.bHasBuiltFlag = bFoundBuilt;
		if (!bFoundBuilt)
		{
			Info.bIsBuilt = true;
		}
		return Info;
	}
}

bool UTDTowerCombatLibrary::IsShotBlockedByOtherTower(
	const UObject* WorldContextObject,
	FVector Origin,
	FVector TargetLocation)
{
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return false;
	}

	AActor* FiringTower = TDTowerCombatPrivate::ResolveFiringTower(WorldContextObject);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDTowerShot), false, FiringTower);
	Params.bReturnPhysicalMaterial = false;

	TArray<FHitResult> Hits;
	World->LineTraceMultiByChannel(Hits, Origin, TargetLocation, ECC_Visibility, Params);

	TArray<FTDTowerShotHitInfo, TInlineAllocator<16>> Classified;
	Classified.Reserve(Hits.Num());
	for (const FHitResult& Hit : Hits)
	{
		Classified.Add(TDTowerCombatPrivate::MakeHitInfo(Hit.GetActor(), FiringTower));
	}
	return FTDTowerCombat::IsShotBlockedByHits(Classified);
}
