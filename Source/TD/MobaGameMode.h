#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "MobaGameMode.generated.h"

class AMobaCameraPawn;
class AMobaPlayerController;

/**
 * Sets default player controller + pawn for free RTS/MOBA camera play.
 * Assign this (or a BP child) as the map GameMode to use the free camera.
 */
UCLASS(Blueprintable)
class TD_API AMobaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AMobaGameMode();
};
