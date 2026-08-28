#include "CrystalCometActor.h"

#include "Components/PointLightComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

ACrystalCometActor::ACrystalCometActor()
{
	PrimaryActorTick.bCanEverTick = true;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere"));

	Core = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CometCore"));
	Core->SetupAttachment(SceneRoot);
	Core->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Core->SetCastShadow(false);
	Core->SetRelativeScale3D(FVector(0.7f));
	if (SphereMesh.Succeeded())
	{
		Core->SetStaticMesh(SphereMesh.Object);
	}

	for (int32 Index = 0; Index < 5; ++Index)
	{
		UStaticMeshComponent* Piece = CreateDefaultSubobject<UStaticMeshComponent>(
			*FString::Printf(TEXT("Trail_%d"), Index));
		Piece->SetupAttachment(SceneRoot);
		Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Piece->SetCastShadow(false);
		Piece->SetRelativeScale3D(FVector(0.45f - Index * 0.065f));
		if (SphereMesh.Succeeded())
		{
			Piece->SetStaticMesh(SphereMesh.Object);
		}
		TrailPieces.Add(Piece);
	}

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("CrystalGlow"));
	Glow->SetupAttachment(SceneRoot);
	Glow->SetLightColor(FLinearColor(0.08f, 0.65f, 1.f));
	Glow->SetIntensity(18000.f);
	Glow->SetAttenuationRadius(1100.f);
	Glow->SetCastShadows(false);
}

void ACrystalCometActor::BeginFall(const FVector& LandingLocation, float Height, float Duration,
	TSubclassOf<AActor> CrystalClass, TFunction<void(AActor*)> OnLanded)
{
	TargetLocation = LandingLocation;
	StartLocation = LandingLocation + FVector(-Height * 0.28f, Height * 0.18f, Height);
	FallDuration = FMath::Max(0.25f, Duration);
	PendingCrystalClass = CrystalClass;
	LandedCallback = MoveTemp(OnLanded);
	Elapsed = 0.f;
	bFalling = true;
	SetActorLocation(StartLocation);
}

void ACrystalCometActor::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bFalling)
	{
		Elapsed += DeltaSeconds;
		const float Alpha = FMath::Clamp(Elapsed / FallDuration, 0.f, 1.f);
		const float Eased = FMath::InterpEaseIn(0.f, 1.f, Alpha, 1.7f);
		const FVector Location = FMath::Lerp(StartLocation, TargetLocation, Eased);
		SetActorLocation(Location);

		const FVector TailDirection = (StartLocation - TargetLocation).GetSafeNormal();
		for (int32 Index = 0; Index < TrailPieces.Num(); ++Index)
		{
			TrailPieces[Index]->SetWorldLocation(Location + TailDirection * (115.f + Index * 95.f));
		}

		if (Alpha >= 1.f)
		{
			Land();
		}
	}
	else if (bImpact)
	{
		ImpactElapsed += DeltaSeconds;
		const float Alpha = FMath::Clamp(ImpactElapsed / 0.4f, 0.f, 1.f);
		Core->SetWorldScale3D(FVector(FMath::Lerp(0.7f, 4.0f, Alpha)));
		Glow->SetIntensity(FMath::Lerp(30000.f, 0.f, Alpha));
		if (Alpha >= 1.f)
		{
			Destroy();
		}
	}
}

void ACrystalCometActor::Land()
{
	bFalling = false;
	bImpact = true;
	ImpactElapsed = 0.f;
	for (UStaticMeshComponent* Piece : TrailPieces)
	{
		Piece->SetVisibility(false);
	}

	AActor* Crystal = nullptr;
	if (PendingCrystalClass && GetWorld())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Crystal = GetWorld()->SpawnActor<AActor>(PendingCrystalClass, TargetLocation, FRotator::ZeroRotator, Params);
	}
	if (LandedCallback)
	{
		LandedCallback(Crystal);
	}
}
