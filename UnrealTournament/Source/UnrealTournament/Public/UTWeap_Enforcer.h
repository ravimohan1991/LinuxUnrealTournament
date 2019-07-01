// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTDualWeapon.h"
#include "UTWeap_Enforcer.generated.h"

UCLASS(abstract)
class UNREALTOURNAMENT_API AUTWeap_Enforcer : public AUTDualWeapon
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
	TSubclassOf<class AUTWeaponAttachment> SingleWieldAttachmentType;

	/** How much spread increases for each shot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Enforcer)
	float SpreadIncrease;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		USoundBase* ReloadClipSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Weapon")
		float ReloadClipTime;

	FTimerHandle ReloadClipHandle;
	FTimerHandle ReloadSoundHandle;

	virtual void GotoState(class UUTWeaponState* NewState) override;
	virtual void StateChanged() override;
	virtual void ReloadClip();
	virtual void PlayReloadSound();
	virtual bool HasAnyAmmo() override;

	/** How much spread increases for each shot */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Enforcer)
	float SpreadResetInterval;

	/** Max spread  */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Enforcer)
	float MaxSpread;

	/** Last time a shot was fired (used for calculating spread). */
	UPROPERTY(BlueprintReadWrite, Category = Enforcer)
	float LastFireTime;

	/** Stopping power against players with melee weapons.  Overrides normal momentum imparted by bullets. */
	UPROPERTY(BlueprintReadWrite, Category = Enforcer)
	float StoppingPower;
		
	UPROPERTY()
	int32 FireCount;

	UPROPERTY()
	int32 ImpactCount;

	virtual void PlayFiringEffects() override;
	virtual void PlayImpactEffects_Implementation(const FVector& TargetLoc, uint8 FireMode, const FVector& SpawnLocation, const FRotator& SpawnRotation) override;

	virtual void FireInstantHit(bool bDealDamage, FHitResult* OutHit) override;
	virtual void FireShot() override;
		
	virtual	float GetImpartedMomentumMag(AActor* HitActor) override;

	virtual void FiringInfoUpdated_Implementation(uint8 InFireMode, uint8 FlashCount, FVector InFlashLocation) override;

	virtual void DualEquipFinished() override;

	/** Call to modify our spread */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Weapon")
	void ModifySpread();
};