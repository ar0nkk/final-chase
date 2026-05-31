#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "WCTTypes.h"
#include "WCTGameMode.generated.h"

class AWCTGameState;
class AWCTPlayerState;
class APlayerController;
class AWCTPlayerController;

UCLASS()
class FINALCHASE_API AWCTGameMode : public AGameModeBase
{
    GENERATED_BODY()

public:
    AWCTGameMode();

    virtual void BeginPlay() override;
    virtual void PostLogin(APlayerController* NewPlayer) override;
    virtual void Logout(AController* Exiting) override;

    UFUNCTION(BlueprintCallable, Category = "WCT|Round")
    void ReportCapture(AController* Capturer, AController* Captured);

protected:
    UPROPERTY(EditDefaultsOnly, Category = "WCT|Round")
    int32 TotalRounds;

    UPROPERTY(EditDefaultsOnly, Category = "WCT|Round")
    float PreRoundSeconds;

    UPROPERTY(EditDefaultsOnly, Category = "WCT|Round")
    float ChasingSeconds;

    UPROPERTY(EditDefaultsOnly, Category = "WCT|Round")
    float RoundEndSeconds;

private:
    void TryStartMatch();
    void StartNextRound();
    void BeginPreRound();
    void BeginChasing();
    void EndRound(AController* Winner, bool bRunnerSurvived);
    void FinishMatch();
    void AssignRolesForNewRound();
    void MovePlayersToStart();
    void TeleportControllerToTag(AController* Controller, FName StartTag) const;
    AActor* FindPlayerStartByTag(FName StartTag) const;
    void UpdateTimeRemaining();

    void SetControllerMovementEnabled(AController* Controller, bool bEnabled) const;
    AController* FindOtherController(AController* Controller) const;
    AWCTGameState* GetWCTGameState() const;
    AWCTPlayerState* GetWCTPlayerState(AController* Controller) const;

    FTimerHandle PhaseEndTimerHandle;
    FTimerHandle TimeRemainingTimerHandle;
    FTimerHandle RoundTransitionTimerHandle;

    float RoundPhaseEndTime;
    bool bMatchStarted;

    TWeakObjectPtr<AController> HostController;
    TWeakObjectPtr<AController> RunnerController;
    TWeakObjectPtr<AController> ChaserController;
    TWeakObjectPtr<AController> NextRunnerController;
};
