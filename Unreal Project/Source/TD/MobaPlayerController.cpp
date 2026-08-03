#include "MobaPlayerController.h"

#include "MobaCameraPawn.h"
#include "MinimapWidget.h"
#include "TDUIInputLibrary.h"

#include "AIController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "NavigationSystem.h"
#include "TimerManager.h"

AMobaPlayerController::AMobaPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	bEnableTouchEvents = false;
	DefaultMouseCursor = EMouseCursor::Default;
	PrimaryActorTick.bCanEverTick = true;

	CameraPawnClass = AMobaCameraPawn::StaticClass();
}

void AMobaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ApplyMobaInputMode();
	InitializeMobaCamera();
	if (bShowMinimap)
	{
		ShowMinimap();
	}
}

UMinimapWidget* AMobaPlayerController::ShowMinimap()
{
	if (MinimapWidget && IsValid(MinimapWidget))
	{
		if (!MinimapWidget->IsInViewport())
		{
			MinimapWidget->AddToViewport(20);
		}
		return MinimapWidget;
	}

	UClass* WidgetClass = MinimapWidgetClass
		? MinimapWidgetClass.Get()
		: UMinimapWidget::StaticClass();

	MinimapWidget = CreateWidget<UMinimapWidget>(this, WidgetClass);
	if (MinimapWidget)
	{
		// Wider bounds by default for greybox maps; free camera will overwrite if constrained.
		MinimapWidget->SetWorldBounds(FVector2D(-12000.f, -12000.f), FVector2D(12000.f, 12000.f));
		if (AMobaCameraPawn* Cam = GetMobaCameraPawn())
		{
			if (Cam->bConstrainToWorldBounds)
			{
				MinimapWidget->SetWorldBounds(Cam->MinimumWorldBounds, Cam->MaximumWorldBounds);
			}
		}
		MinimapWidget->AddToViewport(20);
	}
	return MinimapWidget;
}

void AMobaPlayerController::HideMinimap()
{
	if (MinimapWidget)
	{
		MinimapWidget->RemoveFromParent();
		MinimapWidget = nullptr;
	}
}

void AMobaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	// Camera Enhanced Input binds on AMobaCameraPawn when possessed.
	// Mapping context is also pushed here so it works if the camera is view-target only.
	if (CameraMappingContext)
	{
		if (ULocalPlayer* LP = GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
			{
				Subsystem->AddMappingContext(CameraMappingContext, CameraMappingPriority);
			}
		}
	}
}

void AMobaPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (AMobaCameraPawn* Cam = Cast<AMobaCameraPawn>(InPawn))
	{
		CachedCameraPawn = Cam;
		// Champion may already be set; do not overwrite with the camera.
	}
	else
	{
		WireChampionFromPawn(InPawn);
		if (bPossessCameraPawn && bAutoSpawnCameraPawn)
		{
			// Deferred: champion was possessed first — switch to free camera.
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateUObject(
					this, &AMobaPlayerController::InitializeMobaCamera));
			}
		}
	}
}

void AMobaPlayerController::OnUnPossess()
{
	Super::OnUnPossess();
}

void AMobaPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AMobaPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// Stop 3D actor click-through (hex pads, world OnClicked) when the cursor is over the store/HUD.
	// UMG buttons still work via Slate; they do not need bEnableClickEvents.
	const bool bBlockWorld = ShouldBlockWorldClickInput();
	bEnableClickEvents = !bBlockWorld;

	HandleClickToMoveChampion();
}

void AMobaPlayerController::HandleClickToMoveChampion()
{
	if (!bEnableClickToMoveChampion || ShouldBlockWorldClickInput())
	{
		return;
	}

	// Right mouse = move champion. LMB stays free for store / abilities.
	if (!WasInputKeyJustPressed(EKeys::RightMouseButton))
	{
		return;
	}

	APawn* Champion = GetControlledChampion();
	if (!Champion)
	{
		return;
	}

	FHitResult Hit;
	const bool bHit = GetHitResultUnderCursor(ClickMoveTraceChannel, true, Hit);
	if (!bHit || !Hit.bBlockingHit)
	{
		return;
	}

	// Ignore invalid / very high hits (sky / empty).
	if (Hit.ImpactPoint.ContainsNaN())
	{
		return;
	}

	MoveChampionToLocation(Hit.ImpactPoint);
}

void AMobaPlayerController::EnsureChampionHasAIController(APawn* Champion)
{
	if (!Champion || !GetWorld())
	{
		return;
	}

	if (Cast<AAIController>(Champion->GetController()))
	{
		return;
	}

	// Unpossessed pawn needs an AI controller for SimpleMoveToLocation.
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = Champion;
	AAIController* AIC = GetWorld()->SpawnActor<AAIController>(
		AAIController::StaticClass(), Champion->GetActorLocation(), FRotator::ZeroRotator, Params);
	if (AIC)
	{
		AIC->Possess(Champion);
	}
}

void AMobaPlayerController::MoveChampionToLocation(const FVector& WorldLocation)
{
	APawn* Champion = GetControlledChampion();
	if (!Champion)
	{
		return;
	}

	EnsureChampionHasAIController(Champion);
	UAIBlueprintHelperLibrary::SimpleMoveToLocation(Champion->GetController(), WorldLocation);
}

