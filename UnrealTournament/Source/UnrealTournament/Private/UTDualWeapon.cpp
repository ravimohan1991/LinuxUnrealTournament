// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"

#include "Particles/ParticleSystemComponent.h"

#include "UnrealNetwork.h"

#include "UTImpactEffect.h"

#include "UTWeaponAttachment.h"
#include "UTWeaponState.h"
#include "UTWeaponStateActive.h"
#include "UTWeaponStateInactive.h"
#include "UTWeaponStateEquipping.h"
#include "UTWeaponStateFiring.h"
#include "UTWeaponStateUnequipping.h"
#include "UTWeaponStateEquipping_DualWeapon.h"
#include "UTWeaponStateUnequipping_DualWeapon.h"

#include "UTDualWeapon.h"

AUTDualWeapon::AUTDualWeapon(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DualBringUpTime = 0.36f;
	DualPutDownTime = 0.3f;
	bDualWeaponMode = false;
	bBecomeDual = false;

	bFireLeftSide = false;
	bFireLeftSideImpact = false;

	LeftMesh = ObjectInitializer.CreateDefaultSubobject<USkeletalMeshComponent>(this, TEXT("LeftMesh"));
	LeftMesh->SetOnlyOwnerSee(true);
	LeftMesh->SetupAttachment(RootComponent);
    LeftMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;//EMeshComponentUpdateFlag::OnlyTickPoseWhenRendered;
	LeftMesh->bSelfShadowOnly = true;
	LeftMesh->bHiddenInGame = true;

	DualWeaponEquippingState = ObjectInitializer.CreateDefaultSubobject<UUTWeaponStateEquipping_DualWeapon>(this, TEXT("DualWeaponEquippingState"));
	DualWeaponUnequippingState = ObjectInitializer.CreateDefaultSubobject<UUTWeaponStateUnequipping_DualWeapon>(this, TEXT("DualWeaponUnequippingState"));
}

float AUTDualWeapon::GetPutDownTime()
{
	return bDualWeaponMode ? DualPutDownTime : PutDownTime;
}

float AUTDualWeapon::GetBringUpTime()
{
	return bDualWeaponMode ? DualBringUpTime : BringUpTime;
}

bool AUTDualWeapon::StackPickup_Implementation(AUTInventory* ContainedInv)
{
	if (!bBecomeDual)
	{
		BecomeDual();
	}
	return Super::StackPickup_Implementation(ContainedInv);
}

void AUTDualWeapon::BecomeDual()
{
	if (Role == ROLE_Authority)
	{
		if (bBecomeDual)
		{
			return;
		}

		MaxAmmo *= 2;
	}
	bBecomeDual = true;

	//For spectators this may not have been set
	if (DualWeaponEquippingState->EquipTime == 0.0f)
	{
		DualWeaponEquippingState->EquipTime = GetBringUpTime();
	}

	// pick up the second enforcer
	AttachLeftMesh();
	UpdateWeaponRenderScaleOnLeftMesh();

	// the UneqippingState needs to be updated so that both guns are lowered during weapon switch
	UnequippingState = DualWeaponUnequippingState;

	BaseAISelectRating = FMath::Max<float>(BaseAISelectRating, 0.6f);

	//Setup a timer to fire once the equip animation finishes
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, this, &AUTDualWeapon::DualEquipFinished, DualWeaponEquippingState->EquipTime);
}

void AUTDualWeapon::GotoEquippingState(float OverflowTime)
{
	GotoState(DualWeaponEquippingState);
	if (CurrentState == DualWeaponEquippingState)
	{
		DualWeaponEquippingState->StartEquip(OverflowTime);
	}
}

void AUTDualWeapon::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(AUTDualWeapon, bBecomeDual, COND_None);
}

