#include "MobaPlayerController.h"

#include "AbilityBarWidget.h"
#include "MobaCameraPawn.h"
#include "MinimapWidget.h"
#include "CameraOrbitGizmoWidget.h"
#include "MapDiscoveryComponent.h"
#include "WorldFogOfWarComponent.h"
#include "TDUIInputLibrary.h"
#include "TDEnemyPathLibrary.h"
#include "TDChampionClickMove.h"
#include "TDChampionDropLand.h"
#include "FloatingDamageTextWidget.h"

#include "AIController.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
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
#include "LandscapeProxy.h"
#include "Engine/StaticMeshActor.h"
#include "TimerManager.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"

namespace MobaSkipDropPrivate
{
	static void StripClickThroughOverlays(UWorld* World)
	{
		if (!World)
		{
			return;
		}

		UClass* const OverlayClasses[] = {
			FSoftClassPath(TEXT("/Game/TD/BP_Crystal.BP_Crystal_C")).TryLoadClass<AActor>(),
			FSoftClassPath(TEXT("/Game/TD/BP_AbilityAimPreview.BP_AbilityAimPreview_C")).TryLoadClass<AActor>(),
		};
		for (UClass* OverlayClass : OverlayClasses)
		{
			if (!OverlayClass)
			{
				continue;
			}
			for (TActorIterator<AActor> It(World, OverlayClass); It; ++It)
			{
				FTDChampionClickMove::StripActorTraceCollision(*It);
			}
		}

		// Terrain collision is configured and saved by setup_terrain_collision.py.
		// Reapplying it here invalidates every navigation tile when PIE begins.
		if (FTDChampionClickMove::ShouldConfigureEnvironmentCollisionAtRuntime())
		{
			for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
			{
				AStaticMeshActor* MeshActor = *It;
				if (!MeshActor || !FTDChampionClickMove::IsKitEnvironmentActorName(MeshActor->GetName()))
				{
					continue;
				}
				FTDChampionClickMove::UseComplexCollisionOnEnvironmentMesh(MeshActor);
			}
		}
	}

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

	MobaSkipDropPrivate::StripClickThroughOverlays(GetWorld());
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
		// Crystal (green) + first spawner (red) markers; crystal also grants live vision.
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
			// Above the minimap so overlapping corner clicks hit the gizmo first.
			CameraOrbitGizmoWidget->AddToViewport(21);
		}
		return CameraOrbitGizmoWidget;
	}

	UClass* WidgetClass = CameraOrbitGizmoWidgetClass
		? CameraOrbitGizmoWidgetClass.Get()
		: UCameraOrbitGizmoWidget::StaticClass();

	CameraOrbitGizmoWidget = CreateWidget<UCameraOrbitGizmoWidget>(this, WidgetClass);
	if (CameraOrbitGizmoWidget)
	{
		// Above the minimap so overlapping corner clicks hit the gizmo first.
		CameraOrbitGizmoWidget->AddToViewport(21);
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

	// Stop 3D actor click-through when the cursor is over the store/HUD.
	// Keep click events on while a tower is selected so pads can receive LMB place.
	const bool bBlockWorld = ShouldBlockWorldClickInput();
	bEnableClickEvents = !bBlockWorld;

	HandleSkipSkyDropInput();
	HandleToggleFogOfWarInput();
	HandleToggleStoreInput();
	HandleClickToMoveChampion();
	UpdateChampionAttack(DeltaTime);
	UpdateChampionDamageTaken();
	UpdateFloatingDamageTexts(DeltaTime);
	UpdateMoveDestinationIndicator(DeltaTime);
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

void AMobaPlayerController::HandleToggleStoreInput()
{
	const bool bPrimaryPressed = ToggleStoreKeyPrimary.IsValid() && WasInputKeyJustPressed(ToggleStoreKeyPrimary);
	const bool bSecondaryPressed = ToggleStoreKeySecondary.IsValid() && WasInputKeyJustPressed(ToggleStoreKeySecondary);
	if (!bPrimaryPressed && !bSecondaryPressed)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TObjectIterator<UAbilityBarWidget> It; It; ++It)
	{
		UAbilityBarWidget* AbilityBar = *It;
		if (IsValid(AbilityBar) && AbilityBar->GetWorld() == World)
		{
			AbilityBar->ToggleStore();
			return;
		}
	}
}

