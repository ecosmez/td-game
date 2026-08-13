#include "TDEnemyPathLibrary.h"

#include "TDEnemyPathSubsystem.h"
#include "TDPathWaypoint.h"

#include "CollisionQueryParams.h"
#include "Engine/Engine.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UnrealType.h"

#include <initializer_list>

namespace TDEnemyPathPrivate
{
	constexpr float CrystalReachDistance = 200.f;
	constexpr float DefaultLookAhead = 220.f;
	constexpr int32 DefaultSamplesPerSegment = 12;

	static const FSoftClassPath TrashEnemyClass(TEXT("/Game/TD/BP_Enemy.BP_Enemy_C"));
	static const FSoftClassPath RangedEnemyClass(TEXT("/Game/TD/BP_RangedEnemy.BP_RangedEnemy_C"));
	static const FSoftClassPath BossEnemyClass(TEXT("/Game/TD/BP_Boss.BP_Boss_C"));

	static void ScreenMsg(const FString& Text, const FLinearColor& Color, float Seconds = 2.f)
	{
		if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, Seconds, Color.ToFColor(true), Text);
		}
		UE_LOG(LogTemp, Log, TEXT("%s"), *Text);
	}

	static FProperty* FindNamedProp(const UObject* Obj, FName Name)
	{
		if (!Obj)
		{
			return nullptr;
		}
		if (FProperty* Prop = Obj->GetClass()->FindPropertyByName(Name))
		{
			return Prop;
		}
		const FString Wanted = Name.ToString();
		for (TFieldIterator<FProperty> It(Obj->GetClass()); It; ++It)
		{
			if (It->GetName().Equals(Wanted, ESearchCase::IgnoreCase))
			{
				return *It;
			}
		}
		return nullptr;
	}

	static bool ReadInt(const UObject* Obj, std::initializer_list<FName> Names, int32& OutValue)
	{
		if (!Obj)
		{
			return false;
		}
		for (FName Name : Names)
		{
			if (const FProperty* Prop = FindNamedProp(Obj, Name))
			{
				if (const FIntProperty* IntProp = CastField<FIntProperty>(Prop))
				{
					OutValue = IntProp->GetPropertyValue_InContainer(Obj);
					return true;
				}
				if (const FInt64Property* Int64Prop = CastField<FInt64Property>(Prop))
				{
					OutValue = static_cast<int32>(Int64Prop->GetPropertyValue_InContainer(Obj));
					return true;
				}
				if (const FByteProperty* ByteProp = CastField<FByteProperty>(Prop))
				{
					OutValue = ByteProp->GetPropertyValue_InContainer(Obj);
					return true;
				}
				if (const FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
				{
					OutValue = FMath::RoundToInt(FloatProp->GetPropertyValue_InContainer(Obj));
					return true;
				}
				if (const FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Prop))
				{
					OutValue = FMath::RoundToInt(DoubleProp->GetPropertyValue_InContainer(Obj));
					return true;
				}
			}
		}
		return false;
	}

	static bool ReadBool(const UObject* Obj, std::initializer_list<FName> Names, bool& OutValue)
	{
		if (!Obj)
		{
			return false;
		}
		for (FName Name : Names)
		{
			if (const FBoolProperty* Prop = CastField<FBoolProperty>(FindNamedProp(Obj, Name)))
			{
				OutValue = Prop->GetPropertyValue_InContainer(Obj);
				return true;
			}
		}
		return false;
	}

	static bool ReadFloat(const UObject* Obj, std::initializer_list<FName> Names, float& OutValue)
	{
		if (!Obj)
		{
			return false;
		}
		for (FName Name : Names)
		{
			if (const FProperty* Prop = FindNamedProp(Obj, Name))
			{
				if (const FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
				{
					OutValue = FloatProp->GetPropertyValue_InContainer(Obj);
					return true;
				}
				if (const FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Prop))
				{
					OutValue = static_cast<float>(DoubleProp->GetPropertyValue_InContainer(Obj));
					return true;
				}
			}
		}
		return false;
	}

	static bool WriteInt(UObject* Obj, std::initializer_list<FName> Names, int32 Value)
	{
		if (!Obj)
		{
			return false;
		}
		for (FName Name : Names)
		{
			if (FProperty* Prop = FindNamedProp(Obj, Name))
			{
				if (FIntProperty* IntProp = CastField<FIntProperty>(Prop))
				{
					IntProp->SetPropertyValue_InContainer(Obj, Value);
					return true;
				}
				if (FInt64Property* Int64Prop = CastField<FInt64Property>(Prop))
				{
					Int64Prop->SetPropertyValue_InContainer(Obj, static_cast<int64>(Value));
					return true;
				}
			}
		}
		return false;
	}

	static bool WriteBool(UObject* Obj, std::initializer_list<FName> Names, bool Value)
	{
		if (!Obj)
		{
			return false;
		}
		for (FName Name : Names)
		{
			if (FBoolProperty* Prop = CastField<FBoolProperty>(FindNamedProp(Obj, Name)))
			{
				Prop->SetPropertyValue_InContainer(Obj, Value);
				return true;
			}
		}
		return false;
	}

	static bool WriteFloat(UObject* Obj, std::initializer_list<FName> Names, float Value)
	{
		if (!Obj)
		{
			return false;
		}
		for (FName Name : Names)
		{
			if (FProperty* Prop = FindNamedProp(Obj, Name))
			{
				if (FFloatProperty* FloatProp = CastField<FFloatProperty>(Prop))
				{
					FloatProp->SetPropertyValue_InContainer(Obj, Value);
					return true;
				}
				if (FDoubleProperty* DoubleProp = CastField<FDoubleProperty>(Prop))
				{
					DoubleProp->SetPropertyValue_InContainer(Obj, static_cast<double>(Value));
					return true;
				}
			}
		}
		return false;
	}

	static AActor* ReadActor(const UObject* Obj, std::initializer_list<FName> Names)
	{
		if (!Obj)
		{
			return nullptr;
		}
		for (FName Name : Names)
		{
			if (const FObjectPropertyBase* Prop = FindFProperty<FObjectPropertyBase>(Obj->GetClass(), Name))
			{
				if (UObject* Value = Prop->GetObjectPropertyValue_InContainer(Obj))
				{
					return Cast<AActor>(Value);
				}
			}
		}
		return nullptr;
	}

	static bool WriteActor(UObject* Obj, std::initializer_list<FName> Names, AActor* Value)
	{
		if (!Obj)
		{
			return false;
		}
		for (FName Name : Names)
		{
			if (FObjectPropertyBase* Prop = FindFProperty<FObjectPropertyBase>(Obj->GetClass(), Name))
			{
				if (!Value || Value->IsA(Prop->PropertyClass))
				{
					Prop->SetObjectPropertyValue_InContainer(Obj, Value);
					return true;
				}
			}
		}
		return false;
	}

	static void WriteVectorArray(UObject* Obj, FName Name, const TArray<FVector>& Values)
	{
		if (!Obj)
		{
			return;
		}
		FArrayProperty* ArrayProp = FindFProperty<FArrayProperty>(Obj->GetClass(), Name);
		if (!ArrayProp)
		{
			return;
		}
		const FStructProperty* InnerStruct = CastField<FStructProperty>(ArrayProp->Inner);
		if (!InnerStruct || InnerStruct->Struct != TBaseStructure<FVector>::Get())
		{
			return;
		}

		FScriptArrayHelper Helper(ArrayProp, ArrayProp->ContainerPtrToValuePtr<void>(Obj));
		Helper.Resize(Values.Num());
		for (int32 i = 0; i < Values.Num(); ++i)
		{
			*reinterpret_cast<FVector*>(Helper.GetRawPtr(i)) = Values[i];
		}
	}

	static int32 ReadIntOr(const UObject* Obj, std::initializer_list<FName> Names, int32 DefaultValue)
	{
		int32 Value = DefaultValue;
		ReadInt(Obj, Names, Value);
		return Value;
	}

	static bool ReadBoolOr(const UObject* Obj, std::initializer_list<FName> Names, bool DefaultValue)
	{
		bool Value = DefaultValue;
		ReadBool(Obj, Names, Value);
		return Value;
	}

	static float ReadFloatOr(const UObject* Obj, std::initializer_list<FName> Names, float DefaultValue)
	{
		float Value = DefaultValue;
		ReadFloat(Obj, Names, Value);
		return Value;
	}

	static void CallNoParam(AActor* Actor, FName FunctionName)
	{
		if (!Actor)
		{
			return;
		}
		if (UFunction* Fn = Actor->FindFunction(FunctionName))
		{
			Actor->ProcessEvent(Fn, nullptr);
		}
	}

	static void CallFloatParam(AActor* Actor, FName FunctionName, float Value)
	{
		if (!Actor)
		{
			return;
		}
		if (UFunction* Fn = Actor->FindFunction(FunctionName))
		{
			struct FFloatParam
			{
				float DeltaSeconds;
			};
			FFloatParam Params;
			Params.DeltaSeconds = Value;
			Actor->ProcessEvent(Fn, &Params);
		}
	}

	static UTDEnemyPathSubsystem* GetPathSys(const UObject* WorldContext)
	{
		const UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
		return World ? World->GetSubsystem<UTDEnemyPathSubsystem>() : nullptr;
	}

	static FVector SnapToGround(UWorld* World, const FVector& Desired, float GroundOffset, AActor* Ignore)
	{
		if (!World)
		{
			return Desired;
		}

		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(TDEnemyPathGround), true, Ignore);
		const FVector Start(Desired.X, Desired.Y, Desired.Z + 2500.f);
		const FVector End(Desired.X, Desired.Y, Desired.Z - 5000.f);
		if (World->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
		{
			return FVector(Desired.X, Desired.Y, Hit.ImpactPoint.Z + GroundOffset);
		}
		return Desired;
	}

	struct FWaypointInfo
	{
		ATDPathWaypoint* Actor = nullptr;
		int32 Index = 0;
		int32 RouteId = 0;
		bool bOverLane = true;
		FVector Location = FVector::ZeroVector;
	};

	static void GatherWaypoints(UWorld* World, TArray<FWaypointInfo>& Out)
	{
		Out.Reset();
		if (!World)
		{
			return;
		}

		for (TActorIterator<ATDPathWaypoint> It(World); It; ++It)
		{
			ATDPathWaypoint* WP = *It;
			if (!IsValid(WP))
			{
				continue;
			}

			FWaypointInfo Info;
			Info.Actor = WP;
			Info.Location = WP->GetActorLocation();
			Info.Index = WP->GetPathIndex();
			Info.RouteId = WP->GetRouteId();
			Info.bOverLane = WP->IsOverLane();
			Out.Add(Info);
		}
	}

	static bool ChooseLanePoints(
		const TArray<FWaypointInfo>& All,
		int32 RouteId,
		bool bUseLanePreference,
		bool bPreferOverLane,
		TArray<FVector>& OutPoints)
	{
		OutPoints.Reset();

		int32 MaxIndex = -1;
		for (const FWaypointInfo& Info : All)
		{
			if (Info.RouteId == RouteId)
			{
				MaxIndex = FMath::Max(MaxIndex, Info.Index);
			}
		}
		if (MaxIndex < 0)
		{
			return false;
		}

		for (int32 Index = 0; Index <= MaxIndex; ++Index)
		{
			TArray<const FWaypointInfo*> Preferred;
			TArray<const FWaypointInfo*> Fallback;
			for (const FWaypointInfo& Info : All)
			{
				if (Info.RouteId != RouteId || Info.Index != Index)
				{
					continue;
				}
				Fallback.Add(&Info);
				if (!bUseLanePreference || Info.bOverLane == bPreferOverLane)
				{
					Preferred.Add(&Info);
				}
			}

			const TArray<const FWaypointInfo*>& Candidates = Preferred.Num() > 0 ? Preferred : Fallback;
			if (Candidates.Num() == 0)
			{
				continue;
			}
			const int32 Pick = FMath::RandRange(0, Candidates.Num() - 1);
			OutPoints.Add(Candidates[Pick]->Location);
		}

		return OutPoints.Num() > 0;
	}

	static FVector SampleAtDistance(const FTDEnemyPathState& State, float Distance, FVector& OutTangent)
	{
		OutTangent = FVector::ForwardVector;
		if (State.Samples.Num() == 0)
		{
			return FVector::ZeroVector;
		}
		if (State.Samples.Num() == 1 || State.TotalLength <= KINDA_SMALL_NUMBER)
		{
			return State.Samples[0];
		}

		const float Clamped = FMath::Clamp(Distance, 0.f, State.TotalLength);
		int32 Segment = 0;
		for (int32 i = 1; i < State.CumLength.Num(); ++i)
		{
			if (Clamped <= State.CumLength[i])
			{
				Segment = i - 1;
				break;
			}
			Segment = i - 1;
		}

		const int32 NextIndex = FMath::Min(Segment + 1, State.Samples.Num() - 1);
		const FVector A = State.Samples[Segment];
		const FVector B = State.Samples[NextIndex];
		const float SegStart = State.CumLength.IsValidIndex(Segment) ? State.CumLength[Segment] : 0.f;
		const float SegEnd = State.CumLength.IsValidIndex(NextIndex) ? State.CumLength[NextIndex] : State.TotalLength;
		const float SegLen = FMath::Max(SegEnd - SegStart, KINDA_SMALL_NUMBER);
		const float Alpha = FMath::Clamp((Clamped - SegStart) / SegLen, 0.f, 1.f);
		OutTangent = (B - A).GetSafeNormal();
		if (OutTangent.IsNearlyZero())
		{
			OutTangent = FVector::ForwardVector;
		}
		return FMath::Lerp(A, B, Alpha);
	}

	static void BuildState(UWorld* World, AActor* Enemy, FTDEnemyPathState& State, const TArray<FVector>& Waypoints)
	{
		State.Waypoints = Waypoints;
		State.Distance = 0.f;
		State.Samples.Reset();
		State.CumLength.Reset();
		State.TotalLength = 0.f;
		State.bValid = false;
		State.bReachedNotified = false;

		if (Waypoints.Num() == 0)
		{
			return;
		}

		int32 SamplesPerSeg = DefaultSamplesPerSegment;
		int32 SamplesOverride = 0;
		if (ReadInt(Enemy, { TEXT("PathCurveSamples") }, SamplesOverride) && SamplesOverride > 1)
		{
			SamplesPerSeg = SamplesOverride;
		}

		const float GroundOffset = ReadFloatOr(Enemy, { TEXT("GroundOffset") }, 0.f);
		TArray<FVector> Snapped = Waypoints;
		for (FVector& Point : Snapped)
		{
			Point = SnapToGround(World, Point, GroundOffset, Enemy);
		}

		if (AActor* Crystal = ReadActor(Enemy, { TEXT("CrystalActor") }))
		{
			const FVector CrystalPt = SnapToGround(World, Crystal->GetActorLocation(), GroundOffset, Enemy);
			if (Snapped.Num() == 0 || FVector::Dist(Snapped.Last(), CrystalPt) > 50.f)
			{
				Snapped.Add(CrystalPt);
			}
		}

		UTDEnemyPathLibrary::TessellateCatmullRom(Snapped, State.Samples, SamplesPerSeg);
		if (State.Samples.Num() == 0)
		{
			return;
		}

		State.CumLength.SetNum(State.Samples.Num());
		State.CumLength[0] = 0.f;
		for (int32 i = 1; i < State.Samples.Num(); ++i)
		{
			State.TotalLength += FVector::Dist(State.Samples[i - 1], State.Samples[i]);
			State.CumLength[i] = State.TotalLength;
		}
		State.bValid = true;

		WriteVectorArray(Enemy, TEXT("Waypoints"), Waypoints);
		WriteInt(Enemy, { TEXT("WaypointIndex"), TEXT("waypointIndex") }, 0);
	}

	static FTransform SpawnXformFromRoute(UWorld* World, int32 RouteId, bool bPreferOverLane)
	{
		TArray<FWaypointInfo> All;
		GatherWaypoints(World, All);

		TArray<const FWaypointInfo*> Preferred;
		TArray<const FWaypointInfo*> Fallback;
		int32 MinIndex = MAX_int32;
		for (const FWaypointInfo& Info : All)
		{
			if (Info.RouteId == RouteId)
			{
				MinIndex = FMath::Min(MinIndex, Info.Index);
			}
		}
		if (MinIndex == MAX_int32)
		{
			return FTransform::Identity;
		}

		for (const FWaypointInfo& Info : All)
		{
			if (Info.RouteId != RouteId || Info.Index != MinIndex)
			{
				continue;
			}
			Fallback.Add(&Info);
			if (Info.bOverLane == bPreferOverLane)
			{
				Preferred.Add(&Info);
			}
		}

		const TArray<const FWaypointInfo*>& Candidates = Preferred.Num() > 0 ? Preferred : Fallback;
		if (Candidates.Num() == 0)
		{
			return FTransform::Identity;
		}

		const FWaypointInfo* Pick = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];
		return FTransform(FRotator::ZeroRotator, Pick->Location);
	}

	static bool AnyAliveOfClass(UWorld* World, const FSoftClassPath& ClassPath)
	{
		UClass* Class = ClassPath.TryLoadClass<AActor>();
		if (!World || !Class)
		{
			return false;
		}
		for (TActorIterator<AActor> It(World, Class); It; ++It)
		{
			if (IsValid(*It))
			{
				return true;
			}
		}
		return false;
	}

	/** Pick Over/Under at Index 0 for this spawner's route. Does not steal other routes. */
	static bool PickLaneSpawnForRoute(UWorld* World, int32 RouteId, bool& OutOverLane, FVector& OutLocation)
	{
		TArray<FWaypointInfo> All;
		GatherWaypoints(World, All);
		if (All.Num() == 0)
		{
			return false;
		}

		int32 MinIndex = MAX_int32;
		for (const FWaypointInfo& Info : All)
		{
			if (Info.RouteId == RouteId)
			{
				MinIndex = FMath::Min(MinIndex, Info.Index);
			}
		}
		if (MinIndex == MAX_int32)
		{
			return false;
		}

		TArray<const FWaypointInfo*> Spawns;
		for (const FWaypointInfo& Info : All)
		{
			if (Info.RouteId == RouteId && Info.Index == MinIndex)
			{
				Spawns.Add(&Info);
			}
		}
		if (Spawns.Num() == 0)
		{
			return false;
		}

		const FWaypointInfo* Pick = Spawns[FMath::RandRange(0, Spawns.Num() - 1)];
		OutOverLane = Pick->bOverLane;
		OutLocation = Pick->Location;
		return true;
	}

	static bool IsBossWaveFor(AActor* Spawner, int32 WaveNumber)
	{
		if (ReadBoolOr(Spawner, { TEXT("IsBossWave"), TEXT("bIsBossWave") }, false))
		{
			return true;
		}
		const int32 BossWaveNumber = ReadIntOr(Spawner, { TEXT("BossWaveNumber") }, 0);
		return BossWaveNumber > 0 && WaveNumber > 0 && (WaveNumber % BossWaveNumber) == 0;
	}

	/** Log this spawner's route; each spawner keeps its own lane. */
	static bool PrepareWaveSpawn(AActor* Spawner)
	{
		if (!IsValid(Spawner))
		{
			return false;
		}

		UWorld* World = Spawner->GetWorld();
		if (!World)
		{
			return false;
		}

		const int32 RouteId = ReadIntOr(Spawner, { TEXT("routeId"), TEXT("RouteId") }, 0);
		const int32 WaveNumber = ReadIntOr(Spawner, { TEXT("WaveNumber") }, 1);
		bool bOverLane = true;
		FVector UnusedLocation = FVector::ZeroVector;
		if (!PickLaneSpawnForRoute(World, RouteId, bOverLane, UnusedLocation)
			&& SpawnXformFromRoute(World, RouteId, true).GetLocation().IsNearlyZero())
		{
			ScreenMsg(
				FString::Printf(TEXT("Spawner: no BP_Waypoint actors for RouteId %d"), RouteId),
				FLinearColor(1.f, 0.3f, 0.2f), 4.f);
			return false;
		}

		ScreenMsg(
			FString::Printf(TEXT("Wave %d spawn: route %d"), WaveNumber, RouteId),
			FLinearColor(0.35f, 0.85f, 1.f), 3.f);
		return true;
	}
}

