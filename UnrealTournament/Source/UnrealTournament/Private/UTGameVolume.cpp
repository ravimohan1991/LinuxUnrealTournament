// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTGameVolume.h"
#include "UTDmgType_Suicide.h"
#include "UTGameState.h"
#include "UTSupplyChest.h"
#include "UTPlayerState.h"
#include "UTTeleporter.h"
#include "UTFlagRunGameState.h"
#include "UTCharacterVoice.h"
#include "UTRallyPoint.h"

AUTGameVolume::AUTGameVolume(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	VolumeName = FText::GetEmpty();
	TeamIndex = 255;
	bShowOnMinimap = true;
	bIsNoRallyZone = false;
	bIsTeamSafeVolume = false;
	bIsTeleportZone = false;
	RouteID = -1;
	bReportDefenseStatus = false;
	bHasBeenEntered = false;
	bHasFCEntry = false;
	MinEnemyInBaseInterval = 7.f;

	static ConstructorHelpers::FObjectFinder<USoundBase> AlarmSoundFinder(TEXT("SoundCue'/Game/RestrictedAssets/Audio/Gameplay/A_FlagRunBaseAlarm.A_FlagRunBaseAlarm'"));
	AlarmSound = AlarmSoundFinder.Object;

	GetBrushComponent()->SetCollisionObjectType(COLLISION_GAMEVOLUME);

	//bTestBaseEntry = true;
}

// FIXMESTEVE - temp: remove after all flagrun maps resaved.
void AUTGameVolume::PostLoad()
{
	Super::PostLoad();
	bIsDefenderBase = bIsDefenderBase || (bIsNoRallyZone && !bIsTeamSafeVolume);
}

void AUTGameVolume::Reset_Implementation()
{
	bHasBeenEntered = false;
	bHasFCEntry = false;
}

void AUTGameVolume::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if ((VoiceLinesSet == NAME_None) && !VolumeName.IsEmpty() && !bIsTeleportZone && !bIsTeamSafeVolume)
	{
		VoiceLinesSet = UUTCharacterVoice::StaticClass()->GetDefaultObject<UUTCharacterVoice>()->GetFallbackLines(FName(*VolumeName.ToString()));
		if (VoiceLinesSet == NAME_None)
		{
			UE_LOG(UT, Warning, TEXT("No voice lines found for %s"), *VolumeName.ToString());
		}
	}
}

int32 AUTGameVolume::DetermineEntryDirection(AUTCharacter* EnteringCharacter, AUTFlagRunGameState* GS)
{
	// determine entry direction
	int32 DirectionSwitch = 5;
	AUTGameObjective* Objective = GS->GetFlagBase(GS->bRedToCap ? 1 : 0);
	if (Objective)
	{
		FVector Dir = EnteringCharacter->GetActorLocation() - Objective->GetActorLocation();
		FVector Dir2D = Dir;
		Dir2D.Z = 0.f;
		Dir2D = Dir2D.GetSafeNormal();
		FVector Facing = Objective->GetActorRotation().Vector();
		if (bTestBaseEntry)
		{
			DrawDebugLine(GetWorld(), Objective->GetActorLocation(), Objective->GetActorLocation() + 1000.f*Facing, FColor::Green, true);
			UE_LOG(UT, Warning, TEXT("Angle is %f"), FMath::Abs(Facing | Dir2D));
			// FIXMESTEVE show angle
		}
		if ((FMath::Abs(Dir.Z) >= Objective->IncomingHeightOffset) && (FMath::Abs(Facing | Dir2D) > Objective->HighLowDot))
		{
			DirectionSwitch = (Dir.Z >= Objective->IncomingHeightOffset) ? 1 : 2;
		}
		else if (FMath::Abs(Facing | Dir2D) > Objective->ForwardDot)
		{
			DirectionSwitch = 0;
		}
		else
		{
			// left or right
			FVector Left = Facing ^ FVector(0.f, 0.f, 1.f);
			DirectionSwitch = ((Left | Dir) > 0.f) ? 3 : 4;
		}
	}
	return DirectionSwitch;
}

