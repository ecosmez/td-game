#include "TDPathWaypoint.h"

#include "TDEnemyPathLibrary.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "UObject/UnrealType.h"

ATDPathWaypoint::ATDPathWaypoint()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;

	USceneComponent* Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	PathPreviewSpline = CreateDefaultSubobject<USplineComponent>(TEXT("PathPreviewSpline"));
	PathPreviewSpline->SetupAttachment(Root);
	PathPreviewSpline->SetRelativeLocation(FVector::ZeroVector);
	PathPreviewSpline->SetClosedLoop(false);
	PathPreviewSpline->SetMobility(EComponentMobility::Movable);
	PathPreviewSpline->SetHiddenInGame(true);
	PathPreviewSpline->SetDrawDebug(true);
	PathPreviewSpline->ClearSplinePoints(false);
}

void ATDPathWaypoint::BeginPlay()
{
	Super::BeginPlay();
	RebuildPathPreview();
	SetActorTickEnabled(ShouldShowPathPreview());
}

void ATDPathWaypoint::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Editor viewport ticks when ShouldTickIfViewportsOnly is used; keep play-mode
	// debug arrows refreshed every frame.
	if (World->IsGameWorld() && ShouldShowPathPreview())
	{
		DrawPlayDebug();
	}
}

bool ATDPathWaypoint::ShouldTickIfViewportsOnly() const
{
	return bDrawPathPreview;
}

void ATDPathWaypoint::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	if (PathPreviewSpline)
	{
		PathPreviewSpline->SetRelativeLocation(FVector::ZeroVector);
	}
	RebuildPathPreview();
}

#if WITH_EDITOR
void ATDPathWaypoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	RebuildPathPreview();
}

void ATDPathWaypoint::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);
	if (bFinished)
	{
		RebuildPathPreview();
		if (UWorld* World = GetWorld())
		{
			for (TActorIterator<ATDPathWaypoint> It(World); It; ++It)
			{
				if (*It && *It != this)
				{
					(*It)->RebuildPathPreview();
				}
			}
		}
	}
}
#endif

int32 ATDPathWaypoint::GetPathIndex() const
{
	int32 Value = 0;
	TryGetIntProp(TEXT("Index"), Value);
	return Value;
}

int32 ATDPathWaypoint::GetRouteId() const
{
	int32 Value = 0;
	if (!TryGetIntProp(TEXT("routeId"), Value))
	{
		TryGetIntProp(TEXT("RouteId"), Value);
	}
	return Value;
}

bool ATDPathWaypoint::IsOverLane() const
{
	bool bValue = true;
	if (!TryGetBoolProp(TEXT("bOverLane"), bValue))
	{
		TryGetBoolProp(TEXT("OverLane"), bValue);
	}
	return bValue;
}

bool ATDPathWaypoint::TryGetIntProp(FName Name, int32& OutValue) const
{
	if (const FProperty* Prop = GetClass()->FindPropertyByName(Name))
	{
		if (const FIntProperty* IntProp = CastField<FIntProperty>(Prop))
		{
			OutValue = IntProp->GetPropertyValue_InContainer(this);
			return true;
		}
		if (const FInt64Property* Int64Prop = CastField<FInt64Property>(Prop))
		{
			OutValue = static_cast<int32>(Int64Prop->GetPropertyValue_InContainer(this));
			return true;
		}
		if (const FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
		{
			OutValue = ByteProp->GetPropertyValue_InContainer(this);
			return true;
		}
	}
	return false;
}

bool ATDPathWaypoint::TryGetBoolProp(FName Name, bool& OutValue) const
{
	if (const FProperty* Prop = GetClass()->FindPropertyByName(Name))
	{
		if (const FBoolProperty* BoolProp = CastField<FBoolProperty>(Prop))
		{
			OutValue = BoolProp->GetPropertyValue_InContainer(this);
			return true;
		}
	}
	return false;
}

bool ATDPathWaypoint::ShouldShowPathPreview() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	if (World->IsGameWorld())
	{
		return bDrawPathPreviewInPlay;
	}

	return bDrawPathPreview;
}

void ATDPathWaypoint::GatherNextLocations(TArray<FVector>& OutWorldLocations) const
{
	OutWorldLocations.Reset();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const int32 MyIndex = GetPathIndex();
	const int32 MyRouteId = GetRouteId();
	const bool bMyOverLane = IsOverLane();

	const int32 NextIndex = MyIndex + 1;
	for (TActorIterator<ATDPathWaypoint> It(World); It; ++It)
	{
		ATDPathWaypoint* Other = *It;
		if (!Other || Other == this)
		{
			continue;
		}

		const int32 OtherIndex = Other->GetPathIndex();
		const int32 OtherRouteId = Other->GetRouteId();
		const bool bOtherOverLane = Other->IsOverLane();

		if (OtherIndex != NextIndex || OtherRouteId != MyRouteId || bOtherOverLane != bMyOverLane)
		{
			continue;
		}

		OutWorldLocations.Add(Other->GetActorLocation());
	}
}

