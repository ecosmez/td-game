#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WorldFogOfWarComponent.generated.h"

class UMapDiscoveryComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UPostProcessComponent;
class UStaticMeshComponent;
class UTexture2D;

/**
 * 3D world fog of war driven by UMapDiscoveryComponent live vision.
 * Spawns a world-hosted actor (PP + fog plane) so it works on rootless PlayerControllers.
 */
UCLASS(ClassGroup = (TD), meta = (BlueprintSpawnableComponent))
class TD_API UWorldFogOfWarComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWorldFogOfWarComponent();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	/** Master switch for 3D world fog. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Fog Of War")
	bool bEnabled = true;

	/** Soft path post-process material (MapCenterOrtho, FogMask, FogColor, FogIntensity). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Fog Of War|Post Process")
	TSoftObjectPtr<UMaterialInterface> PostProcessMaterial =
		TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/TD/Materials/M_WorldFogOfWar_PP.M_WorldFogOfWar_PP")));

	/** Overall PP fog strength (scales mask alpha). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Fog Of War|Post Process", meta = (ClampMin = "0.0", ClampMax = "2.0"))
	float FogIntensity = 1.0f;

	/**
	 * Draw a horizontal fog plane (reliable for top-down cameras).
	 * Kept on when post-process is available so fog is still visible mid-setup.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Fog Of War|Plane")
	bool bUseFallbackPlane = true;

	/** Draw plane even if post-process material loaded successfully. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Fog Of War|Plane")
	bool bAlwaysDrawPlane = true;

	/** Soft path material for the FOW plane (FogMask + FogColor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Fog Of War|Plane")
	TSoftObjectPtr<UMaterialInterface> PlaneMaterial =
		TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/Game/TD/Materials/M_WorldFogOfWar_Plane.M_WorldFogOfWar_Plane")));

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Fog Of War|Plane")
	TObjectPtr<UMaterialInterface> PlaneMaterialOverride = nullptr;

	/** World Z of the horizontal fog plane when auto-fit is off. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Fog Of War|Plane")
	float FallbackPlaneHeight = 350.0f;

	/** Extra Z lift above explorer for the fog plane. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Fog Of War|Plane")
	float FallbackPlaneHeightOffset = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Fog Of War|Plane")
	bool bAutoFitPlaneHeightToExplorer = true;

	/** Optional hard reference override for PP material. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "World Fog Of War|Post Process")
	TObjectPtr<UMaterialInterface> PostProcessMaterialOverride = nullptr;

	UFUNCTION(BlueprintCallable, Category = "World Fog Of War")
	void SetEnabled(bool bInEnabled);

	UFUNCTION(BlueprintCallable, Category = "World Fog Of War")
	void SetDiscoverySource(UMapDiscoveryComponent* InDiscovery);

protected:
	void EnsureResources();
	void DestroyResources();
	void UpdateFogPresentation();
	UMapDiscoveryComponent* ResolveDiscovery() const;
	void ApplyPostProcessParams(UTexture2D* FogTex, float CenterX, float CenterY, float Ortho, const FLinearColor& FogColor);
	void UpdateFallbackPlane(UTexture2D* FogTex, float CenterX, float CenterY, float Ortho);

	UPROPERTY(Transient)
	TObjectPtr<UMapDiscoveryComponent> DiscoverySource = nullptr;

	/** World actor hosting PP + plane (not the PlayerController). */
	UPROPERTY(Transient)
	TObjectPtr<AActor> HostActor = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UPostProcessComponent> PostProcess = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> PostProcessMID = nullptr;

	/** @deprecated kept for binary safety; use FallbackMesh on HostActor. */
	UPROPERTY(Transient)
	TObjectPtr<AActor> FallbackPlaneActor = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UStaticMeshComponent> FallbackMesh = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> FallbackMID = nullptr;

	bool bTriedLoadPPMaterial = false;
	bool bHasPPMaterial = false;
};
