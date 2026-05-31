#include "WCTDiveVaultComponent.h"

#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/MovementComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Net/UnrealNetwork.h"

UWCTDiveVaultComponent::UWCTDiveVaultComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetIsReplicatedByDefault(true);
}

void UWCTDiveVaultComponent::BeginPlay()
{
    Super::BeginPlay();
    SetComponentTickEnabled(false);
}

void UWCTDiveVaultComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    FinishDiveVault();
    Super::EndPlay(EndPlayReason);
}

void UWCTDiveVaultComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    AActor* Owner = GetOwner();
    if (!bIsDiveVaulting || !Owner || !Owner->HasAuthority())
    {
        return;
    }

    DiveVaultElapsed += DeltaTime;

    if (DiveVaultElapsed < DiveVaultMoveDelay)
    {
        return;
    }

    BeginDiveVaultMovement();

    const float MoveElapsed = DiveVaultElapsed - DiveVaultMoveDelay;
    const float Alpha = FMath::Clamp(MoveElapsed / FMath::Max(DiveVaultDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
    FVector NewLocation = FMath::Lerp(DiveVaultStartLocation, DiveVaultLandingLocation, Alpha);
    NewLocation.Z += FMath::Sin(Alpha * PI) * DiveVaultHeight;

    FHitResult MoveHit;
    Owner->SetActorLocation(NewLocation, true, &MoveHit);

    if (Alpha >= 1.0f || MoveHit.bBlockingHit)
    {
        FinishDiveVault();
    }
}

void UWCTDiveVaultComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(UWCTDiveVaultComponent, bIsDiveVaulting);
}

void UWCTDiveVaultComponent::StartDiveVault()
{
    AActor* Owner = GetOwner();
    if (!Owner)
    {
        return;
    }

    FVector LandingLocation;
    if (!CanDiveVault(LandingLocation))
    {
        if (Owner->HasAuthority())
        {
            ClientDiveVaultRejected();
        }

        return;
    }

    if (!Owner->HasAuthority())
    {
        if (bPredictMontageOnOwningClient)
        {
            PlayDiveVaultMontage();
        }

        DisableMoveMappingContext();
        ServerStartDiveVault();
        return;
    }

    bIsDiveVaulting = true;
    bDiveVaultMovementStarted = false;
    DiveVaultElapsed = 0.0f;
    DiveVaultStartLocation = Owner->GetActorLocation();
    DiveVaultLandingLocation = LandingLocation;

    MulticastPlayDiveVaultMontage();

    SetComponentTickEnabled(true);
    DisableMoveMappingContext();
    BP_OnDiveVaultStarted();
}

void UWCTDiveVaultComponent::ServerStartDiveVault_Implementation()
{
    StartDiveVault();
}

void UWCTDiveVaultComponent::ClientDiveVaultRejected_Implementation()
{
    RestoreMoveMappingContext();
}

void UWCTDiveVaultComponent::MulticastPlayDiveVaultMontage_Implementation()
{
    PlayDiveVaultMontage();
}

bool UWCTDiveVaultComponent::CanDiveVault(FVector& OutLandingLocation) const
{
    const AActor* Owner = GetOwner();
    if (bIsDiveVaulting || !Owner || !GetWorld())
    {
        return false;
    }

    const FVector ActorLocation = Owner->GetActorLocation();
    const FVector Forward = Owner->GetActorForwardVector();
    const FVector TraceStart = ActorLocation + FVector(0.0f, 0.0f, 35.0f);
    const FVector TraceEnd = TraceStart + Forward * DiveVaultDistance;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(DiveVaultTrace), false, Owner);
    FHitResult ObstacleHit;
    const bool bHitObstacle = GetWorld()->LineTraceSingleByChannel(
        ObstacleHit,
        TraceStart,
        TraceEnd,
        VaultTraceChannel,
        Params
    );

    if (bHitObstacle)
    {
        const float RelativeObstacleHeight = ObstacleHit.ImpactPoint.Z - ActorLocation.Z;
        if (RelativeObstacleHeight > MaxVaultObstacleHeight)
        {
            return false;
        }
    }

    OutLandingLocation = ActorLocation + Forward * DiveVaultDistance;

    FHitResult GroundHit;
    const FVector GroundTraceStart = OutLandingLocation + FVector(0.0f, 0.0f, 200.0f);
    const FVector GroundTraceEnd = OutLandingLocation - FVector(0.0f, 0.0f, 300.0f);
    const bool bFoundGround = GetWorld()->LineTraceSingleByChannel(
        GroundHit,
        GroundTraceStart,
        GroundTraceEnd,
        VaultTraceChannel,
        Params
    );

    if (bFoundGround)
    {
        const UCapsuleComponent* Capsule = FindCapsuleComponent();
        const float HalfHeight = Capsule ? Capsule->GetScaledCapsuleHalfHeight() : DefaultCapsuleHalfHeight;
        OutLandingLocation.Z = GroundHit.ImpactPoint.Z + HalfHeight + LandingClearance;
    }

    if (bRequireLandingRoom)
    {
        const UCapsuleComponent* Capsule = FindCapsuleComponent();
        const float RoomCheckHalfHeight = Capsule ? Capsule->GetUnscaledCapsuleHalfHeight() : DefaultCapsuleHalfHeight;
        return HasRoomForCapsuleAt(OutLandingLocation, RoomCheckHalfHeight);
    }

    return true;
}

