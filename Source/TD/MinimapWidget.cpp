#include "MinimapWidget.h"

#include "MapDiscoveryComponent.h"
#include "MobaCameraPawn.h"
#include "MobaPlayerController.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/LightComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SizeBox.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/Brush.h"
#include "Engine/Light.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/SkyLight.h"
#include "Engine/Texture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Info.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Particles/ParticleSystemComponent.h"
#include "Styling/SlateBrush.h"
#include "Components/SlateWrapperTypes.h"
#include "InputCoreTypes.h"

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
	// Critical: AddToViewport widgets fill the entire screen. Visible on the root
	// captures every LMB as a "minimap click" and remaps screen UV → world, which
	// teleports the camera (or champion) when clicking the ground.
	ApplyHitTestPolicy();
	BindMapPointerEvents();
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

	// Diablo-style fog: black overlay with alpha punched out as the champion explores.
	FogImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("MinimapFog"));
	FogImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	FogImage->SetColorAndOpacity(FLinearColor::White);
	if (UCanvasPanelSlot* FogSlot = MapCanvas->AddChildToCanvas(FogImage))
	{
		FogSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		FogSlot->SetOffsets(FMargin(0.f));
		FogSlot->SetZOrder(1);
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
		ChampSlot->SetZOrder(3);
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
		CamSlot->SetZOrder(2);
		CameraSlot = CamSlot;
	}

	// Crystal — green, above fog, below champion.
	CrystalMarker = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CrystalMarker"));
	CrystalMarker->SetBrushColor(CrystalMarkerColor);
	CrystalMarker->SetPadding(FMargin(0.f));
	CrystalMarker->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* CrystalPanelSlot = MapCanvas->AddChildToCanvas(CrystalMarker))
	{
		CrystalPanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		CrystalPanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CrystalPanelSlot->SetSize(FVector2D(14.f, 14.f));
		CrystalPanelSlot->SetZOrder(4);
		CrystalSlot = CrystalPanelSlot;
	}

	// First enemy spawn — red.
	EnemySpawnMarker = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("EnemySpawnMarker"));
	EnemySpawnMarker->SetBrushColor(EnemySpawnMarkerColor);
	EnemySpawnMarker->SetPadding(FMargin(0.f));
	EnemySpawnMarker->SetVisibility(ESlateVisibility::Collapsed);
	if (UCanvasPanelSlot* SpawnPanelSlot = MapCanvas->AddChildToCanvas(EnemySpawnMarker))
	{
		SpawnPanelSlot->SetAnchors(FAnchors(0.f, 0.f));
		SpawnPanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		SpawnPanelSlot->SetSize(FVector2D(14.f, 14.f));
		SpawnPanelSlot->SetZOrder(4);
		EnemySpawnSlot = SpawnPanelSlot;
	}

	ApplyHitTestPolicy();
	BindMapPointerEvents();
}

void UMinimapWidget::ApplyHitTestPolicy()
{
	// Root / empty canvas pass clicks through to the world.
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	if (RootCanvas)
	{
		RootCanvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	}
	// Only the sized frame (bottom-right) participates in hit tests.
	if (FrameSizeBox)
	{
		FrameSizeBox->SetVisibility(ESlateVisibility::Visible);
	}
	if (FrameBorder)
	{
		FrameBorder->SetVisibility(ESlateVisibility::Visible);
	}
	if (MapImage)
	{
		// Visible leaf so LMB hits the map image directly (not only SObjectWidget bubble).
		MapImage->SetVisibility(ESlateVisibility::Visible);
	}
}

