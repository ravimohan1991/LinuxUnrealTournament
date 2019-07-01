// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTProj_Rocket.h"
#include "UTInventory.h"
#include "UTTimedPowerup.h"
#include "UTGameState.h"
#include "UTBot.h"

#include "UTRocketSalvoBoost.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTRocketSalvoBoost : public AUTTimedPowerup
{
	GENERATED_BODY()
public:
	AUTRocketSalvoBoost(const FObjectInitializer& OI)
		: Super(OI)
	{
		TimeRemaining = 1.0f;
		TriggeredTime = 1.0f;
		TargetingRange = 8000.0f;
		MinRockets = 3;
		MaxTargets = 10;
		CeilingCheckHeight = 1500.0f;
	}

	/** the projectile to use */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TSubclassOf<AUTProj_Rocket> ProjClass;
	/** range to look for enemies */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float TargetingRange;
	/** minimum number of rockets per pulse */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MinRockets;
	/** maximum number of targets */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 MaxTargets;
	/** how high up to check for ceiling warning */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float CeilingCheckHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FCanvasIcon TargetIndicator;

	virtual void GivenTo(AUTCharacter* NewOwner, bool bAutoActivate) override
	{
		FTimerHandle Unused;
		GetWorldTimerManager().SetTimer(Unused, this, &AUTRocketSalvoBoost::FireSalvo, 0.4f, true);
		Super::GivenTo(NewOwner, bAutoActivate);
	}
    virtual void Removed() override
	{
		Super::Removed();
		GetWorldTimerManager().ClearAllTimersForObject(this);
	}

	// warning: may be called on CDO
	TArray<APawn*> GatherTargets(APawn* User) const
	{
		AUTGameState* GS = User->GetWorld()->GetGameState<AUTGameState>();
		const FVector MyLoc = User->GetActorLocation();
		TArray<APawn*> Targets;
		for (FConstPawnIterator It = User->GetWorld()->GetPawnIterator(); It; ++It)
		{
			if (It->IsValid() &&
				It->Get() != User &&
                !It->Get()->GetTearOff() &&
				(It->Get()->GetActorLocation() - MyLoc).Size() < TargetingRange &&
				((It->Get()->GetActorLocation() - MyLoc).GetSafeNormal() | User->GetActorRotation().Vector()) > 0.0f &&
                (GS == nullptr || !GS->OnSameTeam(It->Get(), User)))
			{
				Targets.Add(It->Get());
			}
		}
		Targets.Sort([this, MyLoc](const APawn& A, const APawn& B) { return (A.GetActorLocation() - MyLoc).Size() < (B.GetActorLocation() - MyLoc).Size(); });
		return Targets;
	}

	UFUNCTION()
	virtual void FireSalvo()
	{
		AUTCharacter* SavedUTOwner = UTOwner; // rocket launch can cause Owner to die
		TArray<APawn*> Targets = GatherTargets(SavedUTOwner);
		FActorSpawnParameters Params;
		Params.Instigator = SavedUTOwner;
		const FVector SpawnLoc = SavedUTOwner->GetActorLocation() + SavedUTOwner->GetActorRotation().Vector() * SavedUTOwner->GetSimpleCollisionRadius() + FVector(0.0f, 0.0f, SavedUTOwner->GetSimpleCollisionHalfHeight() * 0.9f);
		for (int32 i = FMath::Clamp<int32>(Targets.Num(), MinRockets, MaxTargets) - 1; i >= 0; i--)
		{
			AUTProj_Rocket* Rocket = GetWorld()->SpawnActor<AUTProj_Rocket>(ProjClass, SpawnLoc + FMath::VRand() * (SavedUTOwner->GetSimpleCollisionRadius() * 0.5f), SavedUTOwner->GetActorRotation(), Params);
			if (Rocket != nullptr && i < Targets.Num())
			{
				Rocket->TargetActor = Targets[i];
				AUTBot* EnemyAI = (Targets[i] != nullptr) ? Cast<AUTBot>(Targets[i]->GetController()) : nullptr;
				if (EnemyAI != NULL)
				{
					EnemyAI->ReceiveProjWarning(Rocket);
				}
			}
		}
	}

	virtual float GetBoostPowerRating_Implementation(AUTBot* B) const override
	{
		if ( B->GetWorld()->SweepTestByChannel( B->GetPawn()->GetActorLocation(), B->GetPawn()->GetActorLocation() + FVector(0.0f, 0.0f, CeilingCheckHeight) + B->GetPawn()->GetActorRotation().Vector() * 500.0f,
											FQuat::Identity, COLLISION_TRACE_WEAPONNOCHARACTER, FCollisionShape::MakeSphere(10.0f), FCollisionQueryParams(NAME_None, true, B->GetPawn()), WorldResponseParams ) )
		{
			// no room
			return 0.0f;
		}
		else
		{
			TArray<APawn*> PotentialTargets = B->GetEnemiesNear(B->GetPawn()->GetActorLocation(), TargetingRange, false);
			int32 Count = 0;
			for (APawn* Target : PotentialTargets)
			{
				if (B->IsEnemyVisible(Target) && ((Target->GetActorLocation() - B->GetPawn()->GetActorLocation()).GetSafeNormal() | B->GetPawn()->GetActorRotation().Vector()) > 0.0f)
				{
					Count++;
				}
			}
			return 0.25f * (Count - 2);
		}
	}

	virtual void DrawBoostHUD_Implementation(AUTHUD* Hud, UCanvas* C, APawn* P) const override
	{
		const FVector CeilingTestLoc = P->GetActorLocation() + FVector(0.0f, 0.0f, CeilingCheckHeight);
		bool bHitCeiling = P->GetWorld()->SweepTestByChannel(P->GetActorLocation(), CeilingTestLoc, FQuat::Identity, COLLISION_TRACE_WEAPONNOCHARACTER, FCollisionShape::MakeSphere(10.0f), FCollisionQueryParams(NAME_None, true, P), WorldResponseParams);
		TArray<APawn*> Targets = GatherTargets(P);
		for (int32 i = FMath::Min<int32>(Targets.Num(), MaxTargets) - 1; i >= 0; i--)
		{
			if (P->GetWorld()->TimeSeconds - Targets[i]->GetLastRenderTime() < 0.5f)
			{
				FVector Pos = C->Project(Targets[i]->GetActorLocation());
				if (Pos.X > 0.0f && Pos.Y > 0.0f && Pos.X < C->SizeX && Pos.Y < C->SizeY && Pos.Z > 0.0f)
				{
					C->DrawColor = FColor::Red;
					bool bBlocked = P->GetWorld()->SweepTestByChannel(P->GetActorLocation(), Targets[i]->GetActorLocation(), FQuat::Identity, COLLISION_TRACE_WEAPONNOCHARACTER, FCollisionShape::MakeSphere(10.0f), FCollisionQueryParams(NAME_None, true, Targets[i]), WorldResponseParams);
					// try both from ceiling loc direct to target as well as trace direct overhead and then down
					if (bBlocked && !bHitCeiling && P->GetWorld()->SweepTestByChannel(CeilingTestLoc, Targets[i]->GetActorLocation(), FQuat::Identity, COLLISION_TRACE_WEAPONNOCHARACTER, FCollisionShape::MakeSphere(10.0f), FCollisionQueryParams(NAME_None, true, Targets[i]), WorldResponseParams))
					{
						FVector OverheadLoc = Targets[i]->GetActorLocation();
						OverheadLoc.Z = CeilingTestLoc.Z;
						bBlocked = P->GetWorld()->SweepTestByChannel(CeilingTestLoc, OverheadLoc, FQuat::Identity, COLLISION_TRACE_WEAPONNOCHARACTER, FCollisionShape::MakeSphere(10.0f), FCollisionQueryParams(NAME_None, true, Targets[i]), WorldResponseParams) ||
							P->GetWorld()->SweepTestByChannel(OverheadLoc, Targets[i]->GetActorLocation(), FQuat::Identity, COLLISION_TRACE_WEAPONNOCHARACTER, FCollisionShape::MakeSphere(10.0f), FCollisionQueryParams(NAME_None, true, Targets[i]), WorldResponseParams);
					}
					if (bBlocked)
					{
						C->DrawColor = FColor(192, 192, 192, 192);
					}
					FVector2D Size(64.0f, 64.0f);
					Size *= C->ClipX / 1920.0f * FMath::Clamp<float>(1.0f - (Targets[i]->GetActorLocation() - P->GetActorLocation()).Size() / TargetingRange, 0.25f, 1.0f);
					C->DrawTile(TargetIndicator.Texture, Pos.X - Size.X * 0.5f, Pos.Y - Size.Y * 0.5f, Size.X, Size.Y, TargetIndicator.U, TargetIndicator.V, TargetIndicator.UL, TargetIndicator.VL);
				}
			}
		}
	}
};
