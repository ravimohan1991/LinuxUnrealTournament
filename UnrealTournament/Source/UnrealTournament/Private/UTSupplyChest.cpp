// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTSupplyChest.h"
#include "UnrealNetwork.h"
#include "UTCharacter.h"

AUTSupplyChest::AUTSupplyChest(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsActive = true;
}

bool AUTSupplyChest::GiveHealth(AUTCharacter* Char)
{
	if (!bIsActive || (Role != ROLE_Authority) || !Char || Char->IsDead() || Char->IsFeigningDeath())
	{
		return false;
	}
	if (Char->Health < Char->HealthMax)
	{
		Char->Health = Char->HealthMax;
		Char->OnHealthUpdated();
		return true;
	}
	return false;
}

bool AUTSupplyChest::GiveAmmo(AUTCharacter* Char)
{
	if (!bIsActive || (Role != ROLE_Authority) || (Char == nullptr) || Char->IsDead() || Char->IsFeigningDeath())
	{
		return false;
	}

	// just give weapons, should already have them 
	Char->DeactivateSpawnProtection();
	bool bResult = false;
	for (int32 i=0; i<Weapons.Num(); i++)
	{
		if (Weapons[i])
		{
			AUTWeapon* Existing = Char->FindInventoryType(Weapons[i], true);
			int32 OldAmmo = Existing ? Existing->Ammo : 0;
			if (Existing == NULL || !Existing->StackLockerPickup(nullptr))
			{
				FActorSpawnParameters Params;
				Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
				Params.Instigator = Char;
				AUTInventory* NewInventory = GetWorld()->SpawnActor<AUTInventory>(Weapons[i], GetActorLocation(), GetActorRotation(), Params);
				if (NewInventory != nullptr)
				{
					NewInventory->bFromLocker = true;
					Char->AddInventory(NewInventory, true);
				}
			}
			bResult = bResult || (Existing && (Existing->Ammo > OldAmmo));

			//Add to the stats pickup count
			const AUTInventory* Inventory = (Weapons[i] != nullptr) ? Weapons[i].GetDefaultObject() : nullptr;
			if (Inventory && Inventory->StatsNameCount != NAME_None)
			{
                AUTPlayerState* PS = Cast<AUTPlayerState>(Char->GetPlayerState());
				if (PS)
				{
					PS->ModifyStatsValue(Inventory->StatsNameCount, 1);
					if (PS->Team)
					{
						PS->Team->ModifyStatsValue(Inventory->StatsNameCount, 1);
					}

					AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
					if (GS != nullptr)
					{
						GS->ModifyStatsValue(Inventory->StatsNameCount, 1);
					}
				}
			}
		}
	}

	for (int32 i = 0; i < Char->DefaultCharacterInventory.Num(); i++)
	{
		AUTWeapon* Existing = Cast<AUTWeapon>(Char->FindInventoryType(Char->DefaultCharacterInventory[i], true));
		if (Existing)
		{
			Existing->StackLockerPickup(nullptr);
		}
	}
	AUTGameMode* GM = GetWorld()->GetAuthGameMode<AUTGameMode>();
	if (GM)
	{
		for (int32 i = 0; i < GM->DefaultInventory.Num(); i++)
		{
			AUTWeapon* Existing = Cast<AUTWeapon>(Char->FindInventoryType(GM->DefaultInventory[i], true));
			if (Existing)
			{
				Existing->StackLockerPickup(nullptr);
			}
		}
	}
	return bResult;
}

void AUTSupplyChest::BeginPlay()
{
	Super::BeginPlay();

	// associate as team locker with team volume I am in
	TArray<UBoxComponent*> BoxComponents;
	GetComponents<UBoxComponent>(BoxComponents);
	UBoxComponent* Collision = (BoxComponents.Num() > 0) ? BoxComponents[0] : nullptr;
	if (Collision != nullptr)
	{
		TArray<UPrimitiveComponent*> OverlappingComponents;
		Collision->GetOverlappingComponents(OverlappingComponents);
		MyGameVolume = nullptr;
		int32 BestPriority = -1.f;

		for (auto CompIt = OverlappingComponents.CreateIterator(); CompIt; ++CompIt)
		{
			UPrimitiveComponent* OtherComponent = *CompIt;
            if (OtherComponent && OtherComponent->GetGenerateOverlapEvents())
			{
				AUTGameVolume* V = Cast<AUTGameVolume>(OtherComponent->GetOwner());
				if (V && V->Priority > BestPriority)
				{
					if (V->IsOverlapInVolume(*Collision))
					{
						BestPriority = V->Priority;
						MyGameVolume = V;
					}
				}
			}
		}
		if (MyGameVolume && MyGameVolume->bIsTeamSafeVolume)
		{
			MyGameVolume->TeamLockers.AddUnique(this);
		}
		else
		{
			MyGameVolume = nullptr;
		}
	}
}
