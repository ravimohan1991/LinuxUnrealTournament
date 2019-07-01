// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTDualWeapon.h"
#include "UTWeaponStateEquipping_DualWeapon.h"
#include "UTWeaponStateUnequipping_DualWeapon.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimInstance.h"

void UUTWeaponStateEquipping_DualWeapon::StartEquip(float OverflowTime)
{
	EquipTime -= OverflowTime;
	if (EquipTime <= 0.0f)
	{
		BringUpFinished();
	}
	else
	{
		GetOuterAUTWeapon()->GetWorldTimerManager().SetTimer(BringUpFinishedHandle, this, &UUTWeaponStateEquipping_DualWeapon::BringUpFinished, EquipTime);
		AUTDualWeapon* OuterWeapon = Cast<AUTDualWeapon>(GetOuterAUTWeapon());
		GetOuterAUTWeapon()->PlayWeaponAnim(GetOuterAUTWeapon()->BringUpAnim, GetOuterAUTWeapon()->BringUpAnimHands, GetAnimLengthForScaling(GetOuterAUTWeapon()->BringUpAnim, GetOuterAUTWeapon()->BringUpAnimHands) / EquipTime);
		if ((GetOuterAUTWeapon()->GetUTOwner() != nullptr) && (GetOuterAUTWeapon()->BringUpSound != nullptr))
		{
			UUTGameplayStatics::UTPlaySound(GetWorld(), GetOuterAUTWeapon()->BringUpSound, GetOuterAUTWeapon()->GetUTOwner(), SRT_None);
		}
	}
}