void AMobaPlayerController::ToggleWorldFogOfWar()
{
	SetWorldFogOfWarEnabled(!bEnableWorldFogOfWar);
}

void AMobaPlayerController::SetWorldFogOfWarEnabled(bool bEnabled)
{
	bEnableWorldFogOfWar = bEnabled;
	ApplyFogOfWarVisualState();

	UE_LOG(LogTemp, Log, TEXT("Fog of War: %s"), bEnableWorldFogOfWar ? TEXT("ON") : TEXT("OFF"));
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

	auto IsWalkableGroundHit = [](const FHitResult& Hit) -> bool
	{
		if (!Hit.bBlockingHit)
		{
			return false;
		}
		AActor* HitActor = Hit.GetActor();
		if (!HitActor)
		{
			return true;
		}
		return !FTDChampionClickMove::IsClickThroughActorName(
			HitActor->GetName(),
			HitActor->GetClass()->GetName());
	};

	FHitResult Hit;
	bool bHit = false;
	TArray<FHitResult> Hits;
	FCollisionObjectQueryParams GroundObjects;
	GroundObjects.AddObjectTypesToQuery(ECC_WorldStatic);
	World->LineTraceMultiByObjectType(Hits, Loc, TraceEnd, GroundObjects, Params);
	for (const FHitResult& Candidate : Hits)
	{
		if (IsWalkableGroundHit(Candidate))
		{
			Hit = Candidate;
			bHit = true;
			break;
		}
	}
	if (!bHit)
	{
		Hits.Reset();
		World->LineTraceMultiByChannel(Hits, Loc, TraceEnd, ECC_Visibility, Params);
		for (const FHitResult& Candidate : Hits)
		{
			if (IsWalkableGroundHit(Candidate))
			{
				Hit = Candidate;
				bHit = true;
				break;
			}
		}
	}

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
			// Safety net: BP ShowAbilityHUD Create Widget is often broken after WBP recreate.
			UTDUIInputLibrary::CreateAndShowAbilityBar(this, this, 100);
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
	UTDUIInputLibrary::CreateAndShowAbilityBar(this, this, 100);

	UE_LOG(LogTemp, Log, TEXT("Sky drop skipped (Enter)"));
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
		// Ensure LoL QWER bar appears even if BP ShowAbilityHUD Create Widget is broken.
		if (bWasChampionDropping && !bDropping)
		{
			UTDUIInputLibrary::CreateAndShowAbilityBar(this, this, 100);
		}
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
	if (!bEnableClickToMoveChampion || bDropMode || UTDUIInputLibrary::ShouldBlockChampionClickToMove(this, bBlockCameraInputOverUI))
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
	AActor* AttackTarget = nullptr;
	if (!TraceChampionClick(Champion, Hit, AttackTarget))
	{
		return;
	}

	if (Hit.ImpactPoint.ContainsNaN())
	{
		return;
	}

	if (AttackTarget)
	{
		BeginChampionAttack(Champion, AttackTarget);
		return;
	}

	MoveChampionToLocation(Hit.ImpactPoint);
}

void AMobaPlayerController::BeginChampionAttack(APawn* Champion, AActor* Target)
{
	if (!Champion || !Target)
	{
		return;
	}

	StopDirectMove();
	ChampionAttackTarget = Target;
	ChampionAttackCooldownRemaining = 0.0f;
	ChampionAttackRepathCooldown = 0.0f;
	ChampionAttackLastChaseTarget = Target->GetActorLocation();

	IssueChampionNavMove(Champion, ChampionAttackLastChaseTarget);
}

void AMobaPlayerController::StopChampionAttack()
{
	if (AActor* Target = ChampionAttackTarget.Get())
	{
		UTDEnemyPathLibrary::SetEnemyPathHeld(Target, false);
	}
	ChampionAttackTarget.Reset();
	ChampionAttackCooldownRemaining = 0.0f;
	ChampionAttackRepathCooldown = 0.0f;
}

