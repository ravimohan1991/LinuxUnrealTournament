// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTWeaponState.h"
#include "UTWeaponStateFiring.h"
#include "UTWeaponStateFiring_LoopingFire.h"

UUTWeaponStateFiring_LoopingFire::UUTWeaponStateFiring_LoopingFire(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsInCooldown = false;
	ShortFireThreshold = 0;
}

void UUTWeaponStateFiring_LoopingFire::BeginState(const UUTWeaponState* PrevState)
{
	Super::BeginState(PrevState);

	CurrentShot = 0;

	if (!bIsInCooldown && (BeginFireAnim_Weapon || BeginFireAnim_Hands))
	{
		const float BeginFireTimerTime = BeginFireAnim_Weapon ? BeginFireAnim_Weapon->GetPlayLength() : BeginFireAnim_Hands->GetPlayLength();
	
		PlayBeginFireAnims();
		TransitionBeginToLoopBind();
	}
}

void UUTWeaponStateFiring_LoopingFire::TransitionBeginToLoopBind()
{
	FOnMontageEnded EndDelegate;
	EndDelegate.BindUObject(this, &UUTWeaponStateFiring_LoopingFire::TransitionBeginToLoopExecute);

	FOnMontageBlendingOutStarted BlendDelegate;
	BlendDelegate.BindUObject(this, &UUTWeaponStateFiring_LoopingFire::TransitionBeginToLoopExecute);

	UAnimInstance* AnimInstance_Weapon = GetOuterAUTWeapon()->GetMesh()->GetAnimInstance();
	if (BeginFireAnim_Weapon && AnimInstance_Weapon)
	{
		AnimInstance_Weapon->Montage_SetEndDelegate(EndDelegate, BeginFireAnim_Weapon);
		AnimInstance_Weapon->Montage_SetBlendingOutDelegate(BlendDelegate, BeginFireAnim_Weapon);
	}
	else if (BeginFireAnim_Hands)
	{
		UAnimInstance* AnimInstance_Hands = GetUTOwner()->FirstPersonMesh->GetAnimInstance();
		if (AnimInstance_Hands)
		{
			AnimInstance_Hands->Montage_SetEndDelegate(EndDelegate, BeginFireAnim_Hands);
			AnimInstance_Weapon->Montage_SetBlendingOutDelegate(BlendDelegate, BeginFireAnim_Hands);
		}
	}
}

void UUTWeaponStateFiring_LoopingFire::TransitionBeginToLoopExecute(UAnimMontage* AnimMontage, bool bWasAnimInterupted)
{
	//Don't start looping Anims if we are already in cooldown or if another anim started playing
	if (!bIsInCooldown && !bWasAnimInterupted && (GetOuterAUTWeapon()->GetCurrentState() == this))
	{
		PlayLoopingFireAnims();
	}
}

void UUTWeaponStateFiring_LoopingFire::EndState()
{
	Super::EndState();
}

void UUTWeaponStateFiring_LoopingFire::PlayBeginFireAnims()
{
	if (BeginFireAnim_Weapon)
	{
		UAnimInstance* AnimInstance_Weapon = GetOuterAUTWeapon()->GetMesh()->GetAnimInstance();
		if (AnimInstance_Weapon != NULL)
		{
			AnimInstance_Weapon->Montage_Play(BeginFireAnim_Weapon, 1.f);
		}
	}

	if (BeginFireAnim_Hands && GetUTOwner())
	{
		UAnimInstance* AnimInstance_Hands = GetUTOwner()->FirstPersonMesh->GetAnimInstance();
		if (AnimInstance_Hands)
		{
			AnimInstance_Hands->Montage_Play(BeginFireAnim_Hands, 1.f);
		}
	}
}

void UUTWeaponStateFiring_LoopingFire::PlayLoopingFireAnims()
{
	if (LoopingFireAnim_Weapon)
	{
		UAnimInstance* AnimInstance_Weapon = GetOuterAUTWeapon()->GetMesh()->GetAnimInstance();
		if (AnimInstance_Weapon != NULL)
		{
			AnimInstance_Weapon->Montage_Play(LoopingFireAnim_Weapon, 1.f);
		}
	}

	if (LoopingFireAnim_Hands && GetUTOwner())
	{
		UAnimInstance* AnimInstance_Hands = GetUTOwner()->FirstPersonMesh->GetAnimInstance();
		if (AnimInstance_Hands)
		{
			AnimInstance_Hands->Montage_Play(LoopingFireAnim_Hands, 1.f);
		}
	}
}