FVector UTDEnemyPathLibrary::CatmullRom(const FVector& P0, const FVector& P1, const FVector& P2, const FVector& P3, float T)
{
	const float T2 = T * T;
	const float T3 = T2 * T;
	return 0.5f * (
		(2.f * P1) +
		(-P0 + P2) * T +
		(2.f * P0 - 5.f * P1 + 4.f * P2 - P3) * T2 +
		(-P0 + 3.f * P1 - 3.f * P2 + P3) * T3);
}

void UTDEnemyPathLibrary::TessellateCatmullRom(const TArray<FVector>& Points, TArray<FVector>& OutSamples, int32 SamplesPerSegment)
{
	OutSamples.Reset();
	if (Points.Num() == 0)
	{
		return;
	}
	if (Points.Num() == 1)
	{
		OutSamples.Add(Points[0]);
		return;
	}

	const int32 SegSamples = FMath::Max(SamplesPerSegment, 2);
	OutSamples.Add(Points[0]);
	for (int32 i = 0; i < Points.Num() - 1; ++i)
	{
		const FVector P0 = Points[FMath::Max(i - 1, 0)];
		const FVector P1 = Points[i];
		const FVector P2 = Points[i + 1];
		const FVector P3 = Points[FMath::Min(i + 2, Points.Num() - 1)];
		for (int32 S = 1; S <= SegSamples; ++S)
		{
			const float T = static_cast<float>(S) / static_cast<float>(SegSamples);
			OutSamples.Add(CatmullRom(P0, P1, P2, P3, T));
		}
	}
}

