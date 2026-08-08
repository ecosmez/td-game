#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDPathWaypoint.generated.h"

class USplineComponent;

/**
 * Actor base for path waypoints.
 * Draws PathPreviewSpline to the next Index (same routeId + lane)
 * in the editor and optionally during PIE/play for debugging.
 */
UCLASS(Blueprintable)
class TD_API ATDPathWaypoint : public AActor
{
	GENERATED_BODY()

public:
	ATDPathWaypoint();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual bool ShouldTickIfViewportsOnly() const override;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
#endif

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Path Preview")
	void RebuildPathPreview();

	/** Show connection spline in the editor viewport. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Preview")
	bool bDrawPathPreview = true;

	/** Show path connections during PIE / play for debugging. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Preview")
	bool bDrawPathPreviewInPlay = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Preview")
	FLinearColor OverLaneColor = FLinearColor(0.15f, 1.f, 0.35f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Preview")
	FLinearColor UnderLaneColor = FLinearColor(0.25f, 0.55f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Preview", meta = (ClampMin = "1.0"))
	float ArrowSize = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Preview", meta = (ClampMin = "0.5"))
	float Thickness = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Path Preview", meta = (ClampMin = "0.0"))
	float SphereRadius = 50.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Path Preview")
	TObjectPtr<USplineComponent> PathPreviewSpline;

private:
	bool TryGetIntProp(FName Name, int32& OutValue) const;
	bool TryGetBoolProp(FName Name, bool& OutValue) const;
	bool ShouldShowPathPreview() const;
	void DrawPlayDebug() const;
	void GatherNextLocations(TArray<FVector>& OutWorldLocations) const;
};