void AMobaPlayerController::WireChampionFromPawn(APawn* InPawn)
{
	if (InPawn && !Cast<AMobaCameraPawn>(InPawn))
	{
		ControlledChampion = InPawn;
	}
}

void AMobaPlayerController::InitializeMobaCamera()
{
	// Capture champion if we still possess a non-camera pawn.
	if (APawn* Current = GetPawn())
	{
		if (!Cast<AMobaCameraPawn>(Current))
		{
			SetControlledChampion(Current);
		}
		else
		{
			CachedCameraPawn = Cast<AMobaCameraPawn>(Current);
		}
	}

	// Spawn champion when not already set (free-camera GameMode path).
	if (!GetControlledChampion() && ChampionClass && GetWorld())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		const FVector SpawnLoc = FVector::ZeroVector;
		if (APawn* SpawnedChamp = GetWorld()->SpawnActor<APawn>(ChampionClass, SpawnLoc, FRotator::ZeroRotator, Params))
		{
			SetControlledChampion(SpawnedChamp);
			EnsureChampionHasAIController(SpawnedChamp);
		}
	}

	// Try to find existing camera in the world.
	if (!CachedCameraPawn)
	{
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<AMobaCameraPawn> It(World); It; ++It)
			{
				CachedCameraPawn = *It;
				break;
			}
		}
	}

	if (bAutoSpawnCameraPawn)
	{
		GetOrSpawnCameraPawn();
	}

	if (AMobaCameraPawn* Cam = CachedCameraPawn.Get())
	{
		if (bSnapCameraToChampionOnStart)
		{
			if (APawn* Champ = GetControlledChampion())
			{
				FVector Loc = Champ->GetActorLocation();
				if (CameraPivotHeight != 0.0f)
				{
					Loc.Z = CameraPivotHeight;
				}
				Cam->SetActorLocation(Loc);
			}
		}

		if (bPossessCameraPawn && GetPawn() != Cam)
		{
			// Keep champion alive; after Possess, give it AI for pathfinding.
			APawn* Champ = GetControlledChampion();
			Possess(Cam);
			if (Champ)
			{
				EnsureChampionHasAIController(Champ);
			}
		}
		else if (!bPossessCameraPawn)
		{
			SetViewTarget(Cam);
		}
	}

	ApplyMobaInputMode();
}

APawn* AMobaPlayerController::GetControlledChampion() const
{
	if (ControlledChampion)
	{
		return ControlledChampion;
	}
	// Fallback if champion is still the possessed pawn (camera-as-view-target mode).
	if (APawn* P = GetPawn())
	{
		if (!Cast<AMobaCameraPawn>(P))
		{
			return P;
		}
	}
	return nullptr;
}

void AMobaPlayerController::SetControlledChampion(APawn* NewChampion)
{
	ControlledChampion = NewChampion;
	if (NewChampion)
	{
		EnsureChampionHasAIController(NewChampion);
	}
}

AMobaCameraPawn* AMobaPlayerController::GetMobaCameraPawn() const
{
	if (CachedCameraPawn)
	{
		return CachedCameraPawn;
	}
	return Cast<AMobaCameraPawn>(GetPawn());
}

AMobaCameraPawn* AMobaPlayerController::GetOrSpawnCameraPawn()
{
	if (CachedCameraPawn)
	{
		return CachedCameraPawn;
	}

	if (AMobaCameraPawn* Existing = Cast<AMobaCameraPawn>(GetPawn()))
	{
		CachedCameraPawn = Existing;
		return Existing;
	}

	if (!bAutoSpawnCameraPawn || !GetWorld())
	{
		return nullptr;
	}

	UClass* ClassToSpawn = CameraPawnClass ? CameraPawnClass.Get() : AMobaCameraPawn::StaticClass();
	FActorSpawnParameters Params;
	Params.Owner = this;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	FVector SpawnLoc = FVector::ZeroVector;
	if (APawn* Champ = GetControlledChampion())
	{
		SpawnLoc = Champ->GetActorLocation();
	}
	else if (APawn* P = GetPawn())
	{
		SpawnLoc = P->GetActorLocation();
	}

	if (CameraPivotHeight != 0.0f)
	{
		SpawnLoc.Z = CameraPivotHeight;
	}

	AMobaCameraPawn* Spawned = GetWorld()->SpawnActor<AMobaCameraPawn>(
		ClassToSpawn, SpawnLoc, FRotator::ZeroRotator, Params);
	CachedCameraPawn = Spawned;
	return Spawned;
}

void AMobaPlayerController::ApplyMobaInputMode()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;

	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	// Null focus widget: game + any viewport UI (store, HUD) receive mouse events.
	SetInputMode(Mode);
}

bool AMobaPlayerController::ShouldBlockWorldClickInput() const
{
	return UTDUIInputLibrary::ShouldBlockWorldClickInput(this, bBlockCameraInputOverUI);
}

bool AMobaPlayerController::ShouldBlockCameraInputForUI() const
{
	// Edge-scroll / drag pan: same policy as world clicks.
	return ShouldBlockWorldClickInput();
}
