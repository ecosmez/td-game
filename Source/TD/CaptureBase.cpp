#include "CaptureBase.h"

#include "CaptureChannelWidget.h"
#include "MapDiscoveryComponent.h"
#include "MapFogWorldSubsystem.h"
#include "MobaPlayerController.h"
#include "TDEnemyPathLibrary.h"

#include "Blueprint/UserWidget.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UnrealType.h"

namespace CaptureBasePrivate
{
	const FSoftClassPath TowerClassPath(TEXT("/Game/TD/BP_Tower.BP_Tower_C"));
	const FSoftClassPath EnemyClassPath(TEXT("/Game/TD/BP_Enemy.BP_Enemy_C"));

	UClass* LoadActorClass(const FSoftClassPath& Path)
	{
		return Path.TryLoadClass<AActor>();
	}
}

ACaptureBase::ACaptureBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(SceneRoot);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(
		TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	Beacon = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Beacon"));
	Beacon->SetupAttachment(SceneRoot);
	Beacon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Beacon->SetCanEverAffectNavigation(false);
	Beacon->SetRelativeScale3D(FVector(1.4f, 1.4f, 0.35f));
	if (CylinderMesh.Succeeded())
	{
		Beacon->SetStaticMesh(CylinderMesh.Object);
	}

	ChannelBar = CreateDefaultSubobject<UWidgetComponent>(TEXT("ChannelBar"));
	ChannelBar->SetupAttachment(SceneRoot);
	ChannelBar->SetWidgetSpace(EWidgetSpace::Screen);
	ChannelBar->SetDrawAtDesiredSize(false);
	ChannelBar->SetDrawSize(FVector2D(220.f, 22.f));
	ChannelBar->SetPivot(FVector2D(0.5f, 1.f));
	ChannelBar->SetRelativeLocation(FVector(0.f, 0.f, 220.f));
	ChannelBar->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ChannelBar->SetWidgetClass(UCaptureChannelWidget::StaticClass());
	ChannelBar->SetVisibility(false);
}

void ACaptureBase::BeginPlay()
{
	Super::BeginPlay();
	HideAllPads();
	bInert = !ValidatePads();
	if (bInert)
	{
		UE_LOG(LogTemp, Error,
			TEXT("CaptureBase %s has invalid pad assignment and will stay inert."), *GetName());
	}
	PreviousStarterTowersAlive = 0;

	if (ChannelBar)
	{
		if (UCaptureChannelWidget* Widget = CreateWidget<UCaptureChannelWidget>(
			GetWorld(), UCaptureChannelWidget::StaticClass()))
		{
			ChannelBar->SetWidget(Widget);
		}
	}

	UTDEnemyPathLibrary::ApplyLaneDecorationCollision(this);
	for (AActor* Pad : StarterPads)
	{
		UTDEnemyPathLibrary::ApplyLaneDecorationCollision(Pad);
	}
	for (AActor* Pad : ExtraPads)
	{
		UTDEnemyPathLibrary::ApplyLaneDecorationCollision(Pad);
	}
}

void ACaptureBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (bInert)
	{
		return;
	}

	const int32 Starters = CountStarterTowers();
	AActor* Champion = ResolveChampion();
	FTDCaptureBaseInput Input;
	Input.bChampionInRadius = Champion
		&& FTDCaptureBaseLogic::IsInsideRadius(
			GetActorLocation(), Champion->GetActorLocation(), CaptureRadius);
	Input.EnemiesInRadius = CountLivingEnemiesInRadius();
	Input.DeltaSeconds = DeltaSeconds;
	Input.ChannelDuration = ChannelDuration;
	Input.StarterTowersAlive = Starters;
	Input.PreviousStarterTowersAlive = PreviousStarterTowersAlive;
	Input.ChannelProgress = LastState.ChannelProgress;
	Input.bHeld = LastState.bHeld;
	Input.bExtraPadsUnlocked = LastState.bExtraPadsUnlocked;
	Input.bChannelCompletedThisLife = LastState.bChannelCompletedThisLife;

	LastState = FTDCaptureBaseLogic::Step(Input);
	PreviousStarterTowersAlive = Starters;

	for (AActor* Pad : StarterPads)
	{
		ApplyPadPresentation(Pad, true, FindOccupyingTower(Pad) != nullptr, LastState);
	}
	for (AActor* Pad : ExtraPads)
	{
		ApplyPadPresentation(Pad, false, FindOccupyingTower(Pad) != nullptr, LastState);
	}

	ApplyExtraTowerPower(LastState.bExtraTowersPowered);
	ApplyVision(LastState);
	UpdateChannelBar(LastState);
}

