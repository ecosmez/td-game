#include "MapDiscoveryComponent.h"

#include "Engine/Texture2D.h"
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
	if (bEnabled)
	{
		EnsureFogTexture();
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
		UpdateFromExplorer();
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
	UpdateFromExplorer();
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
	// Force an immediate stamp for the new explorer.
	bHasStamp = false;
}

void UMapDiscoveryComponent::ResetDiscovery()
{
	ClearFogTexture();
	bHasStamp = false;
	ApplyPermanentReveals(true);
	UpdateFromExplorer();
}

void UMapDiscoveryComponent::RevealAtWorldLocation(const FVector& WorldLocation)
{
	if (!bEnabled)
	{
		return;
	}
	EnsureFogTexture();
	EnsureExplorerInsideBounds(WorldLocation);
	StampAtNormalized(WorldToNormalized(WorldLocation), DiscoveryRadius);
	LastStampLocation = WorldLocation;
	bHasStamp = true;
	FlushFogTexture();
}

void UMapDiscoveryComponent::RegisterPermanentReveal(const FVector& WorldLocation, float RadiusWorldCm)
{
	const float R = RadiusWorldCm > KINDA_SMALL_NUMBER ? RadiusWorldCm : DiscoveryRadius;
	// Update existing registration at nearly the same spot, else append.
	for (FPermanentReveal& Entry : PermanentReveals)
	{
		if (FVector::DistSquared2D(Entry.Location, WorldLocation) < FMath::Square(50.f))
		{
			Entry.Location = WorldLocation;
			Entry.RadiusCm = R;
			if (bEnabled)
			{
				EnsureFogTexture();
				EnsureExplorerInsideBounds(WorldLocation);
				StampAtNormalized(WorldToNormalized(WorldLocation), R);
				FlushFogTexture();
			}
			return;
		}
	}

	FPermanentReveal NewEntry;
	NewEntry.Location = WorldLocation;
	NewEntry.RadiusCm = R;
	PermanentReveals.Add(NewEntry);

	if (bEnabled)
	{
		EnsureFogTexture();
		EnsureExplorerInsideBounds(WorldLocation);
		StampAtNormalized(WorldToNormalized(WorldLocation), R);
		FlushFogTexture();
	}
}

void UMapDiscoveryComponent::ClearPermanentReveals()
{
	PermanentReveals.Reset();
}

void UMapDiscoveryComponent::ApplyPermanentReveals(bool bFlush)
{
	if (!bEnabled || PermanentReveals.Num() == 0)
	{
		return;
	}
	EnsureFogTexture();
	for (const FPermanentReveal& Entry : PermanentReveals)
	{
		EnsureExplorerInsideBounds(Entry.Location);
		const float R = Entry.RadiusCm > KINDA_SMALL_NUMBER ? Entry.RadiusCm : DiscoveryRadius;
		StampAtNormalized(WorldToNormalized(Entry.Location), R);
	}
	if (bFlush)
	{
		FlushFogTexture();
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

void UMapDiscoveryComponent::EnsureExplorerInsideBounds(const FVector& WorldLocation)
{
	const float Margin = FMath::Max(DiscoveryRadius * 1.25f, 500.f);
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

	// Refuse absurd default free-cam clamps that crush discovery UV precision (±50k with no authored volume).
	const float ExtX = NewMax.X - NewMin.X;
	const float ExtY = NewMax.Y - NewMin.Y;
	const float Ortho = FMath::Max(ExtX, ExtY);
	if (Ortho > 80000.f && DiscoveryRadius > 0.f)
	{
		// Shrink around the explorer + current revealed footprint seed.
		const float Half = FMath::Max(DiscoveryRadius * 8.f, 6000.f);
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

	ClearFogTexture();
	// Restore always-visible landmarks after a fresh mask.
	ApplyPermanentReveals(true);
	bHasStamp = false;
}

void UMapDiscoveryComponent::ClearFogTexture()
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
	FlushFogTexture();
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
	const float Soft = FMath::Clamp(DiscoverySoftness, 0.f, 1.f);
	const float HardUV = RadiusUV * (1.f - Soft);
	const float SoftUV = FMath::Max(RadiusUV * Soft, 1.f / static_cast<float>(FogTextureSize));

	const int32 Size = FogTextureSize;
	const float Cx = FMath::Clamp(NormalizedUV.X, 0.f, 1.f) * static_cast<float>(Size - 1);
	const float Cy = FMath::Clamp(NormalizedUV.Y, 0.f, 1.f) * static_cast<float>(Size - 1);
	const float RadiusPx = RadiusUV * static_cast<float>(Size);
	const float HardPx = HardUV * static_cast<float>(Size);
	const float SoftPx = SoftUV * static_cast<float>(Size);
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

			FColor& Pixel = FogPixels[Y * Size + X];
			if (TargetAlpha < Pixel.A)
			{
				// Premultiply soft edge toward transparent so emissive/opacity both clear.
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

void UMapDiscoveryComponent::UpdateFromExplorer()
{
	if (!bEnabled)
	{
		return;
	}
	EnsureFogTexture();

	AActor* Actor = Explorer.Get();
	if (!IsValid(Actor))
	{
		return;
	}

	const FVector Loc = Actor->GetActorLocation();
	EnsureExplorerInsideBounds(Loc);

	const bool bMovedFarEnough = !bHasStamp
		|| FVector::DistSquared2D(Loc, LastStampLocation)
			>= FMath::Square(FMath::Max(0.f, StampDistance));

	if (bMovedFarEnough)
	{
		StampAtNormalized(WorldToNormalized(Loc), DiscoveryRadius);
		LastStampLocation = Loc;
		bHasStamp = true;
		// Keep permanent landmarks punched through if bounds shifted.
		ApplyPermanentReveals(false);
		FlushFogTexture();
	}
}
