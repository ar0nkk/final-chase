#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WCTPlayerController.generated.h"

UCLASS()
class FINALCHASE_API AWCTPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    UFUNCTION(Client, Reliable)
    void ClientSetMoveInputEnabled(bool bEnabled);

    UFUNCTION(BlueprintImplementableEvent, Category = "WCT|Input")
    void BP_SetMoveInputEnabled(bool bEnabled);
};