bool ACaptureBase::IsPadBuildable(const AActor* Pad) const
{
	if (bInert || !Pad)
	{
		return false;
	}

	const bool bOccupied = FindOccupyingTower(Pad) != nullptr;
	bool bStarter = false;
	bool bExtra = false;
	for (const TObjectPtr<AActor>& Entry : StarterPads)
	{
		if (Entry.Get() == Pad)
		{
			bStarter = true;
			break;
		}
	}
	for (const TObjectPtr<AActor>& Entry : ExtraPads)
	{
		if (Entry.Get() == Pad)
		{
			bExtra = true;
			break;
		}
	}
	if (!bStarter && !bExtra)
	{
		return true;
	}
	return FTDCaptureBaseLogic::IsPadBuildable(bStarter, bOccupied, LastState);
}

bool ACaptureBase::ValidatePads()
{
	TSet<const AActor*> Unique;
	bool bAnyNull = false;
	for (AActor* Pad : StarterPads)
	{
		if (!IsValid(Pad))
		{
			bAnyNull = true;
			continue;
		}
		Unique.Add(Pad);
	}
	for (AActor* Pad : ExtraPads)
	{
		if (!IsValid(Pad))
		{
			bAnyNull = true;
			continue;
		}
		Unique.Add(Pad);
	}
	return FTDCaptureBaseLogic::ArePadAssignmentsValid(
		StarterPads.Num(), ExtraPads.Num(), Unique.Num(), bAnyNull);
}

void ACaptureBase::HideAllPads()
{
	auto Hide = [](AActor* Pad)
	{
		if (!IsValid(Pad))
		{
			return;
		}
		Pad->SetActorHiddenInGame(true);
		Pad->SetActorEnableCollision(false);
	};
	for (AActor* Pad : StarterPads)
	{
		Hide(Pad);
	}
	for (AActor* Pad : ExtraPads)
	{
		Hide(Pad);
	}
}

void ACaptureBase::ApplyPadPresentation(
	AActor* Pad,
	bool bIsStarter,
	bool bOccupied,
	const FTDCaptureBaseOutput& State)
{
	if (!IsValid(Pad))
	{
		return;
	}

	const bool bBuildable = FTDCaptureBaseLogic::IsPadBuildable(bIsStarter, bOccupied, State);
	const bool bVisible = bIsStarter
		? (!bOccupied && State.bStarterPadsVisible)
		: State.bExtraPadsVisible;
	Pad->SetActorHiddenInGame(!bVisible);
	Pad->SetActorEnableCollision(bBuildable);
	UTDEnemyPathLibrary::ApplyLaneDecorationCollision(Pad);
}

int32 ACaptureBase::CountLivingEnemiesInRadius() const
{
	UWorld* World = GetWorld();
	UClass* EnemyClass = CaptureBasePrivate::LoadActorClass(CaptureBasePrivate::EnemyClassPath);
	if (!World || !EnemyClass)
	{
		return 0;
	}

	int32 Count = 0;
	for (TActorIterator<AActor> It(World, EnemyClass); It; ++It)
	{
		AActor* Enemy = *It;
		if (!IsLivingActor(Enemy))
		{
			continue;
		}
		if (FTDCaptureBaseLogic::IsInsideRadius(GetActorLocation(), Enemy->GetActorLocation(), CaptureRadius))
		{
			++Count;
		}
	}
	return Count;
}

AActor* ACaptureBase::ResolveChampion() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	APlayerController* PC = World->GetFirstPlayerController();
	if (const AMobaPlayerController* Moba = Cast<AMobaPlayerController>(PC))
	{
		if (APawn* Champ = Moba->GetControlledChampion())
		{
			return Champ;
		}
	}
	return PC ? PC->GetPawn() : nullptr;
}

AActor* ACaptureBase::FindOccupyingTower(const AActor* Pad) const
{
	UWorld* World = GetWorld();
	UClass* TowerClass = CaptureBasePrivate::LoadActorClass(CaptureBasePrivate::TowerClassPath);
	if (!World || !TowerClass || !IsValid(Pad))
	{
		return nullptr;
	}

	AActor* Best = nullptr;
	float BestDist = OccupancyRadius;
	for (TActorIterator<AActor> It(World, TowerClass); It; ++It)
	{
		AActor* Tower = *It;
		if (!IsLivingActor(Tower) || IsGhostTower(Tower))
		{
			continue;
		}
		const float Dist = FVector::Dist2D(Pad->GetActorLocation(), Tower->GetActorLocation());
		if (Dist <= BestDist)
		{
			BestDist = Dist;
			Best = Tower;
		}
	}
	return Best;
}

