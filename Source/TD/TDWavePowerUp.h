#pragma once

#include "CoreMinimal.h"

class AActor;

enum class ETDPlayerPowerUp : uint8
{
	None,
	Overcharge,
	RapidFire,
	Payday,
	IcyGround
};

enum class ETDEnemyPowerUp : uint8
{
	None,
	ThickHide,
	Swift,
	Swarm,
	Armored
};

struct FTDWavePowerUpOffer
{
	ETDPlayerPowerUp Player = ETDPlayerPowerUp::None;
	ETDEnemyPowerUp Enemy = ETDEnemyPowerUp::None;
};

struct FTDActiveWavePowerUp
{
	ETDPlayerPowerUp Player = ETDPlayerPowerUp::None;
	ETDEnemyPowerUp Enemy = ETDEnemyPowerUp::None;
	bool bActive = false;
};

struct FTDWavePowerUpInfo
{
	FString Title;
	FString Description;
};

/** Catalog, rolling, and next-wave apply math for between-wave power-up cards. */
struct FTDWavePowerUp
{
	static constexpr int32 CardCount = 3;
	static constexpr int32 RerollCost = 15;
	static constexpr int32 PaydayAmount = 40;

	static FTDWavePowerUpInfo DescribePlayer(ETDPlayerPowerUp Id);
	static FTDWavePowerUpInfo DescribeEnemy(ETDEnemyPowerUp Id);

	static FTDWavePowerUpOffer RollOffer(
		const TArray<ETDPlayerPowerUp>& ExcludePlayer,
		const TArray<ETDEnemyPowerUp>& ExcludeEnemy);
	static void RollDraft(TArray<FTDWavePowerUpOffer>& OutOffers);
	static void RerollCard(TArray<FTDWavePowerUpOffer>& Offers, int32 CardIndex);

	static bool ShouldOfferDraft(const AActor* Spawner);
	static int32 ResolveTotalWaves(const AActor* Spawner);

	static FTDActiveWavePowerUp MakeActive(const FTDWavePowerUpOffer& Offer);

	static float ResolveOutgoingDamageMul(const FTDActiveWavePowerUp& Active);
	static float ResolveIncomingDamageMul(const FTDActiveWavePowerUp& Active);
	static float ResolveDamage(float Amount, const FTDActiveWavePowerUp& Active);
	static float ResolveHealth(float BaseHealth, const FTDActiveWavePowerUp& Active);
	static float ResolveMoveSpeed(float BaseSpeed, const FTDActiveWavePowerUp& Active);
	static float ResolveSlowFactor(float BaseSlow, const FTDActiveWavePowerUp& Active);
	static int32 ResolveEnemyCount(int32 BaseCount, const FTDActiveWavePowerUp& Active);
	static float ResolveSpawnInterval(float BaseInterval, const FTDActiveWavePowerUp& Active);
	static float ResolveAttackInterval(float BaseInterval, const FTDActiveWavePowerUp& Active);
	static int32 ResolvePaydayBonus(const FTDActiveWavePowerUp& Active);

	static AActor* FindBuildManager(const UObject* WorldContext);
	static float ReadResource(const AActor* BuildManager);
	static bool WriteResource(AActor* BuildManager, float Value);
	static bool TrySpendResource(const UObject* WorldContext, int32 Cost);
	static void AddResource(const UObject* WorldContext, int32 Amount);
};
