#include "MobaCameraPawn.h"

#include "MobaPlayerController.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Volume.h"
#include "Components/BrushComponent.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/UnrealType.h"

AMobaCameraPawn::AMobaCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(Root);
	SpringArm->TargetArmLength = DefaultZoom;
	SpringArm->SetRelativeRotation(FRotator(CameraPitch, CameraYaw, 0.0f));
	SpringArm->bDoCollisionTest = false;
	SpringArm->bEnableCameraLag = true;
	SpringArm->CameraLagSpeed = 12.0f;
	SpringArm->bEnableCameraRotationLag = true;
	SpringArm->CameraRotationLagSpeed = 10.0f;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw = false;
	SpringArm->bInheritRoll = false;
	SpringArm->bUsePawnControlRotation = false;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;

	FloatingPawnMovement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("FloatingPawnMovement"));
	FloatingPawnMovement->UpdatedComponent = Root;
	FloatingPawnMovement->MaxSpeed = KeyboardMoveSpeed;
	FloatingPawnMovement->Acceleration = 12000.0f;
	FloatingPawnMovement->Deceleration = 12000.0f;
	FloatingPawnMovement->TurningBoost = 8.0f;
	// Planar motion is owned by Tick (ApplyVelocity / SnapToWorldXY). Disabling the
	// movement component plane constraint avoids forcing Z to the origin plane (Z=0),
	// which pulls the pivot under the floor.
	FloatingPawnMovement->bConstrainToPlane = false;

	AutoPossessPlayer = EAutoReceiveInput::Disabled;
}

void AMobaCameraPawn::BeginPlay()
{
	Super::BeginPlay();

	CurrentZoom = DefaultZoom;
	TargetZoom = DefaultZoom;
	CurrentYaw = CameraYaw;
	TargetYaw = CameraYaw;
	CachedPawnHeight = GetActorLocation().Z;

	if (SpringArm)
	{
		SpringArm->TargetArmLength = CurrentZoom;
		SpringArm->SetRelativeRotation(FRotator(CameraPitch, CurrentYaw, 0.0f));
	}

	RefreshBoundsFromSource();
	UpdatePlanarAxes();
	EnsureInputAssets();
}

void AMobaCameraPawn::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	AddMappingContext();
}

void AMobaCameraPawn::UnPossessed()
{
	RemoveMappingContext();
	Super::UnPossessed();
}

void AMobaCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	EnsureInputAssets();
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		BindEnhancedInput(EIC);
	}
	AddMappingContext();
}

