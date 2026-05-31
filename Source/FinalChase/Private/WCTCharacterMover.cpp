#include "WCTCharacterMover.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"

AWCTCharacterMover::AWCTCharacterMover()
{
    PrimaryActorTick.bCanEverTick = true;
    bReplicates = true;
}

void AWCTCharacterMover::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    if (bIsDiveVaulting && HasAuthority())
    {
        UpdateDiveVault(DeltaSeconds);
    }
}

void AWCTCharacterMover::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AWCTCharacterMover, bIsDiveVaulting);
}

void AWCTCharacterMover::StartDiveVault()
{
    if (!HasAuthority())
    {
        ServerStartDiveVault();
        return;
    }

    FVector LandingLocation;
    if (CanDiveVault(LandingLocation))
    {
        BeginDiveVault(LandingLocation);
    }
}

void AWCTCharacterMover::ServerStartDiveVault_Implementation()
{
    StartDiveVault();
}

bool AWCTCharacterMover::CanDiveVault(FVector& OutLandingLocation) const
{
    if (bIsDiveVaulting || !GetCapsuleComponent())
    {
        return false;
    }

    const FVector ActorLocation = GetActorLocation();
    const FVector Forward = GetActorForwardVector();
    const FVector TraceStart = ActorLocation + FVector(0.0f, 0.0f, 35.0f);
    const FVector TraceEnd = TraceStart + Forward * DiveVaultDistance;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(DiveVaultTrace), false, this);
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
        OutLandingLocation.Z = GroundHit.ImpactPoint.Z + GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + LandingClearance;
    }

    return HasRoomForCapsuleAt(OutLandingLocation, GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight());
}

void AWCTCharacterMover::BeginDiveVault(const FVector& LandingLocation)
{
    if (bIsDiveVaulting)
    {
        return;
    }

    bIsDiveVaulting = true;
    DiveVaultElapsed = 0.0f;
    DiveVaultStartLocation = GetActorLocation();
    DiveVaultLandingLocation = LandingLocation;
    DefaultCapsuleHalfHeight = GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight();
    DefaultGravityScale = GetCharacterMovement()->GravityScale;

    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->SetMovementMode(MOVE_Flying);
    GetCharacterMovement()->GravityScale = 0.0f;

    GetCapsuleComponent()->SetCapsuleHalfHeight(DiveVaultCapsuleHalfHeight, true);

    if (DiveVaultMontage)
    {
        PlayAnimMontage(DiveVaultMontage);
    }

    BP_OnDiveVaultStarted();
}

void AWCTCharacterMover::UpdateDiveVault(float DeltaSeconds)
{
    DiveVaultElapsed += DeltaSeconds;

    const float Alpha = FMath::Clamp(DiveVaultElapsed / FMath::Max(DiveVaultDuration, KINDA_SMALL_NUMBER), 0.0f, 1.0f);
    FVector NewLocation = FMath::Lerp(DiveVaultStartLocation, DiveVaultLandingLocation, Alpha);
    NewLocation.Z += FMath::Sin(Alpha * PI) * DiveVaultHeight;

    FHitResult MoveHit;
    SetActorLocation(NewLocation, true, &MoveHit);

    if (Alpha >= 1.0f || MoveHit.bBlockingHit)
    {
        FinishDiveVault();
    }
}

void AWCTCharacterMover::FinishDiveVault()
{
    if (!bIsDiveVaulting)
    {
        return;
    }

    bIsDiveVaulting = false;

    if (HasRoomForCapsuleAt(GetActorLocation(), DefaultCapsuleHalfHeight))
    {
        GetCapsuleComponent()->SetCapsuleHalfHeight(DefaultCapsuleHalfHeight, true);
    }

    GetCharacterMovement()->GravityScale = DefaultGravityScale;
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);

    BP_OnDiveVaultFinished();
}

bool AWCTCharacterMover::HasRoomForCapsuleAt(const FVector& Location, float CapsuleHalfHeight) const
{
    const UCapsuleComponent* Capsule = GetCapsuleComponent();
    if (!Capsule)
    {
        return false;
    }

    const float CapsuleRadius = Capsule->GetScaledCapsuleRadius();
    const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DiveVaultRoomCheck), false, this);

    return !GetWorld()->OverlapBlockingTestByChannel(
        Location,
        FQuat::Identity,
        GetCapsuleComponent()->GetCollisionObjectType(),
        CapsuleShape,
        Params
    );
}

void AWCTCharacterMover::OnRep_DiveVaulting()
{
    if (bIsDiveVaulting && DiveVaultMontage)
    {
        PlayAnimMontage(DiveVaultMontage);
    }
}