void AMobaPlayerController::UpdateChampionAttack(float DeltaTime)
{
	if (!ChampionAttackTarget.IsValid())
	{
		return;
	}

	APawn* Champion = GetControlledChampion();
	AActor* Target = ChampionAttackTarget.Get();
	if (!Champion || !UTDEnemyPathLibrary::IsAttackableEnemy(Target) || MobaSkipDropPrivate::IsPawnDropping(Champion))
	{
		StopChampionAttack();
		return;
	}

	if (bEnableWorldFogOfWar && bEnableMapDiscovery && MapDiscovery
		&& !MapDiscovery->IsLocationVisible(Target->GetActorLocation()))
	{
		StopChampionAttack();
		return;
	}

	const FVector ChampionLoc = Champion->GetActorLocation();
	const FVector TargetLoc = Target->GetActorLocation();
	const float Dist = FVector::Dist2D(ChampionLoc, TargetLoc);

	// Ground ring under the locked target so it's obvious what the champion is attacking.
	// Single-frame draw (LifeTime -1, not persistent) re-issued every tick it's targeted.
	float TargetRadius = 60.0f;
	float TargetHalfHeight = 90.0f;
	Target->GetSimpleCollisionCylinder(TargetRadius, TargetHalfHeight);
	const FVector RingCenter = TargetLoc - FVector(0.0f, 0.0f, TargetHalfHeight - 8.0f);
	DrawDebugCircle(GetWorld(), RingCenter, TargetRadius + 40.0f, 32, FColor::Red, false, -1.0f, 0,
		4.0f, FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), false);

	if (Dist > ChampionAttackRange)
	{
		// Not in melee range (yet) - let it keep walking its lane instead of freezing it
		// out of range, and re-issue the chase move only when the target has moved enough
		// or on a short throttle, so we don't spam SimpleMoveToLocation / repath every frame.
		UTDEnemyPathLibrary::SetEnemyPathHeld(Target, false);
		ChampionAttackRepathCooldown -= DeltaTime;
		const bool bTargetMoved = FVector::DistSquared(TargetLoc, ChampionAttackLastChaseTarget) > FMath::Square(50.0f);
		if (ChampionAttackRepathCooldown <= 0.0f || bTargetMoved)
		{
			ChampionAttackRepathCooldown = 0.5f;
			ChampionAttackLastChaseTarget = TargetLoc;
			IssueChampionNavMove(Champion, TargetLoc);
		}
		return;
	}

	// In range: stop moving, face the target, and tick the attack cooldown. Freeze the
	// target's own path movement too - otherwise it keeps teleporting back onto its lane
	// every tick while overlapping the now-stationary champion, and physics de-penetration
	// shoving it back out each frame reads as the enemy glitching/flickering in place.
	UTDEnemyPathLibrary::SetEnemyPathHeld(Target, true);
	AbortChampionPathFollowing(Champion);

	FRotator FaceRotation = (TargetLoc - ChampionLoc).GetSafeNormal().Rotation();
	FaceRotation.Pitch = 0.0f;
	FaceRotation.Roll = 0.0f;
	Champion->SetActorRotation(FaceRotation);

	ChampionAttackCooldownRemaining -= DeltaTime;
	if (ChampionAttackCooldownRemaining <= 0.0f)
	{
		UTDEnemyPathLibrary::ApplyDamageToEnemy(Target, ChampionAttackDamage);
		SpawnFloatingDamageText(TargetLoc + FVector(0.0f, 0.0f, TargetHalfHeight * 1.6f), ChampionAttackDamage, OutgoingDamageColor);
		ChampionAttackCooldownRemaining = ChampionAttackInterval;
	}
}

