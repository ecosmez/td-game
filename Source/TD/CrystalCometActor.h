#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CrystalCometActor.generated.h"

class UPointLightComponent;
class USceneComponent;
class UStaticMeshComponent;

UCLASS(NotBlueprintable)
class TD_API ACrystalCometActor : public AActor
{
	GENERATED_BODY()

public:
	ACrystalCometActor();
	virtual void Tick(float DeltaSeconds) override;

	void BeginFall(const FVector& LandingLocation, float Height, float Duration,
		TSubclassOf<AActor> CrystalClass, TFunction<void(AActor*)> OnLanded);

private:
	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY()
	TObjectPtr<UStaticMeshComponent> Core;

	UPROPERTY()
	TArray<TObjectPtr<UStaticMeshComponent>> TrailPieces;

	UPROPERTY()
	TObjectPtr<UPointLightComponent> Glow;

	FVector StartLocation = FVector::ZeroVector;
	FVector TargetLocation = FVector::ZeroVector;
	float FallDuration = 2.5f;
	float Elapsed = 0.f;
	float ImpactElapsed = 0.f;
	bool bFalling = false;
	bool bImpact = false;
	TSubclassOf<AActor> PendingCrystalClass;
	TFunction<void(AActor*)> LandedCallback;

	void Land();
};
