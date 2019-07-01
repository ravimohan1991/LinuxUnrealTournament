// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTWeapon.h"
#include "UTWeaponStateUnequipping.h"
#include "UTWeaponStateUnequipping_DualWeapon.generated.h"

/** base class of states that fire the weapon and live in the weapon's FiringState array */
UCLASS(CustomConstructor)
class UNREALTOURNAMENT_API UUTWeaponStateUnequipping_DualWeapon : public UUTWeaponStateUnequipping
{
	GENERATED_UCLASS_BODY()

	UUTWeaponStateUnequipping_DualWeapon(const FObjectInitializer& OI)
	: Super(OI)
	{}

	virtual void BeginState(const UUTWeaponState* PrevState) override;

};