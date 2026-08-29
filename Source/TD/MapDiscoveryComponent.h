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
	float DiscoveryRadius = 2500.0f;

	/** World-space radius (cm) of current vision around the main crystal. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery", meta = (ClampMin = "100.0", EditCondition = "bEnabled"))
	float CrystalVisionRadius = 8000.0f;

	/** Soft edge as a fraction of each vision radius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnabled"))
	float DiscoverySoftness = 0.35f;

	/** Square fog mask resolution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery", meta = (ClampMin = "64", ClampMax = "1024", EditCondition = "bEnabled"))
	int32 MaskSize = 256;

	/** Re-stamp only after explorer moves this far (cm). 0 = every tick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery", meta = (ClampMin = "0.0", EditCondition = "bEnabled"))
	float StampDistance = 0.0f;

	/** Dim overlay RGB/A where there is no current vision (A = overlay opacity). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery", meta = (EditCondition = "bEnabled"))
	FLinearColor UndiscoveredColor = FLinearColor(0.02f, 0.03f, 0.04f, 0.52f);

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
	void RebuildVisionMask();
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
