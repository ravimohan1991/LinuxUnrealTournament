// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTPathBuilderInterface.h"
#include "UTPlayerState.h"
#include "UTWeapon.h"
#include "UTSupplyChest.generated.h"

UCLASS(abstract, Blueprintable, meta = (ChildCanTick))
class UNREALTOURNAMENT_API AUTSupplyChest : public AActor
{
	GENERATED_UCLASS_BODY()

		UPROPERTY(BlueprintReadWrite)
		bool bIsActive;

	/** List of weapons to give ammo for, and to give on spawn in team game volume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=SupplyChest)
		TArray<TSubclassOf<AUTWeapon>> Weapons;

	UPROPERTY(BlueprintReadOnly)
		class AUTGameVolume* MyGameVolume;

	/** Returns true if player received health. */
	UFUNCTION(BlueprintCallable, Category=SupplyChest)
	bool GiveHealth(class AUTCharacter* Character);

	/** Returns true if player received ammo. */
	UFUNCTION(BlueprintCallable, Category = SupplyChest)
		bool GiveAmmo(class AUTCharacter* Character);

	virtual void BeginPlay() override;
};
