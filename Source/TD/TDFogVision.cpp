#include "TDFogVision.h"

#include "DrawDebugHelpers.h"
#include "Engine/World.h"

bool FTDFogVision::IsLocationVisible(const FVector& Location, TArrayView<const FTDFogVisionSource> Sources)
{
	for (const FTDFogVisionSource& Source : Sources)
	{
		if (Source.RadiusCm <= KINDA_SMALL_NUMBER)
		{
			continue;
		}
		if (FVector::DistSquared2D(Location, Source.Location) <= FMath::Square(Source.RadiusCm))
		{
			return true;
		}
	}
	return false;
}

bool FTDFogVision::IsChampionLocationVisible(bool bInsideRadius, bool bEnvironmentBlocksLineOfSight)
{
	return bInsideRadius && !bEnvironmentBlocksLineOfSight;
}

uint8 FTDFogVision::CompositeFogAlpha(float Distance2D, float RadiusCm, float Softness, uint8 DimAlpha)
{
	if (RadiusCm <= KINDA_SMALL_NUMBER)
	{
		return DimAlpha;
	}

	const float Soft = FMath::Clamp(Softness, 0.f, 1.f);
	const float Hard = RadiusCm * (1.f - Soft);
	const float SoftBand = FMath::Max(RadiusCm * Soft, 0.01f);

	if (Distance2D <= Hard)
	{
		return 0;
	}
	if (Distance2D >= Hard + SoftBand)
	{
		return DimAlpha;
	}

	const float T = (Distance2D - Hard) / SoftBand;
	return static_cast<uint8>(FMath::Clamp(
		FMath::RoundToInt(T * static_cast<float>(DimAlpha)), 0, static_cast<int32>(DimAlpha)));
}

bool FTDFogVision::ShouldHideEnemy(bool bLocationVisible)
{
	return !bLocationVisible;
}

bool FTDFogVision::ShouldSkipClickThroughFoggedEnemy(bool bIsAttackableEnemy, bool bLocationVisible)
{
	return bIsAttackableEnemy && !bLocationVisible;
}

bool FTDVisionPreview::ShouldDraw(bool bIsGameWorld, bool bEnabledInEditor, bool bEnabledInPlay)
{
	return bIsGameWorld ? bEnabledInPlay : bEnabledInEditor;
}

FVector FTDVisionPreview::CircleAxisX()
{
	return FVector(1.f, 0.f, 0.f);
}

FVector FTDVisionPreview::CircleAxisY()
{
	return FVector(0.f, 1.f, 0.f);
}

FVector FTDVisionPreview::GroundLocation(const FVector& ActorLocation)
{
	return ActorLocation + FVector(0.f, 0.f, GroundHeightOffsetCm);
}

FColor FTDVisionPreview::CrystalColor()
{
	return FColor(70, 220, 120);
}

FColor FTDVisionPreview::CaptureBaseColor()
{
	return FColor(70, 180, 255);
}

void FTDVisionPreview::DrawCircle(
	const UWorld* World,
	const FVector& ActorLocation,
	float RadiusCm,
	const FColor& Color)
{
	if (!World || RadiusCm <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	DrawDebugCircle(
		World,
		GroundLocation(ActorLocation),
		RadiusCm,
		SegmentCount,
		Color,
		false,
		-1.f,
		SDPG_Foreground,
		Thickness,
		CircleAxisX(),
		CircleAxisY(),
		false);
}
