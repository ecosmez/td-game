#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbilityTerrainProjectionComponent.generated.h"

class UDecalComponent;
class UStaticMeshComponent;

/** Keeps ability-preview decals aligned without inheriting the driver's non-uniform Z scale. */
UCLASS(ClassGroup=(TD), meta=(BlueprintSpawnableComponent))
class TD_API UAbilityTerrainProjectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAbilityTerrainProjectionComponent();
	static FRotator CalculateProjectionRotation(float DriverYaw, bool bPreserveYaw);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void ResolveComponents();
	void SyncIndicator(UStaticMeshComponent* Driver, UDecalComponent* Decal, bool bPreserveYaw) const;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> RangeDriver = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> AimDriver = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> RangeDecal = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> AimDecal = nullptr;

	static constexpr float BaseIndicatorRadius = 50.f;
	static constexpr float ProjectionDepth = 2000.f;
};