int32 ACaptureBase::CountStarterTowers() const
{
	int32 Count = 0;
	for (AActor* Pad : StarterPads)
	{
		if (FindOccupyingTower(Pad))
		{
			++Count;
		}
	}
	return Count;
}

void ACaptureBase::ApplyExtraTowerPower(bool bPowered)
{
	for (AActor* Pad : ExtraPads)
	{
		AActor* Tower = FindOccupyingTower(Pad);
		if (!IsValid(Tower))
		{
			continue;
		}

		const TWeakObjectPtr<AActor> Key(Tower);
		if (!bPowered)
		{
			if (!SavedCanAttack.Contains(Key))
			{
				bool bCurrent = true;
				ReadCanAttack(Tower, bCurrent);
				SavedCanAttack.Add(Key, bCurrent);
			}
			WriteCanAttack(Tower, false);
		}
		else if (const bool* Saved = SavedCanAttack.Find(Key))
		{
			WriteCanAttack(Tower, *Saved);
			SavedCanAttack.Remove(Key);
		}
	}

	for (auto It = SavedCanAttack.CreateIterator(); It; ++It)
	{
		if (!It.Key().IsValid())
		{
			It.RemoveCurrent();
		}
	}
}

void ACaptureBase::ApplyVision(const FTDCaptureBaseOutput& State)
{
	UMapDiscoveryComponent* Discovery = FindDiscovery();
	if (!State.bHeld)
	{
		if (bVisionRegistered && Discovery)
		{
			Discovery->UnregisterVisionSource(this);
		}
		bVisionRegistered = false;
		return;
	}

	if (!bVisionRegistered && Discovery)
	{
		Discovery->RegisterVisionSource(this, VisionRadius);
		bVisionRegistered = true;
	}
}

UMapDiscoveryComponent* ACaptureBase::FindDiscovery() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	if (const UMapFogWorldSubsystem* Fog = World->GetSubsystem<UMapFogWorldSubsystem>())
	{
		if (UMapDiscoveryComponent* Bound = Fog->GetBoundDiscovery())
		{
			return Bound;
		}
	}

	if (APlayerController* PC = World->GetFirstPlayerController())
	{
		return PC->FindComponentByClass<UMapDiscoveryComponent>();
	}
	return nullptr;
}

void ACaptureBase::UpdateChannelBar(const FTDCaptureBaseOutput& State)
{
	if (!ChannelBar)
	{
		return;
	}

	ChannelBar->SetVisibility(State.bShowChannelBar);
	ChannelBar->SetHiddenInGame(!State.bShowChannelBar);
	if (!State.bShowChannelBar)
	{
		return;
	}

	if (UCaptureChannelWidget* Widget = Cast<UCaptureChannelWidget>(ChannelBar->GetWidget()))
	{
		Widget->SetProgress(State.ChannelProgress);
	}
}

bool ACaptureBase::IsLivingActor(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return false;
	}

	if (const FFloatProperty* Health = FindFProperty<FFloatProperty>(Actor->GetClass(), TEXT("CurrentHealth")))
	{
		return Health->GetPropertyValue_InContainer(Actor) > KINDA_SMALL_NUMBER;
	}
	if (const FIntProperty* HealthInt = FindFProperty<FIntProperty>(Actor->GetClass(), TEXT("CurrentHealth")))
	{
		return HealthInt->GetPropertyValue_InContainer(Actor) > 0;
	}
	return true;
}

bool ACaptureBase::IsGhostTower(const AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return true;
	}
	if (const FBoolProperty* Ghost = FindFProperty<FBoolProperty>(Actor->GetClass(), TEXT("IsGhost")))
	{
		return Ghost->GetPropertyValue_InContainer(Actor);
	}
	return false;
}

bool ACaptureBase::ReadCanAttack(const AActor* Tower, bool& OutValue)
{
	if (!IsValid(Tower))
	{
		return false;
	}
	if (const FBoolProperty* Prop = FindFProperty<FBoolProperty>(Tower->GetClass(), TEXT("CanAttack")))
	{
		OutValue = Prop->GetPropertyValue_InContainer(Tower);
		return true;
	}
	return false;
}

void ACaptureBase::WriteCanAttack(AActor* Tower, bool bValue)
{
	if (!IsValid(Tower))
	{
		return;
	}
	if (FBoolProperty* Prop = FindFProperty<FBoolProperty>(Tower->GetClass(), TEXT("CanAttack")))
	{
		Prop->SetPropertyValue_InContainer(Tower, bValue);
	}
}
