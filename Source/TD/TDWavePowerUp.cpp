#include "TDWavePowerUp.h"

#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "UObject/UnrealType.h"

namespace TDWavePowerUpPrivate
{
	static const ETDPlayerPowerUp PlayerIds[] = {
		ETDPlayerPowerUp::Overcharge,
		ETDPlayerPowerUp::RapidFire,
		ETDPlayerPowerUp::Payday,
		ETDPlayerPowerUp::IcyGround
	};
	static const ETDEnemyPowerUp EnemyIds[] = {
		ETDEnemyPowerUp::ThickHide,
		ETDEnemyPowerUp::Swift,
		ETDEnemyPowerUp::Swarm,
		ETDEnemyPowerUp::Armored
	};

	template <typename TId>
	static TId PickExcluding(const TId* Ids, int32 Num, const TArray<TId>& Exclude)
	{
		TArray<TId> Choices;
		Choices.Reserve(Num);
		for (int32 i = 0; i < Num; ++i)
		{
			if (!Exclude.Contains(Ids[i]))
			{
				Choices.Add(Ids[i]);
			}
		}
		if (Choices.Num() == 0)
		{
			return Ids[FMath::RandRange(0, Num - 1)];
		}
		return Choices[FMath::RandRange(0, Choices.Num() - 1)];
	}

	static bool ReadFloatProp(const UObject* Obj, FName Name, float& OutValue)
	{
		if (!Obj)
		{
			return false;
		}
		if (const FFloatProperty* Prop = FindFProperty<FFloatProperty>(Obj->GetClass(), Name))
		{
			OutValue = Prop->GetPropertyValue_InContainer(Obj);
			return true;
		}
		if (const FDoubleProperty* DProp = FindFProperty<FDoubleProperty>(Obj->GetClass(), Name))
		{
			OutValue = static_cast<float>(DProp->GetPropertyValue_InContainer(Obj));
			return true;
		}
		if (const FIntProperty* IProp = FindFProperty<FIntProperty>(Obj->GetClass(), Name))
		{
			OutValue = static_cast<float>(IProp->GetPropertyValue_InContainer(Obj));
			return true;
		}
		return false;
	}

	static bool WriteFloatProp(UObject* Obj, FName Name, float Value)
	{
		if (!Obj)
		{
			return false;
		}
		if (FFloatProperty* Prop = FindFProperty<FFloatProperty>(Obj->GetClass(), Name))
		{
			Prop->SetPropertyValue_InContainer(Obj, Value);
			return true;
		}
		if (FDoubleProperty* DProp = FindFProperty<FDoubleProperty>(Obj->GetClass(), Name))
		{
			DProp->SetPropertyValue_InContainer(Obj, static_cast<double>(Value));
			return true;
		}
		if (FIntProperty* IProp = FindFProperty<FIntProperty>(Obj->GetClass(), Name))
		{
			IProp->SetPropertyValue_InContainer(Obj, FMath::RoundToInt(Value));
			return true;
		}
		return false;
	}

	static bool ReadIntProp(const UObject* Obj, FName Name, int32& OutValue)
	{
		if (!Obj)
		{
			return false;
		}
		if (const FIntProperty* Prop = FindFProperty<FIntProperty>(Obj->GetClass(), Name))
		{
			OutValue = Prop->GetPropertyValue_InContainer(Obj);
			return true;
		}
		float AsFloat = 0.f;
		if (ReadFloatProp(Obj, Name, AsFloat))
		{
			OutValue = FMath::RoundToInt(AsFloat);
			return true;
		}
		return false;
	}
}

FTDWavePowerUpInfo FTDWavePowerUp::DescribePlayer(ETDPlayerPowerUp Id)
{
	FTDWavePowerUpInfo Info;
	switch (Id)
	{
	case ETDPlayerPowerUp::Overcharge:
		Info.Title = TEXT("Overcharge");
		Info.Description = TEXT("+25% damage to enemies");
		break;
	case ETDPlayerPowerUp::RapidFire:
		Info.Title = TEXT("Rapid Fire");
		Info.Description = TEXT("+20% champion attack speed");
		break;
	case ETDPlayerPowerUp::Payday:
		Info.Title = TEXT("Payday");
		Info.Description = TEXT("+40 Resource now");
		break;
	case ETDPlayerPowerUp::IcyGround:
		Info.Title = TEXT("Icy Ground");
		Info.Description = TEXT("Enemies spawn 15% slower");
		break;
	default:
		Info.Title = TEXT("None");
		Info.Description = TEXT("");
		break;
	}
	return Info;
}

