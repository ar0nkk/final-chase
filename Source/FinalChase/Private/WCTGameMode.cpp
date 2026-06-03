#include "WCTGameMode.h"
#include "WCTGameState.h"
#include "WCTPlayerState.h"
#include "WCTPlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Components/AudioComponent.h"

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

    PlayGlobalStartSound();

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
    // 修复枚举名 EWCTRoundResult
    WCTGameState->SetRoundResult(bRunnerSurvived ? EWCTRoundResult::Evasion : EWCTRoundResult::Tag);

    PlayGlobalEndSound();

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
        // 修复枚举
        WCTGameState->SetRoundResult(EWCTRoundResult::None);
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
            if (APlayerController* PC = It->Get())
            {
                RunnerController = PC;
                break;
            }
        }
    }

    if (RunnerController.IsValid())
        ChaserController = FindOtherController(RunnerController.Get());

    NextRunnerController.Reset();

    if (AWCTPlayerState* RunnerState = GetWCTPlayerState(RunnerController.Get()))
        RunnerState->bIsChaser = false;
    if (AWCTPlayerState* ChaserState = GetWCTPlayerState(ChaserController.Get()))
        ChaserState->bIsChaser = true;
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

    AActor* StartActor = FindPlayerStartByTag(StartTag);
    if (!StartActor)
        return;

    APawn* Pawn = Controller->GetPawn();
    if (Pawn)
        Pawn->TeleportTo(StartActor->GetActorLocation(), StartActor->GetActorRotation(), false, true);
}

AActor* AWCTGameMode::FindPlayerStartByTag(FName StartTag) const
{
    TArray<AActor*> AllStarts;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), AllStarts);

    for (AActor* Item : AllStarts)
    {
        if (Item && Item->ActorHasTag(StartTag))
            return Item;
    }
    return nullptr;
}

void AWCTGameMode::UpdateTimeRemaining()
{
    AWCTGameState* WCTGameState = GetWCTGameState();
    if (!WCTGameState)
        return;

    float Remain = FMath::Max(0.0f, RoundPhaseEndTime - GetWorld()->GetTimeSeconds());
    WCTGameState->SetTimeRemaining(Remain);
}

void AWCTGameMode::SetControllerMovementEnabled(AController* Controller, bool bEnabled) const
{
    if (!Controller)
        return;

    if (APlayerController* PC = Cast<APlayerController>(Controller))
    {
        if (AWCTPlayerController* WCTPC = Cast<AWCTPlayerController>(PC))
            WCTPC->ClientSetMoveInputEnabled(bEnabled);
    }

    ACharacter* Char = Cast<ACharacter>(Controller->GetPawn());
    if (!Char)
        return;

    UCharacterMovementComponent* MoveComp = Char->GetCharacterMovement();
    if (!MoveComp)
        return;

    if (bEnabled)
        MoveComp->SetMovementMode(MOVE_Walking);
    else
    {
        MoveComp->StopMovementImmediately();
        MoveComp->DisableMovement();
    }
}

AController* AWCTGameMode::FindOtherController(AController* Controller) const
{
    if (!Controller)
        return nullptr;

    for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
    {
        APlayerController* CurPC = It->Get();
        if (CurPC && CurPC != Controller)
            return CurPC;
    }
    return nullptr;
}

// 修复GetGameState模板写法
AWCTGameState* AWCTGameMode::GetWCTGameState() const
{
    return GetGameState<AWCTGameState>();
}

AWCTPlayerState* AWCTGameMode::GetWCTPlayerState(AController* Controller) const
{
    if (!Controller) return nullptr;
    return Cast<AWCTPlayerState>(Controller->PlayerState);
}

// 音效固定代码（无查找名字、稳定编译）
void AWCTGameMode::PlayGlobalStartSound()
{
    if (!HasAuthority()) return;

    TArray<AActor*> AudioActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("GlobalAudio"), AudioActors);
    if (AudioActors.IsEmpty()) return;

    AActor* AudioActor = AudioActors[0];
    TArray<UAudioComponent*> AudioList;
    AudioActor->GetComponents<UAudioComponent>(AudioList);

    if (AudioList.Num() > 0)
    {
        AudioList[0]->Stop();
        AudioList[0]->Play();
    }
}

void AWCTGameMode::PlayGlobalEndSound()
{
    if (!HasAuthority()) return;

    TArray<AActor*> AudioActors;
    UGameplayStatics::GetAllActorsWithTag(GetWorld(), FName("GlobalAudio"), AudioActors);
    if (AudioActors.IsEmpty()) return;

    AActor* AudioActor = AudioActors[0];
    TArray<UAudioComponent*> AudioList;
    AudioActor->GetComponents<UAudioComponent>(AudioList);

    if (AudioList.Num() > 1)
    {
        AudioList[1]->Stop();
        AudioList[1]->Play();
    }
}