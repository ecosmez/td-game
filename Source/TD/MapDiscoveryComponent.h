#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "MapDiscoveryComponent.generated.h"

class UTexture2D;

/**
 * Persistent Diablo-style exploration mask in world XY.
 * Stamps a soft circle around explorers; alpha = remaining fog (1 = unknown).
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

	/** World-space radius (cm) revealed around each explorer. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery", meta = (ClampMin = "100.0", EditCondition = "bEnabled"))
	float DiscoveryRadius = 2500.0f;

	/** Soft edge as a fraction of DiscoveryRadius. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery", meta = (ClampMin = "0.0", ClampMax = "1.0", EditCondition = "bEnabled"))
	float DiscoverySoftness = 0.35f;

	/** Square fog mask resolution. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery", meta = (ClampMin = "64", ClampMax = "1024", EditCondition = "bEnabled"))
	int32 MaskSize = 256;

	/** Re-stamp only after explorer moves this far (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery", meta = (ClampMin = "0.0", EditCondition = "bEnabled"))
	float StampDistance = 80.0f;

	/** Fog RGB/A written into the mask (A = full-cover opacity). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Map Discovery", meta = (EditCondition = "bEnabled"))
	FLinearColor UndiscoveredColor = FLinearColor(0.02f, 0.03f, 0.04f, 0.96f);

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
	 * Permanently clear fog in a radius around a world point (crystal, first spawn, etc.).
	 * Survives ResetDiscovery / texture rebuild; reapplied automatically.
	 * RadiusWorldCm <= 0 uses DiscoveryRadius.
	 */
	UFUNCTION(BlueprintCallable, Category = "Map Discovery")
	void RegisterPermanentReveal(const FVector& WorldLocation, float RadiusWorldCm = 0.f);

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

	/** Square orthographic footprint matching minimap WorldToNormalized. */
	UFUNCTION(BlueprintPure, Category = "Map Discovery")
	void GetOrthoWorldRect(float& OutCenterX, float& OutCenterY, float& OutOrthoWidth) const;

	UFUNCTION(BlueprintPure, Category = "Map Discovery")
	FVector2D WorldToNormalized(const FVector& WorldLoc) const;

protected:
	void EnsureFogTexture();
	void ClearFogTexture();
	void FlushFogTexture();
	void StampAtNormalized(const FVector2D& NormalizedUV, float RadiusWorldCm);
	void UpdateFromExplorer();
	void ApplyPermanentReveals(bool bFlush);

	/** Grow XY bounds so the explorer always sits inside the fog UV domain. */
	void EnsureExplorerInsideBounds(const FVector& WorldLocation);

	UPROPERTY(Transient)
	TObjectPtr<UTexture2D> FogTexture = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<AActor> Explorer = nullptr;

	struct FPermanentReveal
	{
		FVector Location = FVector::ZeroVector;
		float RadiusCm = 0.f;
	};

	TArray<FPermanentReveal> PermanentReveals;
	TArray<FColor> FogPixels;
	FVector LastStampLocation = FVector(ForceInitToZero);
	bool bHasStamp = false;
	bool bFogDirty = false;
	int32 FogTextureSize = 0;
};
