// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UnrealNetwork.h"
#include "StatNames.h"
#include "UTWeap_LightningRifle.h"
#include "UTWeapon.h"

AUTWeap_LightningRifle::AUTWeap_LightningRifle(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	FullPowerBonusDamage = 40.f;
	HeadshotDamage = 125.f;
	ChargeSpeed = 1.25f;
	ChainDamage = 30.f;
	ChainRadius = 800.f;
	bSniping = true;
	LowMeshOffset = FVector(0.f, 0.f, -3.f);
	VeryLowMeshOffset = FVector(0.f, 0.f, -12.f);
	ExtraFullPowerFireDelay = 0.3f;
	bShouldPrecacheTutorialAnnouncements = false;
}

void AUTWeap_LightningRifle::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AUTWeap_LightningRifle, bIsCharging);
	DOREPLIFETIME(AUTWeap_LightningRifle, bIsFullyPowered);
}

void AUTWeap_LightningRifle::OnStartedFiring_Implementation()
{
	Super::OnStartedFiring_Implementation();
	bZoomHeld = false;
	ChargePct = 0.f;
}

void AUTWeap_LightningRifle::OnContinuedFiring_Implementation()
{
	Super::OnContinuedFiring_Implementation();
	bZoomHeld = false;
	bIsFullyPowered = false;
	ChargePct = 0.f;
}

void AUTWeap_LightningRifle::OnStoppedFiring_Implementation()
{
	Super::OnStoppedFiring_Implementation();
	if (!bZoomHeld && (CurrentFireMode == 0) && (Role == ROLE_Authority))
	{
		bIsFullyPowered = false;
		bZoomHeld = true;
	}
}

void AUTWeap_LightningRifle::ClearForRemoval()
{
	bZoomHeld = false;
	bIsFullyPowered = false;
	ChargePct = 0.0f;
	bIsCharging = false;
	ZoomState = EZoomState::EZS_NotZoomed;
	if (UTOwner)
	{
		UTOwner->SetFlashExtra(0, CurrentFireMode);
		UTOwner->SetAmbientSound(ChargeSound, true);
	}
}

void AUTWeap_LightningRifle::Removed()
{
	ClearForRemoval();
	Super::Removed();
}

void AUTWeap_LightningRifle::ClientRemoved()
{
	ClearForRemoval();
	Super::ClientRemoved();
}

float AUTWeap_LightningRifle::GetRefireTime(uint8 FireModeNum)
{
	if (FireInterval.IsValidIndex(FireModeNum))
	{
		float Result = FireInterval[FireModeNum];
		bExtendedRefireDelay = false;
		if (bIsFullyPowered)
		{
			Result += ExtraFullPowerFireDelay;
			bExtendedRefireDelay = true;
		}
		if (UTOwner != NULL)
		{
			Result /= UTOwner->GetFireRateMultiplier();
		}
		return FMath::Max<float>(0.01f, Result);
	}
	else
	{
		UE_LOG(UT, Warning, TEXT("Invalid firing mode %i in %s::GetRefireTime()"), int32(FireModeNum), *GetName());
		return 0.1f;
	}
}

bool AUTWeap_LightningRifle::HandleContinuedFiring()
{
	if (bExtendedRefireDelay)
	{
		UpdateTiming();
	}
	return Super::HandleContinuedFiring();
}

void AUTWeap_LightningRifle::OnRep_ZoomState_Implementation()
{
	Super::OnRep_ZoomState_Implementation();
	if (ZoomState == EZoomState::EZS_NotZoomed)
	{
		bZoomHeld = false;
		ChargePct = 0.f;
		if (bIsFullyPowered && (UTOwner->GetWeapon() == this) && (Role == ROLE_Authority))
		{
			UTOwner->SetFlashExtra(0, CurrentFireMode);
		}
		bIsFullyPowered = false;
	}
	else if (ZoomState == EZoomState::EZS_ZoomingIn)
	{
		bZoomHeld = true;
	}
}

