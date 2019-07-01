// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTWeap_Redeemer.h"
#include "UTSquadAI.h"
#include "UTWeaponState.h"
#include "UTWeaponStateEquipping.h"
#include "UTWeaponStateUnequipping.h"
#include "StatNames.h"
#include "UTRedeemerLaunchAnnounce.h"
#include "UTFlagRunGameState.h"
#include "UTGameMessage.h"

AUTWeap_Redeemer::AUTWeap_Redeemer(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	DefaultGroup = 10;
	BringUpTime = 0.9f;
	PutDownTime = 0.6f;
	Ammo = 1;
	MaxAmmo = 1;
	FiringViewKickback = -50.f;
	FiringViewKickbackY = 60.f;
	bMustBeHolstered = true;
	BasePickupDesireability = 1.5f;
	BaseAISelectRating = 1.5f;
	FOVOffset = FVector(1.f, 3.f, 3.f);
	bRecommendSplashDamage = true;

	KillStatsName = NAME_RedeemerKills;
	DeathStatsName = NAME_RedeemerDeaths;
	HitsStatsName = NAME_RedeemerHits;
	ShotsStatsName = NAME_RedeemerShots;

	AmmoWarningAmount = 0;
	AmmoDangerAmount = 0;
	FireSoundAmp = SAT_None;

	PickupSpawnAnnouncement = UUTRedeemerLaunchAnnounce::StaticClass();
	PickupAnnouncementIndex = 2;
	bShouldAnnounceDrops = true;

	WeaponCustomizationTag = EpicWeaponCustomizationTags::Redeemer;
	WeaponSkinCustomizationTag = EpicWeaponSkinCustomizationTags::Redeemer;

	TutorialAnnouncements.Add(TEXT("PriRedeemer"));
	TutorialAnnouncements.Add(TEXT("SecRedeemer"));
	HighlightText = NSLOCTEXT("Weapon", "RedeemerHighlightText", "Fear no Evil");
}

AUTProjectile* AUTWeap_Redeemer::FireProjectile()
{
	if (GetUTOwner() == NULL)
	{
		UE_LOG(UT, Warning, TEXT("%s::FireProjectile(): Weapon is not owned (owner died during firing sequence)"));
		return NULL;
	}
	else if (Role == ROLE_Authority)
	{
		UTOwner->bCanRally = true;
		AUTPlayerState* PS = UTOwner->Controller ? Cast<AUTPlayerState>(UTOwner->Controller->PlayerState) : NULL;
		LaunchTeam = PS && PS->Team ? PS->Team->TeamIndex : 255;
		const FVector SpawnLocation = GetFireStartLoc();
		const FRotator SpawnRotation = GetAdjustedAim(SpawnLocation);
		UTOwner->IncrementFlashCount(CurrentFireMode);
		if (PS && (ShotsStatsName != NAME_None))
		{
			PS->ModifyStatsValue(ShotsStatsName, 1);
		}

		// spawn the projectile at the muzzle
		FActorSpawnParameters Params;
		Params.Instigator = UTOwner;
		RemoteRedeemer = GetWorld()->SpawnActor<AUTRemoteRedeemer>(RemoteRedeemerClass, SpawnLocation, SpawnRotation, Params);
		if (!RemoteRedeemer)
		{
			Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			RemoteRedeemer = GetWorld()->SpawnActor<AUTRemoteRedeemer>(RemoteRedeemerClass, UTOwner->GetActorLocation(), SpawnRotation, Params);
		}
		if (RemoteRedeemer)
		{
			if (UTOwner && UTOwner->Controller)
			{
				RemoteRedeemer->SetOwner(UTOwner->Controller);
				RemoteRedeemer->ForceReplication();
				RemoteRedeemer->TryToDrive(UTOwner);
			}

            RemoteRedeemer->CollisionComp->SetGenerateOverlapEvents(true);
			LaunchedMissile = RemoteRedeemer;
			if (LaunchedMissile != nullptr)
			{
				FTimerHandle TempHandle;
				GetWorldTimerManager().SetTimer(TempHandle, this, &AUTWeap_Redeemer::AnnounceLaunch, 0.5f, false);
			}
		}
		else
		{
			UE_LOG(UT, Warning, TEXT("Could not spawn remote redeemer"));
		}
	}
	return NULL;
}

void AUTWeap_Redeemer::GivenTo(AUTCharacter* NewOwner, bool bAutoActivate)
{
	Super::GivenTo(NewOwner, bAutoActivate);
	if (NewOwner)
	{
		NewOwner->bCanRally = false;
	}
}

void AUTWeap_Redeemer::DropFrom(const FVector& StartLocation, const FVector& TossVelocity)
{
	if (UTOwner)
	{
		UTOwner->bCanRally = true;
	}
	Super::DropFrom(StartLocation, TossVelocity);
}

