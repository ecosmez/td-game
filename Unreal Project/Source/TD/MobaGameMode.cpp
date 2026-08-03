#include "MobaGameMode.h"
#include "MobaCameraPawn.h"
#include "MobaPlayerController.h"

AMobaGameMode::AMobaGameMode()
{
	// Canonical for all maps: free-camera MOBA controller.
	// BP_TopDownGameMode also pins PlayerControllerClass to this class.
	PlayerControllerClass = AMobaPlayerController::StaticClass();
	DefaultPawnClass = AMobaCameraPawn::StaticClass();
}
