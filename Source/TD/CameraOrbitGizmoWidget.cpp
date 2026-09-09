#include "CameraOrbitGizmoWidget.h"

#include "MinimapWidget.h"
#include "MobaCameraPawn.h"
#include "MobaPlayerController.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "UObject/UObjectIterator.h"

namespace CameraOrbitGizmoPrivate
{
	static FLinearColor RingBg(0.06f, 0.08f, 0.12f, 0.72f);
	static FLinearColor RingFrame(0.55f, 0.62f, 0.72f, 0.95f);
	static FLinearColor HubBg(0.10f, 0.14f, 0.20f, 0.90f);
	static FLinearColor HandleBg(0.85f, 0.78f, 0.35f, 1.0f);
	static FLinearColor HandleFrame(0.95f, 0.90f, 0.55f, 1.0f);
}

UCameraOrbitGizmoWidget::UCameraOrbitGizmoWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
}

void UCameraOrbitGizmoWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureBuilt();
}

void UCameraOrbitGizmoWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureBuilt();
	ApplyHitTestPolicy();
}

void UCameraOrbitGizmoWidget::EnsureBuilt()
{
	if (bBuilt && RingBorder && HandleBorder)
	{
		return;
	}
	if (!WidgetTree)
	{
		return;
	}

	BindDesignerWidgets();
	BindPointerEvents();
	bBuilt = RingBorder != nullptr && HandleBorder != nullptr && FrameSizeBox != nullptr;
	if (bBuilt)
	{
		ApplyHitTestPolicy();
	}
}

const TCHAR* UCameraOrbitGizmoWidget::GetWidgetBlueprintPath()
{
	return TEXT("/Game/TD/UI/WBP_CameraOrbitGizmo.WBP_CameraOrbitGizmo_C");
}

TSubclassOf<UCameraOrbitGizmoWidget> UCameraOrbitGizmoWidget::ResolveWidgetClass()
{
	return LoadClass<UCameraOrbitGizmoWidget>(nullptr, GetWidgetBlueprintPath());
}

void UCameraOrbitGizmoWidget::BindDesignerWidgets()
{
	RootCanvas = Cast<UCanvasPanel>(GetWidgetFromName(TEXT("OrbitGizmoRoot")));
	if (!RootCanvas)
	{
		RootCanvas = Cast<UCanvasPanel>(WidgetTree ? WidgetTree->RootWidget : nullptr);
	}
	FrameSizeBox = Cast<USizeBox>(GetWidgetFromName(TEXT("OrbitGizmoSizeBox")));
	RingBorder = Cast<UBorder>(GetWidgetFromName(TEXT("OrbitRing")));
	HubBorder = Cast<UBorder>(GetWidgetFromName(TEXT("OrbitHub")));
	HandleBorder = Cast<UBorder>(GetWidgetFromName(TEXT("OrbitHandle")));
	HandleCanvas = Cast<UCanvasPanel>(GetWidgetFromName(TEXT("OrbitHandleCanvas")));
	FrameSlot = FrameSizeBox ? Cast<UCanvasPanelSlot>(FrameSizeBox->Slot) : nullptr;
	HandleSlot = HandleBorder ? Cast<UCanvasPanelSlot>(HandleBorder->Slot) : nullptr;
}

void UCameraOrbitGizmoWidget::BindPointerEvents()
{
	if (RingBorder)
	{
		RingBorder->OnMouseButtonDownEvent.BindDynamic(this, &UCameraOrbitGizmoWidget::OnGizmoMouseButtonDown);
		RingBorder->OnMouseMoveEvent.BindDynamic(this, &UCameraOrbitGizmoWidget::OnGizmoMouseMove);
		RingBorder->OnMouseButtonUpEvent.BindDynamic(this, &UCameraOrbitGizmoWidget::OnGizmoMouseButtonUp);
	}
	if (HandleBorder)
	{
		HandleBorder->OnMouseButtonDownEvent.BindDynamic(this, &UCameraOrbitGizmoWidget::OnGizmoMouseButtonDown);
	}
}

UMinimapWidget* UCameraOrbitGizmoWidget::ResolveMinimap() const
{
	if (const AMobaPlayerController* MPC = Cast<AMobaPlayerController>(GetOwningPlayer()))
	{
		if (UMinimapWidget* Mini = MPC->GetMinimapWidget())
		{
			return Mini;
		}
	}

	APlayerController* OwnerPC = GetOwningPlayer();
	for (TObjectIterator<UMinimapWidget> It; It; ++It)
	{
		UMinimapWidget* Mini = *It;
		if (Mini && Mini->IsInViewport() && Mini->GetOwningPlayer() == OwnerPC)
		{
			return Mini;
		}
	}
	return nullptr;
}

