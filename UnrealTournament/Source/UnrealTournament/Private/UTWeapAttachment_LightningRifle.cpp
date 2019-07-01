// Fill out your copyright notice in the Description page of Project Settings.

#include "UnrealTournament.h"
#include "UTImpactEffect.h"
#include "UTWeaponStateFiringBurst.h"
#include "UTWeapAttachment_LightningRifle.h"
#include "UTCharacter.h"


AUTWeapAttachment_LightningRifle::AUTWeapAttachment_LightningRifle(const FObjectInitializer& OI)
	: Super(OI)
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	ChainRadius = 800.f;
}

void AUTWeapAttachment_LightningRifle::BeginPlay()
{
	Super::BeginPlay();

	TInlineComponentArray<UParticleSystemComponent*> PSComponents(this);
	for (UParticleSystemComponent* PSComponent : PSComponents)
	{
		if (PSComponent->GetName() == TEXT("PoweredGlow"))
		{
			PoweredGlowComponent = PSComponent;
			break;
		}
	}
}

void AUTWeapAttachment_LightningRifle::Tick(float DeltaSeconds)
{
	if (UTOwner && PoweredGlowComponent)
	{
		PoweredGlowComponent->SetActive(UTOwner->FlashExtra == 4);
	}
	Super::Tick(DeltaSeconds);
}

void AUTWeapAttachment_LightningRifle::PlayFiringEffects()
{
	uint8 RealFireMode = UTOwner->FireMode;
	RealFireEffect = (FireEffect.Num() > 1) ? FireEffect[1] : nullptr;
	if ((UTOwner->FlashExtra != 0))
	{
		UTOwner->FireMode = 1;
		if ((UTOwner->FlashExtra > 1) && (FireEffect.Num() > 2))
		{
			FireEffect[1] = (UTOwner->FlashExtra == 4) ? nullptr : FireEffect[2];
		}
	}
	Super::PlayFiringEffects();
	if (FireEffect.Num() > 1)
	{
		FireEffect[1] = RealFireEffect;
	}
	UTOwner->FireMode = RealFireMode;
}

void AUTWeapAttachment_LightningRifle::ModifyFireEffect_Implementation(class UParticleSystemComponent* Effect)
{
	if (UTOwner && (UTOwner->FlashExtra != 0))
	{
		if (UTOwner->FlashExtra == 1)
		{
			CustomTimeDilation = 0.5f;
		}
		else
		{
			CustomTimeDilation = 0.25f;
			if (UTOwner->FlashExtra == 3)
			{
				ChainEffects();
			}
		}
	}
}

void AUTWeapAttachment_LightningRifle::ChainEffects()
{
	if (!UTOwner || !RealFireEffect)
	{
		return;
	}

	static FName NAME_HitLocation(TEXT("HitLocation"));
	static FName NAME_LocalHitLocation(TEXT("LocalHitLocation"));
	static FName NAME_ChainEffects = FName(TEXT("ChainEffects"));
	FVector BeamHitLocation = UTOwner->FlashLocation.Position;
	FCollisionQueryParams SphereParams(NAME_ChainEffects, true, UTOwner);

	// query scene to see what we hit
	TArray<FOverlapResult> Overlaps;
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
	if (GS)
	{
		GetWorld()->OverlapMultiByChannel(Overlaps, BeamHitLocation, FQuat::Identity, COLLISION_TRACE_WEAPON, FCollisionShape::MakeSphere(ChainRadius), SphereParams);

		// collate into per-actor list of hit components
		TMap< AActor*, TArray<FHitResult> > OverlapComponentMap;
		for (int32 Idx = 0; Idx < Overlaps.Num(); ++Idx)
		{
			FOverlapResult const& Overlap = Overlaps[Idx];
			AUTCharacter* const OverlapChar = Cast<AUTCharacter>(Overlap.GetActor());

			if (OverlapChar && !OverlapChar->IsDead() && !GS->OnSameTeam(UTOwner, OverlapChar) && Overlap.Component.IsValid())
			{
				FHitResult Hit(OverlapChar, Overlap.Component.Get(), OverlapChar->GetActorLocation(), FVector(0, 0, 1.f));
				if (UUTGameplayStatics::ComponentIsVisibleFrom(Overlap.Component.Get(), BeamHitLocation, UTOwner, Hit, nullptr))
				{
					TArray<FHitResult>& HitList = OverlapComponentMap.FindOrAdd(OverlapChar);
					HitList.Add(Hit);
				}
			}
		}

		for (TMap<AActor*, TArray<FHitResult> >::TIterator It(OverlapComponentMap); It; ++It)
		{
			AActor* const Victim = It.Key();
			AUTCharacter* const VictimPawn = Cast<AUTCharacter>(Victim);
			FVector BeamSpawn = VictimPawn->GetActorLocation();
			UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), RealFireEffect, BeamHitLocation, (BeamSpawn - BeamHitLocation).Rotation(), true);
			PSC->SetVectorParameter(NAME_HitLocation, BeamSpawn);
            PSC->SetVectorParameter(NAME_LocalHitLocation, PSC->GetComponentToWorld().InverseTransformPosition(BeamSpawn));
			PSC->CustomTimeDilation = 0.2f;
		}
	}
}





