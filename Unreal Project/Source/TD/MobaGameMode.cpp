#include "MobaGameMode.h"
#include "MobaCameraPawn.h"
#include "MobaPlayerController.h"

AMobaGameMode::AMobaGameMode()
{
	PlayerControllerClass = AMobaPlayerController::StaticClass();
	DefaultPawnClass = AMobaCameraPawn::StaticClass();
	// HUD / spectator left to project defaults / BP overrides.
}