void ATDPathWaypoint::DrawPlayDebug() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const int32 MyIndex = GetPathIndex();
	const int32 MyRouteId = GetRouteId();
	const bool bMyOverLane = IsOverLane();

	const FVector Start = GetActorLocation();
	const FColor DrawColor = (bMyOverLane ? OverLaneColor : UnderLaneColor).ToFColor(true);

	DrawDebugSphere(World, Start, SphereRadius, 14, DrawColor, false, 0.f, SDPG_Foreground, Thickness * 0.45f);
	DrawDebugString(
		World,
		Start + FVector(0.f, 0.f, SphereRadius + 40.f),
		FString::Printf(TEXT("WP %d | R%d | %s"), MyIndex, MyRouteId, bMyOverLane ? TEXT("Over") : TEXT("Under")),
		nullptr,
		DrawColor,
		0.f,
		true,
		1.35f);

	TArray<FVector> NextLocations;
	GatherNextLocations(NextLocations);
	for (const FVector& End : NextLocations)
	{
		DrawDebugDirectionalArrow(
			World,
			Start,
			End,
			ArrowSize,
			DrawColor,
			false,
			0.f,
			SDPG_Foreground,
			Thickness);
	}
}

void ATDPathWaypoint::RebuildPathPreview()
{
	if (!PathPreviewSpline)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World || !ShouldShowPathPreview())
	{
		PathPreviewSpline->SetVisibility(false);
		PathPreviewSpline->SetHiddenInGame(true);
		PathPreviewSpline->ClearSplinePoints(true);
		return;
	}

	const bool bInPlay = World->IsGameWorld();
	PathPreviewSpline->SetVisibility(true);
	PathPreviewSpline->SetHiddenInGame(!bInPlay);
	PathPreviewSpline->SetDrawDebug(true);
	PathPreviewSpline->SetRelativeLocation(FVector::ZeroVector);

	const bool bMyOverLane = IsOverLane();

	const FLinearColor Color = bMyOverLane ? OverLaneColor : UnderLaneColor;
#if WITH_EDITORONLY_DATA
	PathPreviewSpline->EditorUnselectedSplineSegmentColor = Color;
	PathPreviewSpline->EditorSelectedSplineSegmentColor = FLinearColor(1.f, 0.9f, 0.2f, 1.f);
	PathPreviewSpline->ScaleVisualizationWidth = 18.f;
#endif

	TArray<FVector> NextLocations;
	GatherNextLocations(NextLocations);

	const FTransform SelfXform = GetActorTransform();
	PathPreviewSpline->ClearSplinePoints(false);

	FVector P0, P1, P2, P3;
	if (GatherCurveControls(P0, P1, P2, P3))
	{
		constexpr int32 SegSamples = 12;
		PathPreviewSpline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
		for (int32 S = 1; S <= SegSamples; ++S)
		{
			const float T = static_cast<float>(S) / static_cast<float>(SegSamples);
			const FVector WorldPt = UTDEnemyPathLibrary::CatmullRom(P0, P1, P2, P3, T);
			PathPreviewSpline->AddSplinePoint(SelfXform.InverseTransformPosition(WorldPt), ESplineCoordinateSpace::Local, false);
		}
		for (int32 i = 0; i < PathPreviewSpline->GetNumberOfSplinePoints(); ++i)
		{
			PathPreviewSpline->SetSplinePointType(i, ESplinePointType::Linear, false);
		}
	}
	else if (NextLocations.Num() > 0)
	{
		PathPreviewSpline->AddSplinePoint(FVector::ZeroVector, ESplineCoordinateSpace::Local, false);
		for (const FVector& WorldNext : NextLocations)
		{
			PathPreviewSpline->AddSplinePoint(SelfXform.InverseTransformPosition(WorldNext), ESplineCoordinateSpace::Local, false);
		}
		for (int32 i = 0; i < PathPreviewSpline->GetNumberOfSplinePoints(); ++i)
		{
			PathPreviewSpline->SetSplinePointType(i, ESplinePointType::Linear, false);
		}
	}
	PathPreviewSpline->UpdateSpline();

	if (bInPlay)
	{
		DrawPlayDebug();
	}
}

FVector ATDPathWaypoint::FindLaneLocationAtIndex(int32 Index, const FVector& Fallback) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return Fallback;
	}

	const int32 MyRouteId = GetRouteId();
	const bool bMyOverLane = IsOverLane();
	for (TActorIterator<ATDPathWaypoint> It(World); It; ++It)
	{
		ATDPathWaypoint* Other = *It;
		if (!Other)
		{
			continue;
		}
		if (Other->GetRouteId() == MyRouteId && Other->GetPathIndex() == Index && Other->IsOverLane() == bMyOverLane)
		{
			return Other->GetActorLocation();
		}
	}
	return Fallback;
}

bool ATDPathWaypoint::GatherCurveControls(FVector& OutP0, FVector& OutP1, FVector& OutP2, FVector& OutP3) const
{
	TArray<FVector> NextLocations;
	GatherNextLocations(NextLocations);
	if (NextLocations.Num() == 0)
	{
		return false;
	}

	const int32 MyIndex = GetPathIndex();
	OutP1 = GetActorLocation();
	OutP2 = NextLocations[0];
	OutP0 = FindLaneLocationAtIndex(MyIndex - 1, OutP1);
	OutP3 = FindLaneLocationAtIndex(MyIndex + 2, OutP2);
	return true;
}
