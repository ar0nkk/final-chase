#include "WCTPlayerState.h"
#include "Net/UnrealNetwork.h"

AWCTPlayerState::AWCTPlayerState()
{
    WCTScore = 0;
    bIsChaser = false;
}

void AWCTPlayerState::OnRep_WCTScore()
{
}

void AWCTPlayerState::OnRep_IsChaser()
{
}

void AWCTPlayerState::AddScore(int32 Delta)
{
    WCTScore += Delta;
}

void AWCTPlayerState::SetIsChaser(bool bNewIsChaser)
{
    bIsChaser = bNewIsChaser;
}

void AWCTPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AWCTPlayerState, WCTScore);
    DOREPLIFETIME(AWCTPlayerState, bIsChaser);
}
