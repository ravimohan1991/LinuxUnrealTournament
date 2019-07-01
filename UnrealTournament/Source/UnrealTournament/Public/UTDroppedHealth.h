// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTDroppedPickup.h"
#include "UTPickupMessage.h"
#include "UTPickupHealth.h"
#include "UTSquadAI.h"

#include "UTDroppedHealth.generated.h"

// TODO: investigate options to reduce code duplication with UTPickupHealth
UCLASS()
class AUTDroppedHealth : public AUTDroppedPickup
{
	GENERATED_BODY()
public:
	AUTDroppedHealth(const FObjectInitializer& OI)
		: Super(OI)
	{
		HealAmount = 25;
		BaseDesireability = 0.4f;
		InitialLifeSpan = 5.0f;
	}

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	USoundBase* PickupSound;

	/** amount of health to restore */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pickup)
	int32 HealAmount;
	/** if true, heal amount goes to SuperHealthMax instead of HealthMax */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Pickup)
	bool bSuperHeal;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float BaseDesireability;

	/** return the upper limit this pickup can increase the character's health to */
	UFUNCTION(BlueprintCallable, Category = Pickup)
	int32 GetHealMax(AUTCharacter* P)
	{
		if (P == NULL)
		{
			return 0;
		}
		else
		{
			return bSuperHeal ? P->SuperHealthMax : P->HealthMax;
		}
	}

	virtual bool AllowPickupBy_Implementation(APawn* TouchedBy, bool bDefaultAllowPickup) override
	{
		AUTCharacter* P = Cast<AUTCharacter>(TouchedBy);
		return Super::AllowPickupBy_Implementation(TouchedBy, bDefaultAllowPickup && P != NULL && !P->IsRagdoll() && (bSuperHeal || P->Health < GetHealMax(P)));
	}
	virtual void GiveTo_Implementation(APawn* Target) override
	{
		AUTCharacter* P = Cast<AUTCharacter>(Target);
		if (P != NULL)
		{
			AUTPlayerController* UTPC = (Target != nullptr) ? Cast<AUTPlayerController>(Target->GetController()) : nullptr;
			if (UTPC != nullptr)
			{
				UTPC->ClientReceiveLocalizedMessage(UUTPickupMessage::StaticClass(), 0, NULL, NULL, AUTPickupHealth::StaticClass());
			}
			P->Health = FMath::Max<int32>(P->Health, FMath::Min<int32>(P->Health + HealAmount, GetHealMax(P)));
			P->OnHealthUpdated();
			if (P->Health >= 100)
			{
				P->HealthRemovalAssists.Empty();
			}
		}
	}

	virtual float BotDesireability_Implementation(APawn* Asker, AController* RequestOwner, float PathDistance) override
	{
		AUTCharacter* P = Cast<AUTCharacter>(Asker);
		if (P == NULL)
		{
			return 0.0f;
		}
		else
		{
			AUTBot* B = Cast<AUTBot>(RequestOwner);

			float Desire = FMath::Min<int32>(P->Health + HealAmount, GetHealMax(P)) - P->Health;

			if (P->GetWeapon() != NULL && P->GetWeapon()->BaseAISelectRating > 0.5f)
			{
				Desire *= 1.7f;
			}
			if (bSuperHeal || P->Health < 45)
			{
				Desire = FMath::Min<float>(0.025f * Desire, 2.2);
				if (bSuperHeal && B != NULL && B->Skill + B->Personality.Tactics >= 4.0f) // TODO: work off of whether bot is powerup timing, since it's a related strategy
				{
					// high skill bots keep considering powerups that they don't need if they can still pick them up
					// to deny the enemy any chance of getting them
					Desire = FMath::Max<float>(Desire, 0.001f);
				}
				return Desire;
			}
			else
			{
				if (Desire > 6.0f)
				{
					Desire = FMath::Max<float>(Desire, 25.0f);
				}
				// TODO
				//else if (UTBot(C) != None && UTBot(C).bHuntPlayer)
				//	return 0;

				return FMath::Min<float>(0.017f * Desire, 2.0);
			}
		}
	}
	virtual float DetourWeight_Implementation(APawn* Asker, float PathDistance) override
	{
		AUTCharacter* P = Cast<AUTCharacter>(Asker);
		if (P == NULL)
		{
			return 0.0f;
		}
		else
		{
			// reduce distance for low value health pickups
			// TODO: maybe increase value if multiple adjacent pickups?
			int32 ActualHeal = FMath::Min<int32>(P->Health + HealAmount, GetHealMax(P)) - P->Health;
			if (PathDistance > float(ActualHeal * 200))
			{
				return 0.0f;
			}
			else
			{
				AUTBot* B = Cast<AUTBot>(P->Controller);
				if (B != NULL && B->GetSquad() != NULL && B->GetSquad()->HasHighPriorityObjective(B) && P->Health > P->HealthMax * 0.65f)
				{
					return ActualHeal * 0.01f;
				}
				else
				{
					return ActualHeal * 0.02f;
				}
			}
		}
	}

	virtual USoundBase* GetPickupSound_Implementation() const
	{
		return PickupSound;
	}
};