void AUTGameVolume::ActorEnteredVolume(class AActor* Other)
{
	AUTCharacter* P = Cast<AUTCharacter>(Other);
	if ((Role == ROLE_Authority) && P)
	{
		P->LastGameVolume = this;
		AUTFlagRunGameState* GS = GetWorld()->GetGameState<AUTFlagRunGameState>();
        if (GS != nullptr && P->GetPlayerState() != nullptr && !GS->IsMatchIntermission() && (GS->IsMatchInProgress() || (Cast<AUTPlayerState>(P->GetPlayerState()) && ((AUTPlayerState*)(P->GetPlayerState()))->bIsWarmingUp)))
		{
			if (bIsTeamSafeVolume)
			{
				// friendlies are invulnerable, enemies must die
				if (!GS->OnSameTeam(this, P))
				{
					P->TakeDamage(1000.f, FDamageEvent(UUTDmgType_Suicide::StaticClass()), nullptr, this);
				}
				else
				{
					P->EnteredSafeVolumeTime = GetWorld()->GetTimeSeconds();
                    if ((P->Health < 80) && Cast<AUTPlayerState>(P->GetPlayerState()) && (GetWorld()->GetTimeSeconds() - ((AUTPlayerState*)(P->GetPlayerState()))->LastNeedHealthTime > 20.f))
					{
                        ((AUTPlayerState*)(P->GetPlayerState()))->LastNeedHealthTime = GetWorld()->GetTimeSeconds();
                        ((AUTPlayerState*)(P->GetPlayerState()))->AnnounceStatus(StatusMessage::NeedHealth, 0, true);
					}
				}
			}
			else
			{
				// failsafe
				P->bHasLeftSafeVolume = true;
			}
			if (bIsTeleportZone)
			{
				if (AssociatedTeleporter == nullptr)
				{
					for (FActorIterator It(GetWorld()); It; ++It)
					{
						AssociatedTeleporter = Cast<AUTTeleporter>(*It);
						if (AssociatedTeleporter)
						{
							break;
						}
					}
				}
				if (AssociatedTeleporter)
				{
					AssociatedTeleporter->OnOverlapBegin(this, P);
				}
			}
			else if (P->GetCarriedObject())
			{
				/*if (VoiceLinesSet != NAME_None)
				{
					UE_LOG(UT, Warning, TEXT("VoiceLineSet %s for %s location %s"), *VoiceLinesSet.ToString(), *GetName(), *VolumeName.ToString());
					//((AUTPlayerState *)(P->PlayerState))->AnnounceStatus(VoiceLinesSet, 1);
				}
				else
				{
					UE_LOG(UT, Warning, TEXT("No VoiceLineSet for %s location %s"), *GetName(), *VolumeName.ToString());
				}*/
				if (bIsDefenderBase && !P->GetCarriedObject()->bWasInEnemyBase)
				{
					P->GetCarriedObject()->PlayAlarm();
					P->GetCarriedObject()->EnteredEnemyBaseTime = GetWorld()->GetTimeSeconds();
				}

				// possibly play incoming warning
				if ((bPlayIncomingWarning || bIsDefenderBase) && !P->GetCarriedObject()->bWasInEnemyBase && (GetWorld()->GetTimeSeconds() - GS->LastIncomingWarningTime > 3.f))
				{
					WarnFCIncoming(P);
				}

				// possibly announce flag carrier changed zones
				if (bIsDefenderBase && !P->GetCarriedObject()->bWasInEnemyBase && (GetWorld()->GetTimeSeconds() - FMath::Min(GS->LastEnemyFCEnteringBaseTime, GS->LastEnteringEnemyBaseTime) > 2.f))
				{
                    if ((GetWorld()->GetTimeSeconds() - GS->LastEnteringEnemyBaseTime > 2.f) && Cast<AUTPlayerState>(P->GetPlayerState()))
					{
                        ((AUTPlayerState *)(P->GetPlayerState()))->AnnounceStatus(StatusMessage::ImGoingIn);
						if (VoiceLinesSet != NAME_None)
						{
                            GS->UpdateFCFriendlyLocation(((AUTPlayerState *)(P->GetPlayerState())), this);
						}
						GS->LastEnteringEnemyBaseTime = GetWorld()->GetTimeSeconds();
					}
					if (GetWorld()->GetTimeSeconds() - GS->LastEnemyFCEnteringBaseTime > 2.f)
					{
						AUTPlayerState* PS = GetBestWarner(P); 
						if (PS)
						{
							if (VoiceLinesSet != NAME_None)
							{
								GS->UpdateFCEnemyLocation(PS, this);
							}
							GS->LastEnemyFCEnteringBaseTime = GetWorld()->GetTimeSeconds();
						}
					}
				}
				else if ((GetWorld()->GetTimeSeconds() - FMath::Min(GS->LastFriendlyLocationReportTime, GS->LastEnemyLocationReportTime) > 1.f) || bIsWarningZone || !bHasFCEntry)
				{
                    if ((VoiceLinesSet != NAME_None) && ((GetWorld()->GetTimeSeconds() - GS->LastFriendlyLocationReportTime > 1.f) || !bHasFCEntry) && Cast<AUTPlayerState>(P->GetPlayerState()) && (GS->LastFriendlyLocationName != VoiceLinesSet))
					{
                        GS->UpdateFCFriendlyLocation(((AUTPlayerState *)(P->GetPlayerState())), this);
					}
					if ((VoiceLinesSet != NAME_None) && P->GetCarriedObject()->bCurrentlyPinged && P->GetCarriedObject()->LastPinger && ((GetWorld()->GetTimeSeconds() - GS->LastEnemyLocationReportTime > 1.f) || !bHasFCEntry) && (GS->LastEnemyLocationName != VoiceLinesSet))
					{
						GS->UpdateFCEnemyLocation(P->GetCarriedObject()->LastPinger, this);
					}
					else if (bIsWarningZone && !P->bWasInWarningZone && !bIsDefenderBase)
					{
						// force ping if important zone, wasn't already in important zone
						// also do if no pinger if important zone
						P->GetCarriedObject()->LastPingedTime = FMath::Max(P->GetCarriedObject()->LastPingedTime, GetWorld()->GetTimeSeconds() - P->GetCarriedObject()->PingedDuration + 1.f);
						if (VoiceLinesSet != NAME_None)
						{
							AUTPlayerState* Warner = GetBestWarner(P);
							if (Warner && ((GetWorld()->GetTimeSeconds() - GS->LastEnemyLocationReportTime > 1.f) || !bHasFCEntry))
							{
								GS->UpdateFCEnemyLocation(Warner, this);
							}
						}
					}
				}
				P->bWasInWarningZone = bIsWarningZone;
				P->GetCarriedObject()->bWasInEnemyBase = bIsDefenderBase;
				bHasFCEntry = true;
			}
            else if (bIsDefenderBase && Cast<AUTPlayerState>(P->GetPlayerState()) && ((AUTPlayerState*)(P->GetPlayerState()))->Team && (GS->bRedToCap == (((AUTPlayerState*)(P->GetPlayerState()))->Team->TeamIndex == 0)))
			{
				// warn base is under attack
				if (GetWorld()->GetTimeSeconds() - GS->LastEnemyEnteringBaseTime > MinEnemyInBaseInterval)
				{
					AUTPlayerState* PS = GetBestWarner(P);
					if (PS)
					{
						PS->AnnounceStatus(StatusMessage::BaseUnderAttack);
						GS->LastEnemyEnteringBaseTime = GetWorld()->GetTimeSeconds();
					}
				}
			}
			else if (!bHasBeenEntered && bReportDefenseStatus && (VoiceLinesSet != NAME_None))
			{
                AUTPlayerState* PS = Cast<AUTPlayerState>(P->GetPlayerState());
				AUTFlagRunGameState* FRGS = GetWorld()->GetGameState<AUTFlagRunGameState>();
				if (PS && PS->Team && FRGS && (FRGS->bRedToCap == (PS->Team->TeamIndex == 1)))
				{
					PS->AnnounceLocation(this, 2);
				}
			}
		}
		bHasBeenEntered = true;
	}
	else if ((Role != ROLE_Authority) && P && bIsTeamSafeVolume && !P->bHasLeftSafeVolume && P->GetController() && (TeamLockers.Num() > 0) && TeamLockers[0])
	{
		TeamLockers[0]->GiveAmmo(P);
	}

	if (!VolumeName.IsEmpty() && P)
	{
		//P->LastKnownLocation = this;
        AUTPlayerState* PS = Cast<AUTPlayerState>(P->GetPlayerState());
		if (PS != nullptr)
		{
			PS->LastKnownLocation = this;
		}
	}
}

