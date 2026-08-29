#include "TDFogVision.h"

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