void AMobaPlayerController::UpdateChampionDamageTaken()
{
	APawn* Champion = GetControlledChampion();
	if (!Champion)
	{
		LastKnownChampionHealth = -1.0f;
		LastKnownChampionForHealth.Reset();
		return;
	}

	// Champion swapped (respawn/repossess) - reseed the baseline instead of diffing
	// against a stale pawn's health.
	if (LastKnownChampionForHealth.Get() != Champion)
	{
		LastKnownChampionForHealth = Champion;
		LastKnownChampionHealth = -1.0f;
	}

	float CurrentHealth = 0.0f;
	if (!MobaSkipDropPrivate::ReadFloatProp(Champion, FName(TEXT("CurrentHealth")), CurrentHealth))
	{
		return;
	}

	if (LastKnownChampionHealth >= 0.0f && CurrentHealth < LastKnownChampionHealth - KINDA_SMALL_NUMBER)
	{
		const float DamageTaken = LastKnownChampionHealth - CurrentHealth;
		float Radius = 40.0f;
		float HalfHeight = 90.0f;
		Champion->GetSimpleCollisionCylinder(Radius, HalfHeight);
		SpawnFloatingDamageText(Champion->GetActorLocation() + FVector(0.0f, 0.0f, HalfHeight * 1.6f), DamageTaken, IncomingDamageColor);
	}

	LastKnownChampionHealth = CurrentHealth;
}

void AMobaPlayerController::SpawnFloatingDamageText(const FVector& WorldLocation, float Amount, const FLinearColor& Color)
{
	UFloatingDamageTextWidget* DamageWidget = CreateWidget<UFloatingDamageTextWidget>(this, UFloatingDamageTextWidget::StaticClass());
	if (!DamageWidget)
	{
		return;
	}

	DamageWidget->SetDamageText(Amount, Color);
	DamageWidget->AddToViewport(200);

	FTDFloatingDamageEntry& Entry = FloatingDamageEntries.AddDefaulted_GetRef();
	Entry.Widget = DamageWidget;
	Entry.WorldLocation = WorldLocation;
	Entry.Duration = FMath::Max(FloatingDamageDuration, 0.1f);
}

void AMobaPlayerController::UpdateFloatingDamageTexts(float DeltaTime)
{
	for (int32 Index = FloatingDamageEntries.Num() - 1; Index >= 0; --Index)
	{
		FTDFloatingDamageEntry& Entry = FloatingDamageEntries[Index];
		Entry.Elapsed += DeltaTime;

		UFloatingDamageTextWidget* Widget = Entry.Widget;
		if (!IsValid(Widget) || Entry.Elapsed >= Entry.Duration)
		{
			if (IsValid(Widget))
			{
				Widget->RemoveFromParent();
			}
			FloatingDamageEntries.RemoveAtSwap(Index);
			continue;
		}

		const float Alpha = FMath::Clamp(Entry.Elapsed / Entry.Duration, 0.0f, 1.0f);
		const FVector DriftedLocation = Entry.WorldLocation + FVector(0.0f, 0.0f, FloatingDamageRiseSpeed * Entry.Elapsed);

		FVector2D ScreenPos;
		if (ProjectWorldLocationToScreen(DriftedLocation, ScreenPos))
		{
			const FVector2D HalfSize = Widget->GetDesiredSize() * 0.5f;
			Widget->SetPositionInViewport(ScreenPos - HalfSize, true);

			// Hold at full opacity, then fade out over the back half of the lifetime.
			constexpr float FadeOutStart = 0.55f;
			const float Opacity = Alpha <= FadeOutStart ? 1.0f : 1.0f - ((Alpha - FadeOutStart) / (1.0f - FadeOutStart));
			Widget->SetRenderOpacity(FMath::Clamp(Opacity, 0.0f, 1.0f));
		}
	}
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

	StopChampionAttack();
	EnsureChampionHasAIController(Champion);

	FVector Projected = WorldLocation;
	const bool bHasProjection = ProjectClickToNavMesh(Champion, WorldLocation, Projected);
	const bool bNavOk = HasCompleteNavPathTo(Champion, WorldLocation);
	const ETDChampionGroundMoveMode Mode = FTDChampionClickMove::ChooseMoveMode(
		bNavOk,
		bHasProjection,
		Champion->GetActorLocation().Z,
		WorldLocation.Z,
		CliffDropFallbackZ);
	const FVector Dest = FTDChampionClickMove::ResolveMoveDestination(
		WorldLocation, Mode, bHasProjection, Projected);
	BeginMoveDestinationIndicator(Dest);

	if (Mode == ETDChampionGroundMoveMode::DirectXY)
	{
		StartDirectMoveTo(Dest);
		return;
	}

	StopDirectMove();
	IssueChampionNavMove(Champion, Dest);
}