void UMinimapWidget::BindMapPointerEvents()
{
	// Leaf image down-handler (Image only exposes ButtonDown). Capture + drag use native path after LMB.
	if (MapImage)
	{
		MapImage->OnMouseButtonDownEvent.BindDynamic(this, &UMinimapWidget::OnMapMouseButtonDown);
	}
	// Border still receives events in padding and if image brush is unbound.
	if (FrameBorder)
	{
		FrameBorder->OnMouseButtonDownEvent.BindDynamic(this, &UMinimapWidget::OnMapMouseButtonDown);
		FrameBorder->OnMouseMoveEvent.BindDynamic(this, &UMinimapWidget::OnMapMouseMove);
		FrameBorder->OnMouseButtonUpEvent.BindDynamic(this, &UMinimapWidget::OnMapMouseButtonUp);
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
	// Only seed discovery when it still looks unauthored (huge / free-cam).
	if (DiscoverySource)
	{
		float Cx = 0.f, Cy = 0.f, Ortho = 1.f;
		DiscoverySource->GetOrthoWorldRect(Cx, Cy, Ortho);
		if (Ortho > 70000.f)
		{
			DiscoverySource->SetWorldBounds(WorldMin, WorldMax);
		}
	}
	RefreshCaptureSettings();
}

void UMinimapWidget::SetDiscoverySource(UMapDiscoveryComponent* InDiscovery)
{
	DiscoverySource = InDiscovery;
	if (DiscoverySource)
	{
		// Minimap follows shared discovery bounds / settings — discovery is the authority.
		// Only seed bounds into discovery when it still has defaults and we have a tight fit.
		float DiscCx = 0.f, DiscCy = 0.f, DiscOrtho = 1.f;
		DiscoverySource->GetOrthoWorldRect(DiscCx, DiscCy, DiscOrtho);
		if (DiscOrtho > 70000.f)
		{
			// Discovery still on free-cam-sized defaults — seed from minimap fit.
			DiscoverySource->SetWorldBounds(WorldMin, WorldMax);
		}
		else
		{
			// Pull authoritative bounds onto the minimap so capture lines up.
			FVector2D DMin = DiscoverySource->WorldMin;
			FVector2D DMax = DiscoverySource->WorldMax;
			WorldMin = DMin;
			WorldMax = DMax;
		}
		DiscoverySource->DiscoveryRadius = DiscoveryRadius;
		DiscoverySource->DiscoverySoftness = DiscoverySoftness;
		if (DiscoverySource->MaskSize != DiscoveryMaskSize && DiscoveryMaskSize >= 64)
		{
			DiscoverySource->MaskSize = DiscoveryMaskSize;
		}
		DiscoverySource->StampDistance = DiscoveryStampDistance;
		DiscoverySource->UndiscoveredColor = UndiscoveredColor;
		// Never disable shared discovery from the minimap path — world FOW also uses it.
		if (bMapDiscoveryEnabled)
		{
			DiscoverySource->SetEnabled(true);
		}
		if (APawn* Champ = ResolveChampion())
		{
			DiscoverySource->SetExplorer(Champ);
		}
		// Cache landmarks + punch FOW (shared mask → minimap + world FOW).
		RefreshLandmarks();
	}
}

void UMinimapWidget::SetMapDiscoveryEnabled(bool bEnabled)
{
	bMapDiscoveryEnabled = bEnabled;
	if (DiscoverySource)
	{
		DiscoverySource->SetEnabled(bEnabled);
	}
	if (!bMapDiscoveryEnabled)
	{
		if (FogImage)
		{
			FogImage->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	EnsureDiscoveryFog();
	// Force a stamp at current position next tick / immediately.
	bHasDiscoveryStamp = false;
	UpdateMapDiscovery();
}

void UMinimapWidget::ResetMapDiscovery()
{
	if (DiscoverySource)
	{
		DiscoverySource->ResetDiscovery();
		UpdateMapDiscovery();
		return;
	}
	ClearDiscoveryFog();
	bHasDiscoveryStamp = false;
	if (bMapDiscoveryEnabled)
	{
		UpdateMapDiscovery();
	}
}

void UMinimapWidget::RevealAtWorldLocation(const FVector& WorldLocation)
{
	if (!bMapDiscoveryEnabled)
	{
		return;
	}
	if (DiscoverySource)
	{
		DiscoverySource->RevealAtWorldLocation(WorldLocation);
		UpdateMapDiscovery();
		return;
	}
	EnsureDiscoveryFog();
	StampDiscoveryAtNormalized(WorldToNormalized(WorldLocation));
	LastDiscoveryStampLocation = WorldLocation;
	bHasDiscoveryStamp = true;
	FlushDiscoveryFogTexture();
}

void UMinimapWidget::EnsureDiscoveryFog()
{
	if (!bMapDiscoveryEnabled)
	{
		if (FogImage)
		{
			FogImage->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	// Shared discovery component owns the fog texture.
	if (DiscoverySource)
	{
		UTexture2D* SharedFog = DiscoverySource->GetFogTexture();
		if (FogImage && SharedFog)
		{
			const int32 Size = SharedFog->GetSizeX();
			ApplyCaptureBrush(
				FogImage,
				SharedFog,
				FVector2D(static_cast<float>(Size), static_cast<float>(Size)));
			ApplyMinimapAxisFlip();
			FogImage->SetVisibility(ESlateVisibility::HitTestInvisible);
			FogImage->SetColorAndOpacity(FLinearColor::White);
		}
		return;
	}

	const int32 Size = FMath::Clamp(DiscoveryMaskSize, 64, 1024);
	const bool bNeedNewTexture = !DiscoveryFogTexture || DiscoveryFogTextureSize != Size;
	if (bNeedNewTexture)
	{
		DiscoveryFogTextureSize = Size;
		DiscoveryFogPixels.SetNumUninitialized(Size * Size);

		// Transient so we can UpdateTextureRegions every stamp without asset dependency.
		DiscoveryFogTexture = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);
		if (DiscoveryFogTexture)
		{
			DiscoveryFogTexture->CompressionSettings = TC_VectorDisplacementmap;
			DiscoveryFogTexture->SRGB = false;
			DiscoveryFogTexture->Filter = TF_Bilinear;
			DiscoveryFogTexture->AddressX = TA_Clamp;
			DiscoveryFogTexture->AddressY = TA_Clamp;
			DiscoveryFogTexture->UpdateResource();
		}

		ClearDiscoveryFog();
		bHasDiscoveryStamp = false;
	}

	if (FogImage && DiscoveryFogTexture)
	{
		ApplyCaptureBrush(
			FogImage,
			DiscoveryFogTexture,
			FVector2D(static_cast<float>(Size), static_cast<float>(Size)));
		ApplyMinimapAxisFlip();
		FogImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		FogImage->SetColorAndOpacity(FLinearColor::White);
	}
}

void UMinimapWidget::UpdateMapDiscovery()
{
	if (!bMapDiscoveryEnabled)
	{
		if (FogImage && FogImage->GetVisibility() != ESlateVisibility::Collapsed)
		{
			FogImage->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	// Shared discovery owns stamping + bounds; minimap only mirrors the fog texture.
	if (DiscoverySource)
	{
		// Pull bounds for capture / markers only — do not overwrite discovery UV domain every tick
		// or reveals walk off the player as the minimap refits.
		float Cx = 0.f, Cy = 0.f, Ortho = 1.f;
		DiscoverySource->GetOrthoWorldRect(Cx, Cy, Ortho);
		const float Half = Ortho * 0.5f;
		WorldMin = FVector2D(Cx - Half, Cy - Half);
		WorldMax = FVector2D(Cx + Half, Cy + Half);

		if (APawn* Champ = ResolveChampion())
		{
			DiscoverySource->SetExplorer(Champ);
		}
		EnsureDiscoveryFog();
		return;
	}

	EnsureDiscoveryFog();

	APawn* Champ = ResolveChampion();
	if (!Champ)
	{
		return;
	}

	const FVector Loc = Champ->GetActorLocation();
	const bool bMovedFarEnough = !bHasDiscoveryStamp
		|| FVector::DistSquared2D(Loc, LastDiscoveryStampLocation)
			>= FMath::Square(FMath::Max(0.f, DiscoveryStampDistance));

	if (bMovedFarEnough)
	{
		StampDiscoveryAtNormalized(WorldToNormalized(Loc));
		LastDiscoveryStampLocation = Loc;
		bHasDiscoveryStamp = true;
		FlushDiscoveryFogTexture();
	}
}

void UMinimapWidget::SyncBoundsFromCamera()
{
	// Only honor camera bounds when they come from an explicit bounds source in the level.
	// The free-cam defaults (±50k) are huge clamps and turn the minimap into empty solid colour.
	if (AMobaCameraPawn* Cam = ResolveCameraPawn())
	{
		if (Cam->bConstrainToWorldBounds && Cam->WorldBoundsSource)
		{
			WorldMin = Cam->MinimumWorldBounds;
			WorldMax = Cam->MaximumWorldBounds;
			bDidAutoFitBounds = true; // don't fight a authored bounds volume
		}
	}
}

void UMinimapWidget::RefitBoundsFromLevel()
{
	UWorld* World = GetWorld();
	if (!World || !bAutoFitBoundsToLevel)
	{
		return;
	}

	FBox2D Accum(ForceInit);
	bool bAny = false;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor || !IsValid(Actor) || Actor->IsHidden())
		{
			continue;
		}

		// Skip system / non-geometry actors that inflate or confuse bounds.
		// AInfo covers sky atmosphere / clouds / nav managers in many setups.
		if (Actor->IsA(AInfo::StaticClass())
			|| Actor->IsA(APlayerController::StaticClass())
			|| Actor->IsA(APawn::StaticClass())
			|| Actor->IsA(APlayerStart::StaticClass())
			|| Actor->IsA(ASceneCapture2D::StaticClass())
			|| Actor->IsA(ABrush::StaticClass())
			|| Actor->IsA(APostProcessVolume::StaticClass())
			|| Actor->IsA(ALight::StaticClass())
			|| Actor->IsA(ASkyLight::StaticClass()))
		{
			continue;
		}

		// Prefer mesh-bearing components over full actor bounds (nav volumes, etc.).
		bool bUsedComp = false;
		TInlineComponentArray<UPrimitiveComponent*> PrimComps(Actor);
		for (UPrimitiveComponent* Prim : PrimComps)
		{
			if (!Prim || !Prim->IsRegistered() || !Prim->IsVisible())
			{
				continue;
			}
			// Lights/particles don't define the playable floor layout.
			if (Prim->IsA(ULightComponentBase::StaticClass())
				|| Prim->IsA(UParticleSystemComponent::StaticClass()))
			{
				continue;
			}

			const FBoxSphereBounds B = Prim->Bounds;
			const FVector Ext = B.BoxExtent;
			if (Ext.X < 5.f && Ext.Y < 5.f)
			{
				continue;
			}
			// Ignore giant volumes used only for lighting / nav.
			if (Ext.X > 80000.f || Ext.Y > 80000.f)
			{
				continue;
			}

			const FVector Min = B.Origin - Ext;
			const FVector Max = B.Origin + Ext;
			Accum += FVector2D(Min.X, Min.Y);
			Accum += FVector2D(Max.X, Max.Y);
			bAny = true;
			bUsedComp = true;
		}

		if (!bUsedComp)
		{
			FVector Origin;
			FVector Extent;
			Actor->GetActorBounds(false, Origin, Extent);
			if (Extent.X < 5.f && Extent.Y < 5.f)
			{
				continue;
			}
			if (Extent.X > 80000.f || Extent.Y > 80000.f)
			{
				continue;
			}
			Accum += FVector2D(Origin.X - Extent.X, Origin.Y - Extent.Y);
			Accum += FVector2D(Origin.X + Extent.X, Origin.Y + Extent.Y);
			bAny = true;
		}
	}

	if (!bAny || !Accum.bIsValid)
	{
		return;
	}

	const float Pad = FMath::Max(0.f, AutoFitBoundsPadding);
	const FVector2D NewMin(Accum.Min.X - Pad, Accum.Min.Y - Pad);
	const FVector2D NewMax(Accum.Max.X + Pad, Accum.Max.Y + Pad);

	// Ignore degenerate envelopes.
	if ((NewMax.X - NewMin.X) < 200.f || (NewMax.Y - NewMin.Y) < 200.f)
	{
		return;
	}

	WorldMin = NewMin;
	WorldMax = NewMax;
	bDidAutoFitBounds = true;
	RefreshCaptureSettings();
}

void UMinimapWidget::GetOrthoWorldRect(float& OutCenterX, float& OutCenterY, float& OutOrthoWidth) const
{
	const float MinX = FMath::Min(WorldMin.X, WorldMax.X);
	const float MaxX = FMath::Max(WorldMin.X, WorldMax.X);
	const float MinY = FMath::Min(WorldMin.Y, WorldMax.Y);
	const float MaxY = FMath::Max(WorldMin.Y, WorldMax.Y);
	OutCenterX = (MinX + MaxX) * 0.5f;
	OutCenterY = (MinY + MaxY) * 0.5f;
	// Square orthographic volume: cover full bounds so the RT is a true top-down map projection.
	const float ExtX = FMath::Max(100.f, MaxX - MinX);
	const float ExtY = FMath::Max(100.f, MaxY - MinY);
	OutOrthoWidth = FMath::Max(ExtX, ExtY);
}

void UMinimapWidget::ConfigureSceneCapture()
{
	if (!CaptureActor)
	{
		return;
	}

	USceneCaptureComponent2D* Cap = CaptureActor->GetCaptureComponent2D();
	if (!Cap)
	{
		return;
	}

	Cap->TextureTarget = RenderTarget;
	Cap->ProjectionType = ECameraProjectionMode::Orthographic;
	// Lit final colour of the level — same identity as the playable scene, top-down.
	Cap->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;
	Cap->bCaptureEveryFrame = false;
	Cap->bCaptureOnMovement = false;
	Cap->bAlwaysPersistRenderingState = true;
	Cap->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	Cap->bUseCustomProjectionMatrix = false;
	Cap->MaxViewDistanceOverride = -1.f;
	// Composite cleanly into UMG without HDR bloom wash.
	Cap->CompositeMode = ESceneCaptureCompositeMode::SCCM_Overwrite;

	FEngineShowFlags& Flags = Cap->ShowFlags;
	// Readable map: lit geometry, no weather / heavy post that kills small-RT clarity.
	Flags.SetLighting(true);
	Flags.SetSkyLighting(true);
	Flags.SetStaticMeshes(true);
	Flags.SetInstancedStaticMeshes(true);
	Flags.SetLandscape(true);
	Flags.SetBSP(true);
	Flags.SetNaniteMeshes(true);
	Flags.SetSkeletalMeshes(true);
	Flags.SetInstancedFoliage(true);
	Flags.SetInstancedGrass(false);

	Flags.SetAtmosphere(false);
	Flags.SetFog(false);
	Flags.SetVolumetricFog(false);
	Flags.SetCloud(false);
	Flags.SetBloom(false);
	Flags.SetEyeAdaptation(false);
	Flags.SetToneCurve(false);
	Flags.SetVignette(false);
	Flags.SetMotionBlur(false);
	Flags.SetDepthOfField(false);
	Flags.SetAntiAliasing(true);
	Flags.SetTemporalAA(false);
	Flags.SetScreenSpaceReflections(false);
	Flags.SetContactShadows(false);
	Flags.SetDynamicShadows(true);
	Flags.SetAmbientOcclusion(false);
	Flags.SetGlobalIllumination(false);
	Flags.SetPostProcessing(true);
	Flags.SetParticles(false);
	Flags.SetNiagara(false);
	Flags.SetTranslucency(true);
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

	if (bAutoFitBoundsToLevel && !bDidAutoFitBounds)
	{
		RefitBoundsFromLevel();
	}

	const int32 Size = FMath::Clamp(RenderTargetSize, 64, 1024);
	if (!RenderTarget)
	{
		RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(
			this, Size, Size, RTF_RGBA8, FLinearColor(0.05f, 0.07f, 0.06f, 1.f), false);
	}

	if (!CaptureActor)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.ObjectFlags |= RF_Transient;
		Params.Owner = GetOwningPlayer();
		// Top-down: pitch -90 looks at ground. Yaw -90 for the RT; WorldToNormalized
		// mirrors X so clicks/markers match the capture (pan left → camera left).
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

	ConfigureSceneCapture();

	if (MapImage && RenderTarget)
	{
		ApplyCaptureBrush(
			MapImage,
			RenderTarget,
			FVector2D(static_cast<float>(Size), static_cast<float>(Size)));
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

	DiscoveryFogTexture = nullptr;
	DiscoveryFogPixels.Reset();
	DiscoveryFogTextureSize = 0;
	bHasDiscoveryStamp = false;
	bDiscoveryFogDirty = false;
	bDidAutoFitBounds = false;
	AutoFitTimer = 0.f;
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
		ConfigureSceneCapture();
		UpdateCaptureTransform();
	}
}

void UMinimapWidget::UpdateCaptureTransform()
{
	if (!CaptureActor)
	{
		return;
	}

	float CenterX = 0.f;
	float CenterY = 0.f;
	float Ortho = 1000.f;
	GetOrthoWorldRect(CenterX, CenterY, Ortho);

	CaptureActor->SetActorLocation(FVector(CenterX, CenterY, CaptureHeight));
	CaptureActor->SetActorRotation(FRotator(-90.f, -90.f, 0.f));

	if (USceneCaptureComponent2D* Cap = CaptureActor->GetCaptureComponent2D())
	{
		Cap->ProjectionType = ECameraProjectionMode::Orthographic;
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
	// Same square orthographic footprint as the scene capture.
	float CenterX = 0.f;
	float CenterY = 0.f;
	float Ortho = 1.f;
	GetOrthoWorldRect(CenterX, CenterY, Ortho);
	const float Half = Ortho * 0.5f;

	// Capture yaw -90 mirrors world X on the RT; invert U so markers/clicks match the picture.
	const float U = 0.5f - (WorldLoc.X - CenterX) / Ortho;
	// Screen +Y is down; world +Y maps up the image with capture yaw -90.
	const float V = 0.5f - (WorldLoc.Y - CenterY) / Ortho;
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

void UMinimapWidget::ClearDiscoveryFog()
{
	if (DiscoveryFogTextureSize <= 0)
	{
		return;
	}

	const FColor FogColor = UndiscoveredColor.ToFColor(true);
	// Alpha encodes remaining fog (FogColor.A = full cover). Stamps only lower alpha.
	const FColor Pixel(FogColor.R, FogColor.G, FogColor.B, FogColor.A);
	for (FColor& P : DiscoveryFogPixels)
	{
		P = Pixel;
	}
	bDiscoveryFogDirty = true;
	FlushDiscoveryFogTexture();
}

void UMinimapWidget::StampDiscoveryAtNormalized(const FVector2D& NormalizedUV)
{
	if (DiscoveryFogTextureSize <= 0 || DiscoveryFogPixels.Num() != DiscoveryFogTextureSize * DiscoveryFogTextureSize)
	{
		return;
	}

	float CenterX = 0.f;
	float CenterY = 0.f;
	float Ortho = 1.f;
	GetOrthoWorldRect(CenterX, CenterY, Ortho);
	if (Ortho <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float RadiusWorld = FMath::Max(50.f, DiscoveryRadius);
	const float RadiusUV = RadiusWorld / Ortho;
	const float Soft = FMath::Clamp(DiscoverySoftness, 0.f, 1.f);
	// Inner fully-clear radius + soft falloff ring (matches Diablo's soft reveal edge).
	const float HardUV = RadiusUV * (1.f - Soft);
	const float SoftUV = FMath::Max(RadiusUV * Soft, 1.f / static_cast<float>(DiscoveryFogTextureSize));

	const int32 Size = DiscoveryFogTextureSize;
	const float Cx = FMath::Clamp(NormalizedUV.X, 0.f, 1.f) * static_cast<float>(Size - 1);
	const float Cy = FMath::Clamp(NormalizedUV.Y, 0.f, 1.f) * static_cast<float>(Size - 1);
	const float RadiusPx = RadiusUV * static_cast<float>(Size);
	const float HardPx = HardUV * static_cast<float>(Size);
	const float SoftPx = SoftUV * static_cast<float>(Size);
	const uint8 MaxFogAlpha = UndiscoveredColor.ToFColor(true).A;

	const int32 MinX = FMath::Clamp(FMath::FloorToInt(Cx - RadiusPx - 1.f), 0, Size - 1);
	const int32 MaxX = FMath::Clamp(FMath::CeilToInt(Cx + RadiusPx + 1.f), 0, Size - 1);
	const int32 MinY = FMath::Clamp(FMath::FloorToInt(Cy - RadiusPx - 1.f), 0, Size - 1);
	const int32 MaxY = FMath::Clamp(FMath::CeilToInt(Cy + RadiusPx + 1.f), 0, Size - 1);

	bool bChanged = false;
	for (int32 Y = MinY; Y <= MaxY; ++Y)
	{
		for (int32 X = MinX; X <= MaxX; ++X)
		{
			const float DX = static_cast<float>(X) - Cx;
			const float DY = static_cast<float>(Y) - Cy;
			const float Dist = FMath::Sqrt(DX * DX + DY * DY);

			uint8 TargetAlpha = MaxFogAlpha;
			if (Dist <= HardPx)
			{
				TargetAlpha = 0;
			}
			else if (Dist < HardPx + SoftPx)
			{
				const float T = (Dist - HardPx) / SoftPx;
				TargetAlpha = static_cast<uint8>(FMath::Clamp(
					FMath::RoundToInt(T * static_cast<float>(MaxFogAlpha)), 0, static_cast<int32>(MaxFogAlpha)));
			}
			else
			{
				continue;
			}

			FColor& Pixel = DiscoveryFogPixels[Y * Size + X];
			if (TargetAlpha < Pixel.A)
			{
				Pixel.A = TargetAlpha;
				bChanged = true;
			}
		}
	}

	if (bChanged)
	{
		bDiscoveryFogDirty = true;
	}
}

void UMinimapWidget::FlushDiscoveryFogTexture()
{
	if (!bDiscoveryFogDirty || !DiscoveryFogTexture || DiscoveryFogTextureSize <= 0)
	{
		return;
	}
	if (DiscoveryFogPixels.Num() != DiscoveryFogTextureSize * DiscoveryFogTextureSize)
	{
		return;
	}

	const int32 Size = DiscoveryFogTextureSize;
	const int32 Bytes = Size * Size * static_cast<int32>(sizeof(FColor));

	// Own copies until the RHI upload finishes — pixels may be re-stamped next move.
	uint8* SrcCopy = new uint8[Bytes];
	FMemory::Memcpy(SrcCopy, DiscoveryFogPixels.GetData(), Bytes);
	FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, Size, Size);

	DiscoveryFogTexture->UpdateTextureRegions(
		0,
		1,
		Region,
		static_cast<uint32>(Size * sizeof(FColor)),
		sizeof(FColor),
		SrcCopy,
		[](uint8* SrcData, const FUpdateTextureRegion2D* Regions)
		{
			delete[] SrcData;
			delete Regions;
		});
	bDiscoveryFogDirty = false;
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

	// Crystal (green)
	if (bShowCrystalMarker && CrystalMarker && CrystalSlot)
	{
		if (AActor* Crystal = CachedCrystalActor.Get())
		{
			PlaceMarker(CrystalMarker, CrystalSlot, WorldToNormalized(Crystal->GetActorLocation()), CrystalMarkerHalfSize);
			CrystalMarker->SetBrushColor(CrystalMarkerColor);
		}
		else
		{
			CrystalMarker->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else if (CrystalMarker)
	{
		CrystalMarker->SetVisibility(ESlateVisibility::Collapsed);
	}

	// First enemy spawn (red)
	if (bShowEnemySpawnMarker && EnemySpawnMarker && EnemySpawnSlot)
	{
		if (AActor* Spawn = CachedEnemySpawnActor.Get())
		{
			PlaceMarker(EnemySpawnMarker, EnemySpawnSlot, WorldToNormalized(Spawn->GetActorLocation()), EnemySpawnMarkerHalfSize);
			EnemySpawnMarker->SetBrushColor(EnemySpawnMarkerColor);
		}
		else
		{
			EnemySpawnMarker->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else if (EnemySpawnMarker)
	{
		EnemySpawnMarker->SetVisibility(ESlateVisibility::Collapsed);
	}
}

AActor* UMinimapWidget::FindFirstActorOfSoftClass(const FSoftClassPath& ClassPath) const
{
	UWorld* World = GetWorld();
	if (!World || !ClassPath.IsValid())
	{
		return nullptr;
	}

	UClass* Cls = ClassPath.TryLoadClass<AActor>();
	if (!Cls)
	{
		return nullptr;
	}

	AActor* First = nullptr;
	for (TActorIterator<AActor> It(World, Cls); It; ++It)
	{
		AActor* Actor = *It;
		if (!IsValid(Actor) || Actor->IsActorBeingDestroyed())
		{
			continue;
		}
		// Prefer non-hidden game instances; first by iteration order for "first spawn".
		if (!First)
		{
			First = Actor;
		}
		// Keep the earliest-spawned / lowest name as a stable "first" pick.
		if (Actor->GetName() < First->GetName())
		{
			First = Actor;
		}
	}
	return First;
}

void UMinimapWidget::RegisterLandmarkFogReveals()
{
	if (!DiscoverySource)
	{
		return;
	}

	const float Radius = FMath::Max(100.f, LandmarkRevealRadius);

	if (AActor* Crystal = CachedCrystalActor.Get())
	{
		DiscoverySource->RegisterPermanentReveal(Crystal->GetActorLocation(), Radius);
	}
	if (AActor* Spawn = CachedEnemySpawnActor.Get())
	{
		DiscoverySource->RegisterPermanentReveal(Spawn->GetActorLocation(), Radius);
	}
}

void UMinimapWidget::RefreshLandmarks()
{
	CachedCrystalActor = FindFirstActorOfSoftClass(CrystalActorClass);
	CachedEnemySpawnActor = FindFirstActorOfSoftClass(EnemySpawnerActorClass);
	RegisterLandmarkFogReveals();
	bLandmarksRegistered = CachedCrystalActor.IsValid() || CachedEnemySpawnActor.IsValid();
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
	// Never fall back to the gameplay champion — minimap pans the free camera only.
	if (AMobaPlayerController* MPC = Cast<AMobaPlayerController>(GetOwningPlayer()))
	{
		if (AMobaCameraPawn* Cam = MPC->GetMobaCameraPawn())
		{
			return Cam;
		}
		// Prefer existing world camera before spawning a new one mid-click.
		if (UWorld* World = MPC->GetWorld())
		{
			TActorIterator<AMobaCameraPawn> It(World);
			if (It)
			{
				return *It;
			}
		}
		return MPC->GetOrSpawnCameraPawn();
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (AMobaCameraPawn* Cam = Cast<AMobaCameraPawn>(PC->GetPawn()))
		{
			return Cam;
		}
		if (AMobaCameraPawn* Cam = Cast<AMobaCameraPawn>(PC->GetViewTarget()))
		{
			return Cam;
		}
		if (UWorld* World = PC->GetWorld())
		{
			TActorIterator<AMobaCameraPawn> It(World);
			if (It)
			{
				return *It;
			}
		}
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

	float CenterX = 0.f;
	float CenterY = 0.f;
	float Ortho = 1.f;
	GetOrthoWorldRect(CenterX, CenterY, Ortho);

	// Must match WorldToNormalized (X inverted for SceneCapture yaw -90).
	const float WorldX = CenterX - (U - 0.5f) * Ortho;
	const float WorldY = CenterY - (V - 0.5f) * Ortho;

	float Z = 0.f;
	if (AMobaCameraPawn* Cam = ResolveCameraPawn())
	{
		// Prefer free-camera height for any world point derived from the minimap.
		Z = Cam->GetActorLocation().Z;
	}
	else if (APawn* Champ = ResolveChampion())
	{
		Z = Champ->GetActorLocation().Z;
	}

	OutWorld = FVector(WorldX, WorldY, Z);
	return true;
}

bool UMinimapWidget::TryPointerToWorld(const FPointerEvent& MouseEvent, FVector& OutWorld) const
{
	// InGeometry from UserWidget is full-viewport; always map against the frame.
	const UWidget* Frame = FrameSizeBox
		? static_cast<const UWidget*>(FrameSizeBox.Get())
		: static_cast<const UWidget*>(FrameBorder.Get());
	if (!Frame)
	{
		return false;
	}

	const FGeometry& Geo = Frame->GetCachedGeometry();
	const FVector2D Size = Geo.GetLocalSize();
	if (Size.X <= KINDA_SMALL_NUMBER || Size.Y <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector2D Local = Geo.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	if (Local.X < 0.f || Local.Y < 0.f || Local.X > Size.X || Local.Y > Size.Y)
	{
		return false;
	}

	return LocalToWorld(Local, Geo, OutWorld);
}

bool UMinimapWidget::IsScreenPosOverMap(FVector2D ScreenPos) const
{
	const UWidget* Frame = FrameSizeBox
		? static_cast<const UWidget*>(FrameSizeBox.Get())
		: static_cast<const UWidget*>(FrameBorder.Get());
	if (!Frame || !IsInViewport())
	{
		return false;
	}

	const FGeometry& Geo = Frame->GetCachedGeometry();
	const FVector2D Size = Geo.GetLocalSize();
	if (Size.X <= KINDA_SMALL_NUMBER || Size.Y <= KINDA_SMALL_NUMBER)
	{
		return false;
	}

	const FVector2D Local = Geo.AbsoluteToLocal(ScreenPos);
	return Local.X >= 0.f && Local.Y >= 0.f && Local.X <= Size.X && Local.Y <= Size.Y;
}

void UMinimapWidget::ApplyCaptureBrush(UImage* TargetImage, UTexture* Texture, const FVector2D& ImageSize) const
{
	if (!TargetImage || !Texture)
	{
		return;
	}

	FSlateBrush Brush = TargetImage->GetBrush();
	Brush.SetResourceObject(Texture);
	Brush.DrawAs = ESlateBrushDrawType::Image;
	Brush.ImageSize = ImageSize;
	Brush.TintColor = FSlateColor(FLinearColor::White);
	Brush.SetUVRegion(FBox2D(FVector2D(0.f, 0.f), FVector2D(1.f, 1.f)));
	TargetImage->SetBrush(Brush);
	TargetImage->SetColorAndOpacity(FLinearColor::White);
}

void UMinimapWidget::ApplyMinimapAxisFlip() const
{
	// Markers/clicks use inverted U (SceneCapture yaw -90). Discovery fog stamps use
	// the unflipped world FOW UV domain — mirror only the fog overlay so holes line up.
	if (FogImage)
	{
		FogImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		FogImage->SetRenderScale(FVector2D(-1.f, 1.f));
	}
}

void UMinimapWidget::PanCameraToWorld(const FVector& WorldLoc)
{
	// Strictly camera-only. Never move the champion/view-target character — that was
	// teleporting the player off geo (Z kept, XY void) so they fell forever.
	AMobaCameraPawn* Cam = ResolveCameraPawn();
	if (!Cam)
	{
		return;
	}

	// Guard: never treat the controlled champion as the free camera.
	if (const AMobaPlayerController* MPC = Cast<AMobaPlayerController>(GetOwningPlayer()))
	{
		if (APawn* Champ = MPC->GetControlledChampion())
		{
			if (Cam == Champ)
			{
				return;
			}
		}
	}

	Cam->SnapToWorldXY(WorldLoc);
}

void UMinimapWidget::MoveChampionToWorld(const FVector& WorldLoc)
{
	AMobaPlayerController* MPC = Cast<AMobaPlayerController>(GetOwningPlayer());
	if (!MPC || !MPC->bEnableClickToMoveChampion)
	{
		return;
	}

	MPC->MoveChampionToLocation(WorldLoc);
}

FReply UMinimapWidget::HandleMapPointerDown(const FPointerEvent& MouseEvent)
{
	const FKey Button = MouseEvent.GetEffectingButton();

	if (Button == EKeys::RightMouseButton)
	{
		if (!bClickToMoveChampion)
		{
			return FReply::Unhandled();
		}

		FVector World;
		if (!TryPointerToWorld(MouseEvent, World))
		{
			return FReply::Unhandled();
		}

		MoveChampionToWorld(World);
		return FReply::Handled();
	}

	if (!bClickToPanCamera || Button != EKeys::LeftMouseButton)
	{
		return FReply::Unhandled();
	}

	FVector World;
	if (!TryPointerToWorld(MouseEvent, World))
	{
		return FReply::Unhandled();
	}

	PanCameraToWorld(World);
	bDragging = true;
	return FReply::Handled().CaptureMouse(TakeWidget());
}

FReply UMinimapWidget::HandleMapPointerMove(const FPointerEvent& MouseEvent)
{
	if (!bDragging || !bClickToPanCamera)
	{
		return FReply::Unhandled();
	}

	FVector World;
	if (TryPointerToWorld(MouseEvent, World))
	{
		PanCameraToWorld(World);
	}
	return FReply::Handled();
}

FReply UMinimapWidget::HandleMapPointerUp(const FPointerEvent& MouseEvent)
{
	if (bDragging && MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		bDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

FEventReply UMinimapWidget::OnMapMouseButtonDown(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	FEventReply Reply;
	Reply.NativeReply = HandleMapPointerDown(MouseEvent);
	return Reply;
}

FEventReply UMinimapWidget::OnMapMouseMove(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	FEventReply Reply;
	Reply.NativeReply = HandleMapPointerMove(MouseEvent);
	return Reply;
}

FEventReply UMinimapWidget::OnMapMouseButtonUp(FGeometry MyGeometry, const FPointerEvent& MouseEvent)
{
	FEventReply Reply;
	Reply.NativeReply = HandleMapPointerUp(MouseEvent);
	return Reply;
}

FReply UMinimapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FReply Reply = HandleMapPointerDown(InMouseEvent);
	if (Reply.IsEventHandled())
	{
		return Reply;
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UMinimapWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bDragging && (HasMouseCapture() || InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton)))
	{
		const FReply Reply = HandleMapPointerMove(InMouseEvent);
		if (Reply.IsEventHandled())
		{
			return Reply;
		}
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

FReply UMinimapWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	const FReply Reply = HandleMapPointerUp(InMouseEvent);
	if (Reply.IsEventHandled())
	{
		return Reply;
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

	if (bAutoFitBoundsToLevel)
	{
		const float Rescan = FMath::Max(0.f, AutoFitRescanInterval);
		if (!bDidAutoFitBounds)
		{
			RefitBoundsFromLevel();
		}
		else if (Rescan > KINDA_SMALL_NUMBER)
		{
			AutoFitTimer += InDeltaTime;
			if (AutoFitTimer >= Rescan)
			{
				AutoFitTimer = 0.f;
				const AMobaCameraPawn* Cam = ResolveCameraPawn();
				if (!(Cam && Cam->WorldBoundsSource))
				{
					RefitBoundsFromLevel();
				}
			}
		}
	}

	UpdateCapture(InDeltaTime);
	UpdateMapDiscovery();

	// Landmarks may appear after the widget (level streaming) — retry while missing.
	LandmarkRefreshTimer += InDeltaTime;
	const bool bMissingLandmark =
		(bShowCrystalMarker && !CachedCrystalActor.IsValid())
		|| (bShowEnemySpawnMarker && !CachedEnemySpawnActor.IsValid());
	if (!bLandmarksRegistered || (bMissingLandmark && LandmarkRefreshTimer >= 1.0f))
	{
		RefreshLandmarks();
		LandmarkRefreshTimer = 0.f;
	}

	UpdateMarkers();
}
