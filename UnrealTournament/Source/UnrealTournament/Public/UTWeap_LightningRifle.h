// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTWeap_Sniper.h"
#include "UTWeap_LightningRifle.generated.h"

UCLASS(abstract)
class UNREALTOURNAMENT_API AUTWeap_LightningRifle : public AUTWeap_Sniper
{
	GENERATED_UCLASS_BODY()

		UPROPERTY(BlueprintReadWrite, Category = LightningRifle)
		float ChargePct;

	virtual void DrawWeaponCrosshair_Implementation(UUTHUDWidget* WeaponHudWidget, float RenderDelta) override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = LightningRifle)
		float FullPowerHeadshotDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = LightningRifle)
		float FullPowerBonusDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = LightningRifle)
		float ChainDamage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = LightningRifle)
		TSubclassOf<UUTDamageType> ChainDamageType;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = LinkGun)
		float ChainRadius;

	/** How fast charge increases to value of 1 (fully charged).  Scaling for time. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = LightningRifle)
		float ChargeSpeed;

	/** Extra refire delay after fire fully powered shot. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = LightningRifle)
		float ExtraFullPowerFireDelay;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = LightningRifle)
		USoundBase* FullyPoweredHitEnemySound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = LightningRifle)
		USoundBase* FullyPoweredNoHitEnemySound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = LightningRifle)
		USoundBase* ChargeSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = LightningRifle)
		USoundBase* FullyPoweredSound;

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing=OnRepCharging, Category = LightningRifle)
		bool bIsCharging;

	UPROPERTY(BlueprintReadWrite, Replicated, Category = LightningRifle)
		bool bIsFullyPowered;

	/** True when zoom button is pressed. */
	UPROPERTY(BlueprintReadWrite, Replicated, Category = LightningRifle)
		bool bZoomHeld;

	UPROPERTY()
		bool bExtendedRefireDelay;

	UFUNCTION()
		void OnRepCharging();

	virtual void ClearForRemoval();

	virtual void ChainLightning(FHitResult Hit);

	virtual bool CanHeadShot() override;
	virtual int32 GetHitScanDamage() override;
	virtual void SetFlashExtra(AActor* HitActor) override;
	virtual bool ShouldAIDelayFiring_Implementation() override;
	virtual void Tick(float DeltaTime) override;
	virtual void FireShot() override;
	virtual void OnStartedFiring_Implementation() override;
	virtual void OnContinuedFiring_Implementation() override;
	virtual void OnStoppedFiring_Implementation() override;
	virtual void OnRep_ZoomState_Implementation() override;
	virtual void Removed() override;
	virtual void ClientRemoved() override;
	virtual void PlayImpactEffects_Implementation(const FVector& TargetLoc, uint8 FireMode, const FVector& SpawnLocation, const FRotator& SpawnRotation) override;
	virtual void PlayFiringSound(uint8 EffectFiringMode) override;
	virtual float GetRefireTime(uint8 FireModeNum) override;
	virtual bool HandleContinuedFiring() override;
};
