// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTDroppedAmmoBox.h"
#include "UTInventory.h"

#include "UTAmmoBoost.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTAmmoBoost : public AUTInventory
{
	GENERATED_BODY()
public:
	AUTAmmoBoost(const FObjectInitializer& OI)
		: Super(OI)
	{
		NumBoxes = 6;
		AmmoRefillPct = 0.2f;
		TossSpeedXY = 1500.0f;
		TossSpeedZ = 750.0f;
	}

	/** number of ammo boxes to spawn */
	UPROPERTY(EditDefaultsOnly)
	int32 NumBoxes;
	/** refill percent per box */
	UPROPERTY(EditDefaultsOnly)
	float AmmoRefillPct;
	/** toss speed */
	UPROPERTY(EditDefaultsOnly)
	float TossSpeedXY;
	UPROPERTY(EditDefaultsOnly)
	float TossSpeedZ;

	virtual bool HandleGivenTo_Implementation(AUTCharacter* NewOwner) override
	{
		TSubclassOf<AUTDroppedAmmoBox> BoxType = *DroppedPickupClass;
		if (BoxType == nullptr)
		{
			BoxType = AUTDroppedAmmoBox::StaticClass();
		}
		FActorSpawnParameters Params;
		Params.Instigator = NewOwner;
		for (int32 i = 0; i < NumBoxes; i++)
		{
			const FRotator XYDir(0.0f, 360.0f / NumBoxes * i, 0.0f);
			AUTDroppedAmmoBox* Pickup = NewOwner->GetWorld()->SpawnActor<AUTDroppedAmmoBox>(BoxType, NewOwner->GetActorLocation(), XYDir, Params);
			if (Pickup != NULL)
			{
				Pickup->Movement->Velocity = XYDir.Vector() * TossSpeedXY + FVector(0.0f, 0.0f, TossSpeedZ);
				Pickup->GlobalRestorePct = AmmoRefillPct;
				Pickup->SetLifeSpan(30.0f);
			}
		}
		return true;
	}

	virtual float GetBoostPowerRating_Implementation(AUTBot* B) const override
	{
		int32 NumWeapsNeedingAmmo = 0;
		for (TInventoryIterator<AUTWeapon> It(B->GetUTChar()); It; ++It)
		{
			if (It->Ammo < FMath::Max<int32>(1, It->GetClass()->GetDefaultObject<AUTWeapon>()->Ammo / 2))
			{
				NumWeapsNeedingAmmo++;
			}
		}
		if (NumWeapsNeedingAmmo < 3)
		{
			return 0.0f;
		}
		else
		{
			float Rating = 0.3f;
			AUTPlayerState* PS = Cast<AUTPlayerState>(B->PlayerState);
			if (PS != nullptr && PS->Team != nullptr)
			{
				for (AController* C : PS->Team->GetTeamMembers())
				{
					if (C != nullptr && C != B && C->GetPawn() != nullptr && (C->GetPawn()->GetActorLocation() - B->GetPawn()->GetActorLocation()).Size() < 2000.0f)
					{
						Rating += 0.15f;
					}
				}
			}
			return Rating;
		}
	}
};