void AMobaCameraPawn::EnsureInputAssets()
{
	// Prefer Content assets when already assigned / loadable.
	if (!MoveAction)
	{
		MoveAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/TD/Input/MobaCamera/IA_CameraMove.IA_CameraMove"));
	}
	if (!ZoomAction)
	{
		ZoomAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/TD/Input/MobaCamera/IA_CameraZoom.IA_CameraZoom"));
	}
	if (!RotateAction)
	{
		RotateAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/TD/Input/MobaCamera/IA_CameraRotate.IA_CameraRotate"));
	}
	if (!DragAction)
	{
		DragAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/TD/Input/MobaCamera/IA_CameraDrag.IA_CameraDrag"));
	}
	if (!FocusChampionAction)
	{
		FocusChampionAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/TD/Input/MobaCamera/IA_CameraFocusChampion.IA_CameraFocusChampion"));
	}
	if (!CameraMappingContext)
	{
		CameraMappingContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/TD/Input/MobaCamera/IMC_MobaCamera.IMC_MobaCamera"));
	}

	const bool bNeedRuntime =
		!MoveAction || !ZoomAction || !RotateAction || !DragAction || !FocusChampionAction || !CameraMappingContext;

	if (!bNeedRuntime)
	{
		return;
	}

	// Runtime fallbacks so the camera works even before Content assets are created.
	if (!MoveAction)
	{
		MoveAction = NewObject<UInputAction>(this, TEXT("IA_CameraMove_Runtime"), RF_Transient);
		MoveAction->ValueType = EInputActionValueType::Axis2D;
	}
	if (!ZoomAction)
	{
		ZoomAction = NewObject<UInputAction>(this, TEXT("IA_CameraZoom_Runtime"), RF_Transient);
		ZoomAction->ValueType = EInputActionValueType::Axis1D;
	}
	if (!RotateAction)
	{
		RotateAction = NewObject<UInputAction>(this, TEXT("IA_CameraRotate_Runtime"), RF_Transient);
		RotateAction->ValueType = EInputActionValueType::Axis1D;
	}
	if (!DragAction)
	{
		DragAction = NewObject<UInputAction>(this, TEXT("IA_CameraDrag_Runtime"), RF_Transient);
		DragAction->ValueType = EInputActionValueType::Boolean;
	}
	if (!FocusChampionAction)
	{
		FocusChampionAction = NewObject<UInputAction>(this, TEXT("IA_CameraFocusChampion_Runtime"), RF_Transient);
		FocusChampionAction->ValueType = EInputActionValueType::Boolean;
	}

	if (!CameraMappingContext)
	{
		CameraMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_MobaCamera_Runtime"), RF_Transient);

		// WASD (Axis2D: X=right, Y=forward)
		{
			FEnhancedActionKeyMapping& W = CameraMappingContext->MapKey(MoveAction, EKeys::W);
			if (UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(CameraMappingContext))
			{
				Swizzle->Order = EInputAxisSwizzle::YXZ; // W drives Y
				W.Modifiers.Add(Swizzle);
			}
			FEnhancedActionKeyMapping& S = CameraMappingContext->MapKey(MoveAction, EKeys::S);
			if (UInputModifierNegate* Neg = NewObject<UInputModifierNegate>(CameraMappingContext))
			{
				S.Modifiers.Add(Neg);
			}
			if (UInputModifierSwizzleAxis* Swizzle = NewObject<UInputModifierSwizzleAxis>(CameraMappingContext))
			{
				Swizzle->Order = EInputAxisSwizzle::YXZ;
				S.Modifiers.Add(Swizzle);
			}
			CameraMappingContext->MapKey(MoveAction, EKeys::D);
			FEnhancedActionKeyMapping& A = CameraMappingContext->MapKey(MoveAction, EKeys::A);
			if (UInputModifierNegate* Neg = NewObject<UInputModifierNegate>(CameraMappingContext))
			{
				A.Modifiers.Add(Neg);
			}
		}

		// Mouse wheel zoom
		CameraMappingContext->MapKey(ZoomAction, EKeys::MouseWheelAxis);

		// Q / E rotate (optional)
		{
			CameraMappingContext->MapKey(RotateAction, EKeys::E);
			FEnhancedActionKeyMapping& Q = CameraMappingContext->MapKey(RotateAction, EKeys::Q);
			if (UInputModifierNegate* Neg = NewObject<UInputModifierNegate>(CameraMappingContext))
			{
				Q.Modifiers.Add(Neg);
			}
		}

		CameraMappingContext->MapKey(DragAction, EKeys::MiddleMouseButton);
		CameraMappingContext->MapKey(FocusChampionAction, EKeys::SpaceBar);
	}
}

void AMobaCameraPawn::BindEnhancedInput(UEnhancedInputComponent* EIC)
{
	if (!EIC || bInputBound)
	{
		return;
	}

	if (MoveAction)
	{
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AMobaCameraPawn::OnMove);
		EIC->BindAction(MoveAction, ETriggerEvent::Completed, this, &AMobaCameraPawn::OnMove);
		EIC->BindAction(MoveAction, ETriggerEvent::Canceled, this, &AMobaCameraPawn::OnMove);
	}
	if (ZoomAction)
	{
		EIC->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AMobaCameraPawn::OnZoom);
	}
	if (RotateAction)
	{
		EIC->BindAction(RotateAction, ETriggerEvent::Triggered, this, &AMobaCameraPawn::OnRotate);
		EIC->BindAction(RotateAction, ETriggerEvent::Completed, this, &AMobaCameraPawn::OnRotate);
		EIC->BindAction(RotateAction, ETriggerEvent::Canceled, this, &AMobaCameraPawn::OnRotate);
	}
	if (DragAction)
	{
		EIC->BindAction(DragAction, ETriggerEvent::Started, this, &AMobaCameraPawn::OnDragStarted);
		EIC->BindAction(DragAction, ETriggerEvent::Completed, this, &AMobaCameraPawn::OnDragCompleted);
		EIC->BindAction(DragAction, ETriggerEvent::Canceled, this, &AMobaCameraPawn::OnDragCompleted);
	}
	if (FocusChampionAction)
	{
		EIC->BindAction(FocusChampionAction, ETriggerEvent::Started, this, &AMobaCameraPawn::OnFocusStarted);
		EIC->BindAction(FocusChampionAction, ETriggerEvent::Completed, this, &AMobaCameraPawn::OnFocusCompleted);
		EIC->BindAction(FocusChampionAction, ETriggerEvent::Canceled, this, &AMobaCameraPawn::OnFocusCompleted);
	}

	bInputBound = true;
}