void UTDEnemyPathLibrary::ChooseEnemyPath(AActor* Enemy)
{
	using namespace TDEnemyPathPrivate;

	if (!IsValid(Enemy))
	{
		return;
	}

	UWorld* World = Enemy->GetWorld();
	UTDEnemyPathSubsystem* Sys = GetPathSys(Enemy);
	if (!World || !Sys)
	{
		ScreenMsg(TEXT("ChoosePath: no world/path subsystem"), FLinearColor(1.f, 0.3f, 0.2f));
		return;
	}

	const int32 RouteId = ReadIntOr(Enemy, { TEXT("routeId"), TEXT("RouteId") }, 0);
	const bool bUseLanePreference = ReadBoolOr(Enemy, { TEXT("bUseLanePreference"), TEXT("UseLanePreference") }, true);
	const bool bPreferOverLane = ReadBoolOr(Enemy, { TEXT("bPreferOverLane"), TEXT("PreferOverLane") }, true);

	TArray<FWaypointInfo> All;
	GatherWaypoints(World, All);

	TArray<FVector> Points;
	if (!ChooseLanePoints(All, RouteId, bUseLanePreference, bPreferOverLane, Points))
	{
		ScreenMsg(TEXT("ChoosePath: no BP_Waypoint actors for this routeId"), FLinearColor(1.f, 0.35f, 0.2f), 4.f);
		Sys->Remove(Enemy);
		WriteVectorArray(Enemy, TEXT("Waypoints"), Points);
		return;
	}

	FTDEnemyPathState& State = Sys->FindOrAdd(Enemy);
	BuildState(World, Enemy, State, Points);
	ScreenMsg(TEXT("Enemy resolved path (waypoints + curve)"), FLinearColor(0.2f, 1.f, 0.4f));
}

