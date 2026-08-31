#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "TDFogVision.h"
#include "MapDiscoveryComponent.generated.h"

class UTexture2D;

/**
 * League-style live vision mask in world XY.
 * Dim overlay everywhere; current vision (champion + registered sources such as
 * the main crystal) punches fully clear holes that close when sources leave.
 * Shared by minimap fog overlay and 3D world fog of war.
 */
UCLASS(ClassGroup = (TD), meta = (BlueprintSpawnableComponent))
class TD_API UMapDiscoveryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UMapDiscoveryComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** When false, no fog mask is maintained (full vision). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery")
	bool bEnabled = true;

	/** World-space radius (cm) of current vision around the explorer (champion). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery", meta = (ClampMin = "100.0", EditCondition = "bEnabled"))
	float DiscoveryRadius = 4000.0f;

	/** World-space radius (cm) of current vision around the main crystal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery", meta = (ClampMin = "100.0", EditCondition = "bEnabled"))
	float CrystalVisionRadius = 8000.0f;

	/** Soft edge as a fraction of each vision radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnabled"))
	float DiscoverySoftness = 0.35f;

	/** Number of environment traces used to shape champion vision. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery|Line Of Sight", meta = (ClampMin = "32", ClampMax = "512", EditCondition = "bEnabled"))
	int32 VisionRayCount = 128;

	/** Trace height above the champion ground position. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery|Line Of Sight", meta = (ClampMin = "0.0", EditCondition = "bEnabled"))
	float VisionTraceHeight = 100.0f;

	/** Pull the visible edge back from a blocking surface to avoid leaking around walls. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery|Line Of Sight", meta = (ClampMin = "0.0", EditCondition = "bEnabled"))
	float VisionBlockerPadding = 20.0f;

	/** Width in world units of the soft visual edge around the champion's visible area. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery|Line Of Sight", meta = (ClampMin = "1.0", EditCondition = "bEnabled"))
	float VisionEdgeSoftnessCm = 180.0f;

	/** Speed of the visual fog transition while the champion moves. Enemies still hide immediately. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery|Line Of Sight", meta = (ClampMin = "0.1", EditCondition = "bEnabled"))
	float FogTransitionSpeed = 10.0f;

	/** Square fog mask resolution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery", meta = (ClampMin = "64", ClampMax = "1024", EditCondition = "bEnabled"))
	int32 MaskSize = 256;

	/** Re-stamp only after explorer moves this far (cm). 0 = every tick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery", meta = (ClampMin = "0.0", EditCondition = "bEnabled"))
	float StampDistance = 0.0f;

	/** Dim overlay RGB/A where there is no current vision (A = overlay opacity). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery", meta = (EditCondition = "bEnabled"))
	FLinearColor UndiscoveredColor = FLinearColor(0.0f, 0.0f, 0.0f, 0.32f);

	/** World XY bounds used for UV mapping (same square ortho as the minimap). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery|World")
	FVector2D WorldMin = FVector2D(-12000.0f, -12000.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery|World")
	FVector2D WorldMax = FVector2D(12000.0f, 12000.0f);

	UFUNCTION(BlueprintCallable, Category = "Map Discovery")
	void SetEnabled(bool bInEnabled);

	UFUNCTION(BlueprintCallable, Category = "Map Discovery")
	void SetWorldBounds(FVector2D InMin, FVector2D InMax);

	UFUNCTION(BlueprintCallable, Category = "Map Discovery")
	void RevealAtWorldLocation(const FVector& WorldLocation);

	/**
	 * Register a live vision circle that follows Actor (crystal, etc.).
	 * RadiusWorldCm <= 0 uses CrystalVisionRadius.
	 */
	UFUNCTION(BlueprintCallable, Category = "Map Discovery")
	void RegisterVisionSource(AActor* Actor, float RadiusWorldCm = 0.f);

	/** Register a static live vision circle at a world point. */
	UFUNCTION(BlueprintCallable, Category = "Map Discovery")
	void RegisterVisionSourceAt(const FVector& WorldLocation, float RadiusWorldCm = 0.f);

	/** Remove this actor's live vision circle. Leaves other sources (crystal, etc.) intact. */
	UFUNCTION(BlueprintCallable, Category = "Map Discovery")
	bool UnregisterVisionSource(AActor* Actor);

	/** @deprecated Live vision replacement for the old Diablo permanent reveal. */
	UFUNCTION(BlueprintCallable, Category = "Map Discovery", meta = (DeprecatedFunction, DeprecationMessage = "Use RegisterVisionSource / RegisterVisionSourceAt"))
	void RegisterPermanentReveal(const FVector& WorldLocation, float RadiusWorldCm = 0.f);

	UFUNCTION(BlueprintCallable, Category = "Map Discovery")
	void ClearVisionSources();

	UFUNCTION(BlueprintCallable, Category = "Map Discovery")
	void ClearPermanentReveals();

	UFUNCTION(BlueprintCallable, Category = "Map Discovery")
	void ResetDiscovery();

	/** Stamp from this actor each tick (usually the controlled champion). */
	UFUNCTION(BlueprintCallable, Category = "Map Discovery")
	void SetExplorer(AActor* InExplorer);

	UFUNCTION(BlueprintPure, Category = "Map Discovery")
	UTexture2D* GetFogTexture()
	{
		if (bEnabled)
		{
			EnsureFogTexture();
		}
		return FogTexture;
	}

	UFUNCTION(BlueprintPure, Category = "Map Discovery")
	bool IsDiscoveryEnabled() const { return bEnabled; }

	/** True if WorldLocation is inside current vision (champion or registered sources). */
	UFUNCTION(BlueprintPure, Category = "Map Discovery")
	bool IsLocationVisible(const FVector& WorldLocation) const;

	/** Square orthographic footprint matching minimap WorldToNormalized. */
	UFUNCTION(BlueprintPure, Category = "Map Discovery")
	void GetOrthoWorldRect(float& OutCenterX, float& OutCenterY, float& OutOrthoWidth) const;

	UFUNCTION(BlueprintPure, Category = "Map Discovery")
	FVector2D WorldToNormalized(const FVector& WorldLoc) const;

protected:
	void EnsureFogTexture();
	void FillDimFog();
	void FlushFogTexture();
	void StampAtNormalized(const FVector2D& NormalizedUV, float RadiusWorldCm);
	void StampExplorerLineOfSight();
	void BuildExplorerVisionRayDistances(TArray<float>& OutRayDistances) const;
	bool IsEnvironmentBlockingExplorerLine(const FVector& WorldLocation) const;
	void RebuildVisionMask(float DeltaTime = 0.0f);
	void GatherVisionSources(TArray<FTDFogVisionSource>& OutSources) const;

	/** Grow XY bounds so the point + radius always sits inside the fog UV domain. */
	void EnsurePointInsideBounds(const FVector& WorldLocation, float RadiusWorldCm);

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> FogTexture = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AActor> Explorer = nullptr;

	struct FVisionSource
	{
		TWeakObjectPtr<AActor> Actor;
		FVector Location = FVector::ZeroVector;
		float RadiusCm = 0.f;
	};

	TArray<FVisionSource> VisionSources;
	TArray<FColor> FogPixels;
	FVector LastStampLocation = FVector(ForceInitToZero);
	bool bHasStamp = false;
	bool bFogDirty = false;
	int32 FogTextureSize = 0;
};