void UCameraOrbitGizmoWidget::ApplyHitTestPolicy()
{
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (RootCanvas)
	{
		RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (FrameSizeBox)
	{
		// Square chrome must not eat hits in the corners; only the circular ring does.
		FrameSizeBox->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (RingBorder)
	{
		RingBorder->SetVisibility(ESlateVisibility::Visible);
	}
	if (HubBorder)
	{
		// Let ring receive drags through the hub.
		HubBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (HandleCanvas)
	{
		HandleCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	if (HandleBorder)
	{
		HandleBorder->SetVisibility(ESlateVisibility::Visible);
	}
}

void UCameraOrbitGizmoWidget::UpdateHandleFromYaw(float YawDegrees)
{
	if (!HandleSlot)
	{
		return;
	}

	// Screen angle: 0 = right, positive CCW. Unreal yaw 0 = +X (right on top-down).
	// Map yaw so handle sits along the camera's planar look direction on the gizmo.
	const float Radians = FMath::DegreesToRadians(YawDegrees);
	const float Half = GizmoSize * 0.5f;
	const float Radius = Half * HandleRadiusFraction;
	const float X = Half + FMath::Cos(Radians) * Radius;
	const float Y = Half - FMath::Sin(Radians) * Radius;
	HandleSlot->SetPosition(FVector2D(X, Y));
	HandleSlot->SetSize(FVector2D(HandleSize, HandleSize));
}

bool UCameraOrbitGizmoWidget::TryPointerAngle(const FPointerEvent& MouseEvent, float& OutScreenAngleDegrees) const
{
	if (!FrameSizeBox)
	{
		return false;
	}

	const FGeometry& Geo = FrameSizeBox->GetCachedGeometry();
	const FVector2D Size = Geo.GetLocalSize();
	if (Size.X <= KINDA_SMALL_NUMBER || Size.Y <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector2D Local = Geo.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	const FVector2D Center(Size.X * 0.5f, Size.Y * 0.5f);
	const FVector2D Delta = Local - Center;
	if (Delta.SizeSquared() < 4.0f)
	{
		return false;
	}

	// Atan2(Y_up, X): Y is flipped because slate Y grows downward.
	OutScreenAngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(-(Delta.Y), Delta.X));
	return true;
}

AMobaCameraPawn* UCameraOrbitGizmoWidget::ResolveCameraPawn() const
{
	if (AMobaPlayerController* MPC = Cast<AMobaPlayerController>(GetOwningPlayer()))
	{
		return MPC->GetMobaCameraPawn();
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		return Cast<AMobaCameraPawn>(PC->GetPawn());
	}
	return nullptr;
}

bool UCameraOrbitGizmoWidget::IsScreenPosOverGizmo(FVector2D ScreenPos) const
{
	if (!FrameSizeBox || !IsInViewport())
	{
		return false;
	}

	const FGeometry& Geo = FrameSizeBox->GetCachedGeometry();
	const FVector2D Size = Geo.GetLocalSize();
	if (Size.X <= KINDA_SMALL_NUMBER || Size.Y <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector2D Local = Geo.AbsoluteToLocal(ScreenPos);
	const FVector2D Center(Size.X * 0.5f, Size.Y * 0.5f);
	const float Radius = FMath::Min(Size.X, Size.Y) * 0.5f;
	return (Local - Center).SizeSquared() <= FMath::Square(Radius);
}

FEventReply UCameraOrbitGizmoWidget::OnGizmoMouseButtonDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	FEventReply Reply;
	Reply.NativeReply = NativeOnMouseButtonDown(MyGeometry, MouseEvent);
	return Reply;
}

FEventReply UCameraOrbitGizmoWidget::OnGizmoMouseMove(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	FEventReply Reply;
	Reply.NativeReply = NativeOnMouseMove(MyGeometry, MouseEvent);
	return Reply;
}

FEventReply UCameraOrbitGizmoWidget::OnGizmoMouseButtonUp(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	FEventReply Reply;
	Reply.NativeReply = NativeOnMouseButtonUp(MyGeometry, MouseEvent);
	return Reply;
}

FReply UCameraOrbitGizmoWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!InMouseEvent.GetEffectingButton().IsMouseButton()
		|| InMouseEvent.GetEffectingButton() != EKeys::LeftMouseButton)
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	if (!IsScreenPosOverGizmo(InMouseEvent.GetScreenSpacePosition()))
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	float ScreenAngle = 0.0f;
	AMobaCameraPawn* Cam = ResolveCameraPawn();
	if (!Cam || !TryPointerAngle(InMouseEvent, ScreenAngle))
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	bDragging = true;
	DragYawOffset = Cam->GetOrbitYaw() - ScreenAngle;
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UCameraOrbitGizmoWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bDragging || !(HasMouseCapture() || InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton)))
	{
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
	}

	float ScreenAngle = 0.0f;
	if (!TryPointerAngle(InMouseEvent, ScreenAngle))
	{
		return FReply::Handled();
	}

	if (AMobaCameraPawn* Cam = ResolveCameraPawn())
	{
		const float NewYaw = ScreenAngle + DragYawOffset;
		Cam->SetOrbitYaw(NewYaw, false);
		UpdateHandleFromYaw(NewYaw);
	}
	return FReply::Handled();
}

FReply UCameraOrbitGizmoWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UCameraOrbitGizmoWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bBuilt)
	{
		EnsureBuilt();
	}

	if (!bDragging)
	{
		if (AMobaCameraPawn* Cam = ResolveCameraPawn())
		{
			UpdateHandleFromYaw(Cam->GetOrbitYaw());
		}
	}
}