void AUTWeap_Redeemer::AnnounceLaunch()
{
    if (LaunchedMissile && !LaunchedMissile->IsPendingKillPending() && !LaunchedMissile->GetTearOff() && (Cast<AUTProjectile>(LaunchedMissile) ? !Cast<AUTProjectile>(LaunchedMissile)->bExploded : (RemoteRedeemer && !RemoteRedeemer->bExploded)))
	{
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
			if (PC)
			{
				int32 MessageIndex = (PC->GetTeamNum() == LaunchTeam) ? 1 : 0;
				PC->ClientReceiveLocalizedMessage(UUTRedeemerLaunchAnnounce::StaticClass(), MessageIndex);
			}
		}
	}
}

void AUTWeap_Redeemer::AddAmmo(int32 Amount)
{
	if (RemoteRedeemer && !RemoteRedeemer->IsPendingKillPending())
	{
		return;
	}
	Super::AddAmmo(Amount);
}

float AUTWeap_Redeemer::SuggestAttackStyle_Implementation()
{
	return -1.0f;
}
float AUTWeap_Redeemer::SuggestDefenseStyle_Implementation()
{
	return -1.0f;
}

void AUTWeap_Redeemer::BringUp(float OverflowTime)
{
	EquipStartTime = GetWorld()->GetTimeSeconds();
	Super::BringUp(OverflowTime);
	UUTGameplayStatics::UTPlaySound(GetWorld(), GlobalBringupSound, UTOwner, SRT_AllButOwner);
}

bool AUTWeap_Redeemer::PutDown()
{
/*	if ((Ammo > 0) && GetWorld()->GetGameState<AUTFlagRunGameState>())
	{
		if (UTOwner && Cast<AUTPlayerController>(UTOwner->GetController()))
		{
			Cast<AUTPlayerController>(UTOwner->GetController())->ClientReceiveLocalizedMessage(UUTGameMessage::StaticClass(), 99);
		}
		return false;
	}*/
	if (Super::PutDown())
	{
		UUTGameplayStatics::UTPlaySound(GetWorld(), GlobalPutDownSound, UTOwner, SRT_AllButOwner);
		return true;
	}
	return false;
}

void AUTWeap_Redeemer::DetachFromOwner_Implementation()
{
	Super::DetachFromOwner_Implementation();

	//If we have used up all our redeemer ammo and are putting it down, go ahead and destroy the redeemer 
	// as you should not be able to give it more ammo
	if (Ammo <= 0)
	{
		// wait till put down complete
		FTimerHandle TempHandle;
		GetWorldTimerManager().SetTimer(TempHandle, this, &AUTWeap_Redeemer::RemoveRedeemer, 0.02f, false);
	}
}

void AUTWeap_Redeemer::RemoveRedeemer()
{
	//If we have used up all our redeemer ammo and are putting it down, go ahead and destroy the redeemer 
	// as you should not be able to give it more ammo
	if (UTOwner)
	{
		UTOwner->RemoveInventory(this);
	}
	Destroy();
}

float AUTWeap_Redeemer::GetAISelectRating_Implementation()
{
	AUTBot* B = Cast<AUTBot>(GetUTOwner()->Controller);
	if (B == NULL)
	{
		return BaseAISelectRating;
	}
	// TODO:
	//else if (B->IsShootingObjective())
	else if (false)
	{
		return 2.0f;
	}
	else
	{
		float ExplosionRadius = 4000.0f;
		if (ProjClass.Num() > 0 && ProjClass[0] != NULL)
		{
			ExplosionRadius = ProjClass[0].GetDefaultObject()->DamageParams.OuterRadius;
		}
		// avoid switching to at suicide range unless it will kill more enemies/high priority enemy and bot thinks it can get away with the long switch time
		if (B->GetEnemy() == NULL || (UTOwner->GetWeapon() != this && !B->GetSquad()->MustKeepEnemy(B, B->GetEnemy()) && (B->GetEnemyLocation(B->GetEnemy(), false) - UTOwner->GetActorLocation()).Size() < ExplosionRadius * 1.1f))
		{
			if (B->IsEnemyVisible(B->GetEnemy()) && B->Skill + B->Personality.Tactics < 3.0f + 1.5f * FMath::FRand() && (B->Personality.Aggressiveness <= 0.0f || FMath::FRand() > B->Personality.Aggressiveness))
			{
				return 0.4f;
			}
			else
			{
				TArray<APawn*> Enemies = B->GetEnemiesNear(B->GetEnemyLocation(B->GetEnemy(), true), ExplosionRadius * 0.75f, true);
				if (Enemies.Num() > 2)
				{
					return BaseAISelectRating;
				}
				else
				{
					for (APawn* TestEnemy : Enemies)
					{
						if (B->GetSquad()->MustKeepEnemy(B, TestEnemy))
						{
							return BaseAISelectRating;
						}
					}
					return 0.4f;
				}
			}
		}
		else
		{
			return BaseAISelectRating;
		}
	}
}

