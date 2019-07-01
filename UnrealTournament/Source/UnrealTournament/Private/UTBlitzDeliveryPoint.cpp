// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "Net/UnrealNetwork.h"
#include "UTBlitzDeliveryPoint.h"

AUTBlitzDeliveryPoint::AUTBlitzDeliveryPoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	ShowDefenseEffect = 255;
	Mesh->SetHiddenInGame(true);
}

void AUTBlitzDeliveryPoint::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AUTBlitzDeliveryPoint, ShowDefenseEffect);
}

void AUTBlitzDeliveryPoint::CreateCarriedObject()
{
	SpawnDefenseEffect();
}

void AUTBlitzDeliveryPoint::SpawnDefenseEffect()
{
	if (DefensePSC)
	{
		DefensePSC->ActivateSystem(false);
		DefensePSC->UnregisterComponent();
		DefensePSC = nullptr;
	}
	if (GetNetMode() != NM_DedicatedServer)
	{
		UParticleSystem* DesiredEffect = (TeamNum == 0) ? RedDefenseEffect : BlueDefenseEffect;
		DefensePSC = UGameplayStatics::SpawnEmitterAtLocation(this, DesiredEffect, GetActorLocation() + FVector(0.f, 0.f, 80.f), GetActorRotation());
		DefensePSC->SetTemplate(DesiredEffect);
		DefensePSC->ActivateSystem(true);
	}
	ShowDefenseEffect = TeamNum;
}

void AUTBlitzDeliveryPoint::OnDefenseEffectChanged()
{
	if (ShowDefenseEffect < 2)
	{
		SpawnDefenseEffect();
	}
}
