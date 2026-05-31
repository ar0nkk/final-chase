#include "WCTGameState.h"
#include "Net/UnrealNetwork.h"

AWCTGameState::AWCTGameState()
{
    RoundPhase = ERoundPhase::WaitingForPlayers;
    CurrentRound = 0;
    TimeRemaining = 0.0f;
    RoundResult = EWCTRoundResult::None;
    MatchWinner = EWCTMatchWinner::None;
}

void AWCTGameState::OnRep_RoundPhase()
{
    BP_OnRoundPhaseChanged(RoundPhase);
}

void AWCTGameState::OnRep_CurrentRound()
{
    BP_OnRoundChanged(CurrentRound);
}

void AWCTGameState::SetRoundPhase(ERoundPhase NewPhase)
{
    RoundPhase = NewPhase;
}

void AWCTGameState::SetCurrentRound(int32 NewRound)
{
    CurrentRound = NewRound;
}

void AWCTGameState::SetTimeRemaining(float NewTime)
{
    TimeRemaining = NewTime;
}

void AWCTGameState::SetRoundResult(EWCTRoundResult NewResult)
{
    RoundResult = NewResult;
}

void AWCTGameState::SetMatchWinner(EWCTMatchWinner NewWinner)
{
    MatchWinner = NewWinner;
}

void AWCTGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AWCTGameState, RoundPhase);
    DOREPLIFETIME(AWCTGameState, CurrentRound);
    DOREPLIFETIME(AWCTGameState, TimeRemaining);
    DOREPLIFETIME(AWCTGameState, RoundResult);
    DOREPLIFETIME(AWCTGameState, MatchWinner);
}
