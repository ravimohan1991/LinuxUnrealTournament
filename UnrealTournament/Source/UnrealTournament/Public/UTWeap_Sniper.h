// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTWeap_Sniper.generated.h"

UCLASS(Abstract)
class UNREALTOURNAMENT_API AUTWeap_Sniper : public AUTWeapon
{
	GENERATED_UCLASS_BODY()

	/** target head area size bonus when moving slowly (crouch speed or less) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Damage)
	float StoppedHeadshotScale;

	/** target head area size bonus when moving slowly (crouch speed or less) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Damage)
	float SlowHeadshotScale;

	/** target head area size bonus when have just stopped accelerating */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Damage)
	float AimedHeadshotScale;

	/** target head area size bonus when moving quickly */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Damage)
	float RunningHeadshotScale;

	/** damage for headshot (instant hit only) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InstantHitDamage)
	float HeadshotDamage;

	/** damage for headshot blocked by helmet */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InstantHitDamage)
		float BlockedHeadshotDamage;

	/** damage type for headshot (instant hit only, if NULL use standard damage type) */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = InstantHitDamage)
	TSubclassOf<UDamageType> HeadshotDamageType;

	virtual float GetHeadshotScale(AUTCharacter* HeadshotTarget) const override;

	virtual AUTProjectile* FireProjectile();
	virtual void FireInstantHit(bool bDealDamage = true, FHitResult* OutHit = NULL);
	virtual void PlayPredictedImpactEffects(FVector ImpactLoc) override;

	virtual float GetAISelectRating_Implementation() override;

	virtual void OnRep_ZoomState_Implementation() override;

	/** Return true if shots from this weapon can get headshot bonus. */
	virtual bool CanHeadShot();

	/** Return hitscan damage amount. */
	virtual int32 GetHitScanDamage();

	virtual void SetFlashExtra(AActor* HitActor) {};

	virtual float SuggestAttackStyle_Implementation() override
	{
		return -0.4;
	}
	virtual float SuggestDefenseStyle_Implementation() override
	{
		return 0.2;
	}
};