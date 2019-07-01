// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTProj_Lightning.h"
#include "UTProj_TransDisk.h"

AUTProj_Lightning::AUTProj_Lightning(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bLowPriorityLight = true;
}

bool AUTProj_Lightning::ShouldIgnoreHit_Implementation(AActor* OtherActor, UPrimitiveComponent* OtherComp)
{
	if (Super::ShouldIgnoreHit_Implementation(OtherActor, OtherComp))
	{
		return (Cast<AUTProj_TransDisk>(OtherActor) == NULL);
	}
	return false;
}