bool AUTWeap_LightningRifle::ShouldAIDelayFiring_Implementation()
{
	// AI in zoomed mode checks if it should wait for charge
	AUTBot* B = (UTOwner != nullptr) ? Cast<AUTBot>(UTOwner->Controller) : nullptr;
	if (B != nullptr && (ZoomState == EZoomState::EZS_Zoomed || ZoomState == EZoomState::EZS_ZoomingIn) && !bIsFullyPowered && B->GetEnemy() != nullptr && GetWorld()->TimeSeconds - B->LastUnderFireTime > 5.0f - B->Skill * (0.5f + FMath::FRand()))
	{
		const FBotEnemyInfo* EnemyInfo = B->GetEnemyInfo(B->GetEnemy(), true);
		// not if uncharged shot might be fatal
		return (!EnemyInfo->bHasExactHealth || InstantHitInfo.Num() == 0 || FMath::TruncToInt(100.0f * EnemyInfo->EffectiveHealthPct) > InstantHitInfo[0].Damage);
	}
	else
	{
		return false;
	}
}

void AUTWeap_LightningRifle::DrawWeaponCrosshair_Implementation(UUTHUDWidget* WeaponHudWidget, float RenderDelta)
{
	Super::DrawWeaponCrosshair_Implementation(WeaponHudWidget, RenderDelta);

	if ((ChargePct > 0.f) && (ZoomState != EZoomState::EZS_NotZoomed) && WeaponHudWidget && WeaponHudWidget->UTHUDOwner)
	{
		float Width = 150.f;
		float Height = 21.f;
		float WidthScale = 0.625f;
		float HeightScale = (ChargePct == 1.f) ? 1.f : 0.5f;
		//	WeaponHudWidget->DrawTexture(WeaponHudWidget->UTHUDOwner->HUDAtlas, 0.f, 96.f, Scale*Width, Scale*Height, 127, 671, Width, Height, 0.7f, FLinearColor::White, FVector2D(0.5f, 0.5f));
		WeaponHudWidget->DrawTexture(WeaponHudWidget->UTHUDOwner->HUDAtlas, 0.f, 40.f, WidthScale*Width*ChargePct, HeightScale*Height, 127, 641, Width, Height, 0.7f, BLUEHUDCOLOR, FVector2D(0.5f, 0.5f));
		if (ChargePct == 1.f)
		{
			WeaponHudWidget->DrawText(NSLOCTEXT("LightningRifle", "Charged", "CHARGED"), 0.f, 37.f, WeaponHudWidget->UTHUDOwner->TinyFont, 0.75f, 1.f, FLinearColor::Yellow, ETextHorzPos::Center, ETextVertPos::Center);
		}
		WeaponHudWidget->DrawTexture(WeaponHudWidget->UTHUDOwner->HUDAtlas, 0.f, 40.f, WidthScale*Width, HeightScale*Height, 127, 612, Width, Height, 1.f, FLinearColor::White, FVector2D(0.5f, 0.5f));
	}
}

bool AUTWeap_LightningRifle::CanHeadShot()
{
	return bIsFullyPowered;
}

int32 AUTWeap_LightningRifle::GetHitScanDamage()
{
	return InstantHitInfo[CurrentFireMode].Damage + (bIsFullyPowered ? FullPowerBonusDamage : 0.f);
}

void AUTWeap_LightningRifle::PlayFiringSound(uint8 EffectFiringMode)
{
	if (bIsFullyPowered && (Role == ROLE_Authority))
	{
		// will choose fire sound depending on hit or miss
		return;
	}
	else
	{
		Super::PlayFiringSound(EffectFiringMode);
	}
}

void AUTWeap_LightningRifle::SetFlashExtra(AActor* HitActor)
{
	if (UTOwner && (Role == ROLE_Authority))
	{
		if (bIsFullyPowered)
		{
			AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
			if (Cast<AUTCharacter>(HitActor) && GS && !GS->OnSameTeam(UTOwner, HitActor))
			{
				UTOwner->SetFlashExtra(3, CurrentFireMode);
				UUTGameplayStatics::UTPlaySound(GetWorld(), FullyPoweredHitEnemySound, UTOwner, SRT_All, false, FVector::ZeroVector, GetCurrentTargetPC(), NULL, true, FireSoundAmp);
			}
			else
			{
				UTOwner->SetFlashExtra(2, CurrentFireMode);
				UUTGameplayStatics::UTPlaySound(GetWorld(), FullyPoweredNoHitEnemySound, UTOwner, SRT_All, false, FVector::ZeroVector, GetCurrentTargetPC(), NULL, true, FireSoundAmp);
			}
		}
		else
		{
			UTOwner->SetFlashExtra(1, CurrentFireMode);
		}
	}
}