void AUTDualWeapon::DualEquipFinished()
{
	if (!bDualWeaponMode)
	{
		bDualWeaponMode = true;
		FireInterval = FireIntervalDualWield;

		//Reset the FireRate timer
		if (Cast<UUTWeaponStateFiring>(CurrentState) != NULL)
		{
			((UUTWeaponStateFiring*)CurrentState)->UpdateTiming();
		}

		//Update the animation since the stance has changed
		//Change the weapon attachment
		AttachmentType = DualWieldAttachmentType;

		if (UTOwner != NULL && UTOwner->GetWeapon() == this)
		{
			GetUTOwner()->SetWeaponAttachmentClass(AttachmentType);
			if (ShouldPlay1PVisuals())
			{
				UpdateWeaponHand();
			}
		}

		if (Role == ROLE_Authority)
		{
			OnRep_AttachmentType();
		}
	}
}

void AUTDualWeapon::BringUp(float OverflowTime)
{
	Super::BringUp(OverflowTime);

	if (bDualWeaponMode && (Dual_BringUpHand != NULL) && UTOwner && UTOwner->FirstPersonMesh)
	{
		UAnimInstance* HandAnimInstance = UTOwner->FirstPersonMesh->GetAnimInstance();
		if (HandAnimInstance != NULL)
		{
			HandAnimInstance->Montage_Play(Dual_BringUpHand, Dual_BringUpHand->SequenceLength / DualWeaponEquippingState->EquipTime);
		}
	}
}

bool AUTDualWeapon::PutDown()
{
	const bool Result = Super::PutDown();

	if ((Result))
	{
		if (bDualWeaponMode)
		{
			if (UTOwner && UTOwner->FirstPersonMesh)
			{
				UAnimInstance* HandsAnimInstance = UTOwner->FirstPersonMesh->GetAnimInstance();
				if (HandsAnimInstance && Dual_PutDownHand)
				{
					HandsAnimInstance->Montage_Play(Dual_PutDownHand, Dual_PutDownHand->SequenceLength / DualWeaponEquippingState->EquipTime);
				}
			}

			if (LeftMesh && Dual_PutDownLeftWeapon)
			{
				UAnimInstance* LeftAnimInstance = LeftMesh->GetAnimInstance();
				if (LeftAnimInstance)
				{
					LeftAnimInstance->Montage_Play(Dual_PutDownLeftWeapon, Dual_PutDownLeftWeapon->SequenceLength / DualWeaponEquippingState->EquipTime);
				}
			}

			if (Mesh && Dual_PutDownRightWeapon)
			{
				UAnimInstance* RightAnimInstance = Mesh->GetAnimInstance();
				if (RightAnimInstance)
				{
					RightAnimInstance->Montage_Play(Dual_PutDownRightWeapon, Dual_PutDownRightWeapon->SequenceLength / DualWeaponEquippingState->EquipTime);
				}
			}
		}
	}

	return Result;
}

void AUTDualWeapon::UpdateOverlays()
{
	UpdateOverlaysShared(this, GetUTOwner(), Mesh, OverlayEffectParams, OverlayMesh);
	if (bBecomeDual)
	{
		UpdateOverlaysShared(this, GetUTOwner(), LeftMesh, OverlayEffectParams, LeftOverlayMesh);
	}
}

void AUTDualWeapon::SetSkin(UMaterialInterface* NewSkin)
{
	if (LeftMesh != NULL)
	{
		if (NewSkin != NULL)
		{
			for (int32 i = 0; i < LeftMesh->GetNumMaterials(); i++)
			{
				LeftMesh->SetMaterial(i, NewSkin);
			}
		}
		else
		{
			for (int32 i = 0; i < LeftMesh->GetNumMaterials(); i++)
			{
				LeftMesh->SetMaterial(i, GetClass()->GetDefaultObject<AUTDualWeapon>()->LeftMesh->GetMaterial(i));
			}
		}

		LeftMeshMIDs.Empty();
		for (int i = 0; i < LeftMesh->GetNumMaterials(); i++)
		{
			LeftMeshMIDs.Add(LeftMesh->CreateAndSetMaterialInstanceDynamic(i));
		}
	}

	Super::SetSkin(NewSkin);
}