FTDWavePowerUpInfo FTDWavePowerUp::DescribeEnemy(ETDEnemyPowerUp Id)
{
	FTDWavePowerUpInfo Info;
	switch (Id)
	{
	case ETDEnemyPowerUp::ThickHide:
		Info.Title = TEXT("Thick Hide");
		Info.Description = TEXT("+25% enemy HP");
		break;
	case ETDEnemyPowerUp::Swift:
		Info.Title = TEXT("Swift");
		Info.Description = TEXT("+20% enemy move speed");
		break;
	case ETDEnemyPowerUp::Swarm:
		Info.Title = TEXT("Swarm");
		Info.Description = TEXT("+20% enemy count");
		break;
	case ETDEnemyPowerUp::Armored:
		Info.Title = TEXT("Armored");
		Info.Description = TEXT("Enemies take 20% less damage");
		break;
	default:
		Info.Title = TEXT("None");
		Info.Description = TEXT("");
		break;
	}
	return Info;
}

FTDWavePowerUpOffer FTDWavePowerUp::RollOffer(
	const TArray<ETDPlayerPowerUp>& ExcludePlayer,
	const TArray<ETDEnemyPowerUp>& ExcludeEnemy)
{
	using namespace TDWavePowerUpPrivate;
	FTDWavePowerUpOffer Offer;
	Offer.Player = PickExcluding(PlayerIds, UE_ARRAY_COUNT(PlayerIds), ExcludePlayer);
	Offer.Enemy = PickExcluding(EnemyIds, UE_ARRAY_COUNT(EnemyIds), ExcludeEnemy);
	return Offer;
}

void FTDWavePowerUp::RollDraft(TArray<FTDWavePowerUpOffer>& OutOffers)
{
	OutOffers.Reset();
	TArray<ETDPlayerPowerUp> UsedPlayer;
	TArray<ETDEnemyPowerUp> UsedEnemy;
	for (int32 i = 0; i < CardCount; ++i)
	{
		const FTDWavePowerUpOffer Offer = RollOffer(UsedPlayer, UsedEnemy);
		OutOffers.Add(Offer);
		if (Offer.Player != ETDPlayerPowerUp::None)
		{
			UsedPlayer.Add(Offer.Player);
		}
		if (Offer.Enemy != ETDEnemyPowerUp::None)
		{
			UsedEnemy.Add(Offer.Enemy);
		}
	}
}

void FTDWavePowerUp::RerollCard(TArray<FTDWavePowerUpOffer>& Offers, int32 CardIndex)
{
	if (!Offers.IsValidIndex(CardIndex))
	{
		return;
	}

	TArray<ETDPlayerPowerUp> UsedPlayer;
	TArray<ETDEnemyPowerUp> UsedEnemy;
	for (int32 i = 0; i < Offers.Num(); ++i)
	{
		if (i == CardIndex)
		{
			continue;
		}
		if (Offers[i].Player != ETDPlayerPowerUp::None)
		{
			UsedPlayer.Add(Offers[i].Player);
		}
		if (Offers[i].Enemy != ETDEnemyPowerUp::None)
		{
			UsedEnemy.Add(Offers[i].Enemy);
		}
	}
	Offers[CardIndex] = RollOffer(UsedPlayer, UsedEnemy);
}

int32 FTDWavePowerUp::ResolveTotalWaves(const AActor* Spawner)
{
	using namespace TDWavePowerUpPrivate;
	int32 Count = 7;
	if (Spawner)
	{
		int32 FromSpawner = 0;
		if (ReadIntProp(Spawner, FName(TEXT("TotalWaves")), FromSpawner)
			|| ReadIntProp(Spawner, FName(TEXT("MaxWaves")), FromSpawner)
			|| ReadIntProp(Spawner, FName(TEXT("WaveCount")), FromSpawner))
		{
			if (FromSpawner > 0)
			{
				Count = FromSpawner;
			}
		}
		int32 BossWave = 0;
		if (ReadIntProp(Spawner, FName(TEXT("BossWaveNumber")), BossWave) && BossWave > Count)
		{
			Count = BossWave;
		}
	}
	return FMath::Clamp(Count, 1, 20);
}

bool FTDWavePowerUp::ShouldOfferDraft(const AActor* Spawner)
{
	using namespace TDWavePowerUpPrivate;
	if (!Spawner)
	{
		return false;
	}
	int32 WaveNumber = 1;
	ReadIntProp(Spawner, FName(TEXT("WaveNumber")), WaveNumber);
	WaveNumber = FMath::Max(WaveNumber, 1);
	return WaveNumber < ResolveTotalWaves(Spawner);
}

FTDActiveWavePowerUp FTDWavePowerUp::MakeActive(const FTDWavePowerUpOffer& Offer)
{
	FTDActiveWavePowerUp Active;
	Active.Player = Offer.Player;
	Active.Enemy = Offer.Enemy;
	Active.bActive = true;
	return Active;
}

