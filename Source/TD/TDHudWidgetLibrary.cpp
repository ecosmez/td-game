#include "TDHudWidgetLibrary.h"

#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"

UClass* UTDHudWidgetLibrary::LoadHudClass(const TCHAR* Path)
{
	if (!Path || !Path[0])
	{
		return nullptr;
	}
	return LoadClass<UUserWidget>(nullptr, Path);
}

UUserWidget* UTDHudWidgetLibrary::CreateHudWidget(APlayerController* OwningPlayer, const TCHAR* Path)
{
	if (!OwningPlayer)
	{
		UE_LOG(LogTemp, Error, TEXT("HUD spawn failed: no player controller for %s"), Path ? Path : TEXT("(null)"));
		return nullptr;
	}

	UClass* WidgetClass = LoadHudClass(Path);
	if (!WidgetClass)
	{
		UE_LOG(LogTemp, Error, TEXT("HUD WBP missing or failed to load: %s"), Path ? Path : TEXT("(null)"));
		return nullptr;
	}

	return CreateWidget<UUserWidget>(OwningPlayer, WidgetClass);
}
