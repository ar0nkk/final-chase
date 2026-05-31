#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WCTDiveVaultComponent.generated.h"

class UAnimMontage;
class UCapsuleComponent;
class UInputMappingContext;
class UMovementComponent;
class USkeletalMeshComponent;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class FINALCHASE_API UWCTDiveVaultComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UWCTDiveVaultComponent();

    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "WCT|Movement|Dive Vault")
    void StartDiveVault();

    UFUNCTION(BlueprintCallable, Category = "WCT|Movement|Dive Vault")
    bool CanDiveVault(FVector& OutLandingLocation) const;

    UFUNCTION(BlueprintPure, Category = "WCT|Movement|Dive Vault")
    bool IsDiveVaulting() const { return bIsDiveVaulting; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WCT|Movement|Dive Vault")
    TObjectPtr<UAnimMontage> DiveVaultMontage;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WCT|Movement|Dive Vault", meta = (ClampMin = "0.1"))
    float DiveVaultDuration = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WCT|Movement|Dive Vault", meta = (ClampMin = "0.0"))
    float DiveVaultMoveDelay = 0.16f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WCT|Movement|Dive Vault", meta = (ClampMin = "0.1"))
    float DiveVaultMontagePlayRate = 1.35f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WCT|Movement|Dive Vault", meta = (ClampMin = "0.0"))
    float DiveVaultDistance = 420.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WCT|Movement|Dive Vault", meta = (ClampMin = "0.0"))
    float DiveVaultHeight = 140.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WCT|Movement|Dive Vault", meta = (ClampMin = "1.0"))
    float DiveVaultCapsuleHalfHeight = 50.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WCT|Movement|Dive Vault", meta = (ClampMin = "1.0"))
    float DefaultCapsuleHalfHeight = 88.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WCT|Movement|Dive Vault", meta = (ClampMin = "0.0"))
    float MaxVaultObstacleHeight = 120.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WCT|Movement|Dive Vault", meta = (ClampMin = "0.0"))
    float LandingClearance = 35.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WCT|Movement|Dive Vault")
    bool bDeactivateMovementDuringVault = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WCT|Movement|Dive Vault")
    bool bPredictMontageOnOwningClient = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WCT|Movement|Dive Vault")
    bool bRequireLandingRoom = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WCT|Movement|Dive Vault")
    TEnumAsByte<ECollisionChannel> VaultTraceChannel = ECC_Visibility;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WCT|Movement|Dive Vault|Input")
    bool bDisableMoveMappingContextDuringVault = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WCT|Movement|Dive Vault|Input")
    TObjectPtr<UInputMappingContext> MoveMappingContextToDisable;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "WCT|Movement|Dive Vault|Input")
    int32 MoveMappingContextPriority = 0;

    UFUNCTION(BlueprintImplementableEvent, Category = "WCT|Movement|Dive Vault")
    void BP_OnDiveVaultStarted();

    UFUNCTION(BlueprintImplementableEvent, Category = "WCT|Movement|Dive Vault")
    void BP_OnDiveVaultMovementStarted();

    UFUNCTION(BlueprintImplementableEvent, Category = "WCT|Movement|Dive Vault")
    void BP_OnDiveVaultFinished();

private:
    UFUNCTION(Server, Reliable)
    void ServerStartDiveVault();

    UFUNCTION(NetMulticast, Reliable)
    void MulticastPlayDiveVaultMontage();

    UFUNCTION(Client, Reliable)
    void ClientDiveVaultRejected();

    UFUNCTION()
    void OnRep_IsDiveVaulting();

    void BeginDiveVaultMovement();
    void FinishDiveVault();
    void PlayDiveVaultMontage();
    void DisableMoveMappingContext();
    void RestoreMoveMappingContext();
    bool IsLocallyControlledOwner() const;

    UFUNCTION()
    void OnDiveVaultMontageEnded(UAnimMontage* Montage, bool bInterrupted);

    bool HasRoomForCapsuleAt(const FVector& Location, float CapsuleHalfHeight) const;
    UCapsuleComponent* FindCapsuleComponent() const;
    USkeletalMeshComponent* FindMeshComponent() const;
    UMovementComponent* FindMovementComponent() const;

    UPROPERTY(ReplicatedUsing = OnRep_IsDiveVaulting)
    bool bIsDiveVaulting = false;

    bool bDiveVaultMovementStarted = false;
    bool bMovementWasActive = false;
    bool bMoveMappingContextDisabled = false;

    FVector DiveVaultStartLocation = FVector::ZeroVector;
    FVector DiveVaultLandingLocation = FVector::ZeroVector;
    float DiveVaultElapsed = 0.0f;
};