bool AUTWeap_Redeemer::CanAttack_Implementation(AActor* Target, const FVector& TargetLoc, bool bDirectOnly, bool bPreferCurrentMode, uint8& BestFireMode, FVector& OptimalTargetLoc)
{
	bool bResult = Super::CanAttack_Implementation(Target, TargetLoc, bDirectOnly, bPreferCurrentMode, BestFireMode, OptimalTargetLoc);
	// TODO: support guided fire
	BestFireMode = 0;
	return bResult;
}

void AUTWeap_Redeemer::DrawWeaponCrosshair_Implementation(UUTHUDWidget* WeaponHudWidget, float RenderDelta)
{
	FText RedeemerText = NSLOCTEXT("Redeemer", "Redeemer", "Redeemer(TM) Missile Launch System");
	FText InitializingText = NSLOCTEXT("Redeemer", "Initializing", "Initializing...");
	FText OnlineText = NSLOCTEXT("Redeemer", "Online", "ON-LINE");
	FText GuidanceText = NSLOCTEXT("Redeemer", "Guidance", "Guidance Systems");
	FText FireText = NSLOCTEXT("Redeemer", "PressFire", "Press [FIRE] to launch guided missile.");
	FText DotText = NSLOCTEXT("Redeemer", "Dot", "*");

	float RenderScale = 1.f;
	float PosX = 50.f * RenderScale;
	float PosY = 0.f;
	float TimeShown = GetWorld()->GetTimeSeconds() - EquipStartTime;
	float FrameHeight = FMath::Clamp(1024.f*TimeShown - 256.f, 16.f, 112.f) * RenderScale;
	float BackAlpha = 0.3f;
	WeaponHudWidget->DrawTexture(WeaponHudWidget->UTHUDOwner->ScoreboardAtlas, PosX, PosY, 320.f*RenderScale, FrameHeight, 149, 138, 32, 32, BackAlpha, FLinearColor::Black);

	if (TimeShown > 0.5f)
	{
		FVector2D TextSize = WeaponHudWidget->DrawText(RedeemerText, PosX, PosY, WeaponHudWidget->UTHUDOwner->TinyFont, FLinearColor::Black, RenderScale, 1.0f, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Top);
		PosY += TextSize.Y*RenderScale;
		FVector2D GuidanceSize = WeaponHudWidget->DrawText(GuidanceText, PosX, PosY, WeaponHudWidget->UTHUDOwner->TinyFont, FLinearColor::Black, RenderScale, 1.0f, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Top);
		float StatusOffset = GuidanceSize.X*RenderScale + 16.f*RenderScale;
		if (CurrentState == ActiveState)
		{
			WeaponHudWidget->DrawText(OnlineText, PosX + StatusOffset, PosY, WeaponHudWidget->UTHUDOwner->TinyFont, FLinearColor::Black, RenderScale, 1.0f, FLinearColor::Yellow, ETextHorzPos::Left, ETextVertPos::Top);
			WeaponHudWidget->DrawText(FireText, PosX, PosY + GuidanceSize.Y*RenderScale, WeaponHudWidget->UTHUDOwner->TinyFont, FLinearColor::Black, RenderScale, 1.0f, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Top);
		}
		else if (CurrentState == EquippingState)
		{
			float Opacity = FMath::Max(0.f, FMath::Sin(8.f*GetWorld()->GetTimeSeconds()));
			WeaponHudWidget->DrawText(InitializingText, PosX + StatusOffset, PosY, WeaponHudWidget->UTHUDOwner->TinyFont, FLinearColor::Black, RenderScale, Opacity, FLinearColor::Yellow, ETextHorzPos::Left, ETextVertPos::Top);
		}
	}
	else
	{
		int32 NumDots = 30 * TimeShown;
		for (int32 i = 0; i < NumDots; i++)
		{
			WeaponHudWidget->DrawText(DotText, PosX, PosY - 8.f*RenderScale, WeaponHudWidget->UTHUDOwner->TinyFont, FLinearColor::Black, RenderScale, 1.f, FLinearColor::Yellow, ETextHorzPos::Left, ETextVertPos::Top);
			PosX += 8.f;
		}
	}

	//Draw the crosshair
	Super::DrawWeaponCrosshair_Implementation(WeaponHudWidget, RenderDelta);
}


