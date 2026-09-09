#pragma once

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Templates/SubclassOf.h"
#include "TDHudWidgetLibrary.generated.h"

class APlayerController;
class UUserWidget;
class UWidgetTree;

/**
 * Designer HUD widget blueprints under /Game/TD/UI.
 * Runtime spawn always loads these WBPs — never the native C++ class.
 */
UCLASS()
class TD_API UTDHudWidgetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	template <typename TWidget>
	static TWidget* FindNamed(UWidgetTree* Tree, const TCHAR* Name)
	{
		return Tree ? Cast<TWidget>(Tree->FindWidget(FName(Name))) : nullptr;
	}

	static UClass* LoadHudClass(const TCHAR* Path);
	static UUserWidget* CreateHudWidget(APlayerController* OwningPlayer, const TCHAR* Path);

	template <typename TWidget>
	static TWidget* CreateTypedHudWidget(APlayerController* OwningPlayer, const TCHAR* Path)
	{
		return Cast<TWidget>(CreateHudWidget(OwningPlayer, Path));
	}
};