float FTDWavePowerUp::ResolveOutgoingDamageMul(const FTDActiveWavePowerUp& Active)
{
	if (!Active.bActive)
	{
		return 1.f;
	}
	return (Active.Player == ETDPlayerPowerUp::Overcharge) ? 1.25f : 1.f;
}

float FTDWavePowerUp::ResolveIncomingDamageMul(const FTDActiveWavePowerUp& Active)
{
	if (!Active.bActive)
	{
		return 1.f;
	}
	return (Active.Enemy == ETDEnemyPowerUp::Armored) ? 0.80f : 1.f;
}

float FTDWavePowerUp::ResolveDamage(float Amount, const FTDActiveWavePowerUp& Active)
{
	return Amount * ResolveOutgoingDamageMul(Active) * ResolveIncomingDamageMul(Active);
}

float FTDWavePowerUp::ResolveHealth(float BaseHealth, const FTDActiveWavePowerUp& Active)
{
	if (!Active.bActive)
	{
		return BaseHealth;
	}
	return (Active.Enemy == ETDEnemyPowerUp::ThickHide) ? BaseHealth * 1.25f : BaseHealth;
}

float FTDWavePowerUp::ResolveMoveSpeed(float BaseSpeed, const FTDActiveWavePowerUp& Active)
{
	if (!Active.bActive)
	{
		return BaseSpeed;
	}
	return (Active.Enemy == ETDEnemyPowerUp::Swift) ? BaseSpeed * 1.20f : BaseSpeed;
}

float FTDWavePowerUp::ResolveSlowFactor(float BaseSlow, const FTDActiveWavePowerUp& Active)
{
	if (!Active.bActive || Active.Player != ETDPlayerPowerUp::IcyGround)
	{
		return BaseSlow;
	}
	return BaseSlow * 0.85f;
}

int32 FTDWavePowerUp::ResolveEnemyCount(int32 BaseCount, const FTDActiveWavePowerUp& Active)
{
	if (!Active.bActive || Active.Enemy != ETDEnemyPowerUp::Swarm)
	{
		return BaseCount;
	}
	return FMath::Max(BaseCount, FMath::RoundToInt(static_cast<float>(BaseCount) * 1.20f));
}

float FTDWavePowerUp::ResolveSpawnInterval(float BaseInterval, const FTDActiveWavePowerUp& Active)
{
	(void)Active;
	return BaseInterval;
}

float FTDWavePowerUp::ResolveAttackInterval(float BaseInterval, const FTDActiveWavePowerUp& Active)
{
	if (!Active.bActive || Active.Player != ETDPlayerPowerUp::RapidFire)
	{
		return BaseInterval;
	}
	return BaseInterval / 1.20f;
}

int32 FTDWavePowerUp::ResolvePaydayBonus(const FTDActiveWavePowerUp& Active)
{
	if (!Active.bActive || Active.Player != ETDPlayerPowerUp::Payday)
	{
		return 0;
	}
	return PaydayAmount;
}

AActor* FTDWavePowerUp::FindBuildManager(const UObject* WorldContext)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World)
	{
		return nullptr;
	}
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (IsValid(Actor) && Actor->GetClass()->GetName().Contains(TEXT("BuildManager")))
		{
			return Actor;
		}
	}
	return nullptr;
}

float FTDWavePowerUp::ReadResource(const AActor* BuildManager)
{
	using namespace TDWavePowerUpPrivate;
	float Resource = 0.f;
	if (ReadFloatProp(BuildManager, FName(TEXT("Resource")), Resource))
	{
		return Resource;
	}
	return 0.f;
}

bool FTDWavePowerUp::WriteResource(AActor* BuildManager, float Value)
{
	using namespace TDWavePowerUpPrivate;
	return WriteFloatProp(BuildManager, FName(TEXT("Resource")), Value);
}

bool FTDWavePowerUp::TrySpendResource(const UObject* WorldContext, int32 Cost)
{
	AActor* BM = FindBuildManager(WorldContext);
	if (!BM)
	{
		return false;
	}
	const float Resource = ReadResource(BM);
	if (Resource + 0.01f < static_cast<float>(Cost))
	{
		return false;
	}
	return WriteResource(BM, Resource - static_cast<float>(Cost));
}

void FTDWavePowerUp::AddResource(const UObject* WorldContext, int32 Amount)
{
	AActor* BM = FindBuildManager(WorldContext);
	if (!BM || Amount == 0)
	{
		return;
	}
	WriteResource(BM, ReadResource(BM) + static_cast<float>(Amount));
}