void UTDEnemyPathLibrary::AdvanceEnemyAlongPath(AActor* Enemy, float DeltaSeconds)
{
	using namespace TDEnemyPathPrivate;

	if (!IsValid(Enemy))
	{
		return;
	}

	const bool bAttackingCrystal = ReadBoolOr(Enemy, { TEXT("IsAttackingCrystal"), TEXT("bIsAttackingCrystal") }, false);
	const bool bWallBlocked = ReadBoolOr(Enemy, { TEXT("IsWallBlocked"), TEXT("bIsWallBlocked") }, false);
	const float StunRemaining = ReadFloatOr(Enemy, { TEXT("StunRemaining") }, 0.f);

	if (bAttackingCrystal)
	{
		if (StunRemaining <= 0.f)
		{
			float AttackCooldown = ReadFloatOr(Enemy, { TEXT("AttackCooldown") }, 0.f);
			AttackCooldown -= DeltaSeconds;
			WriteFloat(Enemy, { TEXT("AttackCooldown") }, AttackCooldown);
			if (AttackCooldown <= 0.f)
			{
				CallNoParam(Enemy, TEXT("AttackCrystal"));
			}
		}
		return;
	}

	if (bWallBlocked)
	{
		if (StunRemaining <= 0.f)
		{
			CallFloatParam(Enemy, TEXT("AttackBlockingWall"), DeltaSeconds);
		}
		return;
	}

	if (StunRemaining > 0.f)
	{
		return;
	}

	UTDEnemyPathSubsystem* Sys = GetPathSys(Enemy);
	UWorld* World = Enemy->GetWorld();
	if (!Sys || !World)
	{
		return;
	}

	FTDEnemyPathState* State = Sys->Find(Enemy);
	if (!State || !State->bValid || State->Samples.Num() == 0)
	{
		ChooseEnemyPath(Enemy);
		State = Sys->Find(Enemy);
		if (!State || !State->bValid)
		{
			return;
		}
	}

	const float MoveSpeed = ReadFloatOr(Enemy, { TEXT("MoveSpeed") }, 300.f);
	const float SlowFactor = ReadFloatOr(Enemy, { TEXT("SlowFactor") }, 1.f);
	const float GroundOffset = ReadFloatOr(Enemy, { TEXT("GroundOffset") }, 0.f);
	float LookAhead = DefaultLookAhead;
	ReadFloat(Enemy, { TEXT("PathLookAhead") }, LookAhead);

	State->Distance = FMath::Min(State->Distance + MoveSpeed * SlowFactor * DeltaSeconds, State->TotalLength);

	FVector Tangent = FVector::ForwardVector;
	FVector Location = SampleAtDistance(*State, State->Distance, Tangent);
	Location = SnapToGround(World, Location, GroundOffset, Enemy);

	FVector LookTangent = Tangent;
	const FVector LookPoint = SampleAtDistance(*State, State->Distance + LookAhead, LookTangent);
	FRotator NewRot = (LookPoint - Location).GetSafeNormal().Rotation();
	if ((LookPoint - Location).SizeSquared() < 1.f)
	{
		NewRot = Tangent.Rotation();
	}
	NewRot.Pitch = 0.f;
	NewRot.Roll = 0.f;

	Enemy->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
	Enemy->SetActorRotation(NewRot, ETeleportType::TeleportPhysics);

	if (State->Waypoints.Num() > 0)
	{
		int32 Closest = 0;
		float BestDistSq = TNumericLimits<float>::Max();
		for (int32 i = 0; i < State->Waypoints.Num(); ++i)
		{
			const float DistSq = FVector::DistSquared2D(Location, State->Waypoints[i]);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				Closest = i;
			}
		}
		WriteInt(Enemy, { TEXT("WaypointIndex"), TEXT("waypointIndex") }, Closest);
	}

	const bool bFinished = State->TotalLength > 1.f && State->Distance >= State->TotalLength - 1.f;
	AActor* Crystal = ReadActor(Enemy, { TEXT("CrystalActor") });
	const float DistToCrystal = Crystal ? FVector::Dist(Location, Crystal->GetActorLocation()) : 0.f;
	if (!State->bReachedNotified && (bFinished || (Crystal && DistToCrystal <= CrystalReachDistance)))
	{
		State->bReachedNotified = true;
		CallNoParam(Enemy, TEXT("ReachCrystal"));
	}
}

