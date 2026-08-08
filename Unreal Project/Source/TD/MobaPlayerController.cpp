#include "MobaPlayerController.h"

#include "MobaCameraPawn.h"
#include "MinimapWidget.h"
#include "CameraOrbitGizmoWidget.h"
#include "MapDiscoveryComponent.h"
#include "WorldFogOfWarComponent.h"
#include "TDUIInputLibrary.h"

#include "AIController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/SpringArmComponent.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"

namespace MobaSkipDropPrivate
{
	static bool ReadBoolProp(const UObject* Obj, FName Name, bool& OutValue)
	{
		if (!Obj)
		{
			return false;
		}
		if (const FBoolProperty* Prop = FindFProperty<FBoolProperty>(Obj->GetClass(), Name))
		{
			OutValue = Prop->GetPropertyValue_InContainer(Obj);
			return true;
		}
		return false;
	}

	static bool WriteBoolProp(UObject* Obj, FName Name, bool Value)
	{
		if (!Obj)
		{
			return false;
		}
		if (FBoolProperty* Prop = FindFProperty<FBoolProperty>(Obj->GetClass(), Name))
		{
			Prop->SetPropertyValue_InContainer(Obj, Value);
			return true;
		}
		return false;
	}

	static bool ReadFloatProp(const UObject* Obj, FName Name, float& OutValue)
	{
		if (!Obj)
		{
			return false;
		}
		if (const FFloatProperty* Prop = FindFProperty<FFloatProperty>(Obj->GetClass(), Name))
		{
			OutValue = Prop->GetPropertyValue_InContainer(Obj);
			return true;
		}
		if (const FDoubleProperty* DProp = FindFProperty<FDoubleProperty>(Obj->GetClass(), Name))
		{
			OutValue = static_cast<float>(DProp->GetPropertyValue_InContainer(Obj));
			return true;
		}
		return false;
	}

	static bool IsPawnDropping(const APawn* Pawn)
	{
		if (!Pawn)
		{
			return false;
		}
		bool bDropping = false;
		if (ReadBoolProp(Pawn, FName(TEXT("bIsDropping")), bDropping)
			|| ReadBoolProp(Pawn, FName(TEXT("IsDropping")), bDropping)
			|| ReadBoolProp(Pawn, FName(TEXT("isDropping")), bDropping))
		{
			return bDropping;
		}
		return false;
	}

	static void CallNoArgFunction(UObject* Obj, FName FuncName)
	{
		if (!Obj)
		{
			return;
		}
		if (UFunction* Fn = Obj->FindFunction(FuncName))
		{
			if (Fn->NumParms == 0)
			{
				Obj->ProcessEvent(Fn, nullptr);
			}
		}
	}
}

AMobaPlayerController::AMobaPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = true;
	bEnableMouseOverEvents = true;
	bEnableTouchEvents = false;
	DefaultMouseCursor = EMouseCursor::Default;
	PrimaryActorTick.bCanEverTick = true;

	CameraPawnClass = AMobaCameraPawn::StaticClass();

	MapDiscovery = CreateDefaultSubobject<UMapDiscoveryComponent>(TEXT("MapDiscovery"));
	WorldFogOfWar = CreateDefaultSubobject<UWorldFogOfWarComponent>(TEXT("WorldFogOfWar"));
}

