#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "TDSelectedBuildingType.generated.h"

/** Remembers the BuildingType from the last tower-store selection. */
UCLASS()
class TD_API UTDSelectedBuildingTypeSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	void Remember(const FString& SelectedName, const FString& ClassPath, const FString& BuildingType);

	/** BuildingType for the current selection. Empty when this click no longer matches. */
	FString ResolveDeclaredType(const FString& SelectedName, const FString& ClassPath) const;

private:
	FString RememberedName;
	FString RememberedClassPath;
	FString RememberedBuildingType;
};
