#include "TDMinimapProjection.h"

FRotator FTDMinimapProjection::MakeCaptureRotation(float MapYawOffsetDegrees)
{
	return FRotator(CapturePitch, BaseCaptureYaw + MapYawOffsetDegrees, 0.f);
}

void FTDMinimapProjection::GetOrthoWorldRect(
	FVector2D WorldMin,
	FVector2D WorldMax,
	float ZoomFactor,
	float MapYawOffsetDegrees,
	float& OutCenterX,
	float& OutCenterY,
	float& OutOrthoWidth)
{
	const float MinX = FMath::Min(WorldMin.X, WorldMax.X);
	const float MaxX = FMath::Max(WorldMin.X, WorldMax.X);
	const float MinY = FMath::Min(WorldMin.Y, WorldMax.Y);
	const float MaxY = FMath::Max(WorldMin.Y, WorldMax.Y);
	OutCenterX = (MinX + MaxX) * 0.5f;
	OutCenterY = (MinY + MaxY) * 0.5f;

	const float ExtX = FMath::Max(100.f, MaxX - MinX);
	const float ExtY = FMath::Max(100.f, MaxY - MinY);

	float Sin = 0.f;
	float Cos = 0.f;
	FMath::SinCos(&Sin, &Cos, FMath::DegreesToRadians(MapYawOffsetDegrees));
	const float RotW = ExtX * FMath::Abs(Cos) + ExtY * FMath::Abs(Sin);
	const float RotH = ExtX * FMath::Abs(Sin) + ExtY * FMath::Abs(Cos);
	OutOrthoWidth = FMath::Max(RotW, RotH) * FMath::Clamp(ZoomFactor, 0.1f, 1.0f);
}

FVector2D FTDMinimapProjection::WorldToNormalized(
	const FVector& WorldLoc,
	float CenterX,
	float CenterY,
	float Ortho,
	float MapYawOffsetDegrees)
{
	const float SafeOrtho = FMath::Max(Ortho, KINDA_SMALL_NUMBER);
	const float DX = WorldLoc.X - CenterX;
	const float DY = WorldLoc.Y - CenterY;

	float Sin = 0.f;
	float Cos = 0.f;
	FMath::SinCos(&Sin, &Cos, FMath::DegreesToRadians(MapYawOffsetDegrees));
	const float RX = DX * Cos - DY * Sin;
	const float RY = DX * Sin + DY * Cos;

	const float U = 0.5f - RX / SafeOrtho;
	const float V = 0.5f - RY / SafeOrtho;
	return FVector2D(FMath::Clamp(U, 0.f, 1.f), FMath::Clamp(V, 0.f, 1.f));
}

FVector2D FTDMinimapProjection::NormalizedToWorldXY(
	const FVector2D& Normalized,
	float CenterX,
	float CenterY,
	float Ortho,
	float MapYawOffsetDegrees)
{
	const float RX = (0.5f - Normalized.X) * Ortho;
	const float RY = (0.5f - Normalized.Y) * Ortho;

	float Sin = 0.f;
	float Cos = 0.f;
	FMath::SinCos(&Sin, &Cos, FMath::DegreesToRadians(MapYawOffsetDegrees));
	const float DX = RX * Cos + RY * Sin;
	const float DY = -RX * Sin + RY * Cos;
	return FVector2D(CenterX + DX, CenterY + DY);
}

FBox2D FTDMinimapProjection::FitLandscapeBounds(const FBox& LandscapeBounds, float Padding)
{
	if (!LandscapeBounds.IsValid)
	{
		return FBox2D(ForceInit);
	}

	const float Pad = FMath::Max(0.f, Padding);
	return FBox2D(
		FVector2D(LandscapeBounds.Min.X - Pad, LandscapeBounds.Min.Y - Pad),
		FVector2D(LandscapeBounds.Max.X + Pad, LandscapeBounds.Max.Y + Pad));
}
