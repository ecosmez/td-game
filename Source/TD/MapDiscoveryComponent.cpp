#include "MapDiscoveryComponent.h"

#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "TextureResource.h"

UMapDiscoveryComponent::UMapDiscoveryComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
	// Stamp before presentation components read the mask this frame.
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
	bWantsInitializeComponent = true;
}

void UMapDiscoveryComponent::BeginPlay()
{
	Super::BeginPlay();
	// Legacy overlays were nearly opaque. LoL dim vision must keep the map readable.
	if (UndiscoveredColor.A > 0.85f)
	{
		UndiscoveredColor = FLinearColor(0.f, 0.f, 0.f, 0.32f);
	}
	if (bEnabled)
	{
		EnsureFogTexture();
		RebuildVisionMask();
	}
}

void UMapDiscoveryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	FogTexture = nullptr;
	FogPixels.Reset();
	FogTextureSize = 0;
	bHasStamp = false;
	bFogDirty = false;
	Super::EndPlay(EndPlayReason);
}

void UMapDiscoveryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (bEnabled)
	{
		RebuildVisionMask(DeltaTime);
	}
}

void UMapDiscoveryComponent::SetEnabled(bool bInEnabled)
{
	bEnabled = bInEnabled;
	if (!bEnabled)
	{
		return;
	}
	EnsureFogTexture();
	bHasStamp = false;
	RebuildVisionMask();
}

void UMapDiscoveryComponent::SetWorldBounds(FVector2D InMin, FVector2D InMax)
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
}

void UMapDiscoveryComponent::SetExplorer(AActor* InExplorer)
{
	if (Explorer == InExplorer)
	{
		return;
	}
	Explorer = InExplorer;
	bHasStamp = false;
}

void UMapDiscoveryComponent::ResetDiscovery()
{
	bHasStamp = false;
	RebuildVisionMask();
}

void UMapDiscoveryComponent::RevealAtWorldLocation(const FVector& WorldLocation)
{
	RegisterVisionSourceAt(WorldLocation, DiscoveryRadius);
}

void UMapDiscoveryComponent::RegisterPermanentReveal(const FVector& WorldLocation, float RadiusWorldCm)
{
	RegisterVisionSourceAt(WorldLocation, RadiusWorldCm);
}

void UMapDiscoveryComponent::RegisterVisionSource(AActor* Actor, float RadiusWorldCm)
{
	if (!IsValid(Actor))
	{
		return;
	}

	const float R = RadiusWorldCm > KINDA_SMALL_NUMBER ? RadiusWorldCm : CrystalVisionRadius;
	for (FVisionSource& Entry : VisionSources)
	{
		if (Entry.Actor.Get() == Actor)
		{
			Entry.Location = Actor->GetActorLocation();
			Entry.RadiusCm = R;
			return;
		}
	}

	FVisionSource NewEntry;
	NewEntry.Actor = Actor;
	NewEntry.Location = Actor->GetActorLocation();
	NewEntry.RadiusCm = R;
	VisionSources.Add(NewEntry);
	EnsurePointInsideBounds(NewEntry.Location, R);
}

bool UMapDiscoveryComponent::UnregisterVisionSource(AActor* Actor)
{
	if (!IsValid(Actor))
	{
		return false;
	}

	const int32 Before = VisionSources.Num();
	VisionSources.RemoveAll([Actor](const FVisionSource& Entry)
	{
		return Entry.Actor.Get() == Actor;
	});
	return VisionSources.Num() < Before;
}

void UMapDiscoveryComponent::RegisterVisionSourceAt(const FVector& WorldLocation, float RadiusWorldCm)
{
	const float R = RadiusWorldCm > KINDA_SMALL_NUMBER ? RadiusWorldCm : CrystalVisionRadius;
	for (FVisionSource& Entry : VisionSources)
	{
		if (Entry.Actor.IsValid())
		{
			continue;
		}
		if (FVector::DistSquared2D(Entry.Location, WorldLocation) < FMath::Square(50.f))
		{
			Entry.Location = WorldLocation;
			Entry.RadiusCm = R;
			return;
		}
	}

	FVisionSource NewEntry;
	NewEntry.Location = WorldLocation;
	NewEntry.RadiusCm = R;
	VisionSources.Add(NewEntry);
	EnsurePointInsideBounds(WorldLocation, R);
}

void UMapDiscoveryComponent::ClearVisionSources()
{
	VisionSources.Reset();
}

void UMapDiscoveryComponent::ClearPermanentReveals()
{
	ClearVisionSources();
}

