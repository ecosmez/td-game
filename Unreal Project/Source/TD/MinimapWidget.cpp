#include "MinimapWidget.h"

#include "MobaCameraPawn.h"
#include "MobaPlayerController.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Styling/SlateBrush.h"

UMinimapWidget::UMinimapWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(false);
}

void UMinimapWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureBuilt();
}

void UMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureBuilt();
	SetVisibility(ESlateVisibility::Visible);
	EnsureCapture();
	RefreshCaptureSettings();
}

void UMinimapWidget::NativeDestruct()
{
	DestroyCapture();
	Super::NativeDestruct();
}

void UMinimapWidget::EnsureBuilt()
{
	if (bBuilt)
	{
		return;
	}
	if (!WidgetTree)
	{
		return;
	}
	if (!WidgetTree->RootWidget || !FrameBorder)
	{
		BuildDefaultUI();
	}
	bBuilt = FrameBorder != nullptr && MapImage != nullptr;
}

void UMinimapWidget::BuildDefaultUI()
{
	if (!WidgetTree)
	{
		return;
	}

	RootCanvas = Cast<UCanvasPanel>(WidgetTree->RootWidget);
	if (!RootCanvas)
	{
		RootCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MinimapRoot"));
		WidgetTree->RootWidget = RootCanvas;
	}

	FrameSizeBox = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MinimapSizeBox"));
	FrameSizeBox->SetWidthOverride(MinimapSize);
	FrameSizeBox->SetHeightOverride(MinimapSize);

	if (UCanvasPanelSlot* RootSlot = RootCanvas->AddChildToCanvas(FrameSizeBox))
	{
		RootSlot->SetAnchors(FAnchors(1.f, 1.f, 1.f, 1.f));
		RootSlot->SetAlignment(FVector2D(1.f, 1.f));
		RootSlot->SetAutoSize(true);
		RootSlot->SetOffsets(FMargin(0.f, 0.f, ScreenMargin.X, ScreenMargin.Y));
		RootSlot->SetZOrder(50);
	}

	FrameBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MinimapFrame"));
	FrameBorder->SetPadding(FMargin(3.f));
	FrameBorder->SetBrushColor(BorderColor);
	FrameSizeBox->SetContent(FrameBorder);

	UBorder* InnerBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("MinimapInner"));
	InnerBg->SetPadding(FMargin(0.f));
	InnerBg->SetBrushColor(FrameColor);
	FrameBorder->SetContent(InnerBg);

	UCanvasPanel* MapCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("MinimapCanvas"));
	InnerBg->SetContent(MapCanvas);

	MapImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MinimapImage"));
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.TintColor = FSlateColor(FLinearColor(0.12f, 0.18f, 0.14f, 1.f));
		MapImage->SetBrush(Brush);
	}
	if (UCanvasPanelSlot* ImageSlot = MapCanvas->AddChildToCanvas(MapImage))
	{
		ImageSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		ImageSlot->SetOffsets(FMargin(0.f));
		ImageSlot->SetZOrder(0);
	}

	ChampionMarker = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ChampionMarker"));
	ChampionMarker->SetBrushColor(ChampionMarkerColor);
	ChampionMarker->SetPadding(FMargin(0.f));
	ChampionMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* ChampSlot = MapCanvas->AddChildToCanvas(ChampionMarker))
	{
		ChampSlot->SetAnchors(FAnchors(0.f, 0.f));
		ChampSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		ChampSlot->SetSize(FVector2D(12.f, 12.f));
		ChampSlot->SetZOrder(2);
		ChampionSlot = ChampSlot;
	}

	CameraMarker = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CameraMarker"));
	CameraMarker->SetBrushColor(CameraMarkerColor);
	CameraMarker->SetPadding(FMargin(0.f));
	CameraMarker->SetVisibility(ESlateVisibility::HitTestInvisible);
	if (UCanvasPanelSlot* CamSlot = MapCanvas->AddChildToCanvas(CameraMarker))
	{
		CamSlot->SetAnchors(FAnchors(0.f, 0.f));
		CamSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CamSlot->SetSize(FVector2D(10.f, 10.f));
		CamSlot->SetZOrder(1);
		CameraSlot = CamSlot;
	}
}

