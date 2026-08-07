#include "TDPathWaypoint.h"

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

bool ATDPathWaypoint::TryGetIntProp(FName Name, int32& OutValue) const
{
	if (const FProperty* Prop = GetClass()->FindPropertyByName(Name))
	{
		if (const FIntProperty* IntProp = CastField<FIntProperty>(Prop))
		{
			OutValue = IntProp->GetPropertyValue_InContainer(this);
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

	int32 MyIndex = 0;
	int32 MyRouteId = 0;
	bool bMyOverLane = true;
	TryGetIntProp(TEXT("Index"), MyIndex);
	TryGetIntProp(TEXT("routeId"), MyRouteId);
	if (!TryGetBoolProp(TEXT("bOverLane"), bMyOverLane))
	{
		TryGetBoolProp(TEXT("OverLane"), bMyOverLane);
	}

	const int32 NextIndex = MyIndex + 1;
	for (TActorIterator<ATDPathWaypoint> It(World); It; ++It)
	{
		ATDPathWaypoint* Other = *It;
		if (!Other || Other == this)
		{
			continue;
		}

		int32 OtherIndex = 0;
		int32 OtherRouteId = 0;
		bool bOtherOverLane = true;
		Other->TryGetIntProp(TEXT("Index"), OtherIndex);
		Other->TryGetIntProp(TEXT("routeId"), OtherRouteId);
		if (!Other->TryGetBoolProp(TEXT("bOverLane"), bOtherOverLane))
		{
			Other->TryGetBoolProp(TEXT("OverLane"), bOtherOverLane);
		}

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

	int32 MyIndex = 0;
	int32 MyRouteId = 0;
	bool bMyOverLane = true;
	TryGetIntProp(TEXT("Index"), MyIndex);
	TryGetIntProp(TEXT("routeId"), MyRouteId);
	if (!TryGetBoolProp(TEXT("bOverLane"), bMyOverLane))
	{
		TryGetBoolProp(TEXT("OverLane"), bMyOverLane);
	}

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

	int32 MyIndex = 0;
	bool bMyOverLane = true;
	TryGetIntProp(TEXT("Index"), MyIndex);
	if (!TryGetBoolProp(TEXT("bOverLane"), bMyOverLane))
	{
		TryGetBoolProp(TEXT("OverLane"), bMyOverLane);
	}

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
	if (NextLocations.Num() > 0)
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
