#include "AbilityTerrainProjectionComponent.h"

#include "Components/DecalComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Actor.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FAbilityAimProjectionYawTest,
	"TD.Abilities.AimProjectionPreservesYaw",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FAbilityAimProjectionYawTest::RunTest(const FString& Parameters)
{
	const FRotator ProjectionRotation =
		UAbilityTerrainProjectionComponent::CalculateProjectionRotation(135.f, true);

	TestEqual(TEXT("Aim projection stays vertical"), ProjectionRotation.Pitch, -90.0);
	TestEqual(TEXT("Aim decal plane aligns with the marker forward axis"), ProjectionRotation.Yaw, 45.0);
	TestEqual(TEXT("Aim projection has no roll"), ProjectionRotation.Roll, 0.0);
	return true;
}
#endif

UAbilityTerrainProjectionComponent::UAbilityTerrainProjectionComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	PrimaryComponentTick.TickGroup = TG_PostUpdateWork;
}

void UAbilityTerrainProjectionComponent::BeginPlay()
{
	Super::BeginPlay();
	ResolveComponents();
	SyncIndicator(RangeDriver, RangeDecal, false);
	SyncIndicator(AimDriver, AimDecal, true);
}

void UAbilityTerrainProjectionComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!RangeDriver || !AimDriver || !RangeDecal || !AimDecal)
	{
		ResolveComponents();
	}

	SyncIndicator(RangeDriver, RangeDecal, false);
	SyncIndicator(AimDriver, AimDecal, true);
}

FRotator UAbilityTerrainProjectionComponent::CalculateProjectionRotation(
	float DriverYaw, bool bPreserveYaw)
{
	const float ProjectionYaw = bPreserveYaw ? DriverYaw - 90.f : 0.f;
	return FRotator(-90.f, ProjectionYaw, 0.f);
}

void UAbilityTerrainProjectionComponent::ResolveComponents()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TInlineComponentArray<UStaticMeshComponent*> Meshes(Owner);
	for (UStaticMeshComponent* Mesh : Meshes)
	{
		if (Mesh->GetFName() == TEXT("RangeRing"))
		{
			RangeDriver = Mesh;
		}
		else if (Mesh->GetFName() == TEXT("AimMarker"))
		{
			AimDriver = Mesh;
		}
	}

	TInlineComponentArray<UDecalComponent*> Decals(Owner);
	for (UDecalComponent* Decal : Decals)
	{
		if (Decal->GetFName() == TEXT("RangeTerrainDecal"))
		{
			RangeDecal = Decal;
		}
		else if (Decal->GetFName() == TEXT("AimTerrainDecal"))
		{
			AimDecal = Decal;
		}
	}
}

void UAbilityTerrainProjectionComponent::SyncIndicator(
	UStaticMeshComponent* Driver,
	UDecalComponent* Decal,
	bool bPreserveYaw) const
{
	if (!Driver || !Decal)
	{
		return;
	}

	const FVector DriverScale = Driver->GetComponentScale();
	Decal->SetWorldLocation(Driver->GetComponentLocation());
	Decal->SetWorldRotation(CalculateProjectionRotation(
		Driver->GetComponentRotation().Yaw, bPreserveYaw));
	Decal->SetWorldScale3D(FVector::OneVector);
	Decal->DecalSize = FVector(
		ProjectionDepth,
		BaseIndicatorRadius * FMath::Abs(DriverScale.X),
		BaseIndicatorRadius * FMath::Abs(DriverScale.Y));
	Decal->SetVisibility(Driver->IsVisible());
	Decal->SetHiddenInGame(Driver->bHiddenInGame);
	Decal->MarkRenderStateDirty();
}
