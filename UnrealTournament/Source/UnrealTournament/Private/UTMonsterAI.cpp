// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTMonsterAI.h"

void AUTMonsterAI::CheckWeaponFiring(bool bFromWeapon)
{
	if (bOneShotAttacks && bFromWeapon)
	{
		if (GetUTChar() != nullptr)
		{
			GetUTChar()->StopFiring();
			GetWorldTimerManager().SetTimer(CheckWeaponFiringTimerHandle, this, &AUTMonsterAI::CheckWeaponFiringTimed, 1.2f - 0.09f * FMath::Min<float>(10.0f, Skill + Personality.ReactionTime), true);
		}
	}
	// if invis, don't attack until close, under attack, or have flag
	else if ( bFromWeapon || GetUTChar() == nullptr || !GetUTChar()->IsInvisible() || GetWorld()->TimeSeconds - GetUTChar()->LastTakeHitTime < 3.0f ||
		GetUTChar()->GetCarriedObject() != nullptr || GetTarget() == nullptr || (GetTarget()->GetActorLocation() - GetUTChar()->GetActorLocation()).Size() < 2000.0f )
	{
		Super::CheckWeaponFiring(bFromWeapon);
	}
}

void AUTMonsterAI::UpdateControlRotation(float DeltaTime, bool bUpdatePawn)
{
	TGuardValue<FRotator> RotationRateGuard(RotationRate, (GetUTChar() != nullptr && GetUTChar()->GetWeapon() != nullptr && GetUTChar()->GetWeapon()->IsFiring()) ? (RotationRate * FiringRotationRateMult) : RotationRate);
	Super::UpdateControlRotation(DeltaTime, bUpdatePawn);
}

bool AUTMonsterAI::NeedToTurn(const FVector& TargetLoc, bool bForcePrecise)
{
	// if we intentionally made the AI unable to keep up with players strafing around them then don't stop firing because it happened
	if (!bForcePrecise && FiringRotationRateMult < 1.0f && GetUTChar() != nullptr && GetUTChar()->GetWeapon() != nullptr && GetUTChar()->GetWeapon()->IsFiring())
	{
		return false;
	}
	else
	{
		return Super::NeedToTurn(TargetLoc, bForcePrecise);
	}
}