void AMobaPlayerController::BeginMoveDestinationIndicator(const FVector& WorldLocation)
{
	if (!bShowMoveDestinationIndicator || MoveIndicatorDuration <= 0.0f)
	{
		bMoveIndicatorActive = false;
		return;
	}

	bMoveIndicatorActive = true;
	MoveIndicatorLocation = WorldLocation + FVector(0.0f, 0.0f, 5.0f);
	MoveIndicatorElapsed = 0.0f;
}

void AMobaPlayerController::UpdateMoveDestinationIndicator(float DeltaTime)
{
	if (!bMoveIndicatorActive || !GetWorld())
	{
		return;
	}

	float Scale = 1.0f;
	float Intensity = 0.0f;
	if (!FTDChampionClickMove::CalculateMoveIndicatorFrame(
		MoveIndicatorElapsed, MoveIndicatorDuration, Scale, Intensity))
	{
		bMoveIndicatorActive = false;
		return;
	}

	const FLinearColor FrameColor = MoveIndicatorColor * Intensity;
	const FColor DrawColor = FrameColor.ToFColor(true);
	const float Radius = MoveIndicatorRadius * Scale;
	DrawDebugCircle(GetWorld(), MoveIndicatorLocation, Radius, 40, DrawColor, false, -1.0f, 0,
		MoveIndicatorThickness, FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), false);
	DrawDebugCircle(GetWorld(), MoveIndicatorLocation, Radius * 0.72f, 40, DrawColor, false, -1.0f, 0,
		FMath::Max(1.0f, MoveIndicatorThickness * 0.55f),
		FVector(1.0f, 0.0f, 0.0f), FVector(0.0f, 1.0f, 0.0f), false);

	MoveIndicatorElapsed += FMath::Max(DeltaTime, 0.0f);
}

bool AMobaPlayerController::TraceChampionClick(APawn* Champion, FHitResult& OutHit, AActor*& OutAttackTarget) const
{
	OutAttackTarget = nullptr;

	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FVector WorldOrigin;
	FVector WorldDirection;
	if (!DeprojectMousePositionToWorld(WorldOrigin, WorldDirection) || WorldDirection.IsNearlyZero())
	{
		return false;
	}

	FCollisionQueryParams Params(SCENE_QUERY_STAT(ChampionClickMove), false);

	// Mountains and decorative kit meshes remain solid for pawn movement, but they
	// must not steal a map click from the Landscape behind them.
	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		AStaticMeshActor* MeshActor = *It;
		if (MeshActor && FTDChampionClickMove::IsKitEnvironmentActorName(MeshActor->GetName()))
		{
			Params.AddIgnoredActor(MeshActor);
		}
	}

	TArray<FHitResult> Hits;
	const FVector TraceEnd = WorldOrigin + WorldDirection * 100000.f;
	World->LineTraceMultiByChannel(Hits, WorldOrigin, TraceEnd, ClickMoveTraceChannel, Params);

	for (const FHitResult& Hit : Hits)
	{
		if (!Hit.bBlockingHit)
		{
			continue;
		}

		AActor* HitActor = Hit.GetActor();
		const bool bIsAttackableEnemy = HitActor && UTDEnemyPathLibrary::IsAttackableEnemy(HitActor);
		const bool bConcealMinions = bEnableWorldFogOfWar && bEnableMapDiscovery && MapDiscovery;
		const bool bEnemyVisible = !bIsAttackableEnemy
			|| !bConcealMinions
			|| MapDiscovery->IsLocationVisible(HitActor->GetActorLocation());

		const bool bClickThrough = HitActor && FTDChampionClickMove::IsClickThroughActorName(
			HitActor->GetName(),
			HitActor->GetClass()->GetName());
		const ETDChampionClickIntent Intent = FTDChampionClickMove::ClassifyHit(
			HitActor == Champion,
			bEnableChampionAttack,
			bIsAttackableEnemy,
			HitActor && HitActor->IsA<ALandscapeProxy>(),
			bEnemyVisible,
			bClickThrough);

		if (Intent == ETDChampionClickIntent::ContinueTrace)
		{
			continue;
		}
		if (Intent == ETDChampionClickIntent::IgnoreClick)
		{
			return false;
		}

		OutHit = Hit;
		if (Intent == ETDChampionClickIntent::Attack)
		{
			OutAttackTarget = HitActor;
		}
		return true;
	}

	return false;
}

