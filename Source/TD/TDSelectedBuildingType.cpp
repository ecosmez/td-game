#include "TDSelectedBuildingType.h"

#include "TDCaptureBaseLogic.h"

void UTDSelectedBuildingTypeSubsystem::Remember(
	const FString& SelectedName,
	const FString& ClassPath,
	const FString& BuildingType)
{
	RememberedName = SelectedName;
	RememberedClassPath = ClassPath;
	RememberedBuildingType = BuildingType;
}

FString UTDSelectedBuildingTypeSubsystem::ResolveDeclaredType(
	const FString& SelectedName,
	const FString& ClassPath) const
{
	FTDRememberedBuildingType Remembered;
	Remembered.SelectedName = RememberedName;
	Remembered.ClassPath = RememberedClassPath;
	Remembered.BuildingType = RememberedBuildingType;
	return FTDCaptureBaseLogic::ResolveRememberedBuildingType(Remembered, SelectedName, ClassPath);
}