void UMapDiscoveryComponent::GatherVisionSources(TArray<FTDFogVisionSource>& OutSources) const
{
	OutSources.Reset();

	if (AActor* Actor = Explorer.Get())
	{
		if (IsValid(Actor))
		{
			FTDFogVisionSource ExplorerSource;
			ExplorerSource.Location = Actor->GetActorLocation();
			ExplorerSource.RadiusCm = DiscoveryRadius;
			OutSources.Add(ExplorerSource);
		}
	}

	for (const FVisionSource& Entry : VisionSources)
	{
		FTDFogVisionSource Source;
		Source.RadiusCm = Entry.RadiusCm > KINDA_SMALL_NUMBER ? Entry.RadiusCm : CrystalVisionRadius;
		if (AActor* Actor = Entry.Actor.Get())
		{
			if (IsValid(Actor))
			{
				Source.Location = Actor->GetActorLocation();
				OutSources.Add(Source);
				continue;
			}
		}
		Source.Location = Entry.Location;
		OutSources.Add(Source);
	}
}

bool UMapDiscoveryComponent::IsLocationVisible(const FVector& WorldLocation) const
{
	if (!bEnabled)
	{
		return true;
	}

	// Landmark sources such as the crystal grant unconditional radial vision.
	for (const FVisionSource& Entry : VisionSources)
	{
		const FVector SourceLocation = Entry.Actor.IsValid() ? Entry.Actor->GetActorLocation() : Entry.Location;
		const float Radius = Entry.RadiusCm > KINDA_SMALL_NUMBER ? Entry.RadiusCm : CrystalVisionRadius;
		if (FVector::DistSquared2D(WorldLocation, SourceLocation) <= FMath::Square(Radius))
		{
			return true;
		}
	}

	const AActor* ExplorerActor = Explorer.Get();
	const bool bInsideChampionRadius = IsValid(ExplorerActor)
		&& FVector::DistSquared2D(WorldLocation, ExplorerActor->GetActorLocation()) <= FMath::Square(DiscoveryRadius);
	return FTDFogVision::IsChampionLocationVisible(
		bInsideChampionRadius,
		bInsideChampionRadius && IsEnvironmentBlockingExplorerLine(WorldLocation));
}

bool UMapDiscoveryComponent::IsEnvironmentBlockingExplorerLine(const FVector& WorldLocation) const
{
	const AActor* ExplorerActor = Explorer.Get();
	UWorld* World = GetWorld();
	if (!IsValid(ExplorerActor) || !World)
	{
		return true;
	}

	FVector Start = ExplorerActor->GetActorLocation();
	Start.Z += VisionTraceHeight;
	const FVector End(WorldLocation.X, WorldLocation.Y, Start.Z);
	if (FVector::DistSquared2D(Start, End) <= FMath::Square(1.f))
	{
		return false;
	}

	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDFogChampionVision), false, ExplorerActor);
	FHitResult Hit;
	return World->LineTraceSingleByObjectType(Hit, Start, End, Objects, Params);
}

void UMapDiscoveryComponent::BuildExplorerVisionRayDistances(TArray<float>& OutRayDistances) const
{
	const int32 Count = FMath::Clamp(VisionRayCount, 32, 512);
	OutRayDistances.Init(FMath::Max(0.f, DiscoveryRadius), Count);

	const AActor* ExplorerActor = Explorer.Get();
	UWorld* World = GetWorld();
	if (!IsValid(ExplorerActor) || !World)
	{
		OutRayDistances.Init(0.f, Count);
		return;
	}

	FVector Start = ExplorerActor->GetActorLocation();
	Start.Z += VisionTraceHeight;
	FCollisionObjectQueryParams Objects;
	Objects.AddObjectTypesToQuery(ECC_WorldStatic);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(TDFogChampionVisionRays), false, ExplorerActor);

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Angle = UE_TWO_PI * static_cast<float>(Index) / static_cast<float>(Count);
		const FVector Direction(FMath::Cos(Angle), FMath::Sin(Angle), 0.f);
		FHitResult Hit;
		if (World->LineTraceSingleByObjectType(
			Hit, Start, Start + Direction * DiscoveryRadius, Objects, Params))
		{
			OutRayDistances[Index] = FMath::Clamp(Hit.Distance - VisionBlockerPadding, 0.f, DiscoveryRadius);
		}
	}
}