void AUTGameVolume::WarnFCIncoming(AUTCharacter* FlagCarrier)
{
	AUTFlagRunGameState* GS = GetWorld()->GetGameState<AUTFlagRunGameState>();
	if (GS)
	{
		GS->LastIncomingWarningTime = GetWorld()->GetTimeSeconds();
        if (bTestBaseEntry && Cast<AUTPlayerState>(FlagCarrier->GetPlayerState()))
		{
			int32 DirectionSwitch = DetermineEntryDirection(FlagCarrier, GS);
            ((AUTPlayerState *)(FlagCarrier->GetPlayerState()))->AnnounceStatus(StatusMessage::Incoming, DirectionSwitch);
		}
		else
		{
			AUTPlayerState* PS = GetBestWarner(FlagCarrier);
			if (PS)
			{
				int32 DirectionSwitch = DetermineEntryDirection(FlagCarrier, GS);
				PS->AnnounceStatus(StatusMessage::Incoming, DirectionSwitch);
			}
		}
	}
}

AUTPlayerState* AUTGameVolume::GetBestWarner(AUTCharacter* StatusEnemy)
{
	AUTPlayerState* Warner = nullptr;
	AUTFlagRunGameState* GS = GetWorld()->GetGameState<AUTFlagRunGameState>();
	if (StatusEnemy && GS)
	{
		Warner = StatusEnemy->GetCarriedObject() ? StatusEnemy->GetCarriedObject()->LastPinger : nullptr;
		if (Warner == nullptr)
		{
			Warner = StatusEnemy->LastTargeter;
		}
		if (Warner && GS->OnSameTeam(Warner, StatusEnemy))
		{
			Warner = nullptr;
		}
		if (Warner == nullptr)
		{
			for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
			{
				AController* C = Iterator->Get();
				if (C && !GS->OnSameTeam(StatusEnemy, C) && Cast<AUTPlayerState>(C->PlayerState))
				{
					Warner = ((AUTPlayerState*)(C->PlayerState));
					break;
				}
			}
		}
	}
	return Warner;
}

void AUTGameVolume::ActorLeavingVolume(class AActor* Other)
{
	AUTCharacter* UTCharacter = Cast<AUTCharacter>(Other);
	if (UTCharacter)
	{
		if (bIsTeamSafeVolume)
		{
			UTCharacter->bHasLeftSafeVolume = true;
			AUTPlayerController* PC = Cast<AUTPlayerController>(UTCharacter->GetController());
			if (PC)
			{
				PC->LeftSpawnVolumeTime = GetWorld()->GetTimeSeconds();
			}
		}
	}
}

void AUTGameVolume::SetTeamForSideSwap_Implementation(uint8 NewTeamNum)
{
	TeamIndex = NewTeamNum;
}