void AUTDualWeapon::AttachLeftMesh()
{
	if (UTOwner == NULL)
	{
		return;
	}

	if (LeftMesh != NULL && LeftMesh->SkeletalMesh != NULL)
	{
		LeftMesh->SetHiddenInGame(false);
		LeftMesh->AttachToComponent(UTOwner->FirstPersonMesh, FAttachmentTransformRules::KeepRelativeTransform, HandsAttachSocketLeft);
		LeftMesh->bUseAttachParentBound = true;
		LeftMesh->SetRelativeScale3D(FVector(1.0f,1.0f,1.0f));
		if (Cast<APlayerController>(UTOwner->Controller) != NULL && UTOwner->IsLocallyControlled())
		{
			LeftMesh->LastRenderTime = GetWorld()->TimeSeconds;
			LeftMesh->bRecentlyRendered = true;
		}

		if (UTOwner != NULL && UTOwner->GetWeapon() == this)
		{
			if (Dual_BringUpLeftWeaponFirstAttach != NULL)
			{
				UAnimInstance* LeftWeaponAnimInstance = LeftMesh->GetAnimInstance();
				if (LeftWeaponAnimInstance != NULL)
				{
					LeftWeaponAnimInstance->Montage_Play(Dual_BringUpLeftWeaponFirstAttach, Dual_BringUpLeftWeaponFirstAttach->SequenceLength / DualWeaponEquippingState->EquipTime);
				}
			}

			if ((Dual_BringUpLeftHandFirstAttach != NULL) && UTOwner && UTOwner->FirstPersonMesh)
			{
				UAnimInstance* HandAnimInstance = UTOwner->FirstPersonMesh->GetAnimInstance();
				if (HandAnimInstance != NULL)
				{
					HandAnimInstance->Montage_Play(Dual_BringUpLeftHandFirstAttach, Dual_BringUpLeftHandFirstAttach->SequenceLength / DualWeaponEquippingState->EquipTime);
				}
			}

			if (GetNetMode() != NM_DedicatedServer)
			{
				UpdateOverlays();
			}

			if (ShouldPlay1PVisuals())
			{
                LeftMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;//EMeshComponentUpdateFlag::AlwaysTickPose; // needed for anims to be ticked even if weapon is not currently displayed, e.g. sniper zoom
				LeftMesh->LastRenderTime = GetWorld()->TimeSeconds;
				LeftMesh->bRecentlyRendered = true;
				if (LeftOverlayMesh != NULL)
				{
                    LeftOverlayMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
					LeftOverlayMesh->LastRenderTime = GetWorld()->TimeSeconds;
					LeftOverlayMesh->bRecentlyRendered = true;
				}
			}
		}
	}
}

void AUTDualWeapon::UpdateWeaponRenderScaleOnLeftMesh()
{
	static FName FNameScale = TEXT("Scale");
	for (int i = 0; i < LeftMeshMIDs.Num(); i++)
	{
		if (LeftMeshMIDs[i])
		{
			LeftMeshMIDs[i]->SetScalarParameterValue(FNameScale, WeaponRenderScale);
		}
	}
}

void AUTDualWeapon::AttachToOwner_Implementation()
{
	if (UTOwner == NULL)
	{
		return;
	}

	if (bBecomeDual && !bDualWeaponMode)
	{
		DualEquipFinished();
	}

	// attach left mesh
	if (bDualWeaponMode)
	{
		AttachLeftMesh();
		AttachmentType = DualWieldAttachmentType;
		GetUTOwner()->SetWeaponAttachmentClass(AttachmentType);
	}

	Super::AttachToOwner_Implementation();

	if (bDualWeaponMode)
	{
		UpdateWeaponRenderScaleOnLeftMesh();
	}
}

