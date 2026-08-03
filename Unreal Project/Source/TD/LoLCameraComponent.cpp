#include "LoLCameraComponent.h"
#include "MobaPlayerController.h"
#include "TDUIInputLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputMappingContext.h"
#include "Navigation/PathFollowingComponent.h"
#include "UObject/UnrealType.h"
ULoLCameraComponent::ULoLCameraComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}
void ULoLCameraComponent::BeginPlay()
{
	Super::BeginPlay();
	ResolveSpringArm();
	ResolveCamera();
	if (SpringArm)
	{
		bSavedCameraLag = SpringArm->bEnableCameraLag;
		bHasSavedCameraLag = true;
		SpringArm->bEnableCameraLag = false;
	}
	bWasOwnerDropping = IsOwnerDropping();
	LockToChampion();
}
void ULoLCameraComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopLandingZoom(true);
	SetSpringArmLagEnabled(true);
	Super::EndPlay(EndPlayReason);
}
void ULoLCameraComponent::SetCameraEnabled(bool bEnabled)
{
	bCameraEnabled = bEnabled;
	if (bEnabled)
	{
		LockToChampion();
	}
}
void ULoLCameraComponent::SyncFocusFromSpringArm()
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	const FVector ActorLoc = Owner->GetActorLocation();
	if (SpringArm && !bCameraLocked)
	{
		const FVector Rel = SpringArm->GetRelativeLocation();
		CameraFocusLocation = FVector(ActorLoc.X + Rel.X, ActorLoc.Y + Rel.Y, ActorLoc.Z);
	}
	else
	{
		CameraFocusLocation = ActorLoc;
	}
}
void ULoLCameraComponent::LockToChampion()
{
	bCameraLocked = true;
	bEdgeScrollArmed = true;
	if (const AActor* Owner = GetOwner())
	{
		CameraFocusLocation = Owner->GetActorLocation();
	}
	ApplyFocusToSpringArm();
}
void ULoLCameraComponent::UnlockCamera()
{
	// Keep the current view â€” don't snap. Disarm edges until cursor leaves the border
	// so unlocking while the mouse is on an edge doesn't instantly fly away.
	bCameraLocked = false;
	bEdgeScrollArmed = false;
	SyncFocusFromSpringArm();
	ApplyFocusToSpringArm();
}
void ULoLCameraComponent::ToggleCameraLock()
{
	if (bCameraLocked)
	{
		UnlockCamera();
	}
	else
	{
		LockToChampion();
	}
}
bool ULoLCameraComponent::ResolveSpringArm()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}
	if (SpringArmComponentName != NAME_None)
	{
		TArray<USpringArmComponent*> Arms;
		Owner->GetComponents<USpringArmComponent>(Arms);
		for (USpringArmComponent* Arm : Arms)
		{
			if (Arm && Arm->GetFName() == SpringArmComponentName)
			{
				SpringArm = Arm;
				return true;
			}
		}
	}
	SpringArm = Owner->FindComponentByClass<USpringArmComponent>();
	return SpringArm != nullptr;
}
bool ULoLCameraComponent::ResolveCamera()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}
	if (CameraComponentName != NAME_None)
	{
		TArray<UCameraComponent*> Cams;
		Owner->GetComponents<UCameraComponent>(Cams);
		for (UCameraComponent* Cam : Cams)
		{
			if (Cam && Cam->GetFName() == CameraComponentName)
			{
				Camera = Cam;
				return true;
			}
		}
	}
	// Prefer camera under the spring arm when present.
	if (SpringArm)
	{
		TArray<USceneComponent*> Children;
		SpringArm->GetChildrenComponents(true, Children);
		for (USceneComponent* Child : Children)
		{
			if (UCameraComponent* Cam = Cast<UCameraComponent>(Child))
			{
				Camera = Cam;
				return true;
			}
		}
	}
	Camera = Owner->FindComponentByClass<UCameraComponent>();
	return Camera != nullptr;
}
bool ULoLCameraComponent::IsOwnerDropping() const
{
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return false;
	}
	static const FName Names[] = {
		FName(TEXT("IsDropping")),
		FName(TEXT("bIsDropping")),
		FName(TEXT("isDropping"))
	};
	for (const FName& Name : Names)
	{
		if (const FBoolProperty* BoolProp = FindFProperty<FBoolProperty>(Owner->GetClass(), Name))
		{
			return BoolProp->GetPropertyValue_InContainer(Owner);
		}
	}
	return false;
}
APlayerController* ULoLCameraComponent::GetOwnerPlayerController() const
{
	if (const APawn* Pawn = Cast<APawn>(GetOwner()))
	{
		if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			return PC;
		}
		// Free-camera path: champion is AI-possessed; resolve via Moba controller mapping.
		if (UWorld* World = Pawn->GetWorld())
		{
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				if (AMobaPlayerController* MPC = Cast<AMobaPlayerController>(It->Get()))
				{
					if (MPC->GetControlledChampion() == Pawn)
					{
						return MPC;
					}
				}
			}
			return World->GetFirstPlayerController();
		}
	}
	return nullptr;
}
void ULoLCameraComponent::SetSpringArmLagEnabled(bool bEnabled)
{
	if (!SpringArm || !bHasSavedCameraLag)
	{
		return;
	}
	SpringArm->bEnableCameraLag = bEnabled ? bSavedCameraLag : false;
}
bool ULoLCameraComponent::GetViewportMouse(APlayerController* PC, FVector2D& OutMouse, FVector2D& OutSize) const
{
	if (!PC)
	{
		return false;
	}
	OutMouse = UWidgetLayoutLibrary::GetMousePositionOnViewport(PC);
	OutSize = UWidgetLayoutLibrary::GetViewportSize(PC);
	if (OutSize.X <= KINDA_SMALL_NUMBER || OutSize.Y <= KINDA_SMALL_NUMBER)
	{
		int32 SizeX = 0;
		int32 SizeY = 0;
		PC->GetViewportSize(SizeX, SizeY);
		OutSize = FVector2D(static_cast<float>(SizeX), static_cast<float>(SizeY));
		float MouseX = 0.f;
		float MouseY = 0.f;
		if (!PC->GetMousePosition(MouseX, MouseY))
		{
			return false;
		}
		OutMouse = FVector2D(MouseX, MouseY);
	}
	if (OutSize.X <= KINDA_SMALL_NUMBER || OutSize.Y <= KINDA_SMALL_NUMBER)
	{
		return false;
	}
	if (OutMouse.X < 0.f || OutMouse.Y < 0.f || OutMouse.X > OutSize.X || OutMouse.Y > OutSize.Y)
	{
		return false;
	}
	return true;
}
bool ULoLCameraComponent::IsOnAnyEdge(const FVector2D& Mouse, const FVector2D& Size) const
{
	const float Margin = FMath::Clamp(EdgeMargin, 1.f, FMath::Min(Size.X, Size.Y) * 0.25f);
	return Mouse.X <= Margin
		|| Mouse.X >= (Size.X - Margin)
		|| Mouse.Y <= Margin
		|| Mouse.Y >= (Size.Y - Margin);
}
float ULoLCameraComponent::SmoothStep01(float T)
{
	T = FMath::Clamp(T, 0.f, 1.f);
	return T * T * (3.f - 2.f * T);
}
void ULoLCameraComponent::PlayLandingZoom(float SettleArmLength, float SettlePitch)
{
	if (!SpringArm && !ResolveSpringArm())
	{
		return;
	}
	ResolveCamera();
	LandingZoomStartArm = SpringArm->TargetArmLength;
	LandingZoomStartPitch = SpringArm->GetRelativeRotation().Pitch;
	LandingZoomSettleArm = FMath::Max(100.f, SettleArmLength);
	LandingZoomSettlePitchVal = SettlePitch;
	LandingZoomPeakArm = FMath::Max(100.f, LandingZoomSettleArm - FMath::Max(0.f, LandingZoomPunchAmount));
	// If already at settle pitch (e.g. snap happened first), still punch from current arm.
	if (FMath::IsNearlyEqual(LandingZoomStartArm, LandingZoomSettleArm, 1.f)
		&& FMath::IsNearlyEqual(LandingZoomStartPitch, LandingZoomSettlePitchVal, 0.25f))
	{
		// Start slightly longer so the punch-in is visible even after a prior snap.
		LandingZoomStartArm = LandingZoomSettleArm + LandingZoomPunchAmount * 0.35f;
	}
	LandingZoomBaseFov = Camera ? Camera->FieldOfView : 40.f;
	LandingZoomElapsed = 0.f;
	bLandingZoomActive = true;
	bWasOwnerDropping = false;
	// Land stays locked on champion (LoL centers during impact).
	LockToChampion();
	ApplyLandingZoomFrame(0.f);
}
void ULoLCameraComponent::StopLandingZoom(bool bSnapToSettle)
{
	if (!bLandingZoomActive)
	{
		return;
	}
	if (bSnapToSettle && SpringArm)
	{
		SpringArm->TargetArmLength = LandingZoomSettleArm;
		const FRotator Rel = SpringArm->GetRelativeRotation();
		SpringArm->SetRelativeRotation(FRotator(LandingZoomSettlePitchVal, Rel.Yaw, Rel.Roll));
		if (Camera)
		{
			Camera->SetFieldOfView(LandingZoomBaseFov);
		}
	}
	bLandingZoomActive = false;
	LandingZoomElapsed = 0.f;
}
void ULoLCameraComponent::ApplyLandingZoomFrame(float NormalizedTime)
{
	if (!SpringArm)
	{
		return;
	}
	const float T = FMath::Clamp(NormalizedTime, 0.f, 1.f);
	const float PunchT = FMath::Clamp(LandingZoomPunchTime, 0.05f, 0.9f);
	// Arm: start -> peak (punch in) -> settle, smoothstep each leg.
	float Arm = LandingZoomSettleArm;
	if (T <= PunchT)
	{
		const float Local = SmoothStep01(T / PunchT);
		Arm = FMath::Lerp(LandingZoomStartArm, LandingZoomPeakArm, Local);
	}
	else
	{
		const float Local = SmoothStep01((T - PunchT) / (1.f - PunchT));
		Arm = FMath::Lerp(LandingZoomPeakArm, LandingZoomSettleArm, Local);
	}
	SpringArm->TargetArmLength = Arm;
	// Pitch: ease all the way from drop (often -90) to top-down settle; ease-out feel.
	const float PitchAlpha = SmoothStep01(T);
	// Slightly front-load pitch settle so framing recovers with the punch.
	const float PitchBlend = SmoothStep01(FMath::Pow(PitchAlpha, 0.85f));
	const float Pitch = FMath::Lerp(LandingZoomStartPitch, LandingZoomSettlePitchVal, PitchBlend);
	const FRotator Rel = SpringArm->GetRelativeRotation();
	SpringArm->SetRelativeRotation(FRotator(Pitch, Rel.Yaw, Rel.Roll));
	// FOV kick peaks with the punch, returns to base at the end.
	if (Camera && LandingFovKick > KINDA_SMALL_NUMBER)
	{
		float FovKickAlpha = 0.f;
		if (T <= PunchT)
		{
			FovKickAlpha = SmoothStep01(T / PunchT);
		}
		else
		{
			FovKickAlpha = 1.f - SmoothStep01((T - PunchT) / (1.f - PunchT));
		}
		Camera->SetFieldOfView(LandingZoomBaseFov + LandingFovKick * FovKickAlpha);
	}
}
void ULoLCameraComponent::UpdateLandingZoom(float DeltaTime)
{
	const float Duration = FMath::Max(0.05f, LandingZoomDuration);
	LandingZoomElapsed += FMath::Max(0.f, DeltaTime);
	const float T = FMath::Clamp(LandingZoomElapsed / Duration, 0.f, 1.f);
	ApplyLandingZoomFrame(T);
	if (T >= 1.f - KINDA_SMALL_NUMBER)
	{
		// Ensure exact settle values.
		if (SpringArm)
		{
			SpringArm->TargetArmLength = LandingZoomSettleArm;
			const FRotator Rel = SpringArm->GetRelativeRotation();
			SpringArm->SetRelativeRotation(FRotator(LandingZoomSettlePitchVal, Rel.Yaw, Rel.Roll));
		}
		if (Camera)
		{
			Camera->SetFieldOfView(LandingZoomBaseFov);
		}
		bLandingZoomActive = false;
		LandingZoomElapsed = 0.f;
	}
}
void ULoLCameraComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// Store / HUD hit-test: don't let set-destination path the champion under the UI.
	// Also suppresses RMB cancel-from-store from double-firing SimpleMoveToLocation.
	if (APlayerController* PC = GetOwnerPlayerController())
	{
		const bool bBlock = UTDUIInputLibrary::ShouldBlockWorldClickInput(PC, true);
		PC->bEnableClickEvents = !bBlock;

		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsys =
					ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))
			{
				static TWeakObjectPtr<UInputMappingContext> GCachedIMC;
				if (!GCachedIMC.IsValid())
				{
					GCachedIMC = Cast<UInputMappingContext>(StaticLoadObject(
						UInputMappingContext::StaticClass(), nullptr,
						TEXT("/Game/TopDown/Input/IMC_Default.IMC_Default")));
				}
				if (UInputMappingContext* IMC = GCachedIMC.Get())
				{
					// Sticky flag so we only re-add if we stripped destination mapping ourselves.
					static bool bStrippedDefaultIMC = false;
					const bool bHasIMC = Subsys->HasMappingContext(IMC);
					if (bBlock && bHasIMC)
					{
						Subsys->RemoveMappingContext(IMC);
						bStrippedDefaultIMC = true;
					}
					else if (!bBlock && bStrippedDefaultIMC && !bHasIMC)
					{
						Subsys->AddMappingContext(IMC, 0);
						bStrippedDefaultIMC = false;
					}
					else if (!bBlock && bHasIMC)
					{
						bStrippedDefaultIMC = false;
					}
				}
			}
		}

		if (bBlock)
		{
			auto AbortPath = [PC](AActor* Owner)
			{
				if (!Owner)
				{
					return;
				}
				if (UPathFollowingComponent* PathFollow = Owner->FindComponentByClass<UPathFollowingComponent>())
				{
					PathFollow->AbortMove(*PC, FPathFollowingResultFlags::UserAbort);
				}
			};
			AbortPath(PC);
			AbortPath(PC->GetPawn());
			if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
			{
				AbortPath(OwnerPawn);
				if (AController* Ctrl = OwnerPawn->GetController())
				{
					AbortPath(Ctrl);
				}
			}
		}
	}

	if (!SpringArm && !ResolveSpringArm())
	{
		return;
	}
	const bool bDropping = IsOwnerDropping();

	// Allow skip even while LoL pan is disabled during freefall.
	{
		APlayerController* SkipPC = GetOwnerPlayerController();
		if (!SkipPC && GetWorld())
		{
			SkipPC = GetWorld()->GetFirstPlayerController();
		}
		TrySkipSkyDropFromHotkey(SkipPC);
	}

	// Auto: drop finished -> LoL land zoom. Explicit PlayLandingZoom still preferred from BP.
	if (bAutoPlayLandingZoom && bWasOwnerDropping && !bDropping && !bLandingZoomActive)
	{
		float SettleArm = SpringArm->TargetArmLength;
		float SettlePitch = LandingSettlePitch;
		if (const AActor* Owner = GetOwner())
		{
			auto ReadFloat = [Owner](const FName Name, float& OutVal) -> bool
			{
				if (const FDoubleProperty* D = FindFProperty<FDoubleProperty>(Owner->GetClass(), Name))
				{
					OutVal = static_cast<float>(D->GetPropertyValue_InContainer(Owner));
					return true;
				}
				if (const FFloatProperty* F = FindFProperty<FFloatProperty>(Owner->GetClass(), Name))
				{
					OutVal = F->GetPropertyValue_InContainer(Owner);
					return true;
				}
				return false;
			};
			ReadFloat(FName(TEXT("SavedArmLength")), SettleArm);
			ReadFloat(FName(TEXT("TopDownArmPitch")), SettlePitch);
		}
		PlayLandingZoom(SettleArm, SettlePitch);
	}
	bWasOwnerDropping = bDropping;
	if (bLandingZoomActive)
	{
		UpdateLandingZoom(DeltaTime);
		// Keep focus locked on champion through the landing impact.
		if (const AActor* Owner = GetOwner())
		{
			CameraFocusLocation = Owner->GetActorLocation();
		}
		ApplyFocusToSpringArm();
		// Still allow Space recenter / Y only after zoom; skip edge pan while landing.
		return;
	}
	if (!bCameraEnabled)
	{
		return;
	}
	if (bDisableWhileDropping && bDropping)
	{
		// Keep focus locked, but do not touch spring-arm rotation — freefall cam is
		// world-frozen by AMobaPlayerController (only the player may rotate).
		bCameraLocked = true;
		bEdgeScrollArmed = true;
		if (const AActor* Owner = GetOwner())
		{
			CameraFocusLocation = Owner->GetActorLocation();
		}
		if (SpringArm)
		{
			// Position only: never pitch/yaw during drop.
			const float RelZ = SpringArm->GetRelativeLocation().Z;
			SpringArm->SetRelativeLocation(FVector(0.f, 0.f, RelZ));
		}
		return;
	}
	APlayerController* PC = GetOwnerPlayerController();
	if (!PC)
	{
		return;
	}
	if (PC->WasInputKeyJustPressed(EKeys::SpaceBar))
	{
		LockToChampion();
	}
	else if (PC->WasInputKeyJustPressed(EKeys::Y))
	{
		ToggleCameraLock();
	}
	if (bCameraLocked)
	{
		if (const AActor* Owner = GetOwner())
		{
			CameraFocusLocation = Owner->GetActorLocation();
		}
	}
	else
	{
		UpdateEdgePan(DeltaTime, PC);
	}
	ApplyFocusToSpringArm();
}
void ULoLCameraComponent::UpdateEdgePan(float DeltaTime, APlayerController* PC)
{
	FVector2D Mouse(ForceInit);
	FVector2D Size(ForceInit);
	if (!GetViewportMouse(PC, Mouse, Size))
	{
		return;
	}
	const bool bOnEdge = IsOnAnyEdge(Mouse, Size);
	// After unlock, require leaving the edge once before panning.
	if (!bEdgeScrollArmed)
	{
		if (!bOnEdge)
		{
			bEdgeScrollArmed = true;
		}
		return;
	}
	if (!bOnEdge)
	{
		return;
	}
	const float Margin = FMath::Clamp(EdgeMargin, 1.f, FMath::Min(Size.X, Size.Y) * 0.25f);
	const FRotator YawRot(0.f, PC->GetControlRotation().Yaw, 0.f);
	const FVector Forward = FRotationMatrix(YawRot).GetUnitAxis(EAxis::X);
	const FVector Right = FRotationMatrix(YawRot).GetUnitAxis(EAxis::Y);
	FVector EdgePan = FVector::ZeroVector;
	if (Mouse.X <= Margin)
	{
		EdgePan -= Right;
	}
	if (Mouse.X >= (Size.X - Margin))
	{
		EdgePan += Right;
	}
	if (Mouse.Y <= Margin)
	{
		EdgePan += Forward;
	}
	if (Mouse.Y >= (Size.Y - Margin))
	{
		EdgePan -= Forward;
	}
	if (EdgePan.IsNearlyZero())
	{
		return;
	}
	EdgePan.Normalize();
	CameraFocusLocation += EdgePan * EdgeScrollSpeed * DeltaTime;
}
void ULoLCameraComponent::ApplyFocusToSpringArm()
{
	if (!SpringArm)
	{
		return;
	}
	const AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}
	const float RelZ = SpringArm->GetRelativeLocation().Z;
	if (bCameraLocked)
	{
		SpringArm->SetRelativeLocation(FVector(0.f, 0.f, RelZ));
		return;
	}
	const FVector ActorLoc = Owner->GetActorLocation();
	SpringArm->SetRelativeLocation(FVector(
		CameraFocusLocation.X - ActorLoc.X,
		CameraFocusLocation.Y - ActorLoc.Y,
		RelZ));
}

