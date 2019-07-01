// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTProj_Rocket.h"
#include "UnrealNetwork.h"
#include "UTRewardMessage.h"
#include "StatNames.h"
#include "UTWeap_RocketLauncher.h"

AUTProj_Rocket::AUTProj_Rocket(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	DamageParams.BaseDamage = 100;
	DamageParams.OuterRadius = 360.f;  
	DamageParams.InnerRadius = 15.f; 
	DamageParams.MinimumDamage = 20.f;
	Momentum = 140000.0f;
	InitialLifeSpan = 10.f;
	ProjectileMovement->InitialSpeed = 2700.f;
	ProjectileMovement->MaxSpeed = 2700.f;
	ProjectileMovement->ProjectileGravityScale = 0.f;

	PrimaryActorTick.bCanEverTick = true;
	AdjustmentSpeed = 5000.0f;
	bLeadTarget = true;
	bRocketTeamSet = false;
	MaxLeadDistance = 2000.f;
	MinSeekDistance = 100.f;
	MaxTargetLockIndicatorDistance = 5000.f;
}

void AUTProj_Rocket::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (TargetActor != NULL)
	{
		FVector WantedDir = TargetActor->GetActorLocation() - GetActorLocation();
		float Dist = WantedDir.Size();
		if (Dist < FMath::Max(1.f, MinSeekDistance))
		{
			TargetActor = nullptr;
		}
		else
		{
			if (ProjectileMovement->Velocity.IsZero())
			{
				// check if should stop because of intermission
				AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
				if (GS && GS->bNeedToClearIntermission)
				{
					return;
				}
				
			}
			if (bLeadTarget && (Dist < MaxLeadDistance))
			{
				WantedDir += TargetActor->GetVelocity() * Dist / ProjectileMovement->MaxSpeed;
			}
			WantedDir = WantedDir.GetSafeNormal();
			ProjectileMovement->Velocity += WantedDir * AdjustmentSpeed * DeltaTime;
			ProjectileMovement->Velocity = ProjectileMovement->Velocity.GetSafeNormal() * ProjectileMovement->MaxSpeed;

			//If the rocket has passed the target stop following
			if (FVector::DotProduct(WantedDir, ProjectileMovement->Velocity) < 0.0f)
			{
				TargetActor = NULL;
			}
			else if (!bRocketTeamSet && Instigator)
			{
				OnRep_Instigator();
			}
		}
	}
}

void AUTProj_Rocket::OnRep_TargetActor()
{
	AUTCharacter* UTChar = Cast<AUTCharacter>(Instigator);
	AUTWeap_RocketLauncher* RL = UTChar ? Cast<AUTWeap_RocketLauncher>(UTChar->GetWeapon()) : nullptr;
	if (RL && UTChar->IsLocallyViewed())
	{
		RL->TrackingRockets.AddUnique(this);
	}
	else if (TargetActor)
	{
		APlayerController* PC = GEngine->GetFirstLocalPlayerController(GetWorld());
		if (PC && PC->MyHUD && (PC->GetViewTarget() == TargetActor))
		{
			PC->MyHUD->AddPostRenderedActor(this);
		}
	}
}

void AUTProj_Rocket::Destroyed()
{
	Super::Destroyed();

	if (GetWorld()->GetNetMode() != NM_DedicatedServer && GEngine->GetWorldContextFromWorld(GetWorld()) != NULL) // might not be able to get world context when exiting PIE
	{
		APlayerController* PC = GEngine->GetFirstLocalPlayerController(GetWorld());
		if (PC && PC->MyHUD)
		{
			PC->MyHUD->RemovePostRenderedActor(this);
		}
	}
}