void AMobaPlayerController::BeginPlay()
{
	Super::BeginPlay();

	ApplyMobaInputMode();
	InitializeMobaCamera();

	if (MapDiscovery)
	{
		MapDiscovery->SetEnabled(bEnableMapDiscovery);
		// Only use camera bounds when authored (WorldBoundsSource). Default ±50k clamps
		// destroy mask UV precision and leave the stamp invisibly tiny / off explorer.
		if (AMobaCameraPawn* Cam = GetMobaCameraPawn())
		{
			if (Cam->bConstrainToWorldBounds && Cam->WorldBoundsSource)
			{
				MapDiscovery->SetWorldBounds(Cam->MinimumWorldBounds, Cam->MaximumWorldBounds);
			}
		}
		MapDiscovery->SetExplorer(GetControlledChampion());
	}
	if (WorldFogOfWar)
	{
		WorldFogOfWar->SetDiscoverySource(MapDiscovery);
	}
	ApplyFogOfWarVisualState();

	if (bShowMinimap)
	{
		ShowMinimap();
	}

	if (bShowCameraOrbitGizmo)
	{
		ShowCameraOrbitGizmo();
	}

	// After minimap auto-fit, seed discovery once if we never got an authored volume.
	if (MapDiscovery && MinimapWidget)
	{
		float Cx = 0.f, Cy = 0.f, Ortho = 1.f;
		MapDiscovery->GetOrthoWorldRect(Cx, Cy, Ortho);
		// Defaults are ±12k (Ortho 24k). Prefer minimap's tighter level fit for better stamp precision.
		if (Ortho >= 20000.f)
		{
			MapDiscovery->SetWorldBounds(MinimapWidget->WorldMin, MinimapWidget->WorldMax);
		}
		// Crystal (green) + first spawner (red): markers + permanent FOW clear.
		MinimapWidget->RefreshLandmarks();
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
		// Share map discovery mask with the minimap fog overlay.
		if (MapDiscovery)
		{
			MinimapWidget->SetDiscoverySource(MapDiscovery);
		}
		// Apply current FOW toggle / discovery flags to minimap fog visuals.
		ApplyFogOfWarVisualState();
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

UCameraOrbitGizmoWidget* AMobaPlayerController::ShowCameraOrbitGizmo()
{
	if (CameraOrbitGizmoWidget && IsValid(CameraOrbitGizmoWidget))
	{
		if (!CameraOrbitGizmoWidget->IsInViewport())
		{
			CameraOrbitGizmoWidget->AddToViewport(20);
		}
		return CameraOrbitGizmoWidget;
	}

	UClass* WidgetClass = CameraOrbitGizmoWidgetClass
		? CameraOrbitGizmoWidgetClass.Get()
		: UCameraOrbitGizmoWidget::StaticClass();

	CameraOrbitGizmoWidget = CreateWidget<UCameraOrbitGizmoWidget>(this, WidgetClass);
	if (CameraOrbitGizmoWidget)
	{
		CameraOrbitGizmoWidget->AddToViewport(20);
	}
	return CameraOrbitGizmoWidget;
}

void AMobaPlayerController::HideCameraOrbitGizmo()
{
	if (CameraOrbitGizmoWidget)
	{
		CameraOrbitGizmoWidget->RemoveFromParent();
		CameraOrbitGizmoWidget = nullptr;
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

	HandleSkipSkyDropInput();
	HandleToggleFogOfWarInput();
	HandleClickToMoveChampion();
	UpdateDirectMoveChampion(DeltaTime);
	UpdateSkyDropCamera(DeltaTime);
}

void AMobaPlayerController::HandleSkipSkyDropInput()
{
	if (!SkipSkyDropKey.IsValid() || !WasInputKeyJustPressed(SkipSkyDropKey))
	{
		return;
	}
	TrySkipSkyDrop();
}

void AMobaPlayerController::HandleToggleFogOfWarInput()
{
	if (!ToggleFogOfWarKey.IsValid() || !WasInputKeyJustPressed(ToggleFogOfWarKey))
	{
		return;
	}
	ToggleWorldFogOfWar();
}

void AMobaPlayerController::ToggleWorldFogOfWar()
{
	SetWorldFogOfWarEnabled(!bEnableWorldFogOfWar);
}

void AMobaPlayerController::SetWorldFogOfWarEnabled(bool bEnabled)
{
	bEnableWorldFogOfWar = bEnabled;
	ApplyFogOfWarVisualState();

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			99117,
			1.5f,
			bEnableWorldFogOfWar ? FColor::Orange : FColor::Green,
			bEnableWorldFogOfWar ? TEXT("Fog of War: ON") : TEXT("Fog of War: OFF"));
	}
}

void AMobaPlayerController::ApplyFogOfWarVisualState()
{
	const bool bShowWorldFog = bEnableWorldFogOfWar && bEnableMapDiscovery;
	if (WorldFogOfWar)
	{
		WorldFogOfWar->SetEnabled(bShowWorldFog);
	}

	// Minimap fog overlay only — do not call SetMapDiscoveryEnabled (that would
	// stop the shared discovery mask used by world FOW).
	if (MinimapWidget)
	{
		const bool bShowMinimapFog =
			bEnableWorldFogOfWar && bEnableMinimapDiscoveryFog && bEnableMapDiscovery;
		MinimapWidget->bMapDiscoveryEnabled = bShowMinimapFog;
	}
}

bool AMobaPlayerController::TrySkipSkyDrop()
{
	APawn* Champion = GetControlledChampion();
	if (!Champion)
	{
		return false;
	}

	// Prefer BP helper if EventGraph has SkipSkyDrop (snaps + CompleteDropLanding).
	if (UFunction* SkipFn = Champion->FindFunction(FName(TEXT("SkipSkyDrop"))))
	{
		if (SkipFn->NumParms == 0 && MobaSkipDropPrivate::IsPawnDropping(Champion))
		{
			Champion->ProcessEvent(SkipFn, nullptr);
			if (!MobaSkipDropPrivate::IsPawnDropping(Champion))
			{
				ExitDropCameraMode();
			}
			return true;
		}
	}

	if (!MobaSkipDropPrivate::IsPawnDropping(Champion))
	{
		return false;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FVector Loc = Champion->GetActorLocation();
	const FVector TraceEnd(Loc.X, Loc.Y, Loc.Z - 100000.f);

	FCollisionQueryParams Params(SCENE_QUERY_STAT(SkipSkyDrop), true, Champion);
	Params.AddIgnoredActor(Champion);

	FHitResult Hit;
	const bool bHit = World->LineTraceSingleByChannel(Hit, Loc, TraceEnd, ECC_Visibility, Params);

	float CapsuleHalfHeight = 96.f;
	if (const ACharacter* AsChar = Cast<ACharacter>(Champion))
	{
		if (const UCapsuleComponent* Cap = AsChar->GetCapsuleComponent())
		{
			CapsuleHalfHeight = Cap->GetScaledCapsuleHalfHeight();
		}
	}

	if (bHit && Hit.bBlockingHit)
	{
		const FVector LandLoc(Hit.ImpactPoint.X, Hit.ImpactPoint.Y, Hit.ImpactPoint.Z + CapsuleHalfHeight);
		Champion->SetActorLocation(LandLoc, false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (UFunction* CompleteFn = Champion->FindFunction(FName(TEXT("CompleteDropLanding"))))
	{
		if (CompleteFn->NumParms == 0)
		{
			Champion->ProcessEvent(CompleteFn, nullptr);
			// BP clears drop; force MOBA free cam handoff.
			ExitDropCameraMode();
			return true;
		}
	}

	// Fallback when BP landing helpers are missing: mirror OnLanded cleanup via reflection.
	MobaSkipDropPrivate::WriteBoolProp(Champion, FName(TEXT("bIsDropping")), false);
	MobaSkipDropPrivate::WriteBoolProp(Champion, FName(TEXT("IsDropping")), false);

	if (ACharacter* AsChar = Cast<ACharacter>(Champion))
	{
		if (UCharacterMovementComponent* Move = AsChar->GetCharacterMovement())
		{
			float Gravity = 1.f;
			float AirControl = 0.05f;
			MobaSkipDropPrivate::ReadFloatProp(Champion, FName(TEXT("DefaultGravityScale")), Gravity);
			MobaSkipDropPrivate::ReadFloatProp(Champion, FName(TEXT("DefaultAirControl")), AirControl);
			Move->GravityScale = Gravity;
			Move->AirControl = AirControl;
			Move->Velocity = FVector::ZeroVector;
		}
	}

	if (USpringArmComponent* Arm = Champion->FindComponentByClass<USpringArmComponent>())
	{
		float SavedArm = Arm->TargetArmLength;
		float SettlePitch = -55.f;
		MobaSkipDropPrivate::ReadFloatProp(Champion, FName(TEXT("SavedArmLength")), SavedArm);
		MobaSkipDropPrivate::ReadFloatProp(Champion, FName(TEXT("TopDownArmPitch")), SettlePitch);
		Arm->TargetArmLength = SavedArm;
		const FRotator Rel = Arm->GetRelativeRotation();
		Arm->SetRelativeRotation(FRotator(SettlePitch, Rel.Yaw, Rel.Roll));
	}

	ExitDropCameraMode();

	MobaSkipDropPrivate::CallNoArgFunction(Champion, FName(TEXT("ShowAbilityHUD")));

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(
			-1, 4.f, FColor::Cyan, TEXT("Sky drop skipped (Enter)"));
	}
	return true;
}

void AMobaPlayerController::SetDropMode(bool bInDropMode)
{
	bDropMode = bInDropMode;
	if (bInDropMode)
	{
		StopDirectMove();
		EnterDropCameraMode();
	}
	else if (bInDropCamera)
	{
		ExitDropCameraMode();
	}
}

void AMobaPlayerController::UpdateSkyDropCamera(float DeltaTime)
{
	if (!bUseWarzoneDropCamera)
	{
		return;
	}

	APawn* Champion = GetControlledChampion();
	const bool bDropping = MobaSkipDropPrivate::IsPawnDropping(Champion);

	if (bDropping)
	{
		if (!bInDropCamera)
		{
			EnterDropCameraMode();
		}
		else
		{
			ApplyWarzoneDropFraming(Champion, DeltaTime);
		}
	}
	else if (bInDropCamera || bWasChampionDropping)
	{
		// Natural land (OnLanded / CompleteDropLanding) or drop flag cleared.
		ExitDropCameraMode();
	}

	bWasChampionDropping = bDropping;
}

void AMobaPlayerController::EnterDropCameraMode()
{
	if (!bUseWarzoneDropCamera)
	{
		return;
	}

	APawn* Champion = GetControlledChampion();
	if (!Champion)
	{
		return;
	}

	bInDropCamera = true;
	bDropMode = true;
	bEnableClickToMoveChampion = false;
	StopDirectMove();

	// Free cam stays possessed for input setup; view switches to the champion's camera.
	ApplyWarzoneDropFraming(Champion, 0.f);
	SetViewTarget(Champion);

	// Soften free-cam Space during freefall (Space = plane/glide on the champion BP).
	if (AMobaCameraPawn* Cam = GetMobaCameraPawn())
	{
		Cam->SetLockedToChampion(false);
		Cam->SetFocusingChampion(false);
	}
}

void AMobaPlayerController::ApplyWarzoneDropFraming(APawn* Champion, float DeltaTime)
{
	if (!Champion)
	{
		return;
	}

	USpringArmComponent* Arm = Champion->FindComponentByClass<USpringArmComponent>();
	if (!Arm)
	{
		return;
	}

	// Pure top-down boom follows the player, but WORLD rotation is frozen.
	// Only the pawn/body may rotate — drop cam orientation never follows yaw.
	Arm->TargetArmLength = DropCameraArmLength;
	Arm->bDoCollisionTest = false;
	Arm->bEnableCameraLag = false;
	Arm->bEnableCameraRotationLag = false;
	Arm->bUsePawnControlRotation = false;
	Arm->bInheritPitch = false;
	Arm->bInheritYaw = false;
	Arm->bInheritRoll = false;
	Arm->SetUsingAbsoluteRotation(true);
	Arm->SocketOffset = FVector(0.f, 0.f, DropCameraSocketOffsetZ);
	Arm->TargetOffset = FVector::ZeroVector;

	// Fixed world orientation for the entire freefall / land-aim phase.
	Arm->SetWorldRotation(FRotator(DropCameraPitch, DropCameraYaw, 0.f));

	// Keep freefall view on the champion (mouse hits the ground via their camera).
	if (GetViewTarget() != Champion)
	{
		SetViewTarget(Champion);
	}
}

void AMobaPlayerController::ExitDropCameraMode()
{
	if (!bInDropCamera && !bDropMode)
	{
		// Still ensure free-cam view if somehow stuck on champion after land.
		if (GetViewTarget() && GetViewTarget() == GetControlledChampion())
		{
			SwitchViewToMobaCamera(true);
		}
		bDropMode = false;
		return;
	}

	bInDropCamera = false;
	bDropMode = false;
	bEnableClickToMoveChampion = true;

	// Restore spring arm to normal relative rotation so character cam is not left absolute.
	if (APawn* Champion = GetControlledChampion())
	{
		if (USpringArmComponent* Arm = Champion->FindComponentByClass<USpringArmComponent>())
		{
			Arm->SetUsingAbsoluteRotation(false);
			Arm->bEnableCameraLag = true;
		}
	}

	SwitchViewToMobaCamera(true);
}

void AMobaPlayerController::SwitchViewToMobaCamera(bool bBlend)
{
	AMobaCameraPawn* Cam = GetOrSpawnCameraPawn();
	if (!Cam)
	{
		return;
	}

	// Keep free cam possessed for Enhanced Input + WASD-off / mouse pan policy.
	if (bPossessCameraPawn && GetPawn() != Cam)
	{
		APawn* Champ = GetControlledChampion();
		Possess(Cam);
		if (Champ)
		{
			EnsureChampionHasAIController(Champ);
		}
	}

	Cam->RecenterOnChampion(true);
	Cam->SetLockedToChampion(true);

	const float Blend = bBlend ? DropToMobaCameraBlendTime : 0.f;
	if (Blend > KINDA_SMALL_NUMBER)
	{
		SetViewTargetWithBlend(Cam, Blend, VTBlend_EaseInOut, 2.f, false);
	}
	else
	{
		SetViewTarget(Cam);
	}

	ApplyMobaInputMode();
}

void AMobaPlayerController::HandleClickToMoveChampion()
{
	if (!bEnableClickToMoveChampion || bDropMode || ShouldBlockWorldClickInput())
	{
		return;
	}

	// Also suppress path orders while the champion is still falling/dropping.
	if (MobaSkipDropPrivate::IsPawnDropping(GetControlledChampion()))
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

	if (AAIController* ExistingAI = Cast<AAIController>(Champion->GetController()))
	{
		// Fix prior bug: AI Owner must not be the pawn (GetNetConnection recursion:
		// Pawn -> Controller -> Owner(Pawn) -> ...).
		if (ExistingAI->GetOwner() == Champion)
		{
			ExistingAI->SetOwner(this);
		}
		return;
	}

	// Still possessed by a player — attach AI only after free-cam repossess.
	if (Cast<APlayerController>(Champion->GetController()))
	{
		return;
	}

	// Unpossessed champion needs AI for SimpleMoveToLocation.
	// Owner = this PC (never the champion) to avoid GetNetConnection infinite recursion.
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	Params.Owner = this;
	Params.Instigator = Champion;
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

	const float DropZ = Champion->GetActorLocation().Z - WorldLocation.Z;
	const bool bDestinationLower = DropZ >= CliffDropFallbackZ;
	const bool bNavOk = HasCompleteNavPathTo(Champion, WorldLocation);

	if (bNavOk)
	{
		StopDirectMove();
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(Champion->GetController(), WorldLocation);
		return;
	}

	// No complete NavMesh path. If the click is lower (cliff / ledge), steer in XY
	// so CharacterMovement can walk off and fall. Same-height / higher clicks still
	// go through SimpleMove (partial path / nearest poly) as before.
	if (bDestinationLower)
	{
		StartDirectMoveTo(WorldLocation);
		return;
	}

	StopDirectMove();
	UAIBlueprintHelperLibrary::SimpleMoveToLocation(Champion->GetController(), WorldLocation);
}

bool AMobaPlayerController::HasCompleteNavPathTo(APawn* Champion, const FVector& Dest) const
{
	if (!Champion || !GetWorld())
	{
		return false;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
	{
		return false;
	}

	const ANavigationData* NavData = NavSys->GetNavDataForProps(Champion->GetNavAgentPropertiesRef(), Champion->GetNavAgentLocation());
	if (!NavData)
	{
		NavData = NavSys->GetDefaultNavDataInstance();
	}
	if (!NavData)
	{
		return false;
	}

	FPathFindingQuery Query(Champion, *NavData, Champion->GetNavAgentLocation(), Dest);
	Query.SetAllowPartialPaths(false);

	const FPathFindingResult Result = NavSys->FindPathSync(Query);
	return Result.IsSuccessful() && Result.Path.IsValid() && !Result.IsPartial();
}

void AMobaPlayerController::AbortChampionPathFollowing(APawn* Champion)
{
	if (!Champion)
	{
		return;
	}

	auto AbortOn = [](AActor* Target)
	{
		if (!Target)
		{
			return;
		}
		if (UPathFollowingComponent* PathFollow = Target->FindComponentByClass<UPathFollowingComponent>())
		{
			PathFollow->AbortMove(*Target, FPathFollowingResultFlags::UserAbort);
		}
	};

	AbortOn(Champion);
	AbortOn(Champion->GetController());
}

void AMobaPlayerController::StartDirectMoveTo(const FVector& WorldLocation)
{
	APawn* Champion = GetControlledChampion();
	AbortChampionPathFollowing(Champion);

	// Ensure walking off ledges is allowed for this order.
	if (ACharacter* Char = Cast<ACharacter>(Champion))
	{
		if (UCharacterMovementComponent* Move = Char->GetCharacterMovement())
		{
			Move->bCanWalkOffLedges = true;
		}
	}

	DirectMoveTarget = WorldLocation;
	bDirectMoveActive = true;
}

void AMobaPlayerController::StopDirectMove()
{
	bDirectMoveActive = false;
	DirectMoveTarget = FVector::ZeroVector;
}

void AMobaPlayerController::UpdateDirectMoveChampion(float DeltaTime)
{
	if (!bDirectMoveActive)
	{
		return;
	}

	if (bDropMode || !bEnableClickToMoveChampion)
	{
		StopDirectMove();
		return;
	}

	APawn* Champion = GetControlledChampion();
	if (!Champion || MobaSkipDropPrivate::IsPawnDropping(Champion))
	{
		StopDirectMove();
		return;
	}

	const FVector Loc = Champion->GetActorLocation();
	FVector Delta = DirectMoveTarget - Loc;
	Delta.Z = 0.0f;
	const float DistSq = Delta.SizeSquared();
	const float AcceptSq = DirectMoveAcceptanceRadius * DirectMoveAcceptanceRadius;
	if (DistSq <= AcceptSq)
	{
		StopDirectMove();
		return;
	}

	const FVector Dir = Delta.GetSafeNormal();
	if (Dir.IsNearlyZero())
	{
		StopDirectMove();
		return;
	}

	Champion->AddMovementInput(Dir, 1.0f);
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
	// Capture champion if we still possess a non-camera pawn (do not force AI yet).
	if (APawn* Current = GetPawn())
	{
		if (!Cast<AMobaCameraPawn>(Current))
		{
			ControlledChampion = Current;
			if (MapDiscovery)
			{
				MapDiscovery->SetExplorer(Current);
			}
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
		Params.Owner = this;
		const FVector SpawnLoc = FVector::ZeroVector;
		if (APawn* SpawnedChamp = GetWorld()->SpawnActor<APawn>(ChampionClass, SpawnLoc, FRotator::ZeroRotator, Params))
		{
			ControlledChampion = SpawnedChamp;
			if (MapDiscovery)
			{
				MapDiscovery->SetExplorer(SpawnedChamp);
			}
		}
	}

	// Try to find existing camera in the world.
	if (!CachedCameraPawn)
	{
		if (UWorld* World = GetWorld())
		{
			TActorIterator<AMobaCameraPawn> It(World);
			if (It)
			{
				CachedCameraPawn = *It;
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
				// Default sticky lock so drop + landing keep framing on the champion.
				Cam->SetLockedToChampion(true);
			}
		}

		if (bPossessCameraPawn && GetPawn() != Cam)
		{
			// Keep champion alive; after Possess free cam, give champion AI for pathfinding.
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
	else if (APawn* Champ = GetControlledChampion())
	{
		// No free camera — still ensure AI for click-to-move when unpossessed / default path.
		if (GetPawn() != Champ)
		{
			EnsureChampionHasAIController(Champ);
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
	if (MapDiscovery)
	{
		MapDiscovery->SetExplorer(NewChampion);
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