void AMobaCameraPawn::AddMappingContext()
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}
	const ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP)
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
	{
		if (CameraMappingContext)
		{
			Subsystem->AddMappingContext(CameraMappingContext, MappingContextPriority);
		}
	}
}

void AMobaCameraPawn::RemoveMappingContext()
{
	const APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}
	const ULocalPlayer* LP = PC->GetLocalPlayer();
	if (!LP)
	{
		return;
	}
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
	{
		if (CameraMappingContext)
		{
			Subsystem->RemoveMappingContext(CameraMappingContext);
		}
	}
}

void AMobaCameraPawn::OnMove(const FInputActionValue& Value)
{
	if (!IsKeyboardCameraMoveEnabled() || ShouldBlockCameraInputForUI())
	{
		MoveInput = FVector2D::ZeroVector;
		return;
	}
	MoveInput = Value.Get<FVector2D>();
	if (bManualMovementCancelsFocus && !MoveInput.IsNearlyZero() && ShouldFollowChampion())
	{
		// Don't unlock while forcing follow during freefall.
		if (!(bFollowWhileChampionDropping && IsChampionDropping()))
		{
			CancelChampionFollow();
		}
	}
}

void AMobaCameraPawn::OnZoom(const FInputActionValue& Value)
{
	if (ShouldBlockCameraInputForUI())
	{
		return;
	}
	const float Wheel = Value.Get<float>();
	// Wheel up (positive) zooms in (shorter arm).
	TargetZoom = FMath::Clamp(TargetZoom - Wheel * ZoomSpeed, MinimumZoom, MaximumZoom);
}

void AMobaCameraPawn::OnRotate(const FInputActionValue& Value)
{
	if (!bEnableCameraRotation || !IsKeyboardCameraMoveEnabled() || ShouldBlockCameraInputForUI())
	{
		RotateInput = 0.0f;
		return;
	}
	RotateInput = Value.Get<float>();
}

void AMobaCameraPawn::OnDragStarted(const FInputActionValue& Value)
{
	if (ShouldBlockCameraInputForUI())
	{
		return;
	}

	bIsDragging = true;
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		bSavedCursorVisibility = PC->bShowMouseCursor;
		float MX = 0.0f;
		float MY = 0.0f;
		if (PC->GetMousePosition(MX, MY))
		{
			LastDragMousePos = FVector2D(MX, MY);
		}
		if (bHideCursorWhileDragging)
		{
			PC->bShowMouseCursor = false;
		}
	}
	if (bManualMovementCancelsFocus && ShouldFollowChampion()
		&& !(bFollowWhileChampionDropping && IsChampionDropping()))
	{
		CancelChampionFollow();
	}
}

void AMobaCameraPawn::OnDragCompleted(const FInputActionValue& Value)
{
	if (!bIsDragging)
	{
		return;
	}
	bIsDragging = false;
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->bShowMouseCursor = bSavedCursorVisibility || true;
		// Restore without warping: leave cursor where the OS has it.
	}
	LastDragMousePos = FVector2D::ZeroVector;
}

void AMobaCameraPawn::OnFocusStarted(const FInputActionValue& Value)
{
	if (ShouldBlockCameraInputForUI())
	{
		return;
	}
	// Space = sticky lock + hold-to-follow (matches LoL recenter + lock).
	bLockedToChampion = true;
	bFocusChampionHeld = true;
	CurrentVelocity = FVector::ZeroVector;
	DesiredVelocity = FVector::ZeroVector;
}

