// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTDroppedPickup.h"
#include "UTPickupMessage.h"
#include "UTPickupHealth.h"
#include "UTSquadAI.h"

#include "UTDroppedArmor.generated.h"

// TODO: investigate options to reduce code duplication with UTPickupHealth
UCLASS()
class AUTDroppedArmor : public AUTDroppedPickup
{
	GENERATED_BODY()
public:
	AUTDroppedArmor(const FObjectInitializer& ObjectInitializer);
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	USoundBase* PickupSound;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TArray<UStaticMesh*> ArmorMeshes;

	// The Amount of Armor to give
	UPROPERTY(Replicated, ReplicatedUsing=OnArmorAmountReceived)
	int32 ArmorAmount;

	// The type of armor this is
	class AUTArmor* ArmorType;

	virtual void SetArmorAmount(class AUTArmor* inArmorType, int32 NewAmount);

	virtual void PostInitializeComponents() override;
	virtual void GiveTo_Implementation(APawn* Target) override;
	virtual USoundBase* GetPickupSound_Implementation() const;
	UFUNCTION()
	virtual void OnArmorAmountReceived();

	virtual bool AllowPickupBy_Implementation(APawn* TouchedBy, bool bDefaultAllowPickup) override
	{
		AUTCharacter* P = Cast<AUTCharacter>(TouchedBy);
		return Super::AllowPickupBy_Implementation(TouchedBy, bDefaultAllowPickup && P != NULL && !P->IsRagdoll());
	}


};