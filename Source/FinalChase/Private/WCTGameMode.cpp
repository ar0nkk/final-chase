#include "WCTGameMode.h"
#include "WCTGameState.h"
#include "WCTPlayerState.h"
#include "WCTPlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"

AWCTGameMode::AWCTGameMode()
{
    TotalRounds = 8;
    PreRoundSeconds = 8.0f;
    ChasingSeconds = 20.0f;
    RoundEndSeconds = 2.0f;
    RoundPhaseEndTime = 0.0f;
    bMatchStarted = false;
    bStartPlayersAsSpectators = true;

    GameStateClass = AWCTGameState::StaticClass();
    PlayerStateClass = AWCTPlayerState::StaticClass();
}

void AWCTGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (HasAuthority())
    {
        AWCTGameState* WCTGameState = GetWCTGameState();
        if (WCTGameState)
        {
            WCTGameState->SetRoundPhase(ERoundPhase::WaitingForPlayers);
            WCTGameState->SetCurrentRound(0);
            WCTGameState->SetTimeRemaining(0.0f);
        }
    }
}

void AWCTGameMode::PostLogin(APlayerController* NewPlayer)
{
    Super::PostLogin(NewPlayer);

    if (!HostController.IsValid() && NewPlayer && NewPlayer->IsLocalController())
        HostController = NewPlayer;

    TryStartMatch();
}

void AWCTGameMode::Logout(AController* Exiting)
{
    Super::Logout(Exiting);
}

void AWCTGameMode::ReportCapture(AController* Capturer, AController* Captured)
{
    if (!HasAuthority())
        return;

    AWCTGameState* WCTGameState = GetWCTGameState();
    if (!WCTGameState || WCTGameState->RoundPhase != ERoundPhase::Chasing)
        return;

    AWCTPlayerState* CapturerState = GetWCTPlayerState(Capturer);
    AWCTPlayerState* CapturedState = GetWCTPlayerState(Captured);
    if (!CapturerState || !CapturedState)
        return;

    if (!CapturerState->bIsChaser)
        return;

    if (CapturedState->bIsChaser)
        return;

    EndRound(Capturer, false);
}

void AWCTGameMode::TryStartMatch()
{
    if (!HasAuthority() || bMatchStarted)
        return;

    if (GetNumPlayers() < 2)
        return;

    bMatchStarted = true;
    StartNextRound();
}

void AWCTGameMode::StartNextRound()
{
    if (!HasAuthority())
        return;

    AWCTGameState* WCTGameState = GetWCTGameState();
    if (!WCTGameState)
        return;

    if (WCTGameState->CurrentRound >= TotalRounds)
    {
        FinishMatch();
        return;
    }

    WCTGameState->SetCurrentRound(WCTGameState->CurrentRound + 1);
    AssignRolesForNewRound();
    MovePlayersToStart();
    BeginPreRound();
}

void AWCTGameMode::BeginPreRound()
{
    AWCTGameState* WCTGameState = GetWCTGameState();
    if (!WCTGameState)
        return;

    WCTGameState->SetRoundPhase(ERoundPhase::PreRound);
    WCTGameState->SetTimeRemaining(PreRoundSeconds);

    if (ChaserController.IsValid())
        SetControllerMovementEnabled(ChaserController.Get(), false);
    if (RunnerController.IsValid())
        SetControllerMovementEnabled(RunnerController.Get(), true);

    RoundPhaseEndTime = GetWorld()->GetTimeSeconds() + PreRoundSeconds;

    GetWorldTimerManager().ClearTimer(PhaseEndTimerHandle);
    GetWorldTimerManager().SetTimer(
        PhaseEndTimerHandle,
        this,
        &AWCTGameMode::BeginChasing,
        PreRoundSeconds,
        false
    );

    GetWorldTimerManager().ClearTimer(TimeRemainingTimerHandle);
    GetWorldTimerManager().SetTimer(
        TimeRemainingTimerHandle,
        this,
        &AWCTGameMode::UpdateTimeRemaining,
        0.1f,
        true
    );
}

void AWCTGameMode::BeginChasing()
{
    AWCTGameState* WCTGameState = GetWCTGameState();
    if (!WCTGameState)
        return;

    WCTGameState->SetRoundPhase(ERoundPhase::Chasing);
    WCTGameState->SetTimeRemaining(ChasingSeconds);

    if (ChaserController.IsValid())
        SetControllerMovementEnabled(ChaserController.Get(), true);

    RoundPhaseEndTime = GetWorld()->GetTimeSeconds() + ChasingSeconds;

    GetWorldTimerManager().ClearTimer(PhaseEndTimerHandle);
    GetWorldTimerManager().SetTimer(
        PhaseEndTimerHandle,
        FTimerDelegate::CreateWeakLambda(this, [this]()
        {
            if (RunnerController.IsValid())
                EndRound(RunnerController.Get(), true);
        }),
        ChasingSeconds,
        false
    );
}

