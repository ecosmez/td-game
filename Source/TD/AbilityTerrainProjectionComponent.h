#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AbilityTerrainProjectionComponent.generated.h"

class UDecalComponent;
class UStaticMeshComponent;
class UClass;

/** Keeps ability-preview decals aligned without inheriting the driver's non-uniform Z scale. */
UCLASS(ClassGroup=(TD), meta=(BlueprintSpawnableComponent))
class TD_API UAbilityTerrainProjectionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAbilityTerrainProjectionComponent();
	static FRotator CalculateProjectionRotation(float DriverYaw, bool bPreserveYaw);

	/** True only for Landscape/LandscapeStreamingProxy actor classes. */
	static bool IsValidTerrainClass(const UClass* ActorClass);

	/** Shared Blueprint validation used by preview and ability confirmation. */
	UFUNCTION(BlueprintPure, Category="TD|Abilities", meta=(DisplayName="Is Valid Ability Terrain Actor"))
	static bool IsValidAbilityTerrainActor(const AActor* Actor);

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	void ResolveComponents();
	void UpdateCursorTerrainValidity();
	void SyncIndicator(UStaticMeshComponent* Driver, UDecalComponent* Decal, bool bPreserveYaw) const;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> RangeDriver = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> AimDriver = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> RangeDecal = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UDecalComponent> AimDecal = nullptr;

	bool bCursorOnValidTerrain = false;

	static constexpr float BaseIndicatorRadius = 50.f;
	static constexpr float ProjectionDepth = 2000.f;
};
