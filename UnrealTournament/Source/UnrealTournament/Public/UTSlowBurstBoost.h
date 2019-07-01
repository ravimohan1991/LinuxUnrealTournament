// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTInventory.h"
#include "UTSlowBurst.h"

#include "UTSlowBurstBoost.generated.h"

UCLASS()
class AUTSlowBurstBoost : public AUTInventory
{
	GENERATED_BODY()
public:
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AUTSlowBurst> SlowBurstType;

	virtual bool HandleGivenTo_Implementation(AUTCharacter* NewOwner) override
	{
		Super::HandleGivenTo_Implementation(NewOwner);

		FActorSpawnParameters SpawnParams;
		SpawnParams.Instigator = NewOwner;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		SpawnParams.Owner = NewOwner;
		NewOwner->GetWorld()->SpawnActor<AUTSlowBurst>(SlowBurstType, NewOwner->GetActorLocation(), NewOwner->GetActorRotation(), SpawnParams);

		return true;
	}
};