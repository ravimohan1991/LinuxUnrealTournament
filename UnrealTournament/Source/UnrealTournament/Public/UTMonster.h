// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTCharacter.h"
#include "UTMonsterAI.h"
#include "UTReplicatedEmitter.h"
#include "UTDroppedHealth.h"

#include "UTMonster.generated.h"

UCLASS(Abstract, ShowCategories = (CharacterData))
class AUTMonster : public AUTCharacter
{
	GENERATED_BODY()
public:
	AUTMonster(const FObjectInitializer& OI)
		: Super(OI)
	{
		Cost = 5;
		bCanPickupItems = false;
		UTCharacterMovement->bForceTeamCollision = true;
	}
	/** display name shown on HUD/scoreboard/etc */
	UPROPERTY(EditDefaultsOnly)
	FText DisplayName;
	/** cost to spawn in the PvE game's point system */
	UPROPERTY(EditDefaultsOnly)
	int32 Cost;
	/** how many times the monster can respawn before it is removed from the game (<= 0 is infinite, for the peon train) */
	UPROPERTY(EditDefaultsOnly)
	int32 NumRespawns;
	/** AI personality settings to modulate behavior */
	UPROPERTY(EditDefaultsOnly, Meta = (DisplayName = "AI Personality"))
	FBotPersonality AIPersonality;
	/** optional health item dropped on death (always 100% chance) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TSubclassOf<AUTDroppedHealth> HealthDropType;
	/** optional item drop beyond any droppable inventory (note: must have a valid DroppedPickupClass) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TSubclassOf<AUTInventory> ExtraDropType;
	/** optional item drop as a raw DroppedPickup (i.e. that doesn't need an inventory item, like ammo boxes) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TSubclassOf<AUTDroppedPickup> ExtraRawDropType;
	/** chance to drop */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	float DropChance;
	UPROPERTY(EditDefaultsOnly)
	FCanvasIcon HUDIcon;
	/** prevent monster from picking up these types of items (only relevant when bCanPickupItems is true) */
	UPROPERTY(EditDefaultsOnly)
	TArray<TSubclassOf<AUTInventory>> DisallowedPickupTypes;

	/** enables teleport dodge instead of normal dodge mechanic */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TeleportDodgeDistance;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AUTReplicatedEmitter> TeleportDodgeEffect;
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TeleportDodgeCooldown;

private:
	bool bAddingDefaultInventory;
	bool bProcessedDrops;
public:

	virtual bool AddInventory(AUTInventory* InvToAdd, bool bAutoActivate) override
	{
		if (InvToAdd != nullptr && !bAddingDefaultInventory)
		{
			for (TSubclassOf<AUTInventory> TestType : DisallowedPickupTypes)
			{
				if (InvToAdd->GetClass()->IsChildOf(TestType))
				{
					InvToAdd->Destroy();
					return false;
				}
			}
		}
		return Super::AddInventory(InvToAdd, bAutoActivate);
	}

	virtual void AddDefaultInventory(const TArray<TSubclassOf<AUTInventory>>& DefaultInventoryToAdd) override
	{
		TGuardValue<bool> DefaultGuard(bAddingDefaultInventory, true);
		Super::AddDefaultInventory(DefaultInventoryToAdd);
	}

	virtual void ApplyCharacterData(TSubclassOf<class AUTCharacterContent> Data) override
	{}

	virtual void DiscardAllInventory() override
	{
		Super::DiscardAllInventory();

        if (!bProcessedDrops && GetTearOff() && GetNetMode() != NM_Client)
		{
			bProcessedDrops = true;
			if (ExtraDropType != nullptr && FMath::FRand() < DropChance)
			{
				AUTInventory* Inv = CreateInventory<AUTInventory>(ExtraDropType, false);
				TossInventory(Inv);
			}
			if (ExtraRawDropType != nullptr && FMath::FRand() < DropChance)
			{
				AUTDroppedPickup* NewDrop = GetWorld()->SpawnActor<AUTDroppedPickup>(ExtraRawDropType, GetActorLocation(), GetActorRotation());
				if (NewDrop != nullptr)
				{
					NewDrop->Movement->Velocity = FMath::VRand().GetSafeNormal2D() * 300.0f;
					NewDrop->Movement->Velocity.Z = 500.0f;
				}
			}
			if (HealthDropType != nullptr)
			{
				AUTDroppedHealth* NewHealth = GetWorld()->SpawnActor<AUTDroppedHealth>(HealthDropType, GetActorLocation(), GetActorRotation());
				if (NewHealth != nullptr)
				{
					NewHealth->Movement->Velocity = FMath::VRand().GetSafeNormal2D() * 300.0f;
					NewHealth->Movement->Velocity.Z = 500.0f;
				}
			}
		}
	}

	virtual bool CanDodgeInternal_Implementation() const override
	{
		bool bSavedCanJump = UTCharacterMovement->MovementState.bCanJump;
		if (TeleportDodgeDistance > 0.0f)
		{
			UTCharacterMovement->MovementState.bCanJump = true;
			UTCharacterMovement->NavAgentProps.bCanJump = true;
		}
		const bool bResult = Super::CanDodgeInternal_Implementation();
		UTCharacterMovement->MovementState.bCanJump = bSavedCanJump;
		UTCharacterMovement->NavAgentProps.bCanJump = bSavedCanJump;
		return bResult;
	}

	virtual bool Dodge(FVector DodgeDir, FVector DodgeCross) override
	{
		if (TeleportDodgeDistance <= 0.0f)
		{
			return Super::Dodge(DodgeDir, DodgeCross);
		}
		else
		{
			if (!CanDodge())
			{
				return false;
			}
			else
			{
				const FVector OldLoc = GetActorLocation();
				FHitResult Hit;
				FVector TeleportDest;
				if (GetWorld()->LineTraceSingleByChannel(Hit, OldLoc, OldLoc + DodgeDir * TeleportDodgeDistance, ECC_Pawn, FCollisionQueryParams(NAME_None, false, this)))
				{
					TeleportDest = Hit.Location - DodgeDir * GetCapsuleComponent()->GetUnscaledCapsuleRadius();
				}
				else
				{
					TeleportDest = OldLoc + DodgeDir * (TeleportDodgeDistance - GetCapsuleComponent()->GetUnscaledCapsuleRadius());
				}
				if (ACharacter::TeleportTo(TeleportDest, GetActorRotation()))
				{
					if (TeleportDodgeEffect != nullptr)
					{
						GetWorld()->SpawnActor<AUTReplicatedEmitter>(TeleportDodgeEffect, OldLoc, GetActorRotation());
					}
					UTCharacterMovement->DodgeResetTime = UTCharacterMovement->GetCurrentMovementTime() + TeleportDodgeCooldown;
					return true;
				}
				else
				{
					return false;
				}
			}
		}
	}
};