void UMinimapWidget::SetWorldBounds(FVector2D InMin, FVector2D InMax)
{
	WorldMin = InMin;
	WorldMax = InMax;
	if (FMath::IsNearlyEqual(WorldMin.X, WorldMax.X))
	{
		WorldMax.X = WorldMin.X + 1000.f;
	}
	if (FMath::IsNearlyEqual(WorldMin.Y, WorldMax.Y))
	{
		WorldMax.Y = WorldMin.Y + 1000.f;
	}
	RefreshCaptureSettings();
}

void UMinimapWidget::SyncBoundsFromCamera()
{
	if (AMobaCameraPawn* Cam = ResolveCameraPawn())
	{
		if (Cam->bConstrainToWorldBounds)
		{
			WorldMin = Cam->MinimumWorldBounds;
			WorldMax = Cam->MaximumWorldBounds;
		}
	}
}

void UMinimapWidget::EnsureCapture()
{
	if (bCaptureReady && CaptureActor && RenderTarget)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const int32 Size = FMath::Clamp(RenderTargetSize, 64, 1024);
	if (!RenderTarget)
	{
		RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(
			this, Size, Size, RTF_RGBA8, FLinearColor::Black, false);
	}

	if (!CaptureActor)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		Params.Owner = GetOwningPlayer();
		CaptureActor = World->SpawnActor<ASceneCapture2D>(
			ASceneCapture2D::StaticClass(),
			FVector::ZeroVector,
			FRotator(-90.f, -90.f, 0.f),
			Params);
	}

	if (!CaptureActor)
	{
		return;
	}

	if (USceneCaptureComponent2D* Cap = CaptureActor->GetCaptureComponent2D())
	{
		Cap->TextureTarget = RenderTarget;
		Cap->ProjectionType = ECameraProjectionMode::Orthographic;
		Cap->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
		Cap->bCaptureEveryFrame = false;
		Cap->bCaptureOnMovement = false;
		Cap->bAlwaysPersistRenderingState = true;
		Cap->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
		Cap->ShowFlags.SetAtmosphere(false);
		Cap->ShowFlags.SetFog(false);
		Cap->ShowFlags.SetBloom(false);
		Cap->ShowFlags.SetEyeAdaptation(false);
	}

	if (MapImage && RenderTarget)
	{
		FSlateBrush Brush = MapImage->GetBrush();
		Brush.SetResourceObject(RenderTarget);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.ImageSize = FVector2D(static_cast<float>(Size), static_cast<float>(Size));
		Brush.TintColor = FSlateColor(FLinearColor::White);
		MapImage->SetBrush(Brush);
		MapImage->SetColorAndOpacity(FLinearColor::White);
	}

	bCaptureReady = true;
	UpdateCaptureTransform();
	if (USceneCaptureComponent2D* Cap = CaptureActor->GetCaptureComponent2D())
	{
		Cap->CaptureScene();
	}
}

void UMinimapWidget::DestroyCapture()
{
	if (CaptureActor && IsValid(CaptureActor))
	{
		CaptureActor->Destroy();
	}
	CaptureActor = nullptr;
	RenderTarget = nullptr;
	bCaptureReady = false;
}

void UMinimapWidget::RefreshCaptureSettings()
{
	if (FrameSizeBox)
	{
		FrameSizeBox->SetWidthOverride(MinimapSize);
		FrameSizeBox->SetHeightOverride(MinimapSize);
	}
	if (bCaptureReady)
	{
		UpdateCaptureTransform();
	}
}

