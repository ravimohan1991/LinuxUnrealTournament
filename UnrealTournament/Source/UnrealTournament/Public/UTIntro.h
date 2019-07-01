// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTIntro.generated.h"

UCLASS(Blueprintable, Abstract, HideCategories=(Tick,Rendering, Replication, INPUT, Actor))
class UNREALTOURNAMENT_API AUTIntro : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditDefaultsOnly)
	FString DisplayName;

	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* IntroMontage;

	UPROPERTY(EditDefaultsOnly)
	FName SameTeamStartingSection;

	UPROPERTY(EditDefaultsOnly)
	FName EnemyTeamStartingSection;

	UPROPERTY(EditDefaultsOnly)
	FName WeaponReadyStartingSection;
};