FTransform UTDEnemyPathLibrary::GetEnemySpawnTransform(const UObject* WorldContextObject, int32 RouteId, bool bPreferOverLane)
{
	using namespace TDEnemyPathPrivate;
	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return FTransform::Identity;
	}
	return SpawnXformFromRoute(World, RouteId, bPreferOverLane);
}

AActor* UTDEnemyPathLibrary::SpawnNextWaveEnemy(AActor* Spawner)
{
	using namespace TDEnemyPathPrivate;

	if (!IsValid(Spawner))
	{
		return nullptr;
	}

	UWorld* World = Spawner->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	const int32 WaveNumber = ReadIntOr(Spawner, { TEXT("WaveNumber") }, 1);
	const int32 WaveSpawnedCount = ReadIntOr(Spawner, { TEXT("WaveSpawnedCount") }, 0);
	const int32 BaseEnemyHP = ReadIntOr(Spawner, { TEXT("BaseEnemyHP") }, 10);
	const int32 HPPerWave = ReadIntOr(Spawner, { TEXT("HPPerWave") }, 0);
	const int32 RangedStartWave = ReadIntOr(Spawner, { TEXT("RangedStartWave") }, 2);
	AActor* Crystal = ReadActor(Spawner, { TEXT("CrystalActor") });

	const bool bIsBossWave = IsBossWaveFor(Spawner, WaveNumber);
	const int32 TrashHp = BaseEnemyHP + HPPerWave * FMath::Max(WaveNumber - 1, 0);
	const bool bSpawnBoss = bIsBossWave && WaveSpawnedCount == 0;
	const bool bSpawnRanged = !bSpawnBoss && WaveNumber >= RangedStartWave && (WaveSpawnedCount % 3 == 0);

	const int32 RouteId = ReadIntOr(Spawner, { TEXT("routeId"), TEXT("RouteId") }, 0);
	const int32 AlternatePathStartWave = ReadIntOr(Spawner, { TEXT("AlternatePathStartWave") }, 2);
	const bool bPreferOverLane = (WaveNumber < AlternatePathStartWave)
		? true
		: ((WaveSpawnedCount % 2) == 0);
	FTransform SpawnXform = SpawnXformFromRoute(World, RouteId, bPreferOverLane);
	if (SpawnXform.GetLocation().IsNearlyZero())
	{
		TArray<FWaypointInfo> All;
		GatherWaypoints(World, All);
		bool bAnyForRoute = false;
		for (const FWaypointInfo& Info : All)
		{
			if (Info.RouteId == RouteId)
			{
				bAnyForRoute = true;
				break;
			}
		}
		if (!bAnyForRoute)
		{
			ScreenMsg(
				FString::Printf(TEXT("Spawner: no BP_Waypoint actors for RouteId %d"), RouteId),
				FLinearColor(1.f, 0.3f, 0.2f), 4.f);
			return nullptr;
		}
	}

	UClass* SpawnClass = nullptr;
	if (bSpawnBoss)
	{
		SpawnClass = BossEnemyClass.TryLoadClass<AActor>();
		if (!SpawnClass)
		{
			ScreenMsg(TEXT("Boss class missing: /Game/TD/BP_Boss"), FLinearColor(1.f, 0.2f, 0.2f), 4.f);
		}
	}
	else if (bSpawnRanged)
	{
		SpawnClass = RangedEnemyClass.TryLoadClass<AActor>();
	}
	else
	{
		SpawnClass = TrashEnemyClass.TryLoadClass<AActor>();
	}

	if (!SpawnClass)
	{
		ScreenMsg(TEXT("SpawnActor class failed to load"), FLinearColor(1.f, 0.2f, 0.2f), 3.f);
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	Params.Owner = Spawner;
	AActor* Spawned = World->SpawnActor<AActor>(SpawnClass, SpawnXform, Params);
	if (!IsValid(Spawned))
	{
		ScreenMsg(TEXT("SpawnActor null"), FLinearColor(1.f, 0.2f, 0.2f), 3.f);
		return nullptr;
	}

	WriteActor(Spawned, { TEXT("CrystalActor") }, Crystal);
	WriteInt(Spawned, { TEXT("routeId"), TEXT("RouteId") }, RouteId);
	WriteBool(Spawned, { TEXT("bUseLanePreference"), TEXT("UseLanePreference") }, true);
	WriteBool(Spawned, { TEXT("bPreferOverLane"), TEXT("PreferOverLane") }, bPreferOverLane);

	if (!bSpawnBoss)
	{
		WriteFloat(Spawned, { TEXT("MaxHealth") }, static_cast<float>(TrashHp));
		WriteFloat(Spawned, { TEXT("CurrentHealth") }, static_cast<float>(TrashHp));
		WriteInt(Spawned, { TEXT("MaxHealth") }, TrashHp);
		WriteInt(Spawned, { TEXT("CurrentHealth") }, TrashHp);
	}

	ChooseEnemyPath(Spawned);
	WriteInt(Spawner, { TEXT("WaveSpawnedCount") }, WaveSpawnedCount + 1);

	if (bSpawnBoss)
	{
		ScreenMsg(TEXT("Siege Brute spawned!"), FLinearColor(1.f, 0.25f, 0.1f), 3.f);
	}
	else if (bSpawnRanged)
	{
		ScreenMsg(TEXT("Ranged enemy spawned!"), FLinearColor(0.2f, 0.85f, 0.3f), 2.f);
	}

	return Spawned;
}

void UTDEnemyPathLibrary::BeginWaveSpawning(AActor* Spawner)
{
	using namespace TDEnemyPathPrivate;

	if (!IsValid(Spawner))
	{
		return;
	}

	if (!PrepareWaveSpawn(Spawner))
	{
		return;
	}

	WriteBool(Spawner, { TEXT("IsSpawningWave"), TEXT("bIsSpawningWave") }, true);
	WriteInt(Spawner, { TEXT("WaveSpawnedCount") }, 0);
	WriteBool(Spawner, { TEXT("WaitingForClear"), TEXT("WaitingforClear"), TEXT("bWaitingForClear") }, false);

	const float Interval = ReadFloatOr(Spawner, { TEXT("SpawnInterval") }, 1.25f);
	ScreenMsg(TEXT("Spawning enemies!"), FLinearColor(0.2f, 0.8f, 1.f), 2.f);
	UKismetSystemLibrary::K2_SetTimer(Spawner, TEXT("SpawnEnemy"), Interval, true);
	CallNoParam(Spawner, TEXT("SpawnEnemy"));
}

bool UTDEnemyPathLibrary::AreWaveEnemiesAlive(const UObject* WorldContextObject)
{
	using namespace TDEnemyPathPrivate;

	UWorld* World = WorldContextObject ? WorldContextObject->GetWorld() : nullptr;
	if (!World)
	{
		return false;
	}

	return AnyAliveOfClass(World, TrashEnemyClass)
		|| AnyAliveOfClass(World, RangedEnemyClass)
		|| AnyAliveOfClass(World, BossEnemyClass);
}

void UTDEnemyPathLibrary::CheckWaveEnemiesCleared(AActor* Spawner)
{
	using namespace TDEnemyPathPrivate;

	if (!IsValid(Spawner))
	{
		return;
	}

	const bool bWaiting = ReadBoolOr(Spawner,
		{ TEXT("WaitingForClear"), TEXT("WaitingforClear"), TEXT("bWaitingForClear") }, false);
	if (!bWaiting)
	{
		return;
	}

	if (AreWaveEnemiesAlive(Spawner))
	{
		return;
	}

	CallNoParam(Spawner, TEXT("OnWaveCleared"));
}
