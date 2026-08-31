#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TDCaptureBaseLogic.h"
#include "CaptureBase.generated.h"

class UMapDiscoveryComponent;
class UStaticMeshComponent;
class UWidgetComponent;

/**
 * Designer-placed outpost: channel, 3 starter pads, then extra pads + crystal vision.
 */
UCLASS(Blueprintable)
class TD_API ACaptureBase : public AActor
{
	GENERATED_BODY()

public:
	ACaptureBase();

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintPure, Category = "Capture Base")
	bool IsPadBuildable(const AActor* Pad) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture Base", meta = (ClampMin = "100.0"))
	float CaptureRadius = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture Base", meta = (ClampMin = "0.1"))
	float ChannelDuration = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture Base", meta = (ClampMin = "100.0"))
	float VisionRadius = 8000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture Base", meta = (ClampMin = "50.0"))
	float OccupancyRadius = 180.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture Base")
	TArray<TObjectPtr<AActor>> StarterPads;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Capture Base")
	TArray<TObjectPtr<AActor>> ExtraPads;

protected:
	bool ValidatePads();
	void HideAllPads();
	void ApplyPadPresentation(AActor* Pad, bool bIsStarter, bool bOccupied, const FTDCaptureBaseOutput& State);
	int32 CountLivingEnemiesInRadius() const;
	AActor* ResolveChampion() const;
	AActor* FindOccupyingTower(const AActor* Pad) const;
	int32 CountStarterTowers() const;
	void ApplyExtraTowerPower(bool bPowered);
	void ApplyVision(const FTDCaptureBaseOutput& State);
	UMapDiscoveryComponent* FindDiscovery() const;
	void UpdateChannelBar(const FTDCaptureBaseOutput& State);
	static bool IsLivingActor(const AActor* Actor);
	static bool IsGhostTower(const AActor* Actor);
	static bool ReadCanAttack(const AActor* Tower, bool& OutValue);
	static void WriteCanAttack(AActor* Tower, bool bValue);

	UPROPERTY(VisibleAnywhere, Category = "Capture Base")
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Capture Base")
	TObjectPtr<UStaticMeshComponent> Beacon = nullptr;

	UPROPERTY(VisibleAnywhere, Category = "Capture Base")
	TObjectPtr<UWidgetComponent> ChannelBar = nullptr;

	FTDCaptureBaseOutput LastState;
	int32 PreviousStarterTowersAlive = 0;
	bool bInert = false;
	bool bVisionRegistered = false;

	TMap<TWeakObjectPtr<AActor>, bool> SavedCanAttack;
};