void AUTDualWeapon::UpdateWeaponHand()
{
	Super::UpdateWeaponHand();
	if (bDualWeaponMode)
	{
		LeftMesh->SetRelativeLocationAndRotation(GetClass()->GetDefaultObject<AUTDualWeapon>()->LeftMesh->RelativeLocation, GetClass()->GetDefaultObject<AUTDualWeapon>()->LeftMesh->RelativeRotation);
	}
}

void AUTDualWeapon::DetachFromOwner_Implementation()
{
	//TODO revisit this if I split the muzzle flash
	//make sure particle system really stops NOW since we're going to unregister it
	//for (int32 i = 0; i < MuzzleFlash.Num(); i++)
	//{
	//	if (MuzzleFlash[i] != NULL)
	//	{
	//		UParticleSystem* SavedTemplate = MuzzleFlash[i]->Template;
	//		MuzzleFlash[i]->DeactivateSystem();
	//		MuzzleFlash[i]->KillParticlesForced();
	//		// FIXME: KillParticlesForced() doesn't kill particles immediately for GPU particles, but the below does...
	//		MuzzleFlash[i]->SetTemplate(NULL);
	//		MuzzleFlash[i]->SetTemplate(SavedTemplate);
	//	}
	//}

	if (LeftMesh != NULL && LeftMesh->SkeletalMesh != NULL)
	{
		LeftMesh->DetachFromComponent(FDetachmentTransformRules::KeepWorldTransform);
	}

	Super::DetachFromOwner_Implementation();
}

void AUTDualWeapon::PlayWeaponAnim(UAnimMontage* WeaponAnim, UAnimMontage* HandsAnim /* = NULL */, float RateOverride /* = 0.0f */)
{
	//Ignore this function if we are in Dual Enforcer Mode as we are manually handing weapon anims
	if (!bDualWeaponMode)
	{
		Super::PlayWeaponAnim(WeaponAnim, HandsAnim, RateOverride);
	}
}