void UMapDiscoveryComponent::StampExplorerLineOfSight()
{
	const AActor* ExplorerActor = Explorer.Get();
	if (!IsValid(ExplorerActor) || FogTextureSize <= 0)
	{
		return;
	}

	TArray<float> RayDistances;
	BuildExplorerVisionRayDistances(RayDistances);
	if (RayDistances.IsEmpty())
	{
		return;
	}

	float CenterX = 0.f;
	float CenterY = 0.f;
	float Ortho = 1.f;
	GetOrthoWorldRect(CenterX, CenterY, Ortho);
	const FVector ExplorerLocation = ExplorerActor->GetActorLocation();
	const FVector2D CenterUV = WorldToNormalized(ExplorerLocation);
	const int32 Size = FogTextureSize;
	const float WorldPerPx = Ortho / static_cast<float>(Size);
	const float Cx = CenterUV.X * static_cast<float>(Size - 1);
	const float Cy = CenterUV.Y * static_cast<float>(Size - 1);
	const float RadiusPx = DiscoveryRadius / WorldPerPx;
	const uint8 MaxFogAlpha = UndiscoveredColor.ToFColor(true).A;
	const FColor FullFog = UndiscoveredColor.ToFColor(true);
	const int32 MinX = FMath::Clamp(FMath::FloorToInt(Cx - RadiusPx - 1.f), 0, Size - 1);
	const int32 MaxX = FMath::Clamp(FMath::CeilToInt(Cx + RadiusPx + 1.f), 0, Size - 1);
	const int32 MinY = FMath::Clamp(FMath::FloorToInt(Cy - RadiusPx - 1.f), 0, Size - 1);
	const int32 MaxY = FMath::Clamp(FMath::CeilToInt(Cy + RadiusPx + 1.f), 0, Size - 1);

	for (int32 Y = MinY; Y <= MaxY; ++Y)
	{
		for (int32 X = MinX; X <= MaxX; ++X)
		{
			const float DX = static_cast<float>(X) - Cx;
			const float DY = -(static_cast<float>(Y) - Cy);
			const float DistanceWorld = FMath::Sqrt(DX * DX + DY * DY) * WorldPerPx;
			float Angle = FMath::Atan2(DY, DX);
			if (Angle < 0.f)
			{
				Angle += UE_TWO_PI;
			}
			const float RayPosition = Angle * static_cast<float>(RayDistances.Num()) / UE_TWO_PI;
			const int32 RayIndex0 = FMath::FloorToInt(RayPosition) % RayDistances.Num();
			const int32 RayIndex1 = (RayIndex0 + 1) % RayDistances.Num();
			const float VisibleRadius = FMath::Lerp(
				RayDistances[RayIndex0], RayDistances[RayIndex1], FMath::Frac(RayPosition));
			const float EdgeSoftness = VisibleRadius > KINDA_SMALL_NUMBER
				? FMath::Clamp(VisionEdgeSoftnessCm / VisibleRadius, 0.f, 1.f)
				: 0.f;
			const uint8 TargetAlpha = FTDFogVision::CompositeFogAlpha(
				DistanceWorld, VisibleRadius, EdgeSoftness, MaxFogAlpha);
			if (TargetAlpha >= MaxFogAlpha)
			{
				continue;
			}

			FColor& Pixel = FogPixels[Y * Size + X];
			if (TargetAlpha < Pixel.A)
			{
				const float A01 = static_cast<float>(TargetAlpha) / 255.f;
				Pixel.R = static_cast<uint8>(FMath::RoundToInt(FullFog.R * A01));
				Pixel.G = static_cast<uint8>(FMath::RoundToInt(FullFog.G * A01));
				Pixel.B = static_cast<uint8>(FMath::RoundToInt(FullFog.B * A01));
				Pixel.A = TargetAlpha;
				bFogDirty = true;
			}
		}
	}
}

void UMapDiscoveryComponent::GetOrthoWorldRect(float& OutCenterX, float& OutCenterY, float& OutOrthoWidth) const
{
	const float MinX = FMath::Min(WorldMin.X, WorldMax.X);
	const float MaxX = FMath::Max(WorldMin.X, WorldMax.X);
	const float MinY = FMath::Min(WorldMin.Y, WorldMax.Y);
	const float MaxY = FMath::Max(WorldMin.Y, WorldMax.Y);
	OutCenterX = (MinX + MaxX) * 0.5f;
	OutCenterY = (MinY + MaxY) * 0.5f;
	const float ExtX = FMath::Max(100.f, MaxX - MinX);
	const float ExtY = FMath::Max(100.f, MaxY - MinY);
	OutOrthoWidth = FMath::Max(ExtX, ExtY);
}

