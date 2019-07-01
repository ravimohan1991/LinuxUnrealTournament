// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTWeap_Enforcer.h"
#include "UTWeaponState.h"
#include "UTWeaponStateActive.h"
#include "UTWeaponStateFiring.h"
#include "UTWeaponStateFiring_Enforcer.h"
#include "UTWeaponStateFiringBurstEnforcer.h"
#include "UnrealNetwork.h"
#include "StatNames.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"

AUTWeap_Enforcer::AUTWeap_Enforcer(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = true;

	DefaultGroup = 2;
	Ammo = 20;
	MaxAmmo = 40;
	LastFireTime = 0.f;
	SpreadResetInterval = 1.f;
	SpreadIncrease = 0.01f;
	MaxSpread = 0.05f;
	VerticalSpreadScaling = 8.f;
	BringUpTime = 0.28f;
	PutDownTime = 0.2f;
	StoppingPower = 30000.f;
	BaseAISelectRating = 0.4f;
	FireCount = 0;
	ImpactCount = 0;
	bCanThrowWeapon = false;
	FOVOffset = FVector(0.7f, 1.f, 1.f);
	MaxTracerDist = 2500.f;
	bNoDropInTeamSafe = true;
	ReloadClipTime = 2.0f;

	KillStatsName = NAME_EnforcerKills;
	DeathStatsName = NAME_EnforcerDeaths;
	HitsStatsName = NAME_EnforcerHits;
	ShotsStatsName = NAME_EnforcerShots;

	DisplayName = NSLOCTEXT("UTWeap_Enforcer", "DisplayName", "Enforcer");
	bCheckHeadSphere = true;
	bCheckMovingHeadSphere = true;
	bIgnoreShockballs = true;

	WeaponCustomizationTag = EpicWeaponCustomizationTags::Enforcer;
	WeaponSkinCustomizationTag = EpicWeaponSkinCustomizationTags::Enforcer;

	HighlightText = NSLOCTEXT("Weapon", "EnforcerHighlightText", "Gunslinger");
	LowMeshOffset = FVector(0.f, 0.f, -7.f);
	VeryLowMeshOffset = FVector(0.f, 0.f, -15.f);
	MaxVerticalSpread = 2.5f;
	bShouldPrecacheTutorialAnnouncements = false;
}

float AUTWeap_Enforcer::GetImpartedMomentumMag(AActor* HitActor)
{
	AUTCharacter* HitChar = Cast<AUTCharacter>(HitActor);
	if (HitChar && HitChar->IsDead())
	{
		return 20000.f;
	}
	return (HitChar && HitChar->GetWeapon() && HitChar->GetWeapon()->bAffectedByStoppingPower)
		? StoppingPower
		: InstantHitInfo[CurrentFireMode].Momentum;
}

void AUTWeap_Enforcer::FireShot()
{
	Super::FireShot();

	if (GetNetMode() != NM_DedicatedServer)
	{
		FireCount++;

		UUTWeaponStateFiringBurst* BurstFireMode = Cast<UUTWeaponStateFiringBurst>(FiringState[GetCurrentFireMode()]);
		if ((BurstFireMode && FireCount >= BurstFireMode->BurstSize * 2) || (!BurstFireMode && FireCount > 1))
		{
			FireCount = 0;
		}
	}
}

void AUTWeap_Enforcer::FireInstantHit(bool bDealDamage, FHitResult* OutHit)
{
	// burst mode takes care of spread variation itself
	if (!Cast<UUTWeaponStateFiringBurst>(FiringState[GetCurrentFireMode()]))
	{
		ModifySpread();
	}

	Super::FireInstantHit(bDealDamage, OutHit);
	if (UTOwner)
	{
		LastFireTime = UTOwner->GetWorld()->GetTimeSeconds();
	}
}

void AUTWeap_Enforcer::ModifySpread_Implementation()
{
	float TimeSinceFired = UTOwner->GetWorld()->GetTimeSeconds() - LastFireTime;
	float SpreadScalingOverTime = FMath::Max(0.f, 1.f - (TimeSinceFired - FireInterval[GetCurrentFireMode()]) / (SpreadResetInterval - FireInterval[GetCurrentFireMode()]));
	Spread[GetCurrentFireMode()] = FMath::Min(MaxSpread, Spread[GetCurrentFireMode()] + SpreadIncrease) * SpreadScalingOverTime;
}

bool AUTWeap_Enforcer::HasAnyAmmo()
{
	// can always reload
	return true;
}

void AUTWeap_Enforcer::GotoState(UUTWeaponState* NewState)
{
	Super::GotoState(NewState);

	if ((CurrentState == ActiveState) && (Role == ROLE_Authority) && !Super::HasAnyAmmo() && UTOwner)
	{
		// @TODO FIXMESTEVE - if keep this functionality and have animation, need full weapon state to support
		if (Cast<AUTPlayerController>(UTOwner->GetController()))
		{
			GetWorldTimerManager().SetTimer(ReloadSoundHandle, this, &AUTWeap_Enforcer::PlayReloadSound, 0.5f, false);
		}
		GetWorldTimerManager().SetTimer(ReloadClipHandle, this, &AUTWeap_Enforcer::ReloadClip, ReloadClipTime, false);
	}
}