void AUTWeap_LightningRifle::OnRepCharging()
{
}

void AUTWeap_LightningRifle::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (UTOwner)
	{
		if (Role == ROLE_Authority)
		{
			if (UTOwner->GetWeapon() == this)
			{
				// AI chooses zoom or not from here
				AUTBot* B = Cast<AUTBot>(UTOwner->GetController());
				if (B && B->GetPawn())
				{
					bool bWantZoom = false;
					if (B->GetEnemy() != nullptr && B->GetFocusActor() == B->GetEnemy())
					{
						const float EnemyDist = (B->GetEnemyLocation(B->GetEnemy(), false) - B->GetPawn()->GetActorLocation()).Size();
						if (EnemyDist > 1000.0f && (B->IsStopped() || B->IsSniping()))
						{
							bWantZoom = true;
						}
						else if (B->Skill > 4.5f && EnemyDist > 4000.0f)
						{
							bWantZoom = B->Skill + B->Personality.MovementAbility + B->Personality.Accuracy >= 6.0f || GetWorld()->TimeSeconds - B->LastUnderFireTime > 3.0f;
						}
					}
					if (bWantZoom != (ZoomState == EZoomState::EZS_Zoomed || ZoomState == EZoomState::EZS_ZoomingIn))
					{
						// we don't really care about zoom depth for the AI so just do the minimum
						UTOwner->StartFire(1);
						if (UTOwner)
						{
							UTOwner->StopFire(1);
						}
					}
				}
				bIsCharging = (ZoomState == EZoomState::EZS_Zoomed || ZoomState == EZoomState::EZS_ZoomingIn) && !IsFiring();
			}
			else
			{
				bIsCharging = false;
			}
		}
		if (bIsCharging)
		{
			// FIXMESTEVE get charge value through timeline
			UTOwner->SetAmbientSound(ChargeSound, false);
			ChargePct = FMath::Min(1.f, ChargePct + ChargeSpeed*DeltaTime*UTOwner->GetFireRateMultiplier()); 
			UTOwner->ChangeAmbientSoundPitch(ChargeSound, ChargePct);
			bool bWasFullyPowered = bIsFullyPowered;
			bIsFullyPowered = (ChargePct >= 1.f);
			if (bIsFullyPowered && !bWasFullyPowered)
			{
				UTOwner->SetFlashExtra(4, CurrentFireMode);
				if (Cast<AUTPlayerController>(UTOwner->GetController()))
				{
					Cast<AUTPlayerController>(UTOwner->GetController())->UTClientPlaySound(FullyPoweredSound);
				}
				else
				{
					// notify bot we're ready to fire charged
					AUTBot* B = Cast<AUTBot>(UTOwner->GetController());
					if (B != nullptr)
					{
						B->CheckWeaponFiring(true);
					}
				}
			}
			else if (bWasFullyPowered && !bIsFullyPowered)
			{
				UTOwner->SetFlashExtra(0, CurrentFireMode);
			}
		}
		else
		{
			if (bIsFullyPowered && (UTOwner->GetWeapon() == this))
			{
				UTOwner->SetFlashExtra(0, CurrentFireMode);
			}
			ChargePct = 0.0f;
			bIsFullyPowered = false;
			UTOwner->SetAmbientSound(ChargeSound, true);
		}
	}
}

void AUTWeap_LightningRifle::FireShot()
{
	if (UTOwner)
	{
		UTOwner->DeactivateSpawnProtection();
	}

	AmmoCost[0] = bIsFullyPowered ? 2 : 1;
	bool bFullPowerShot = bIsFullyPowered;
	if (!FireShotOverride() && GetUTOwner() != NULL) // script event may kill user
	{
		if ((ZoomState == EZoomState::EZS_NotZoomed) && ProjClass.IsValidIndex(CurrentFireMode) && ProjClass[CurrentFireMode] != NULL)
		{
			FireProjectile();
		}
		else if (InstantHitInfo.IsValidIndex(CurrentFireMode) && InstantHitInfo[CurrentFireMode].DamageType != NULL)
		{
			if (InstantHitInfo[CurrentFireMode].ConeDotAngle > 0.0f)
			{
				FireCone();
			}
			else
			{
				FHitResult OutHit;
				FireInstantHit(true, &OutHit);

				AUTCharacter* PoweredHitCharacter = bFullPowerShot ? Cast<AUTCharacter>(OutHit.Actor.Get()) : nullptr;
				if (PoweredHitCharacter && (!PoweredHitCharacter->bTearOff || (PoweredHitCharacter->TimeOfDeath == GetWorld()->GetTimeSeconds())))
				{
					ChainLightning(OutHit);
				}
			}
		}
		//UE_LOG(UT, Warning, TEXT("FireShot"));
		PlayFiringEffects();
	}
	ConsumeAmmo(0);

	if (GetUTOwner() != NULL)
	{
		GetUTOwner()->InventoryEvent(InventoryEventName::FiredWeapon);
	}
	bIsFullyPowered = false;
	ChargePct = 0.f;
	FireZOffsetTime = 0.f;
}