FVector2D UMapDiscoveryComponent::WorldToNormalized(const FVector& WorldLoc) const
{
	float CenterX = 0.f;
	float CenterY = 0.f;
	float Ortho = 1.f;
	GetOrthoWorldRect(CenterX, CenterY, Ortho);
	const float U = (WorldLoc.X - CenterX) / Ortho + 0.5f;
	// Match M_WorldFogOfWar_* UV: V flips Y so +Y world maps to decreasing V.
	const float V = 0.5f - (WorldLoc.Y - CenterY) / Ortho;
	return FVector2D(FMath::Clamp(U, 0.f, 1.f), FMath::Clamp(V, 0.f, 1.f));
}

void UMapDiscoveryComponent::EnsurePointInsideBounds(const FVector& WorldLocation, float RadiusWorldCm)
{
	const float Margin = FMath::Max(RadiusWorldCm * 1.25f, 500.f);
	const float MinX = FMath::Min(WorldMin.X, WorldMax.X);
	const float MaxX = FMath::Max(WorldMin.X, WorldMax.X);
	const float MinY = FMath::Min(WorldMin.Y, WorldMax.Y);
	const float MaxY = FMath::Max(WorldMin.Y, WorldMax.Y);

	bool bExpanded = false;
	FVector2D NewMin(MinX, MinY);
	FVector2D NewMax(MaxX, MaxY);

	if (WorldLocation.X < MinX + Margin)
	{
		NewMin.X = WorldLocation.X - Margin;
		bExpanded = true;
	}
	if (WorldLocation.X > MaxX - Margin)
	{
		NewMax.X = WorldLocation.X + Margin;
		bExpanded = true;
	}
	if (WorldLocation.Y < MinY + Margin)
	{
		NewMin.Y = WorldLocation.Y - Margin;
		bExpanded = true;
	}
	if (WorldLocation.Y > MaxY - Margin)
	{
		NewMax.Y = WorldLocation.Y + Margin;
		bExpanded = true;
	}

	const float ExtX = NewMax.X - NewMin.X;
	const float ExtY = NewMax.Y - NewMin.Y;
	const float Ortho = FMath::Max(ExtX, ExtY);
	if (Ortho > 80000.f && RadiusWorldCm > 0.f)
	{
		const float Half = FMath::Max(RadiusWorldCm * 8.f, 6000.f);
		NewMin = FVector2D(WorldLocation.X - Half, WorldLocation.Y - Half);
		NewMax = FVector2D(WorldLocation.X + Half, WorldLocation.Y + Half);
		bExpanded = true;
	}

	if (bExpanded)
	{
		WorldMin = NewMin;
		WorldMax = NewMax;
	}
}

void UMapDiscoveryComponent::EnsureFogTexture()
{
	const int32 Size = FMath::Clamp(MaskSize, 64, 1024);
	const bool bNeedNew = !FogTexture || FogTextureSize != Size;
	if (!bNeedNew)
	{
		return;
	}

	FogTextureSize = Size;
	FogPixels.SetNumUninitialized(Size * Size);

	FogTexture = UTexture2D::CreateTransient(Size, Size, PF_B8G8R8A8);
	if (FogTexture)
	{
		FogTexture->CompressionSettings = TC_VectorDisplacementmap;
		FogTexture->SRGB = false;
		FogTexture->Filter = TF_Bilinear;
		FogTexture->AddressX = TA_Clamp;
		FogTexture->AddressY = TA_Clamp;
		FogTexture->NeverStream = true;
		FogTexture->LODGroup = TEXTUREGROUP_Pixels2D;
		FogTexture->UpdateResource();
	}

	FillDimFog();
	bHasStamp = false;
}

void UMapDiscoveryComponent::FillDimFog()
{
	if (FogTextureSize <= 0 || FogPixels.Num() != FogTextureSize * FogTextureSize)
	{
		return;
	}

	const FColor FogColor = UndiscoveredColor.ToFColor(true);
	const FColor Pixel(FogColor.R, FogColor.G, FogColor.B, FogColor.A);
	for (FColor& P : FogPixels)
	{
		P = Pixel;
	}
	bFogDirty = true;
}

