// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTWeapAttachment_LinkGun.h"

void AUTWeapAttachment_LinkGun::UpdatePulseTarget()
{
	PulseTarget = UTOwner->PulseTarget;

	// use an extra muzzle flash slot at the end for the pulse effect
	int32 PulseFlashIndex = WeaponType.GetDefaultObject()->FiringState.Num();
	if (PulseTarget && MuzzleFlash.IsValidIndex(PulseFlashIndex) && MuzzleFlash[PulseFlashIndex] != NULL)
	{
		LastBeamPulseTime = GetWorld()->TimeSeconds;
		MuzzleFlash[PulseFlashIndex]->SetTemplate(PulseSuccessEffect);
		MuzzleFlash[PulseFlashIndex]->SetActorParameter(FName(TEXT("Player")), PulseTarget);
	}
}

void AUTWeapAttachment_LinkGun::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (MuzzleFlash.IsValidIndex(1) && MuzzleFlash[1] != NULL)
	{
		if (UTOwner && (PulseTarget != UTOwner->PulseTarget))
		{
			UpdatePulseTarget();
		}
		static FName NAME_PulseScale(TEXT("PulseScale"));
		float NewScale = 1.0f + FMath::Max<float>(0.0f, 1.0f - (GetWorld()->TimeSeconds - LastBeamPulseTime) / 0.35f);
		MuzzleFlash[1]->SetVectorParameter(NAME_PulseScale, FVector(NewScale, NewScale, NewScale));
	}
}