void AMobaCameraPawn::OnFocusCompleted(const FInputActionValue& Value)
{
	// Release hold only; sticky lock (bLockedToChampion) keeps following after land.
	bFocusChampionHeld = false;
}

void AMobaCameraPawn::SetFocusingChampion(bool bFocusing)
{
	bFocusChampionHeld = bFocusing;
	if (bFocusing)
	{
		bLockedToChampion = true;
		CurrentVelocity = FVector::ZeroVector;
		DesiredVelocity = FVector::ZeroVector;
	}
	else
	{
		bLockedToChampion = false;
	}
}

void AMobaCameraPawn::SetLockedToChampion(bool bLocked)
{
	bLockedToChampion = bLocked;
	if (bLocked)
	{
		bFocusChampionHeld = false;
		CurrentVelocity = FVector::ZeroVector;
		DesiredVelocity = FVector::ZeroVector;
	}
}

void AMobaCameraPawn::CancelChampionFollow()
{
	bLockedToChampion = false;
	bFocusChampionHeld = false;
}

bool AMobaCameraPawn::ShouldFollowChampion() const
{
	if (bFocusChampionHeld || bLockedToChampion)
	{
		return true;
	}
	return bFollowWhileChampionDropping && IsChampionDropping();
}

bool AMobaCameraPawn::IsChampionDropping() const
{
	const APawn* Champion = ResolveFocusChampion();
	if (!Champion)
	{
		return false;
	}
	static const FName Names[] = {
		FName(TEXT("bIsDropping")),
		FName(TEXT("IsDropping")),
		FName(TEXT("isDropping"))
	};
	for (const FName& Name : Names)
	{
		if (const FBoolProperty* BoolProp = FindFProperty<FBoolProperty>(Champion->GetClass(), Name))
		{
			return BoolProp->GetPropertyValue_InContainer(Champion);
		}
	}
	if (const AMobaPlayerController* MPC = GetMobaController())
	{
		return MPC->IsDropMode();
	}
	return false;
}

bool AMobaCameraPawn::IsKeyboardCameraMoveEnabled() const
{
	// Mouse + Space only once grounded (default). Optional during freefall if enabled on cam.
	if (IsChampionDropping())
	{
		return true;
	}
	return bEnableKeyboardCameraMoveWhenGrounded;
}

void AMobaCameraPawn::SetTargetZoom(float NewZoom)
{
	TargetZoom = FMath::Clamp(NewZoom, MinimumZoom, MaximumZoom);
}

void AMobaCameraPawn::SetOrbitYaw(float YawDegrees, bool bInstant)
{
	TargetYaw = YawDegrees;
	if (bInstant)
	{
		CurrentYaw = YawDegrees;
		if (SpringArm)
		{
			const FRotator Rel = SpringArm->GetRelativeRotation();
			SpringArm->SetRelativeRotation(FRotator(CameraPitch, CurrentYaw, Rel.Roll));
		}
		UpdatePlanarAxes();
	}
}

void AMobaCameraPawn::AddOrbitYawDelta(float DeltaDegrees)
{
	if (FMath::IsNearlyZero(DeltaDegrees))
	{
		return;
	}
	SetOrbitYaw(TargetYaw + DeltaDegrees, false);
}

void AMobaCameraPawn::RecenterOnChampion(bool bInstant)
{
	if (APawn* Champion = ResolveFocusChampion())
	{
		FVector Loc = Champion->GetActorLocation();
		Loc.Z = CachedPawnHeight;
		ConstrainLocation(Loc);
		bLockedToChampion = true;
		if (bInstant)
		{
			SetActorLocation(Loc);
			CurrentVelocity = FVector::ZeroVector;
			DesiredVelocity = FVector::ZeroVector;
			bFocusChampionHeld = false;
		}
		else
		{
			bFocusChampionHeld = true;
		}
	}
}

