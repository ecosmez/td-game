#include "TDUIInputLibrary.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "UObject/UnrealType.h"
#include "Widgets/SWidget.h"

namespace TDUIInputLibPrivate
{
	static bool IsViewportShellWidgetType(const FString& TypeName)
	{
		return TypeName == TEXT("SGameLayerManager")
			|| TypeName == TEXT("SViewport")
			|| TypeName == TEXT("SWindow")
			|| TypeName == TEXT("SDPIScaler")
			|| TypeName == TEXT("SInvalidationPanel")
			|| TypeName == TEXT("STooltipPresenter")
			|| TypeName == TEXT("SPopupLayer");
	}
}

bool UTDUIInputLibrary::IsPointerOverHitTestableUI(const UObject* WorldContextObject)
{
	(void)WorldContextObject;

	if (!FSlateApplication::IsInitialized())
	{
		return false;
	}

	FSlateApplication& App = FSlateApplication::Get();
	const FVector2D Cursor = App.GetCursorPos();
	TArray<TSharedRef<SWindow>> Windows;
	App.GetAllVisibleWindowsOrdered(Windows);
	const FWidgetPath Path = App.LocateWindowUnderMouse(Cursor, Windows, /*bIgnoreEnabledStatus=*/false);
	if (!Path.IsValid())
	{
		return false;
	}

	// Leaf → root. Prefer real interactive controls (store buttons, ability slots).
	// Do NOT blanket-block on every SObjectWidget: the tower store uses a full-screen HitArea
	// that would permanently disable click-to-move while the HUD is on screen.
	for (int32 Index = Path.Widgets.Num() - 1; Index >= 0; --Index)
	{
		const TSharedRef<SWidget>& Widget = Path.Widgets[Index].Widget;
		const FString Type = Widget->GetTypeAsString();

		if (TDUIInputLibPrivate::IsViewportShellWidgetType(Type))
		{
			continue;
		}

		if (!Widget->GetVisibility().IsVisible())
		{
			continue;
		}

		// Interactive controls under the cursor (buttons, sliders, etc.).
		if (Widget->IsInteractable())
		{
			return true;
		}

		if (Type.Contains(TEXT("Button"))
			|| Type.Contains(TEXT("Slider"))
			|| Type.Contains(TEXT("ScrollBox"))
			|| Type.Contains(TEXT("ListView"))
			|| Type.Contains(TEXT("CheckBox"))
			|| Type.Contains(TEXT("EditableText"))
			|| Type.Contains(TEXT("ComboBox")))
		{
			return true;
		}

		// Leave full-screen non-interactable HitAreas alone – they should be
		// SelfHitTestInvisible so empty map clicks path and tower buttons stay clickable.
		// Tower build/selection still blocks via IsTowerBuildInteractionActive.
	}

	return false;
}

bool UTDUIInputLibrary::IsTowerBuildInteractionActive(const UObject* WorldContextObject)
{
	const UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return false;
	}

	static const TCHAR* BoolNames[] = {
		TEXT("TowerDragLive"),
		TEXT("bTowerDragLive"),
		TEXT("HasTowerSelected"),
		TEXT("bHasTowerSelected"),
		TEXT("IsPlacingTower"),
		TEXT("bIsPlacingTower"),
		TEXT("IsDraggingTower"),
		TEXT("bIsDraggingTower"),
		TEXT("IsDragging"),
		TEXT("bIsDragging"),
	};

	for (TActorIterator<AActor> It(const_cast<UWorld*>(World)); It; ++It)
	{
		AActor* Actor = *It;
		if (!Actor)
		{
			continue;
		}

		const FString ClassName = Actor->GetClass()->GetName();
		if (!ClassName.Contains(TEXT("BuildManager")))
		{
			continue;
		}

		for (const TCHAR* PropName : BoolNames)
		{
			if (FBoolProperty* BoolProp = FindFProperty<FBoolProperty>(Actor->GetClass(), PropName))
			{
				if (BoolProp->GetPropertyValue_InContainer(Actor))
				{
					return true;
				}
			}
		}
	}

	return false;
}

bool UTDUIInputLibrary::ShouldBlockWorldClickInput(const UObject* WorldContextObject, bool bCheckUI)
{
	if (IsTowerBuildInteractionActive(WorldContextObject))
	{
		return true;
	}
	if (bCheckUI && IsPointerOverHitTestableUI(WorldContextObject))
	{
		return true;
	}
	return false;
}