void UMinimapWidget::UpdateCaptureTransform()
{
	if (!CaptureActor)
	{
		return;
	}

	const float MinX = FMath::Min(WorldMin.X, WorldMax.X);
	const float MaxX = FMath::Max(WorldMin.X, WorldMax.X);
	const float MinY = FMath::Min(WorldMin.Y, WorldMax.Y);
	const float MaxY = FMath::Max(WorldMin.Y, WorldMax.Y);
	const float CenterX = (MinX + MaxX) * 0.5f;
	const float CenterY = (MinY + MaxY) * 0.5f;
	const float ExtX = FMath::Max(100.f, MaxX - MinX);
	const float ExtY = FMath::Max(100.f, MaxY - MinY);
	const float Ortho = FMath::Max(ExtX, ExtY);

	CaptureActor->SetActorLocation(FVector(CenterX, CenterY, CaptureHeight));
	CaptureActor->SetActorRotation(FRotator(-90.f, -90.f, 0.f));

	if (USceneCaptureComponent2D* Cap = CaptureActor->GetCaptureComponent2D())
	{
		Cap->OrthoWidth = Ortho;
		Cap->TextureTarget = RenderTarget;
	}
}

void UMinimapWidget::UpdateCapture(float DeltaTime)
{
	if (!bCaptureReady)
	{
		EnsureCapture();
		return;
	}

	CaptureTimer += DeltaTime;
	const float Interval = FMath::Max(0.f, CaptureInterval);
	if (Interval <= KINDA_SMALL_NUMBER || CaptureTimer >= Interval)
	{
		CaptureTimer = 0.f;
		UpdateCaptureTransform();
		if (CaptureActor)
		{
			if (USceneCaptureComponent2D* Cap = CaptureActor->GetCaptureComponent2D())
			{
				Cap->CaptureScene();
			}
		}
	}
}

FVector2D UMinimapWidget::WorldToNormalized(const FVector& WorldLoc) const
{
	const float MinX = FMath::Min(WorldMin.X, WorldMax.X);
	const float MaxX = FMath::Max(WorldMin.X, WorldMax.X);
	const float MinY = FMath::Min(WorldMin.Y, WorldMax.Y);
	const float MaxY = FMath::Max(WorldMin.Y, WorldMax.Y);
	const float ExtX = FMath::Max(1.f, MaxX - MinX);
	const float ExtY = FMath::Max(1.f, MaxY - MinY);

	const float U = (WorldLoc.X - MinX) / ExtX;
	const float V = 1.f - ((WorldLoc.Y - MinY) / ExtY);
	return FVector2D(FMath::Clamp(U, 0.f, 1.f), FMath::Clamp(V, 0.f, 1.f));
}