void AUTDualWeapon::PlayFiringEffects()
{
	if (UTOwner != NULL)
	{
		if (!bDualWeaponMode)
		{
			Super::PlayFiringEffects();
		}
		else
		{
			if (UTOwner != nullptr)
			{
				//UE_LOG(UT, Warning, TEXT("PlayFiringEffects at %f"), GetWorld()->GetTimeSeconds());
				uint8 EffectFiringMode = (Role == ROLE_Authority || UTOwner->Controller != NULL) ? CurrentFireMode : UTOwner->FireMode;
				PlayFiringSound(EffectFiringMode);

				// reload sound on local shooter
				if ((GetNetMode() != NM_DedicatedServer) && UTOwner && UTOwner->GetLocalViewer())
				{
					if ((Ammo <= LowAmmoThreshold) && (Ammo > 0) && (LowAmmoSound != nullptr))
					{
						AUTGameMode* GameMode = GetWorld()->GetAuthGameMode<AUTGameMode>();
						if (!GameMode || GameMode->bAmmoIsLimited)
						{
							GetWorldTimerManager().SetTimer(PlayLowAmmoSoundHandle, this, &AUTWeapon::PlayLowAmmoSound, LowAmmoSoundDelay, false);
						}
					}
				}
			}
			if (ShouldPlay1PVisuals())
			{
				UAnimInstance* HandAnimInstance = UTOwner->FirstPersonMesh ? UTOwner->FirstPersonMesh->GetAnimInstance() : nullptr;

				UTOwner->TargetEyeOffset.X = FiringViewKickback;
				AUTPlayerController* PC = Cast<AUTPlayerController>(UTOwner->Controller);
				if (PC != NULL)
				{
					PC->AddHUDImpulse(HUDViewKickback);
				}
				// left muzzle flash
				if (bFireLeftSide)
				{
					uint8 LeftHandMuzzleFlashIndex = CurrentFireMode + 2;
					if (MuzzleFlash.IsValidIndex(LeftHandMuzzleFlashIndex) && MuzzleFlash[LeftHandMuzzleFlashIndex] != NULL && MuzzleFlash[LeftHandMuzzleFlashIndex]->Template != NULL)
					{
						// if we detect a looping particle system, then don't reactivate it
						if (!MuzzleFlash[LeftHandMuzzleFlashIndex]->bIsActive || MuzzleFlash[LeftHandMuzzleFlashIndex]->bSuppressSpawning || !IsLoopingParticleSystem(MuzzleFlash[LeftHandMuzzleFlashIndex]->Template))
						{
							MuzzleFlash[LeftHandMuzzleFlashIndex]->ActivateSystem();
						}
					}
				}
				else
				{
					// right muzzle flash
					if (MuzzleFlash.IsValidIndex(CurrentFireMode) && MuzzleFlash[CurrentFireMode] != NULL && MuzzleFlash[CurrentFireMode]->Template != NULL)
					{
						// if we detect a looping particle system, then don't reactivate it
						if (!MuzzleFlash[CurrentFireMode]->bIsActive || !IsLoopingParticleSystem(MuzzleFlash[CurrentFireMode]->Template))
						{
							MuzzleFlash[CurrentFireMode]->ActivateSystem();
						}
					}
				}

				//Firing Right Gun
				if (!bFireLeftSide)
				{
					if ((HandAnimInstance) && Dual_FireAnimationRightHand.IsValidIndex(CurrentFireMode) && (Dual_FireAnimationRightHand[CurrentFireMode] != NULL))
					{
						HandAnimInstance->Montage_Play(Dual_FireAnimationRightHand[CurrentFireMode], UTOwner->GetFireRateMultiplier());
					}

					if (Mesh && Dual_FireAnimationRightWeapon.IsValidIndex(CurrentFireMode) && (Dual_FireAnimationRightWeapon[CurrentFireMode] != NULL))
					{
						UAnimInstance* RightGunAnimInstance = Mesh->GetAnimInstance();
						if (RightGunAnimInstance)
						{
							RightGunAnimInstance->Montage_Play(Dual_FireAnimationRightWeapon[CurrentFireMode], UTOwner->GetFireRateMultiplier());
						}
					}
				}
				// Firing Left Gun
				else
				{
					if ((HandAnimInstance) && Dual_FireAnimationLeftHand.IsValidIndex(CurrentFireMode) && (Dual_FireAnimationLeftHand[CurrentFireMode] != NULL))
					{
						HandAnimInstance->Montage_Play(Dual_FireAnimationLeftHand[CurrentFireMode], UTOwner->GetFireRateMultiplier());
					}

					if (Mesh && Dual_FireAnimationLeftWeapon.IsValidIndex(CurrentFireMode) && (Dual_FireAnimationLeftWeapon[CurrentFireMode] != NULL))
					{
						UAnimInstance* LeftGunAnimInstance = LeftMesh->GetAnimInstance();
						if (LeftGunAnimInstance)
						{
							LeftGunAnimInstance->Montage_Play(Dual_FireAnimationLeftWeapon[CurrentFireMode], UTOwner->GetFireRateMultiplier());
						}
					}
				}
			}
			
			//Alternate every shot
			bFireLeftSide = !bFireLeftSide;
		}
	}
}