bool AMobaPlayerController::ProjectClickToNavMesh(APawn* Champion, const FVector& Point, FVector& OutProjected) const
{
	if (!GetWorld())
	{
		return false;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys)
	{
		return false;
	}

	const FVector Extent(NavProjectHorizontalExtent, NavProjectHorizontalExtent, NavProjectVerticalExtent);
	FNavLocation NavLoc;
	const FNavAgentProperties* AgentProps = Champion ? &Champion->GetNavAgentPropertiesRef() : nullptr;
	if (!NavSys->ProjectPointToNavigation(Point, NavLoc, Extent, AgentProps))
	{
		return false;
	}

	OutProjected = NavLoc.Location;
	return true;
}

void AMobaPlayerController::IssueChampionNavMove(APawn* Champion, const FVector& Dest)
{
	if (!Champion)
	{
		return;
	}

	EnsureChampionHasAIController(Champion);
	if (AAIController* AIC = Cast<AAIController>(Champion->GetController()))
	{
		AIC->MoveToLocation(Dest, -1.f, true, true, true, true, {}, true);
		return;
	}

	UAIBlueprintHelperLibrary::SimpleMoveToLocation(Champion->GetController(), Dest);
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

AActor* AMobaPlayerController::FindMainCrystal() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	static const FSoftClassPath CrystalClassPath(TEXT("/Game/TD/BP_Crystal.BP_Crystal_C"));
	UClass* CrystalClass = CrystalClassPath.TryLoadClass<AActor>();
	if (!CrystalClass)
	{
		return nullptr;
	}

	AActor* First = nullptr;
	for (TActorIterator<AActor> It(World, CrystalClass); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor))
		{
			continue;
		}
		if (!First || Actor->GetName() < First->GetName())
		{
			First = Actor;
		}
	}
	return First;
}

bool AMobaPlayerController::TryGetChampionDropLocationNearMainCrystal(FVector& OutLocation) const
{
	if (!bLandChampionNearMainCrystal)
	{
		return false;
	}

	AActor* Crystal = FindMainCrystal();
	if (!Crystal)
	{
		return false;
	}

	AActor* Spawner = UTDEnemyPathLibrary::GetPrimaryWaveSpawner(this, nullptr);
	OutLocation = FTDChampionDropLand::ComputeLocationNearCrystal(
		Crystal->GetActorLocation(),
		Spawner != nullptr,
		Spawner ? Spawner->GetActorLocation() : FVector::ZeroVector,
		ChampionCrystalLandOffset);
	return true;
}

void AMobaPlayerController::PlaceChampionNearMainCrystal(APawn* Champion)
{
	if (!Champion)
	{
		return;
	}

	FVector LandLocation;
	if (!TryGetChampionDropLocationNearMainCrystal(LandLocation))
	{
		return;
	}

	FVector Loc = Champion->GetActorLocation();
	Loc.X = LandLocation.X;
	Loc.Y = LandLocation.Y;
	Champion->SetActorLocation(Loc, false, nullptr, ETeleportType::TeleportPhysics);
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

	if (APawn* ExistingChamp = GetControlledChampion())
	{
		PlaceChampionNearMainCrystal(ExistingChamp);
	}

	// Spawn champion when not already set (free-camera GameMode path).
	if (!GetControlledChampion() && ChampionClass && GetWorld())
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		Params.Owner = this;
		FVector SpawnLoc = FVector::ZeroVector;
		TryGetChampionDropLocationNearMainCrystal(SpawnLoc);
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
