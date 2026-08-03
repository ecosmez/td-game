#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MapFogWorldSubsystem.generated.h"

class UMapDiscoveryComponent;
class UWorldFogOfWarComponent;

/**
 * Ensures map discovery + 3D FOW exist on the local player, regardless of
 * which PlayerController / GameMode the map uses (TopDown BP, Moba, etc.).
 */
UCLASS()
class TD_API UMapFogWorldSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return true; }
	virtual bool IsTickableInEditor() const override { return false; }
	virtual bool IsTickableWhenPaused() const override { return false; }

	/** When false, subsystem does nothing. */
	UPROPERTY(EditAnywhere, Category = "Map Fog")
	bool bAutoEnable = true;

protected:
	void EnsureOnLocalPlayer();
	AActor* ResolveExplorer(APlayerController* PC) const;

	UPROPERTY(Transient)
	TObjectPtr<UMapDiscoveryComponent> BoundDiscovery = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UWorldFogOfWarComponent> BoundFog = nullptr;

	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerController> BoundPC;

	float RetryTimer = 0.f;
};