void AMobaCameraPawn::SnapToWorldXY(const FVector& WorldLocation)
{
	// Cancel champion lock so tick doesn't immediately pull the camera back.
	CancelChampionFollow();
	CurrentVelocity = FVector::ZeroVector;
	DesiredVelocity = FVector::ZeroVector;

	// Keep current height only — never inherit Z from the minimap click payload
	// (that Z is only for coordinate math and must not push the pivot underground).
	FVector Loc = GetActorLocation();
	Loc.X = WorldLocation.X;
	Loc.Y = WorldLocation.Y;
	if (CachedPawnHeight == 0.0f)
	{
		CachedPawnHeight = Loc.Z;
	}
	Loc.Z = CachedPawnHeight;
	ConstrainLocation(Loc);
	SetActorLocation(Loc, false, nullptr, ETeleportType::TeleportPhysics);

	if (FloatingPawnMovement)
	{
		FloatingPawnMovement->StopMovementImmediately();
		FloatingPawnMovement->Velocity = FVector::ZeroVector;
	}
}

void AMobaCameraPawn::RefreshBoundsFromSource()
{
	if (!WorldBoundsSource)
	{
		return;
	}

	FVector Origin;
	FVector Extent;
	WorldBoundsSource->GetActorBounds(false, Origin, Extent);
	if (const AVolume* Volume = Cast<AVolume>(WorldBoundsSource))
	{
		if (const UBrushComponent* Brush = Volume->GetBrushComponent())
		{
			const FBoxSphereBounds Box = Brush->CalcBounds(Brush->GetComponentTransform());
			Origin = Box.Origin;
			Extent = Box.BoxExtent;
		}
	}

	MinimumWorldBounds = FVector2D(Origin.X - Extent.X, Origin.Y - Extent.Y);
	MaximumWorldBounds = FVector2D(Origin.X + Extent.X, Origin.Y + Extent.Y);
}

void AMobaCameraPawn::UpdatePlanarAxes()
{
	const FRotator YawRotation(0.0f, CurrentYaw, 0.0f);
	ForwardPlanar = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	RightPlanar = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
}

float AMobaCameraPawn::GetZoomSpeedScale() const
{
	if (!bScaleMovementWithZoom)
	{
		return 1.0f;
	}
	const float Span = FMath::Max(1.0f, MaximumZoom - MinimumZoom);
	const float Alpha = FMath::Clamp((CurrentZoom - MinimumZoom) / Span, 0.0f, 1.0f);
	// Slightly faster when zoomed out (0.85x near min, 1.25x near max).
	return FMath::Lerp(0.85f, 1.25f, Alpha);
}

void AMobaCameraPawn::UpdateKeyboardMove(float DeltaTime)
{
	if (!IsKeyboardCameraMoveEnabled() || MoveInput.IsNearlyZero())
	{
		return;
	}

	FVector Dir = ForwardPlanar * MoveInput.Y + RightPlanar * MoveInput.X;
	if (!Dir.Normalize())
	{
		return;
	}

	DesiredVelocity += Dir * (KeyboardMoveSpeed * GetZoomSpeedScale());
}

void AMobaCameraPawn::UpdateEdgeScroll(float DeltaTime)
{
	if (!bEnableEdgeScrolling || bIsDragging || ShouldFollowChampion())
	{
		return;
	}
	if (!HasMouseFocus() || ShouldBlockCameraInputForUI())
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	int32 SizeX = 0;
	int32 SizeY = 0;
	PC->GetViewportSize(SizeX, SizeY);
	if (SizeX <= 0 || SizeY <= 0)
	{
		return;
	}

	const float Threshold = FMath::Max(1.0f, EdgeScrollThreshold);
	FVector2D EdgeDir = FVector2D::ZeroVector;

	if (MouseX <= Threshold)
	{
		EdgeDir.X -= 1.0f;
	}
	else if (MouseX >= static_cast<float>(SizeX) - Threshold)
	{
		EdgeDir.X += 1.0f;
	}

	// Top of screen = forward (negative mouse Y from top-left origin).
	if (MouseY <= Threshold)
	{
		EdgeDir.Y += 1.0f;
	}
	else if (MouseY >= static_cast<float>(SizeY) - Threshold)
	{
		EdgeDir.Y -= 1.0f;
	}

	if (EdgeDir.IsNearlyZero())
	{
		return;
	}

	EdgeDir.Normalize();
	const FVector WorldDir = (RightPlanar * EdgeDir.X + ForwardPlanar * EdgeDir.Y).GetSafeNormal();
	DesiredVelocity += WorldDir * (EdgeScrollSpeed * GetZoomSpeedScale());
}

