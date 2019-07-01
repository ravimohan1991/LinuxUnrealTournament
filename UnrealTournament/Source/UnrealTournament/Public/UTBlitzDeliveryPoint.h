// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTCTFFlagBase.h"
#include "UTBlitzDeliveryPoint.generated.h"

UCLASS(HideCategories = GameObject)
class UNREALTOURNAMENT_API AUTBlitzDeliveryPoint : public AUTCTFFlagBase
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objective)
		UParticleSystem* BlueDefenseEffect;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Objective)
		UParticleSystem* RedDefenseEffect;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Objective)
		UParticleSystemComponent* DefensePSC;

	UPROPERTY(ReplicatedUsing = OnDefenseEffectChanged, BlueprintReadOnly, Category = Objective)
		uint8 ShowDefenseEffect;

	UFUNCTION()
		void OnDefenseEffectChanged();

	virtual void SpawnDefenseEffect();
	virtual void CreateCarriedObject() override;
};

