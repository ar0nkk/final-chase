#include "WCTPlayerController.h"

void AWCTPlayerController::ClientSetMoveInputEnabled_Implementation(bool bEnabled)
{
    BP_SetMoveInputEnabled(bEnabled);
}