void AMobaCameraPawn::UpdateMiddleMouseDrag(float DeltaTime)
{
	if (!bIsDragging)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!PC)
	{
		return;
	}

	float MouseX = 0.0f;
	float MouseY = 0.0f;
	if (!PC->GetMousePosition(MouseX, MouseY))
	{
		return;
	}

	const FVector2D MousePos(MouseX, MouseY);
	const FVector2D MouseDelta = MousePos - LastDragMousePos;
	LastDragMousePos = MousePos;

	if (MouseDelta.IsNearlyZero())
	{
		return;
	}

	// Opposite to mouse movement, planar.
	const FVector DragDirection =
		RightPlanar * (-MouseDelta.X) + ForwardPlanar * MouseDelta.Y;

	// Drag is applied as position offset (not velocity) for responsive LoL-like panning.
	// Accumulate into velocity using scale that remains frame-rate friendly.
	const float Scale = DragMoveSpeed * 100.0f; // px -> cm/s-ish feel
	DesiredVelocity += DragDirection * Scale;

	if (bManualMovementCancelsFocus && !(bFollowWhileChampionDropping && IsChampionDropping()))
	{
		CancelChampionFollow();
	}
}

void AMobaCameraPawn::UpdateChampionFocus(float DeltaTime)
{
	if (!ShouldFollowChampion())
	{
		return;
	}

	APawn* Champion = ResolveFocusChampion();
	if (!Champion)
	{
		return;
	}

	// Re-assert sticky lock when drop ends so post-land stay-on-player continues.
	const bool bDropping = IsChampionDropping();
	if (bDropping && bFollowWhileChampionDropping)
	{
		bLockedToChampion = true;
	}

	FVector TargetLocation = Champion->GetActorLocation();
	TargetLocation.Z = CachedPawnHeight;

	FVector NewLocation = TargetLocation;
	if (!(bDropping && bSnapFollowWhileDropping))
	{
		NewLocation = FMath::VInterpTo(
			GetActorLocation(),
			TargetLocation,
			DeltaTime,
			ChampionFollowInterpolationSpeed);
	}

	ConstrainLocation(NewLocation);
	SetActorLocation(NewLocation);
	CurrentVelocity = FVector::ZeroVector;
	DesiredVelocity = FVector::ZeroVector;
}

void AMobaCameraPawn::UpdateZoom(float DeltaTime)
{
	CurrentZoom = FMath::FInterpTo(CurrentZoom, TargetZoom, DeltaTime, ZoomInterpolationSpeed);
	if (SpringArm)
	{
		SpringArm->TargetArmLength = CurrentZoom;
	}
	if (FloatingPawnMovement)
	{
		FloatingPawnMovement->MaxSpeed = KeyboardMoveSpeed * GetZoomSpeedScale();
	}
}

void AMobaCameraPawn::UpdateRotation(float DeltaTime)
{
	if (bEnableCameraRotation && !FMath::IsNearlyZero(RotateInput))
	{
		TargetYaw += RotateInput * RotationSpeed * DeltaTime;
	}

	CurrentYaw = FMath::FInterpTo(CurrentYaw, TargetYaw, DeltaTime, RotationInterpolationSpeed);
	if (SpringArm)
	{
		const FRotator Rel = SpringArm->GetRelativeRotation();
		SpringArm->SetRelativeRotation(FRotator(CameraPitch, CurrentYaw, Rel.Roll));
	}
	UpdatePlanarAxes();
}

