#pragma once

#include "CoreMinimal.h"
#include "WCTTypes.generated.h"

UENUM(BlueprintType)
enum class ERoundPhase : uint8
{
    WaitingForPlayers UMETA(DisplayName = "Waiting For Players"),
    PreRound UMETA(DisplayName = "Pre-Round"),
    Chasing UMETA(DisplayName = "Chasing"),
    RoundEnd UMETA(DisplayName = "Round End"),
    MatchOver UMETA(DisplayName = "Match Over")
};

UENUM(BlueprintType)
enum class EWCTRoundResult : uint8
{
    None UMETA(DisplayName = "None"),
    Tag UMETA(DisplayName = "Tag"),
    Evasion UMETA(DisplayName = "Evasion")
};

UENUM(BlueprintType)
enum class EWCTMatchWinner : uint8
{
    None UMETA(DisplayName = "None"),
    Runner UMETA(DisplayName = "Runner"),
    Chaser UMETA(DisplayName = "Chaser"),
    Draw UMETA(DisplayName = "Draw")
};