void ULoLCameraComponent::TrySkipSkyDropFromHotkey(APlayerController* PC)
{
	if (!bEnableSkipSkyDropHotkey || !PC || !SkipSkyDropKey.IsValid())
	{
		return;
	}
	if (!PC->WasInputKeyJustPressed(SkipSkyDropKey) || !IsOwnerDropping())
	{
		return;
	}

	// Prefer MOBA controller path (same Champion resolution + CompleteDropLanding helpers).
	if (AMobaPlayerController* MPC = Cast<AMobaPlayerController>(PC))
	{
		MPC->TrySkipSkyDrop();
		return;
	}

	// Non-MOBA controller (e.g. classic top-down): snap + call BP helpers / reflection.
	AActor* Owner = GetOwner();
	APawn* Pawn = Cast<APawn>(Owner);
	if (!Pawn || !GetWorld())
	{
		return;
	}

	if (UFunction* SkipFn = Pawn->FindFunction(FName(TEXT("SkipSkyDrop"))))
	{
		if (SkipFn->NumParms == 0)
		{
			Pawn->ProcessEvent(SkipFn, nullptr);
			return;
		}
	}

	const FVector Loc = Pawn->GetActorLocation();
	FCollisionQueryParams Params(SCENE_QUERY_STAT(LoLSkipSkyDrop), true, Pawn);
	Params.AddIgnoredActor(Pawn);
	FHitResult Hit;
	if (GetWorld()->LineTraceSingleByChannel(
			Hit, Loc, FVector(Loc.X, Loc.Y, Loc.Z - 100000.f), ECC_Visibility, Params)
		&& Hit.bBlockingHit)
	{
		float HalfH = 96.f;
		if (const ACharacter* AsChar = Cast<ACharacter>(Pawn))
		{
			if (const UCapsuleComponent* Cap = AsChar->GetCapsuleComponent())
			{
				HalfH = Cap->GetScaledCapsuleHalfHeight();
			}
		}
		Pawn->SetActorLocation(
			FVector(Hit.ImpactPoint.X, Hit.ImpactPoint.Y, Hit.ImpactPoint.Z + HalfH),
			false, nullptr, ETeleportType::TeleportPhysics);
	}

	if (UFunction* CompleteFn = Pawn->FindFunction(FName(TEXT("CompleteDropLanding"))))
	{
		if (CompleteFn->NumParms == 0)
		{
			Pawn->ProcessEvent(CompleteFn, nullptr);
			return;
		}
	}

	// Minimal reflection fallback.
	if (FBoolProperty* DropProp = FindFProperty<FBoolProperty>(Pawn->GetClass(), FName(TEXT("bIsDropping"))))
	{
		DropProp->SetPropertyValue_InContainer(Pawn, false);
	}
	if (ACharacter* AsChar = Cast<ACharacter>(Pawn))
	{
		if (UCharacterMovementComponent* Move = AsChar->GetCharacterMovement())
		{
			Move->GravityScale = 1.f;
			Move->Velocity = FVector::ZeroVector;
		}
	}
	if (UFunction* HUD = Pawn->FindFunction(FName(TEXT("ShowAbilityHUD"))))
	{
		if (HUD->NumParms == 0)
		{
			Pawn->ProcessEvent(HUD, nullptr);
		}
	}
}
