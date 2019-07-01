// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTWeaponStateFiring_LoopingFire.h"

#include "UTWeaponStateFiringBeam.generated.h"

UCLASS()
class UNREALTOURNAMENT_API UUTWeaponStateFiringBeam : public UUTWeaponStateFiring_LoopingFire
{
	GENERATED_UCLASS_BODY()

	/** beam damage accumulator must reach this value before the damage is applied to the target */
	UPROPERTY(EditDefaultsOnly, Category = Damage)
	int32 MinDamage;

	/** current damage fraction (<= MinDamage) */
	UPROPERTY(BlueprintReadWrite, Category = Damage)
	float Accumulator;

	virtual void EndState() override;
	virtual void FireShot() override;
	virtual void EndFiringSequence(uint8 FireModeNum)
	{
		Super::EndFiringSequence(FireModeNum);
		if (FireModeNum == GetOuterAUTWeapon()->GetCurrentFireMode())
		{
			GetOuterAUTWeapon()->GotoActiveState();
		}
	}
	virtual void Tick(float DeltaTime) override;
};