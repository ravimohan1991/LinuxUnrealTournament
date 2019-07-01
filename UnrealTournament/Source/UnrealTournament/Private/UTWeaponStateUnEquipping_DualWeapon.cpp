// Fill out your copyright notice in the Description page of Project Settings.

#include "UnrealTournament.h"
#include "UTDualWeapon.h"
#include "UTWeaponStateEquipping_DualWeapon.h"
#include "UTWeaponStateUnequipping_DualWeapon.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"

void UUTWeaponStateUnequipping_DualWeapon::BeginState(const UUTWeaponState* PrevState)
{
	const UUTWeaponStateEquipping* PrevEquip = Cast<UUTWeaponStateEquipping>(PrevState);

	// if was previously equipping, pay same amount of time to take back down
	UnequipTime = (PrevEquip != NULL) ? FMath::Min(PrevEquip->PartialEquipTime, GetOuterAUTWeapon()->GetPutDownTime()) : GetOuterAUTWeapon()->GetPutDownTime();
	UnequipTimeElapsed = 0.0f;
	if (UnequipTime <= 0.0f)
	{
		PutDownFinished();
	}
	else
	{
		AUTDualWeapon* OuterWeapon = Cast<AUTDualWeapon>(GetOuterAUTWeapon());
		GetOuterAUTWeapon()->GetWorldTimerManager().SetTimer(PutDownFinishedHandle, this, &UUTWeaponStateUnequipping_DualWeapon::PutDownFinished, UnequipTime);
		if (OuterWeapon->PutDownAnim != NULL)
		{
			UAnimInstance* AnimInstance = OuterWeapon->Mesh->GetAnimInstance();
			if (AnimInstance != NULL)
			{
				AnimInstance->Montage_Play(OuterWeapon->PutDownAnim, OuterWeapon->PutDownAnim->SequenceLength / UnequipTime);
			}

			AnimInstance = OuterWeapon->LeftMesh->GetAnimInstance();
			if (AnimInstance != NULL && OuterWeapon->bDualWeaponMode)
			{
				AnimInstance->Montage_Play(OuterWeapon->PutDownAnim, OuterWeapon->PutDownAnim->SequenceLength / UnequipTime);
			}
		}
	}
}