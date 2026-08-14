#include "TDUIInputLibrary.h"

#include "AbilityBarWidget.h"
#include "ChampionFrameWidget.h"
#include "MinimapWidget.h"
#include "CameraOrbitGizmoWidget.h"
#include "TowerStoreWidget.h"
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/SoftObjectPath.h"
#include "UObject/UnrealType.h"
#include "UObject/UObjectIterator.h"
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

	// Minimap frame is hit-testable but not an SButton; block world path-to under it.
	for (TObjectIterator<UMinimapWidget> It; It; ++It)
	{
		UMinimapWidget* Mini = *It;
		if (Mini && Mini->IsInViewport() && Mini->IsScreenPosOverMap(Cursor))
		{
			return true;
		}
	}

	// Camera orbit gizmo (circular frame, not an SButton).
	for (TObjectIterator<UCameraOrbitGizmoWidget> It; It; ++It)
	{
		UCameraOrbitGizmoWidget* Gizmo = *It;
		if (Gizmo && Gizmo->IsInViewport() && Gizmo->IsScreenPosOverGizmo(Cursor))
		{
			return true;
		}
	}

	// Champion unit frame (avatar + HP chrome, not an SButton).
	for (TObjectIterator<UChampionFrameWidget> It; It; ++It)
	{
		UChampionFrameWidget* Frame = *It;
		if (Frame && Frame->IsInViewport() && Frame->IsScreenPosOverFrame(Cursor))
		{
			return true;
		}
	}

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
		// Tower selection keeps pad click events enabled; champion move is blocked separately.
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
	if (bCheckUI && IsPointerOverHitTestableUI(WorldContextObject))
	{
		return true;
	}
	return false;
}

bool UTDUIInputLibrary::ShouldBlockChampionClickToMove(const UObject* WorldContextObject, bool bCheckUI)
{
	if (IsTowerBuildInteractionActive(WorldContextObject))
	{
		return true;
	}
	return ShouldBlockWorldClickInput(WorldContextObject, bCheckUI);
}

UUserWidget* UTDUIInputLibrary::CreateTowerStoreWidget(UObject* WorldContextObject, APlayerController* OwningPlayer)
{
	APlayerController* PC = OwningPlayer;
	if (!PC)
	{
		if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr)
		{
			PC = World->GetFirstPlayerController();
		}
	}
	if (!PC)
	{
		return nullptr;
	}

	// Prefer designed WBP when cooked/loaded; fall back to C++ store implementation.
	static const FSoftClassPath WbpPath(TEXT("/Game/TD/UI/WBP_TowerStore.WBP_TowerStore_C"));
	UClass* WidgetClass = WbpPath.TryLoadClass<UUserWidget>();
	if (!WidgetClass)
	{
		WidgetClass = UTowerStoreWidget::StaticClass();
	}

	return CreateWidget<UUserWidget>(PC, WidgetClass);
}

UUserWidget* UTDUIInputLibrary::CreateAndShowTowerStore(
	UObject* WorldContextObject,
	APlayerController* OwningPlayer,
	int32 ZOrder)
{
	APlayerController* PC = OwningPlayer;
	if (!PC)
	{
		if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr)
		{
			PC = World->GetFirstPlayerController();
		}
	}
	if (!PC)
	{
		return nullptr;
	}

	UUserWidget* Widget = CreateTowerStoreWidget(WorldContextObject, PC);
	if (!Widget)
	{
		return nullptr;
	}

	if (!Widget->IsInViewport())
	{
		Widget->AddToViewport(ZOrder);
	}

	FInputModeGameAndUI Mode;
	Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	Mode.SetHideCursorDuringCapture(false);
	Mode.SetWidgetToFocus(Widget->TakeWidget());
	PC->SetInputMode(Mode);
	PC->bShowMouseCursor = true;
	PC->bEnableClickEvents = true;

	// Mount closed: open only via ability-bar + or SetStoreOpen(true).
	if (UTowerStoreWidget* Store = Cast<UTowerStoreWidget>(Widget))
	{
		Store->SetStoreOpen(false);
	}

	return Widget;
}

UUserWidget* UTDUIInputLibrary::CreateAbilityBarWidget(UObject* WorldContextObject, APlayerController* OwningPlayer)
{
	APlayerController* PC = OwningPlayer;
	if (!PC)
	{
		if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr)
		{
			PC = World->GetFirstPlayerController();
		}
	}
	if (!PC)
	{
		return nullptr;
	}

	// Prefer designed WBP when cooked/loaded; fall back to C++ ability bar implementation.
	static const FSoftClassPath WbpPath(TEXT("/Game/TD/UI/WBP_AbilityBar.WBP_AbilityBar_C"));
	UClass* WidgetClass = WbpPath.TryLoadClass<UUserWidget>();
	if (!WidgetClass)
	{
		WidgetClass = UAbilityBarWidget::StaticClass();
	}

	return CreateWidget<UUserWidget>(PC, WidgetClass);
}

UUserWidget* UTDUIInputLibrary::CreateAndShowAbilityBar(
	UObject* WorldContextObject,
	APlayerController* OwningPlayer,
	int32 ZOrder)
{
	APlayerController* PC = OwningPlayer;
	if (!PC)
	{
		if (UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::ReturnNull) : nullptr)
		{
			PC = World->GetFirstPlayerController();
		}
	}
	if (!PC)
	{
		return nullptr;
	}

	// Avoid stacking multiple bars if ShowAbilityHUD / land is called more than once.
	for (TObjectIterator<UAbilityBarWidget> It; It; ++It)
	{
		UAbilityBarWidget* Existing = *It;
		if (Existing && Existing->GetOwningPlayer() == PC && Existing->IsInViewport())
		{
			return Existing;
		}
	}

	UUserWidget* Widget = CreateAbilityBarWidget(WorldContextObject, PC);
	if (!Widget)
	{
		return nullptr;
	}

	if (!Widget->IsInViewport())
	{
		Widget->AddToViewport(ZOrder);
	}

	return Widget;
}
