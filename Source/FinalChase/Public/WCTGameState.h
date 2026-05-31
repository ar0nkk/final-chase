#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "WCTTypes.h"
#include "WCTGameState.generated.h"

UCLASS()
class FINALCHASE_API AWCTGameState : public AGameStateBase
{
    GENERATED_BODY()

public:
    AWCTGameState();

    UPROPERTY(ReplicatedUsing = OnRep_RoundPhase, BlueprintReadOnly, Category = "WCT|Round")
    ERoundPhase RoundPhase;

    UPROPERTY(ReplicatedUsing = OnRep_CurrentRound, BlueprintReadOnly, Category = "WCT|Round")
    int32 CurrentRound;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "WCT|Round")
    float TimeRemaining;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "WCT|Round")
    EWCTRoundResult RoundResult;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "WCT|Match")
    EWCTMatchWinner MatchWinner;

    UFUNCTION()
    void OnRep_RoundPhase();

    UFUNCTION()
    void OnRep_CurrentRound();

    UFUNCTION(BlueprintImplementableEvent, Category = "WCT|Round")
    void BP_OnRoundPhaseChanged(ERoundPhase NewPhase);

    UFUNCTION(BlueprintImplementableEvent, Category = "WCT|Round")
    void BP_OnRoundChanged(int32 NewRound);

    void SetRoundPhase(ERoundPhase NewPhase);
    void SetCurrentRound(int32 NewRound);
    void SetTimeRemaining(float NewTime);
    void SetRoundResult(EWCTRoundResult NewResult);
    void SetMatchWinner(EWCTMatchWinner NewWinner);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