void AMobaCameraPawn::ApplyVelocity(float DeltaTime)
{
	if (ShouldFollowChampion())
	{
		return;
	}

	// Accelerate / decelerate toward desired planar velocity.
	CurrentVelocity = FMath::VInterpTo(
		CurrentVelocity,
		DesiredVelocity,
		DeltaTime,
		MovementInterpolationSpeed);

	// Kill tiny residual drift.
	if (CurrentVelocity.SizeSquared() < 1.0f && DesiredVelocity.IsNearlyZero())
	{
		CurrentVelocity = FVector::ZeroVector;
		return;
	}

	FVector NewLocation = GetActorLocation() + CurrentVelocity * DeltaTime;
	NewLocation.Z = CachedPawnHeight;
	ConstrainLocation(NewLocation);
	SetActorLocation(NewLocation);

	// Keep FloatingPawnMovement coherent for other systems.
	if (FloatingPawnMovement)
	{
		FloatingPawnMovement->Velocity = CurrentVelocity;
	}
}

void AMobaCameraPawn::ConstrainLocation(FVector& Location) const
{
	if (!bConstrainToWorldBounds)
	{
		return;
	}

	Location.X = FMath::Clamp(Location.X, MinimumWorldBounds.X, MaximumWorldBounds.X);
	Location.Y = FMath::Clamp(Location.Y, MinimumWorldBounds.Y, MaximumWorldBounds.Y);
	// Z remains configured height.
}

void AMobaCameraPawn::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	DesiredVelocity = FVector::ZeroVector;

	UpdateRotation(DeltaTime);

	// While locked, edge scroll is ignored (WASD / MMB / minimap unlock free cam).
	if (ShouldFollowChampion())
	{
		UpdateChampionFocus(DeltaTime);
	}
	else
	{
		if (bIsDragging)
		{
			UpdateMiddleMouseDrag(DeltaTime);
		}
		else
		{
			UpdateKeyboardMove(DeltaTime);
			UpdateEdgeScroll(DeltaTime);
		}
		ApplyVelocity(DeltaTime);
	}

	UpdateZoom(DeltaTime);
}

AMobaPlayerController* AMobaCameraPawn::GetMobaController() const
{
	return Cast<AMobaPlayerController>(GetController());
}

APawn* AMobaCameraPawn::ResolveFocusChampion() const
{
	if (const AMobaPlayerController* MPC = GetMobaController())
	{
		return MPC->GetControlledChampion();
	}
	// Fallback: any owned pawn that is not this camera.
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (APawn* View = PC->GetPawn())
		{
			if (View != this)
			{
				return View;
			}
		}
	}
	return nullptr;
}

bool AMobaCameraPawn::ShouldBlockCameraInputForUI() const
{
	if (!bBlockCameraInputOverUI)
	{
		return false;
	}

	if (const AMobaPlayerController* MPC = GetMobaController())
	{
		return MPC->ShouldBlockCameraInputForUI();
	}

	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}

	FSlateApplication& App = FSlateApplication::Get();
	const FVector2D Cursor = App.GetCursorPos();
	TArray<TSharedRef<SWindow>> Windows;
	App.GetAllVisibleWindowsOrdered(Windows);
	const FWidgetPath Path = App.LocateWindowUnderMouse(Cursor, Windows, false);
	if (!Path.IsValid())
	{
		return false;
	}

	for (int32 Index = Path.Widgets.Num() - 1; Index >= 0; --Index)
	{
		const TSharedRef<SWidget>& Widget = Path.Widgets[Index].Widget;
		const FString Type = Widget->GetTypeAsString();
		if (Type == TEXT("SGameLayerManager")
			|| Type == TEXT("SViewport")
			|| Type == TEXT("SWindow")
			|| Type == TEXT("SOverlay")
			|| Type == TEXT("SDPIScaler")
			|| Type == TEXT("SInvalidationPanel"))
		{
			continue;
		}

		if (Widget->GetVisibility().IsVisible() && Widget->IsInteractable())
		{
			return true;
		}
	}
	return false;
}

bool AMobaCameraPawn::HasMouseFocus() const
{
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		// GetMousePosition fails when the cursor is outside the viewport / no focus.
		float MX = 0.0f;
		float MY = 0.0f;
		if (!PC->GetMousePosition(MX, MY))
		{
			return false;
		}
	}

	if (GEngine && GEngine->GameViewport)
	{
		// Prefer only when game window is foreground-ish.
		if (FSlateApplication::IsInitialized() && !FSlateApplication::Get().IsActive())
		{
			// Still allow if mouse is over game viewport.
		}
	}
	return true;
}