void AUTProj_Rocket::PostRenderFor(APlayerController* PC, UCanvas* Canvas, FVector CameraPosition, FVector CameraDir)
{
	AUTPlayerController* UTPC = Cast<AUTPlayerController>(PC);
	if (bExploded || IsPendingKillPending() || !UTPC || !UTPC->MyUTHUD || !TargetActor || (PC->GetViewTarget() != TargetActor) || !LockCrosshairTexture || !LockCrosshairTexture->Resource)
	{
		if (PC->MyHUD)
		{
			PC->MyHUD->RemovePostRenderedActor(this);
		}
		return;
	}
	if ((GetWorld()->TimeSeconds - GetLastRenderTime() < 0.1f) && !UTPC->MyUTHUD->bShowScores &&
		FVector::DotProduct(CameraDir, (GetActorLocation() - CameraPosition)) > 0.0f && (UTPC->MyUTHUD == nullptr || !UTPC->MyUTHUD->bShowScores))
	{
		float Dist = (GetActorLocation() - TargetActor->GetActorLocation()).Size();
		if (Dist > MaxTargetLockIndicatorDistance)
		{
			return;
		}
		FVector ScreenPosition = Canvas->Project(GetActorLocation());
		float CrosshairRot = GetWorld()->TimeSeconds * 90.0f;
		float W = LockCrosshairTexture->GetSurfaceWidth();
		float H = LockCrosshairTexture->GetSurfaceHeight();
		FLinearColor DrawColor = FLinearColor::Red;
		DrawColor.A = 0.8f;
		float RenderScale = 2.f * Canvas->ClipX / 1920.f;
		float UL = W / LockCrosshairTexture->Resource->GetSizeX();
		float VL = H / LockCrosshairTexture->Resource->GetSizeY();
		FVector2D RenderPos = FVector2D(ScreenPosition.X - (W*RenderScale * 0.5f), ScreenPosition.Y - (H *RenderScale * 0.5f));
		FCanvasTileItem ImageItem(RenderPos, LockCrosshairTexture->Resource, FVector2D(W*RenderScale, H*RenderScale), FVector2D(0.f, 0.f), FVector2D(UL, VL), DrawColor);
		ImageItem.Rotation = FRotator(0, 2.5f*CrosshairRot, 0);
		ImageItem.PivotPoint = FVector2D(0.5f, 0.5f);
		ImageItem.BlendMode = ESimpleElementBlendMode::SE_BLEND_Translucent;
		Canvas->DrawItem(ImageItem);
	}
}

void AUTProj_Rocket::OnRep_Instigator()
{
	Super::OnRep_Instigator();
	AUTCharacter* UTChar = Cast<AUTCharacter>(Instigator);
	if (UTChar && (GetNetMode() != NM_DedicatedServer))
	{
		bRocketTeamSet = true;
		TArray<UStaticMeshComponent*> MeshComponents;
		GetComponents<UStaticMeshComponent>(MeshComponents);
		static FName NAME_GunGlowsColor(TEXT("Gun_Glows_Color"));
		UStaticMeshComponent* GlowMesh = (MeshComponents.Num() > 0) ? MeshComponents[0] : nullptr;
		MeshMI = GlowMesh ? GlowMesh->CreateAndSetMaterialInstanceDynamic(1) : nullptr;
		if (MeshMI != nullptr)
		{
			FLinearColor NewColor = (UTChar->GetTeamColor() == FLinearColor::White) ? FLinearColor::Red : UTChar->GetTeamColor();
			NewColor.R *= 0.3f;
			NewColor.G *= 0.3f;
			NewColor.B *= 0.3f;
			MeshMI->SetVectorParameterValue(NAME_GunGlowsColor, NewColor);
		}
		if (TargetActor)
		{
			OnRep_TargetActor();
		}
	}
}

void AUTProj_Rocket::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AUTProj_Rocket, TargetActor);
}

void AUTProj_Rocket::DamageImpactedActor_Implementation(AActor* OtherActor, UPrimitiveComponent* OtherComp, const FVector& HitLocation, const FVector& HitNormal)
{
	AUTCharacter* HitCharacter = Cast<AUTCharacter>(OtherActor);
	bPendingSpecialReward = (HitCharacter && AirRocketRewardClass && (HitCharacter->Health > 0) && HitCharacter->GetCharacterMovement() != NULL && (HitCharacter->GetCharacterMovement()->MovementMode == MOVE_Falling) && (GetWorld()->GetTimeSeconds() - HitCharacter->FallingStartTime > 0.3f));

	Super::DamageImpactedActor_Implementation(OtherActor, OtherComp, HitLocation, HitNormal);
	if (bPendingSpecialReward && HitCharacter && (HitCharacter->Health <= 0))
	{
		// Air Rocket reward
		AUTPlayerController* PC = Cast<AUTPlayerController>(InstigatorController);
		if (PC && PC->UTPlayerState)
		{
			int32 AirRoxCount = 0;
			PC->UTPlayerState->ModifyStatsValue(NAME_AirRox, 1);
			PC->UTPlayerState->AddCoolFactorMinorEvent();
			AirRoxCount = PC->UTPlayerState->GetStatsValue(NAME_AirRox);
			PC->SendPersonalMessage(AirRocketRewardClass, AirRoxCount, PC->UTPlayerState, HitCharacter->PlayerState);
		}
	}
	bPendingSpecialReward = false;
}
