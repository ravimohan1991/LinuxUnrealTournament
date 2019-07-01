// temporarily slows characters in the radius, their weapons and projectiles
// one-shot effect (not a volume, does not handle re-entry)
// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTProjectile.h"
#include "UTCharacter.h"
#include "UTGameState.h"
#include "UTWeaponAttachment.h"

#include "UTSlowBurst.generated.h"

UCLASS(Blueprintable)
class AUTSlowBurst : public AActor
{
	GENERATED_BODY()
public:
	AUTSlowBurst(const FObjectInitializer& OI)
		: Super(OI)
	{
		Radius = 5000.0f;
		InitialLifeSpan = 10.0f;
		TargetTimeDilation = 0.4f;
		PrimaryActorTick.bCanEverTick = true;
		PrimaryActorTick.bStartWithTickEnabled = true;
		SetReplicates(true);

		SceneRoot = OI.CreateDefaultSubobject<USceneComponent>(this, TEXT("SceneComponent"));
		RootComponent = SceneRoot;
	}

	UPROPERTY(VisibleAnywhere)
	USceneComponent* SceneRoot;

	/** Radius of effect */
	UPROPERTY(EditDefaultsOnly)
	float Radius;
	/** value to set to targets' CustomTimeDilation */
	UPROPERTY(EditDefaultsOnly)
	float TargetTimeDilation;
	/** effect attached to affected pawns */
	UPROPERTY(EditDefaultsOnly)
	UParticleSystem* SlowedEffect;

	/** Pawns that were hit by the blast */
	UPROPERTY(BlueprintReadOnly)
	TArray<AUTCharacter*> AffectedPawns;
	UPROPERTY(BlueprintReadOnly)
	TArray<UParticleSystemComponent*> AffectedPawnPSCs;
	/** inventory items owned by AffectedPawns that are affected */
	UPROPERTY(BlueprintReadOnly)
	TArray<AUTInventory*> AffectedItems;
	/** Affected projectiles (includes newly spawned when they were created by AffectedPawns) */
	UPROPERTY(BlueprintReadOnly)
	TArray<AUTProjectile*> AffectedProjs;

	FDelegateHandle SpawnDelegateHandle;

	virtual void BeginPlay() override
	{
		Super::BeginPlay();

		if (!IsPendingKill())
		{
			AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
			for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
			{
				if (It->IsValid())
				{
					AUTCharacter* C = Cast<AUTCharacter>(It->Get());
					if (C != nullptr && C != Instigator && (GS == nullptr || !GS->OnSameTeam(C, Instigator)) && (C->GetActorLocation() - GetActorLocation()).Size() < Radius)
					{
						AffectedPawns.Add(C);
						AffectedPawnPSCs.Add(UGameplayStatics::SpawnEmitterAttached(SlowedEffect, C->GetMesh(), NAME_None));
						// we'll apply the time dilation in Tick() since we need to keep updating it for new weapon pickups and the like
					}
				}
			}
			for (TActorIterator<AUTProjectile> It(GetWorld()); It; ++It)
			{
				if ((Instigator == nullptr || GS == nullptr || !GS->OnSameTeam(*It, Instigator)) && (It->GetActorLocation() - GetActorLocation()).Size() < Radius)
				{
					It->CustomTimeDilation = TargetTimeDilation;
					AffectedProjs.Add(*It);
				}
			}
			SpawnDelegateHandle = GetWorld()->AddOnActorSpawnedHandler(FOnActorSpawned::FDelegate::CreateUObject(this, &AUTSlowBurst::OnActorSpawned));
		}
	}

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override
	{
		Super::EndPlay(EndPlayReason);

		for (AUTCharacter* C : AffectedPawns)
		{
			if (C != nullptr)
			{
				C->CustomTimeDilation = C->GetClass()->GetDefaultObject<AUTCharacter>()->CustomTimeDilation;
				if (C->GetWeaponAttachment() != nullptr)
				{
					C->GetWeaponAttachment()->CustomTimeDilation = C->GetWeaponAttachment()->GetClass()->GetDefaultObject<AActor>()->CustomTimeDilation;
				}
			}
		}
		AffectedPawns.Empty();
		for (UParticleSystemComponent* PSC : AffectedPawnPSCs)
		{
			if (PSC != nullptr)
			{
				if (!PSC->bIsActive)
				{
					PSC->DestroyComponent();
				}
				else
				{
					PSC->bAutoDestroy = true;
					PSC->DeactivateSystem();
				}
			}
		}
		AffectedPawnPSCs.Empty();
		for (AUTInventory* Item : AffectedItems)
		{
			if (Item != nullptr)
			{
				Item->CustomTimeDilation = Item->GetClass()->GetDefaultObject<AActor>()->CustomTimeDilation;
			}
		}
		AffectedItems.Empty();
		for (AUTProjectile* Proj : AffectedProjs)
		{
			if (Proj != nullptr)
			{
				Proj->CustomTimeDilation = Proj->GetClass()->GetDefaultObject<AActor>()->CustomTimeDilation;
			}
		}
		AffectedProjs.Empty();

		GetWorld()->RemoveOnActorSpawnedHandler(SpawnDelegateHandle);
	}

	UFUNCTION()
	virtual void OnActorSpawned(AActor* NewActor)
	{
		if (Cast<AUTProjectile>(NewActor) != nullptr && AffectedPawns.Contains(NewActor->Instigator))
		{
			NewActor->CustomTimeDilation = TargetTimeDilation;
			AffectedProjs.Add((AUTProjectile*)NewActor);
		}
	}

	virtual void Tick(float DeltaTime) override
	{
		Super::Tick(DeltaTime);
		
		for (AUTCharacter* C : AffectedPawns)
		{
			if (C != nullptr)
			{
				C->CustomTimeDilation = TargetTimeDilation;
				if (C->GetWeaponAttachment() != nullptr)
				{
					C->GetWeaponAttachment()->CustomTimeDilation = TargetTimeDilation;
				}
				for (TInventoryIterator<AUTWeapon> It(C); It; ++It)
				{
					It->CustomTimeDilation = TargetTimeDilation;
					AffectedItems.AddUnique(*It);
				}
			}
		}
		// check for inventory that changed hands to someone that's not slowed
		for (int32 i = AffectedItems.Num() - 1; i >= 0; i--)
		{
			if (AffectedItems[i] == nullptr)
			{
				AffectedItems.RemoveAt(i);
			}
			else if (AffectedItems[i]->GetUTOwner() == nullptr || !AffectedPawns.Contains(AffectedItems[i]->GetUTOwner()))
			{
				AffectedItems[i]->CustomTimeDilation = AffectedItems[i]->GetClass()->GetDefaultObject<AActor>()->CustomTimeDilation;
				AffectedItems.RemoveAt(i);
			}
		}
		// expire slow effects on dead characters
		for (int32 i = AffectedPawnPSCs.Num() - 1; i >= 0; i--)
		{
			if (AffectedPawnPSCs[i] == nullptr || AffectedPawnPSCs[i]->IsPendingKill())
			{
				AffectedPawnPSCs.RemoveAt(i);
			}
            else if (AffectedPawnPSCs[i]->GetOwner()->GetTearOff() || AffectedPawnPSCs[i]->GetOwner()->IsPendingKillPending())
			{
				if (AffectedPawnPSCs[i]->bIsActive)
				{
					AffectedPawnPSCs[i]->bAutoDestroy = true;
					AffectedPawnPSCs[i]->DeactivateSystem();
				}
				else
				{
					AffectedPawnPSCs[i]->DestroyComponent();
				}
				AffectedPawnPSCs.RemoveAt(i);
			}
		}
	}
};
