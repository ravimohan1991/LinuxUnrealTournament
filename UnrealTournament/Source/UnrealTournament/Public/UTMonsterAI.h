// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTBot.h"

#include "UTMonsterAI.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTMonsterAI : public AUTBot
{
	GENERATED_BODY()
public:
	AUTMonsterAI(const FObjectInitializer& OI)
		: Super(OI)
	{
		FiringRotationRateMult = 1.0f;
	}

	/** class of monster to use, set when the monster can respawn so the controller is left around */
	UPROPERTY()
	TSubclassOf<AUTCharacter> PawnClass;
	
	/** if set, enforce consistent delay time between shots, no holding down fire (difficulty determines delay length) */
	UPROPERTY(EditDefaultsOnly)
	bool bOneShotAttacks;

	/** multiplier to rotation speed when firing */
	UPROPERTY(EditDefaultsOnly)
	float FiringRotationRateMult;

	virtual void CheckWeaponFiring(bool bFromWeapon = true) override;
	virtual void UpdateControlRotation(float DeltaTime, bool bUpdatePawn = true) override;
	virtual bool NeedToTurn(const FVector& TargetLoc, bool bForcePrecise = false) override;
};