void UMinimapWidget::PlaceMarker(UBorder* Marker, UCanvasPanelSlot* MarkerSlot, const FVector2D& Normalized, float HalfSize)
{
	if (!Marker || !MarkerSlot)
	{
		return;
	}

	const float Inner = FMath::Max(8.f, MinimapSize - 6.f);
	MarkerSlot->SetPosition(FVector2D(Normalized.X * Inner, Normalized.Y * Inner));
	MarkerSlot->SetSize(FVector2D(HalfSize * 2.f, HalfSize * 2.f));
	Marker->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UMinimapWidget::UpdateMarkers()
{
	if (bShowChampionMarker && ChampionMarker && ChampionSlot)
	{
		if (APawn* Champ = ResolveChampion())
		{
			PlaceMarker(ChampionMarker, ChampionSlot, WorldToNormalized(Champ->GetActorLocation()), 6.f);
			ChampionMarker->SetBrushColor(ChampionMarkerColor);
		}
		else
		{
			ChampionMarker->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else if (ChampionMarker)
	{
		ChampionMarker->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (bShowCameraMarker && CameraMarker && CameraSlot)
	{
		FVector CamLoc = FVector::ZeroVector;
		bool bHaveCam = false;
		if (AMobaCameraPawn* CamPawn = ResolveCameraPawn())
		{
			CamLoc = CamPawn->GetActorLocation();
			bHaveCam = true;
		}
		else if (APlayerController* PC = GetOwningPlayer())
		{
			if (APawn* P = PC->GetPawn())
			{
				CamLoc = P->GetActorLocation();
				bHaveCam = true;
			}
		}

		if (bHaveCam)
		{
			PlaceMarker(CameraMarker, CameraSlot, WorldToNormalized(CamLoc), 5.f);
			CameraMarker->SetBrushColor(CameraMarkerColor);
		}
		else
		{
			CameraMarker->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else if (CameraMarker)
	{
		CameraMarker->SetVisibility(ESlateVisibility::Collapsed);
	}
}

APawn* UMinimapWidget::ResolveChampion() const
{
	if (const AMobaPlayerController* MPC = Cast<AMobaPlayerController>(GetOwningPlayer()))
	{
		if (APawn* Champ = MPC->GetControlledChampion())
		{
			return Champ;
		}
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		return PC->GetPawn();
	}
	return nullptr;
}

AMobaCameraPawn* UMinimapWidget::ResolveCameraPawn() const
{
	if (const AMobaPlayerController* MPC = Cast<AMobaPlayerController>(GetOwningPlayer()))
	{
		return MPC->GetMobaCameraPawn();
	}
	if (APlayerController* PC = GetOwningPlayer())
	{
		return Cast<AMobaCameraPawn>(PC->GetPawn());
	}
	return nullptr;
}

bool UMinimapWidget::LocalToWorld(const FVector2D& LocalPos, const FGeometry& Geometry, FVector& OutWorld) const
{
	const FVector2D LocalSize = Geometry.GetLocalSize();
	if (LocalSize.X <= KINDA_SMALL_NUMBER || LocalSize.Y <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const float U = FMath::Clamp(LocalPos.X / LocalSize.X, 0.f, 1.f);
	const float V = FMath::Clamp(LocalPos.Y / LocalSize.Y, 0.f, 1.f);

	const float MinX = FMath::Min(WorldMin.X, WorldMax.X);
	const float MaxX = FMath::Max(WorldMin.X, WorldMax.X);
	const float MinY = FMath::Min(WorldMin.Y, WorldMax.Y);
	const float MaxY = FMath::Max(WorldMin.Y, WorldMax.Y);

	const float WorldX = FMath::Lerp(MinX, MaxX, U);
	const float WorldY = FMath::Lerp(MaxY, MinY, V);

	float Z = 0.f;
	if (APawn* Champ = ResolveChampion())
	{
		Z = Champ->GetActorLocation().Z;
	}
	else if (AMobaCameraPawn* Cam = ResolveCameraPawn())
	{
		Z = Cam->GetActorLocation().Z;
	}

	OutWorld = FVector(WorldX, WorldY, Z);
	return true;
}

void UMinimapWidget::PanCameraToWorld(const FVector& WorldLoc)
{
	if (AMobaCameraPawn* Cam = ResolveCameraPawn())
	{
		FVector Loc = WorldLoc;
		Loc.Z = Cam->GetActorLocation().Z;
		if (Cam->bConstrainToWorldBounds)
		{
			Loc.X = FMath::Clamp(Loc.X, Cam->MinimumWorldBounds.X, Cam->MaximumWorldBounds.X);
			Loc.Y = FMath::Clamp(Loc.Y, Cam->MinimumWorldBounds.Y, Cam->MaximumWorldBounds.Y);
		}
		Cam->SetActorLocation(Loc);
		return;
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (APawn* P = PC->GetPawn())
		{
			FVector Loc = P->GetActorLocation();
			Loc.X = WorldLoc.X;
			Loc.Y = WorldLoc.Y;
			P->SetActorLocation(Loc);
		}
	}
}

FReply UMinimapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!bClickToPanCamera || !InMouseEvent.GetEffectingButton().IsMouseButton())
	{
		return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
	}

	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		const FVector2D Local = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		FVector World;
		if (LocalToWorld(Local, InGeometry, World))
		{
			PanCameraToWorld(World);
			bDragging = true;
			return FReply::Handled().CaptureMouse(TakeWidget());
		}
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UMinimapWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging && bClickToPanCamera && HasMouseCapture())
	{
		const FVector2D Local = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
		FVector World;
		if (LocalToWorld(Local, InGeometry, World))
		{
			PanCameraToWorld(World);
		}
		return FReply::Handled();
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UMinimapWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging && InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UMinimapWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bBuilt)
	{
		EnsureBuilt();
	}

	SyncBoundsFromCamera();
	UpdateCapture(InDeltaTime);
	UpdateMarkers();
}