void UMapDiscoveryComponent::StampAtNormalized(const FVector2D& NormalizedUV, float RadiusWorldCm)
{
	if (FogTextureSize <= 0 || FogPixels.Num() != FogTextureSize * FogTextureSize)
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

	const float RadiusWorld = FMath::Max(50.f, RadiusWorldCm > KINDA_SMALL_NUMBER ? RadiusWorldCm : DiscoveryRadius);
	const float RadiusUV = RadiusWorld / Ortho;
	const int32 Size = FogTextureSize;
	const float WorldPerPx = Ortho / static_cast<float>(Size);
	const float Cx = FMath::Clamp(NormalizedUV.X, 0.f, 1.f) * static_cast<float>(Size - 1);
	const float Cy = FMath::Clamp(NormalizedUV.Y, 0.f, 1.f) * static_cast<float>(Size - 1);
	const float RadiusPx = RadiusUV * static_cast<float>(Size);
	const uint8 MaxFogAlpha = UndiscoveredColor.ToFColor(true).A;
	const FColor FullFog = UndiscoveredColor.ToFColor(true);

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
			const float DistPx = FMath::Sqrt(DX * DX + DY * DY);
			const uint8 TargetAlpha = FTDFogVision::CompositeFogAlpha(
				DistPx * WorldPerPx, RadiusWorld, DiscoverySoftness, MaxFogAlpha);
			if (TargetAlpha >= MaxFogAlpha)
			{
				continue;
			}

			FColor& Pixel = FogPixels[Y * Size + X];
			if (TargetAlpha < Pixel.A)
			{
				const float A01 = static_cast<float>(TargetAlpha) / 255.f;
				Pixel.R = static_cast<uint8>(FMath::RoundToInt(FullFog.R * A01));
				Pixel.G = static_cast<uint8>(FMath::RoundToInt(FullFog.G * A01));
				Pixel.B = static_cast<uint8>(FMath::RoundToInt(FullFog.B * A01));
				Pixel.A = TargetAlpha;
				bChanged = true;
			}
		}
	}

	if (bChanged)
	{
		bFogDirty = true;
	}
}

void UMapDiscoveryComponent::FlushFogTexture()
{
	if (!bFogDirty || !FogTexture || FogTextureSize <= 0)
	{
		return;
	}
	if (FogPixels.Num() != FogTextureSize * FogTextureSize)
	{
		return;
	}

	const int32 Size = FogTextureSize;
	const int32 Bytes = Size * Size * static_cast<int32>(sizeof(FColor));
	uint8* SrcCopy = new uint8[Bytes];
	FMemory::Memcpy(SrcCopy, FogPixels.GetData(), Bytes);
	FUpdateTextureRegion2D* Region = new FUpdateTextureRegion2D(0, 0, 0, 0, Size, Size);

	FogTexture->UpdateTextureRegions(
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
	bFogDirty = false;
}

void UMapDiscoveryComponent::RebuildVisionMask(float DeltaTime)
{
	if (!bEnabled)
	{
		return;
	}
	EnsureFogTexture();

	if (const AActor* ExplorerActor = Explorer.Get())
	{
		EnsurePointInsideBounds(ExplorerActor->GetActorLocation(), DiscoveryRadius);
	}
	for (const FVisionSource& Entry : VisionSources)
	{
		const FVector Location = Entry.Actor.IsValid() ? Entry.Actor->GetActorLocation() : Entry.Location;
		EnsurePointInsideBounds(Location, Entry.RadiusCm);
	}

	TArray<FColor> PreviousPixels;
	const bool bBlendTransition = DeltaTime > 0.f && bHasStamp && FogPixels.Num() == FogTextureSize * FogTextureSize;
	if (bBlendTransition)
	{
		PreviousPixels = FogPixels;
	}

	FillDimFog();
	StampExplorerLineOfSight();
	for (const FVisionSource& Entry : VisionSources)
	{
		const FVector Location = Entry.Actor.IsValid() ? Entry.Actor->GetActorLocation() : Entry.Location;
		StampAtNormalized(WorldToNormalized(Location), Entry.RadiusCm);
	}

	if (bBlendTransition && PreviousPixels.Num() == FogPixels.Num())
	{
		const float BlendAlpha = 1.f - FMath::Exp(-FMath::Max(0.1f, FogTransitionSpeed) * DeltaTime);
		for (int32 Index = 0; Index < FogPixels.Num(); ++Index)
		{
			const FColor From = PreviousPixels[Index];
			const FColor To = FogPixels[Index];
			FogPixels[Index] = FColor(
				static_cast<uint8>(FMath::RoundToInt(FMath::Lerp(static_cast<float>(From.R), static_cast<float>(To.R), BlendAlpha))),
				static_cast<uint8>(FMath::RoundToInt(FMath::Lerp(static_cast<float>(From.G), static_cast<float>(To.G), BlendAlpha))),
				static_cast<uint8>(FMath::RoundToInt(FMath::Lerp(static_cast<float>(From.B), static_cast<float>(To.B), BlendAlpha))),
				static_cast<uint8>(FMath::RoundToInt(FMath::Lerp(static_cast<float>(From.A), static_cast<float>(To.A), BlendAlpha))));
		}
		bFogDirty = true;
	}

	if (const AActor* ExplorerActor = Explorer.Get())
	{
		LastStampLocation = ExplorerActor->GetActorLocation();
		bHasStamp = true;
	}

	FlushFogTexture();
}
