#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "WCTCharacterMover.generated.h"

class UAnimMontage;

UCLASS()
class FINALCHASE_API AWCTCharacterMover : public ACharacter
{
    GENERATED_BODY()

public:
    AWCTCharacterMover();

    virtual void Tick(float DeltaSeconds) override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    UFUNCTION(BlueprintCallable, Category = "WCT|Movement|Dive Vault")
    void StartDiveVault();

    UFUNCTION(BlueprintCallable, Category = "WCT|Movement|Dive Vault")
    bool CanDiveVault(FVector& OutLandingLocation) const;

    UFUNCTION(BlueprintPure, Category = "WCT|Movement|Dive Vault")
    bool IsDiveVaulting() const { return bIsDiveVaulting; }

protected:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WCT|Movement|Dive Vault")
    TObjectPtr<UAnimMontage> DiveVaultMontage;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WCT|Movement|Dive Vault", meta = (ClampMin = "0.1"))
    float DiveVaultDuration = 0.55f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WCT|Movement|Dive Vault", meta = (ClampMin = "0.0"))
    float DiveVaultDistance = 420.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WCT|Movement|Dive Vault", meta = (ClampMin = "0.0"))
    float DiveVaultHeight = 130.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WCT|Movement|Dive Vault", meta = (ClampMin = "1.0"))
    float DiveVaultCapsuleHalfHeight = 50.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WCT|Movement|Dive Vault", meta = (ClampMin = "0.0"))
    float MaxVaultObstacleHeight = 120.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WCT|Movement|Dive Vault", meta = (ClampMin = "0.0"))
    float LandingClearance = 35.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "WCT|Movement|Dive Vault")
    TEnumAsByte<ECollisionChannel> VaultTraceChannel = ECC_Visibility;

    UFUNCTION(BlueprintImplementableEvent, Category = "WCT|Movement|Dive Vault")
    void BP_OnDiveVaultStarted();

    UFUNCTION(BlueprintImplementableEvent, Category = "WCT|Movement|Dive Vault")
    void BP_OnDiveVaultFinished();

private:
    UPROPERTY(ReplicatedUsing = OnRep_DiveVaulting)
    bool bIsDiveVaulting = false;

    UFUNCTION()
    void OnRep_DiveVaulting();

    UFUNCTION(Server, Reliable)
    void ServerStartDiveVault();

    void BeginDiveVault(const FVector& LandingLocation);
    void FinishDiveVault();
    void UpdateDiveVault(float DeltaSeconds);
    bool HasRoomForCapsuleAt(const FVector& Location, float CapsuleHalfHeight) const;

    FVector DiveVaultStartLocation = FVector::ZeroVector;
    FVector DiveVaultLandingLocation = FVector::ZeroVector;
    float DiveVaultElapsed = 0.0f;
    float DefaultCapsuleHalfHeight = 88.0f;
    float DefaultGravityScale = 1.0f;
};