void AUTDualWeapon::PlayImpactEffects_Implementation(const FVector& TargetLoc, uint8 FireMode, const FVector& SpawnLocation, const FRotator& SpawnRotation)
{
	if (GetNetMode() != NM_DedicatedServer)
	{
		if (bDualWeaponMode && bFireLeftSideImpact)
		{
			// fire effects
			static FName NAME_HitLocation(TEXT("HitLocation"));
			static FName NAME_LocalHitLocation(TEXT("LocalHitLocation"));

			//TODO: This is a really ugly solution, what if somebody modifies this later
			//Is the best solution really to split out a separate MuzzleFlash too??
			uint8 LeftHandMuzzleFlashIndex = CurrentFireMode + 2;
			const FVector LeftSpawnLocation = (MuzzleFlash.IsValidIndex(LeftHandMuzzleFlashIndex) && MuzzleFlash[LeftHandMuzzleFlashIndex] != NULL) ? MuzzleFlash[LeftHandMuzzleFlashIndex]->GetComponentLocation() : UTOwner->GetActorLocation() + UTOwner->GetControlRotation().RotateVector(FireOffset);
			if (FireEffect.IsValidIndex(CurrentFireMode) && FireEffect[CurrentFireMode] != NULL)
			{
				UParticleSystemComponent* PSC = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FireEffect[CurrentFireMode], LeftSpawnLocation, (MuzzleFlash.IsValidIndex(LeftHandMuzzleFlashIndex) && MuzzleFlash[LeftHandMuzzleFlashIndex] != NULL) ? MuzzleFlash[LeftHandMuzzleFlashIndex]->GetComponentRotation() : (TargetLoc - SpawnLocation).Rotation(), true);
				FVector AdjustedTargetLoc = ((TargetLoc - LeftSpawnLocation).SizeSquared() > 4000000.f)
					? LeftSpawnLocation + MaxTracerDist * (TargetLoc - LeftSpawnLocation).GetSafeNormal()
					: TargetLoc;
				PSC->SetVectorParameter(NAME_HitLocation, AdjustedTargetLoc);
                PSC->SetVectorParameter(NAME_LocalHitLocation, PSC->GetComponentToWorld().InverseTransformPosition(AdjustedTargetLoc));
			}
			// perhaps the muzzle flash also contains hit effect (constant beam, etc) so set the parameter on it instead
			else if (MuzzleFlash.IsValidIndex(LeftHandMuzzleFlashIndex) && MuzzleFlash[LeftHandMuzzleFlashIndex] != NULL)
			{
				MuzzleFlash[LeftHandMuzzleFlashIndex]->SetVectorParameter(NAME_HitLocation, TargetLoc);
                MuzzleFlash[LeftHandMuzzleFlashIndex]->SetVectorParameter(NAME_LocalHitLocation, MuzzleFlash[LeftHandMuzzleFlashIndex]->GetComponentToWorld().InverseTransformPositionNoScale(TargetLoc));
			}

			if ((TargetLoc - LastImpactEffectLocation).Size() >= ImpactEffectSkipDistance || GetWorld()->TimeSeconds - LastImpactEffectTime >= MaxImpactEffectSkipTime)
			{
				if (ImpactEffect.IsValidIndex(CurrentFireMode) && ImpactEffect[CurrentFireMode] != NULL)
				{
					FHitResult ImpactHit = GetImpactEffectHit(UTOwner, LeftSpawnLocation, TargetLoc);
					if (!CancelImpactEffect(ImpactHit))
					{
						ImpactEffect[CurrentFireMode].GetDefaultObject()->SpawnEffect(GetWorld(), FTransform(ImpactHit.Normal.Rotation(), ImpactHit.Location), ImpactHit.Component.Get(), NULL, UTOwner->Controller);
					}
				}
				LastImpactEffectLocation = TargetLoc;
				LastImpactEffectTime = GetWorld()->TimeSeconds;
			}
		}
		else
		{
			Super::PlayImpactEffects_Implementation(TargetLoc, FireMode, SpawnLocation, SpawnRotation);
		}
		
		//Alternate every shot, or every volley in burst mode. Check to see if we are at the end of a burst, and if so switch weapon sides
		if (bDualWeaponMode)
		{
			bFireLeftSideImpact = !bFireLeftSideImpact;
		}
	}
}

void AUTDualWeapon::StateChanged()
{
	if (Cast<UUTWeaponStateActive>(CurrentState))
	{
		//resync these two in case we had a put down / reload
		bFireLeftSideImpact = bFireLeftSide;
	}

	Super::StateChanged();
}