void AUTWeap_LightningRifle::PlayImpactEffects_Implementation(const FVector& TargetLoc, uint8 FireMode, const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	UParticleSystem* RealFireEffect = (FireEffect.Num() > 1) ? FireEffect[0] : nullptr;
	if (UTOwner && (UTOwner->FlashExtra > 1))
	{
		if (FireEffect.Num() > 2)
		{
			FireEffect[0] = FireEffect[2];
		}
	}
	Super::PlayImpactEffects_Implementation(TargetLoc, FireMode, SpawnLocation, SpawnRotation);
	if (FireEffect.Num() > 1)
	{
		FireEffect[0] = RealFireEffect;
	}
}

void AUTWeap_LightningRifle::ChainLightning(FHitResult Hit)
{
	if (!UTOwner || !FireEffect.IsValidIndex(0) || !FireEffect[0])
	{
		return;
	}

	static FName NAME_HitLocation(TEXT("HitLocation"));
	static FName NAME_LocalHitLocation(TEXT("LocalHitLocation"));
	static FName NAME_ChainEffects = FName(TEXT("ChainEffects"));
	FVector BeamHitLocation = Hit.Location;
	FCollisionQueryParams SphereParams(NAME_ChainEffects, true, UTOwner);
	
	// query scene to see what we hit
	TArray<FOverlapResult> Overlaps;
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
	GetWorld()->OverlapMultiByChannel(Overlaps, BeamHitLocation, FQuat::Identity, COLLISION_TRACE_WEAPON, FCollisionShape::MakeSphere(ChainRadius), SphereParams);

	// collate into per-actor list of hit components
	TMap< AActor*, TArray<FHitResult> > OverlapComponentMap;
	for (int32 Idx = 0; Idx < Overlaps.Num(); ++Idx)
	{
		FOverlapResult const& Overlap = Overlaps[Idx];
		AUTCharacter* const OverlapChar = Cast<AUTCharacter>(Overlap.GetActor());
		if (OverlapChar && !OverlapChar->IsDead() && (OverlapChar != Hit.Actor.Get()) && !GS->OnSameTeam(UTOwner, OverlapChar) && Overlap.Component.IsValid())
		{
			FHitResult ChainHit(OverlapChar, Overlap.Component.Get(), OverlapChar->GetActorLocation(), FVector(0, 0, 1.f));
			if (UUTGameplayStatics::ComponentIsVisibleFrom(Overlap.Component.Get(), BeamHitLocation, UTOwner, ChainHit, nullptr))
			{
				TArray<FHitResult>& HitList = OverlapComponentMap.FindOrAdd(OverlapChar);
				HitList.Add(ChainHit);
			}
		}
	}

	for (TMap<AActor*, TArray<FHitResult> >::TIterator It(OverlapComponentMap); It; ++It)
	{
		AActor* const Victim = It.Key();
		AUTCharacter* const VictimPawn = Cast<AUTCharacter>(Victim);
		if (VictimPawn)
		{
			FUTPointDamageEvent DamageEvent(ChainDamage, Hit, FVector::ZeroVector, ChainDamageType);
			VictimPawn->TakeDamage(ChainDamage, DamageEvent, UTOwner->GetController(), this);
			FVector BeamSpawn = VictimPawn->GetActorLocation();
			UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FireEffect[0], BeamHitLocation, (BeamSpawn - BeamHitLocation).Rotation(), true);
			PSC->SetVectorParameter(NAME_HitLocation, BeamSpawn);
			PSC->SetVectorParameter(NAME_LocalHitLocation, PSC->ComponentToWorld.InverseTransformPosition(BeamSpawn));
			PSC->CustomTimeDilation = 0.2f;
		}
	}
}