void AUTWeap_Enforcer::PlayReloadSound()
{
	if (!Super::HasAnyAmmo() && (CurrentState == ActiveState) && (Role == ROLE_Authority))
	{
		Cast<AUTPlayerController>(UTOwner->GetController())->UTClientPlaySound(ReloadClipSound);
	}
}

void AUTWeap_Enforcer::ReloadClip()
{
	if (!Super::HasAnyAmmo() && (CurrentState == ActiveState) && (Role == ROLE_Authority))
	{
		AddAmmo(20);
	}
}

void AUTWeap_Enforcer::FiringInfoUpdated_Implementation(uint8 InFireMode, uint8 FlashCount, FVector InFlashLocation)
{
	CurrentFireMode = InFireMode;
	UUTWeaponStateFiringBurst* BurstFireMode = Cast<UUTWeaponStateFiringBurst>(FiringState[GetCurrentFireMode()]);
	if (InFlashLocation.IsZero())
	{
		FireCount = 0;
		ImpactCount = 0;
		if (BurstFireMode != nullptr)
		{
			BurstFireMode->CurrentShot = 0;
		}
	}
	else
	{
		PlayFiringEffects();

		FireCount++;
		if (BurstFireMode)
		{
			BurstFireMode->CurrentShot++;
		}

		if ((BurstFireMode && FireCount >= BurstFireMode->BurstSize * 2) || (!BurstFireMode && FireCount > 1))
		{
			FireCount = 0;
		}
		if (BurstFireMode && BurstFireMode->CurrentShot >= BurstFireMode->BurstSize)
		{
			BurstFireMode->CurrentShot = 0;
		}
	}
}

void AUTWeap_Enforcer::StateChanged()
{
	Super::StateChanged();

	if (!FiringState.Contains(Cast<UUTWeaponStateFiring>(CurrentState)))
	{
		FireCount = 0;
		ImpactCount = 0;
	}
}

void AUTWeap_Enforcer::PlayFiringEffects()
{
	UUTWeaponStateFiringBurstEnforcer* BurstFireMode = Cast<UUTWeaponStateFiringBurstEnforcer>(FiringState[GetCurrentFireMode()]);

	if (UTOwner != NULL)
	{
		if (!BurstFireMode || (BurstFireMode->CurrentShot == 0))
		{
			Super::PlayFiringEffects();
		}
		else
		{
			if (!bDualWeaponMode)
			{
				if (ShouldPlay1PVisuals())
				{
					UTOwner->TargetEyeOffset.X = FiringViewKickback;
					AUTPlayerController* PC = Cast<AUTPlayerController>(UTOwner->Controller);
					if (PC != NULL)
					{
						PC->AddHUDImpulse(HUDViewKickback);
					}
					// muzzle flash
					if (MuzzleFlash.IsValidIndex(CurrentFireMode) && MuzzleFlash[CurrentFireMode] != NULL && MuzzleFlash[CurrentFireMode]->Template != NULL)
					{
						// if we detect a looping particle system, then don't reactivate it
						if (!MuzzleFlash[CurrentFireMode]->bIsActive || !IsLoopingParticleSystem(MuzzleFlash[CurrentFireMode]->Template))
						{
							MuzzleFlash[CurrentFireMode]->ActivateSystem();
						}
					}
				}
			}
		}
	}
}

void AUTWeap_Enforcer::PlayImpactEffects_Implementation(const FVector& TargetLoc, uint8 FireMode, const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	UUTWeaponStateFiringBurst* BurstFireMode = Cast<UUTWeaponStateFiringBurst>(FiringState[GetCurrentFireMode()]);

	if (GetNetMode() != NM_DedicatedServer)
	{
		const bool bOrigFireLeftSideImpact = bFireLeftSideImpact;
		Super::PlayImpactEffects_Implementation(TargetLoc, FireMode, SpawnLocation, SpawnRotation);
		ImpactCount++;

		if (bDualWeaponMode)
		{
			bFireLeftSideImpact = bOrigFireLeftSideImpact;

			if (!BurstFireMode || ((ImpactCount % BurstFireMode->BurstSize) == 0))
			{
				bFireLeftSideImpact = !bFireLeftSideImpact;
			}
		}

		if ((BurstFireMode && ImpactCount >= BurstFireMode->BurstSize * 2) || (!BurstFireMode && ImpactCount > 1))
		{
			ImpactCount = 0;
		}
	}
}

void AUTWeap_Enforcer::DualEquipFinished()
{
	Super::DualEquipFinished();
}