void UWCTDiveVaultComponent::BeginDiveVaultMovement()
{
    if (bDiveVaultMovementStarted)
    {
        return;
    }

    bDiveVaultMovementStarted = true;

    if (AActor* Owner = GetOwner())
    {
        DiveVaultStartLocation = Owner->GetActorLocation();
    }

    if (UMovementComponent* MovementComponent = FindMovementComponent())
    {
        MovementComponent->StopMovementImmediately();
        bMovementWasActive = MovementComponent->IsActive();

        if (bDeactivateMovementDuringVault)
        {
            MovementComponent->Deactivate();
        }
    }

    if (UCapsuleComponent* Capsule = FindCapsuleComponent())
    {
        Capsule->SetCapsuleHalfHeight(DiveVaultCapsuleHalfHeight, true);
    }

    BP_OnDiveVaultMovementStarted();
}

void UWCTDiveVaultComponent::FinishDiveVault()
{
    if (!bIsDiveVaulting)
    {
        return;
    }

    bIsDiveVaulting = false;
    bDiveVaultMovementStarted = false;
    SetComponentTickEnabled(false);

    if (UCapsuleComponent* Capsule = FindCapsuleComponent())
    {
        Capsule->SetCapsuleHalfHeight(DefaultCapsuleHalfHeight, true);
    }

    if (UMovementComponent* MovementComponent = FindMovementComponent())
    {
        if (bDeactivateMovementDuringVault && bMovementWasActive)
        {
            MovementComponent->Activate();
        }
    }

    RestoreMoveMappingContext();
    BP_OnDiveVaultFinished();
}

void UWCTDiveVaultComponent::OnRep_IsDiveVaulting()
{
    if (bIsDiveVaulting)
    {
        DisableMoveMappingContext();
    }
    else
    {
        RestoreMoveMappingContext();
    }
}

void UWCTDiveVaultComponent::PlayDiveVaultMontage()
{
    if (!DiveVaultMontage)
    {
        return;
    }

    if (USkeletalMeshComponent* Mesh = FindMeshComponent())
    {
        if (UAnimInstance* AnimInstance = Mesh->GetAnimInstance())
        {
            if (AnimInstance->Montage_IsPlaying(DiveVaultMontage))
            {
                return;
            }

            AnimInstance->Montage_Play(DiveVaultMontage, DiveVaultMontagePlayRate);
            FOnMontageEnded EndDelegate;
            EndDelegate.BindUObject(this, &UWCTDiveVaultComponent::OnDiveVaultMontageEnded);
            AnimInstance->Montage_SetEndDelegate(EndDelegate, DiveVaultMontage);
        }
    }
}

void UWCTDiveVaultComponent::OnDiveVaultMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
    const AActor* Owner = GetOwner();
    if (Owner && Owner->HasAuthority() && Montage == DiveVaultMontage && !bDiveVaultMovementStarted)
    {
        FinishDiveVault();
    }
}

bool UWCTDiveVaultComponent::HasRoomForCapsuleAt(const FVector& Location, float CapsuleHalfHeight) const
{
    const AActor* Owner = GetOwner();
    const UCapsuleComponent* Capsule = FindCapsuleComponent();
    if (!Owner || !Capsule || !GetWorld())
    {
        return true;
    }

    const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
    const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DiveVaultRoomCheck), false, Owner);

    return !GetWorld()->OverlapBlockingTestByChannel(
        Location,
        FQuat::Identity,
        Capsule->GetCollisionObjectType(),
        CapsuleShape,
        Params
    );
}

UCapsuleComponent* UWCTDiveVaultComponent::FindCapsuleComponent() const
{
    return GetOwner() ? GetOwner()->FindComponentByClass<UCapsuleComponent>() : nullptr;
}

USkeletalMeshComponent* UWCTDiveVaultComponent::FindMeshComponent() const
{
    return GetOwner() ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
}

UMovementComponent* UWCTDiveVaultComponent::FindMovementComponent() const
{
    return GetOwner() ? GetOwner()->FindComponentByClass<UMovementComponent>() : nullptr;
}

bool UWCTDiveVaultComponent::IsLocallyControlledOwner() const
{
    if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
    {
        return OwnerPawn->IsLocallyControlled();
    }

    return false;
}

void UWCTDiveVaultComponent::DisableMoveMappingContext()
{
    if (!bDisableMoveMappingContextDuringVault || bMoveMappingContextDisabled || !MoveMappingContextToDisable || !IsLocallyControlledOwner())
    {
        return;
    }

    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    const APlayerController* PlayerController = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
    const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
    UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;

    if (InputSubsystem)
    {
        InputSubsystem->RemoveMappingContext(MoveMappingContextToDisable);
        bMoveMappingContextDisabled = true;
    }
}

void UWCTDiveVaultComponent::RestoreMoveMappingContext()
{
    if (!bMoveMappingContextDisabled || !MoveMappingContextToDisable || !IsLocallyControlledOwner())
    {
        return;
    }

    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    const APlayerController* PlayerController = OwnerPawn ? Cast<APlayerController>(OwnerPawn->GetController()) : nullptr;
    const ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr;
    UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>() : nullptr;

    if (InputSubsystem)
    {
        InputSubsystem->AddMappingContext(MoveMappingContextToDisable, MoveMappingContextPriority);
    }

    bMoveMappingContextDisabled = false;
}