void AWCTGameMode::EndRound(AController* Winner, bool bRunnerSurvived)
{
    AWCTGameState* WCTGameState = GetWCTGameState();
    if (!WCTGameState)
        return;

    GetWorldTimerManager().ClearTimer(PhaseEndTimerHandle);
    GetWorldTimerManager().ClearTimer(TimeRemainingTimerHandle);

    WCTGameState->SetRoundPhase(ERoundPhase::RoundEnd);
    if (bRunnerSurvived)
        WCTGameState->SetTimeRemaining(0.0f);
    else
        WCTGameState->SetTimeRemaining(FMath::Max(0.0f, RoundPhaseEndTime - GetWorld()->GetTimeSeconds()));
    WCTGameState->SetRoundResult(bRunnerSurvived ? EWCTRoundResult::Evasion : EWCTRoundResult::Tag);

    if (bRunnerSurvived)
    {
        AWCTPlayerState* WinnerState = GetWCTPlayerState(Winner);
        if (WinnerState)
            WinnerState->AddScore(1);
    }

    NextRunnerController = Winner;

    if (RunnerController.IsValid())
        SetControllerMovementEnabled(RunnerController.Get(), false);
    if (ChaserController.IsValid())
        SetControllerMovementEnabled(ChaserController.Get(), false);

    if (WCTGameState->CurrentRound >= TotalRounds)
    {
        FinishMatch();
        return;
    }

    GetWorldTimerManager().ClearTimer(RoundTransitionTimerHandle);
    GetWorldTimerManager().SetTimer(
        RoundTransitionTimerHandle,
        this,
        &AWCTGameMode::StartNextRound,
        RoundEndSeconds,
        false
    );
}

void AWCTGameMode::FinishMatch()
{
    AWCTGameState* WCTGameState = GetWCTGameState();
    if (WCTGameState)
    {
        WCTGameState->SetRoundPhase(ERoundPhase::MatchOver);
        WCTGameState->SetTimeRemaining(0.0f);
        WCTGameState->SetRoundResult(EWCTRoundResult::None);
        const AWCTPlayerState* RunnerState = RunnerController.IsValid() ? GetWCTPlayerState(RunnerController.Get()) : nullptr;
        const AWCTPlayerState* ChaserState = ChaserController.IsValid() ? GetWCTPlayerState(ChaserController.Get()) : nullptr;
        const int32 RunnerScore = RunnerState ? RunnerState->WCTScore : 0;
        const int32 ChaserScore = ChaserState ? ChaserState->WCTScore : 0;

        if (RunnerScore == ChaserScore)
            WCTGameState->SetMatchWinner(EWCTMatchWinner::Draw);
        else
            WCTGameState->SetMatchWinner(RunnerScore > ChaserScore ? EWCTMatchWinner::Runner : EWCTMatchWinner::Chaser);
    }

    if (RunnerController.IsValid())
        SetControllerMovementEnabled(RunnerController.Get(), false);
    if (ChaserController.IsValid())
        SetControllerMovementEnabled(ChaserController.Get(), false);
}

void AWCTGameMode::AssignRolesForNewRound()
{
    RunnerController.Reset();
    ChaserController.Reset();

    if (NextRunnerController.IsValid())
        RunnerController = NextRunnerController;
    else if (HostController.IsValid())
        RunnerController = HostController;
    else
    {
        for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
        {
            if (AController* Controller = It->Get())
            {
                RunnerController = Controller;
                break;
            }
        }
    }

    if (RunnerController.IsValid())
        ChaserController = FindOtherController(RunnerController.Get());

    NextRunnerController.Reset();

    if (AWCTPlayerState* RunnerState = GetWCTPlayerState(RunnerController.Get()))
        RunnerState->SetIsChaser(false);
    if (AWCTPlayerState* ChaserState = GetWCTPlayerState(ChaserController.Get()))
        ChaserState->SetIsChaser(true);
}

void AWCTGameMode::MovePlayersToStart()
{
    if (RunnerController.IsValid())
        TeleportControllerToTag(RunnerController.Get(), FName("Runner"));
    if (ChaserController.IsValid())
        TeleportControllerToTag(ChaserController.Get(), FName("Chaser"));
}

void AWCTGameMode::TeleportControllerToTag(AController* Controller, FName StartTag) const
{
    if (!Controller)
        return;

    AActor* Start = FindPlayerStartByTag(StartTag);
    if (!Start)
        return;

    APawn* Pawn = Controller->GetPawn();
    if (Pawn)
        Pawn->TeleportTo(Start->GetActorLocation(), Start->GetActorRotation(), false, true);
}

AActor* AWCTGameMode::FindPlayerStartByTag(FName StartTag) const
{
    TArray<AActor*> Starts;
    UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), Starts);

    for (AActor* Start : Starts)
    {
        if (Start && Start->ActorHasTag(StartTag))
            return Start;
    }

    return nullptr;
}

void AWCTGameMode::UpdateTimeRemaining()
{
    AWCTGameState* WCTGameState = GetWCTGameState();
    if (!WCTGameState)
        return;

    const float Remaining = FMath::Max(0.0f, RoundPhaseEndTime - GetWorld()->GetTimeSeconds());
    WCTGameState->SetTimeRemaining(Remaining);
}

void AWCTGameMode::SetControllerMovementEnabled(AController* Controller, bool bEnabled) const
{
    if (!Controller)
        return;

    if (APlayerController* PlayerController = Cast<APlayerController>(Controller))
        if (AWCTPlayerController* WCTController = Cast<AWCTPlayerController>(PlayerController))
            WCTController->ClientSetMoveInputEnabled(bEnabled);

    ACharacter* Character = Cast<ACharacter>(Controller->GetPawn());
    if (!Character)
        return;

    UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
    if (!Movement)
        return;

    if (bEnabled)
        Movement->SetMovementMode(MOVE_Walking);
    else
    {
        Movement->StopMovementImmediately();
        Movement->DisableMovement();
    }
}

AController* AWCTGameMode::FindOtherController(AController* Controller) const
{
    if (!Controller)
        return nullptr;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        AController* Current = It->Get();
        if (Current && Current != Controller)
            return Current;
    }

    return nullptr;
}

AWCTGameState* AWCTGameMode::GetWCTGameState() const
{
    return GetGameState<AWCTGameState>();
}

AWCTPlayerState* AWCTGameMode::GetWCTPlayerState(AController* Controller) const
{
    return Controller ? Cast<AWCTPlayerState>(Controller->PlayerState) : nullptr;
}