void UUTWeaponStateFiring_LoopingFire::PlayEndFireAnims()
{
	if (CurrentShot < ShortFireThreshold && (EndFireShort_Weapon || EndFireShort_Hands))
	{
		if (EndFireShort_Weapon)
		{
			UAnimInstance* AnimInstance_Weapon = GetOuterAUTWeapon()->GetMesh()->GetAnimInstance();
			if (AnimInstance_Weapon != NULL)
			{
				AnimInstance_Weapon->Montage_Play(EndFireShort_Weapon, 1.f);
			}
		}

		if (EndFireShort_Hands && GetUTOwner())
		{
			UAnimInstance* AnimInstance_Hands = GetUTOwner()->FirstPersonMesh->GetAnimInstance();
			if (AnimInstance_Hands)
			{
				AnimInstance_Hands->Montage_Play(EndFireShort_Hands, 1.f);
			}
		}
	}
	else
	{
		if (EndFireAnim_Weapon)
		{
			UAnimInstance* AnimInstance_Weapon = GetOuterAUTWeapon()->GetMesh()->GetAnimInstance();
			if (AnimInstance_Weapon != NULL)
			{
				AnimInstance_Weapon->Montage_Play(EndFireAnim_Weapon, 1.f);
			}
		}

		if (EndFireAnim_Hands && GetUTOwner())
		{
			UAnimInstance* AnimInstance_Hands = GetUTOwner()->FirstPersonMesh->GetAnimInstance();
			if (AnimInstance_Hands)
			{
				AnimInstance_Hands->Montage_Play(EndFireAnim_Hands, 1.f);
			}
		}
	}
}

bool UUTWeaponStateFiring_LoopingFire::CanContinueLoopingFire() const 
{
	// FIXME: HandleContinuedFiring() goes to state active automatically which is not what we want. That would be a better check here instead of this manual check.
	return (GetOuterAUTWeapon()->GetUTOwner()->GetPendingWeapon() == NULL && GetOuterAUTWeapon()->GetUTOwner()->IsPendingFire(GetOuterAUTWeapon()->GetCurrentFireMode()) && GetOuterAUTWeapon()->HasAmmo(GetOuterAUTWeapon()->GetCurrentFireMode()));
}

void UUTWeaponStateFiring_LoopingFire::RefireCheckTimer()
{
	//If we are cooling down, don't do anything until its finished
	if (!bIsInCooldown)
	{
		// query bot to consider whether to still fire, switch modes, etc
		AUTBot* B = Cast<AUTBot>(GetUTOwner()->Controller);
		if (B != NULL)
		{
			B->CheckWeaponFiring();
		}
	
		if (!CanContinueLoopingFire())
		{
			PlayEndFireAnims();
			GetOuterAUTWeapon()->GotoActiveState();
		}
		else
		{
			GetOuterAUTWeapon()->OnContinuedFiring();
			FireShot();
			CurrentShot++;
		}
	}
	else if (bAllowOtherFireModesDuringCooldown)
	{
		// check for any other fire modes pending fire, if so go to active so it can handle swapping modes
		for (uint8 i = 0; i < GetOuterAUTWeapon()->GetNumFireModes(); i++)
		{
			if ((GetOuterAUTWeapon()->GetCurrentFireMode() != i) && (GetOuterAUTWeapon()->GetUTOwner()->IsPendingFire(i)))
			{
				GetOuterAUTWeapon()->GotoActiveState();
			}
		}
	}
}

void UUTWeaponStateFiring_LoopingFire::PlayCoolDownAnims()
{
	if (CoolDownAnim_Weapon)
	{
		UAnimInstance* AnimInstance_Weapon = GetOuterAUTWeapon()->GetMesh()->GetAnimInstance();
		if (AnimInstance_Weapon != NULL)
		{
			AnimInstance_Weapon->Montage_Play(CoolDownAnim_Weapon, 1.f);
		}
	}

	if (CoolDownAnim_Hands && GetUTOwner())
	{
		UAnimInstance* AnimInstance_Hands = GetUTOwner()->FirstPersonMesh->GetAnimInstance();
		if (AnimInstance_Hands)
		{
			AnimInstance_Hands->Montage_Play(CoolDownAnim_Hands, 1.f);
		}
	}
}

void UUTWeaponStateFiring_LoopingFire::EnterCooldown()
{
	if (!bIsInCooldown)
	{
		bIsInCooldown = true;
		PlayCoolDownAnims();
	}
}

void UUTWeaponStateFiring_LoopingFire::ExitCooldown()
{
	if (bIsInCooldown)
	{
		bIsInCooldown = false;

		// Restart looping fire anims if needed
		if (CanContinueLoopingFire())
		{
			PlayLoopingFireAnims();
		}
		
		RefireCheckTimer();
	}
}