// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTWeaponStateFiring.h"

#include "UTWeaponStateFiring_LoopingFire.generated.h"

UCLASS()
class UNREALTOURNAMENT_API UUTWeaponStateFiring_LoopingFire : public UUTWeaponStateFiring
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintReadWrite, Category = LoopingFire)
	bool bIsInCooldown;

	UPROPERTY(EditDefaultsOnly, Category = LoopingFire)
	bool bAllowOtherFireModesDuringCooldown;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* BeginFireAnim_Weapon;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* BeginFireAnim_Hands;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* LoopingFireAnim_Weapon;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* LoopingFireAnim_Hands;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* EndFireAnim_Weapon;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* EndFireAnim_Hands;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* CoolDownAnim_Weapon;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* CoolDownAnim_Hands;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* EndFireShort_Weapon;

	UPROPERTY(EditDefaultsOnly, Category = Animation)
	UAnimMontage* EndFireShort_Hands;

	/** Used to determine if EndFireShort or EndFire is played when the weapon is exiting fire. Short plays if number of shots fired is < this **/
	UPROPERTY(EditDefaultsOnly, Category = Animation)
	int8 ShortFireThreshold;

protected:
	/** current shot since we started firing */
	int32 CurrentShot;
	
	virtual void PlayBeginFireAnims();
	virtual void PlayLoopingFireAnims();
	virtual void PlayEndFireAnims();
	virtual void PlayCoolDownAnims();

	UFUNCTION()
	virtual void TransitionBeginToLoopBind();

	UFUNCTION()
	virtual void TransitionBeginToLoopExecute(UAnimMontage* AnimMontage, bool bWasAnimInterupted);
public:
	virtual bool CanContinueLoopingFire() const;

	virtual void BeginState(const UUTWeaponState* PrevState) override;
	virtual void EndState() override;

	/** called after the refire delay to see what we should do next (generally, fire or go back to active state) */
	virtual void RefireCheckTimer();
	
	virtual void EnterCooldown();
	virtual void ExitCooldown();

};

