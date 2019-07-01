// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTPickupToken.generated.h"

UCLASS(Blueprintable, Abstract)
class UNREALTOURNAMENT_API AUTPickupToken : public AActor
{
	GENERATED_UCLASS_BODY()
		
#if WITH_EDITOR
	virtual void CheckForErrors() override;
#endif

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Pickup)
	FName TokenUniqueID;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = Pickup)
	FString TokenDescription;

	UFUNCTION(BlueprintCallable, Category = Pickup)
	bool HasBeenPickedUpBefore();

	UFUNCTION(BlueprintCallable, Category = Pickup)
	void PickedUp();

	UFUNCTION(BlueprintCallable, Category = Pickup)
	void Revoke();

	UPROPERTY()
	bool bIsPickedUp;
};