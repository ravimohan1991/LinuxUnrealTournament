// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTTeamGameMode.h"
#include "UTFlagRunGame.h"
#include "UTCTFGameMessage.h"
#include "UTCTFRoleMessage.h"
#include "UTCTFRewardMessage.h"
#include "UTCTFMajorMessage.h"
#include "UTFirstBloodMessage.h"
#include "UTPickup.h"
#include "UTGameMessage.h"
#include "UTMutator.h"
#include "UTWorldSettings.h"
#include "Widgets/SUTTabWidget.h"
#include "Dialogs/SUTPlayerInfoDialog.h"
#include "StatNames.h"
#include "Engine/DemoNetDriver.h"
#include "UTShowdownGameMessage.h"
#include "UTShowdownRewardMessage.h"
#include "UTPlayerStart.h"
#include "UTPlayerState.h"
#include "UTFlagRunHUD.h"
#include "UTArmor.h"
#include "UTGhostFlag.h"
#include "UTFlagRunGameState.h"
#include "UTAsymCTFSquadAI.h"
#include "UTWeaponRedirector.h"
#include "UTFlagRunMessage.h"
#include "UTWeap_Translocator.h"
#include "UTReplicatedEmitter.h"
#include "UTATypes.h"
#include "UTGameVolume.h"
#include "Animation/AnimInstance.h"
#include "UTAnalytics.h"
#include "UTRallyPoint.h"
#include "UTRemoteRedeemer.h"
#include "UTFlagRunScoring.h"
#include "UTLineUpHelper.h"
#include "UTShowdownStatusMessage.h"
#include "UTBlitzFlag.h"
#include "UTFlagRunGameState.h"
#include "UTAssistMessage.h"
#include "UTBlitzFlagSpawner.h"
#include "UTBlitzDeliveryPoint.h"

//Special markup for Analytics event so they show up properly in grafana. Should be eventually moved to UTAnalytics.
/*
* @EventName RCTFRoundResult
* @Trigger Sent when a round ends in an RCTF game through Score Alternate Win
* @Type Sent by the Server
* @EventParam FlagCapScore int32 Always 0, just shows that someone capped flag
* @Comments
*/

AUTFlagRunGame::AUTFlagRunGame(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NumTeams = 2;
	bUseTeamStarts = true;

	TimeLimit = 5;
	IntermissionDuration = 28.f;
	RoundLives = 5;
	UnlimitedRespawnWaitTime = 2.f;
	bForceRespawn = true;
	bFirstRoundInitialized = false;
	HUDClass = AUTFlagRunHUD::StaticClass();
	SquadType = AUTAsymCTFSquadAI::StaticClass();
	NumRounds = 6;
	MaxTimeScoreBonus = 180;

	RollingAttackerRespawnDelay = 5.f;
	LastAttackerSpawnTime = 0.f;
	RollingSpawnStartTime = 0.f;
	bRollingAttackerSpawns = true;
	ForceRespawnTime = 0.1f;
	LimitedRespawnWaitTime = 6.f;
	MatchSummaryDelay = 20.f;

	bSitOutDuringRound = false;
	EndOfMatchMessageDelay = 2.5f;
	bUseLevelTiming = true;

	GoldScore = 3;
	SilverScore = 2;
	BronzeScore = 1;
	DefenseScore = 1;
	DisplayName = NSLOCTEXT("UTGameMode", "Blitz", "Blitz");
	bHideInUI = false;
	bWeaponStayActive = false;
	bAllowPickupAnnouncements = true;
	LastEntryDefenseWarningTime = 0.f;
	MapPrefix = TEXT("FR");
	GameStateClass = AUTFlagRunGameState::StaticClass();
	bAllowBoosts = false;
	bGameHasImpactHammer = false;
	FlagPickupDelay = 20;
	bTrackKillAssists = true;
	BlitzScoringClass = AUTFlagRunScoring::StaticClass();
	DefaultMaxPlayers = 10;
	XPMultiplier = 3.5f;
	MatchIntroTime = 2.f;
	bNoDefaultLeaderHat = true;
	bSlowFlagCarrier = false;

	ActivatedPowerupPlaceholderObject = FStringAssetReference(TEXT("/Game/RestrictedAssets/Pickups/Powerups/BP_ActivatedPowerup_UDamage.BP_ActivatedPowerup_UDamage_C"));

	static ConstructorHelpers::FObjectFinder<USoundBase> RampUpMusicFinderC(TEXT("SoundWave'/Game/RestrictedAssets/Audio/Stingers/RampUpMusicC.RampUpMusicC'")); // Pre-round
	RampUpMusic.Add(RampUpMusicFinderC.Object);
	RampUpTime.Add(10.f);

	static ConstructorHelpers::FObjectFinder<USoundBase> RallyFinalSoundFinder(TEXT("SoundWave'/Game/RestrictedAssets/Audio/Stingers/RallyFailed.RallyFailed'"));
	RallyFailedSound = RallyFinalSoundFinder.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> RampUpMusicFinderE(TEXT("SoundWave'/Game/RestrictedAssets/Audio/Stingers/RampUpMusicE.RampUpMusicE'")); // end of game, cut out at beat change
	EndMusic = RampUpMusicFinderE.Object;
	EndMusicTime = 19.1f;
}

void AUTFlagRunGame::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	FActorSpawnParameters SpawnInfo;
	SpawnInfo.Instigator = Instigator;
	BlitzScoring = GetWorld()->SpawnActor<AUTBaseScoring>(BlitzScoringClass, SpawnInfo);
	BlitzScoring->InitFor(this);
}

void AUTFlagRunGame::CreateGameURLOptions(TArray<TSharedPtr<TAttributePropertyBase>>& MenuProps)
{
	MenuProps.Empty();
	if (BotFillCount == 0)
	{
		BotFillCount = DefaultMaxPlayers;
	}
	MenuProps.Add(MakeShareable(new TAttributeProperty<int32>(this, &BotFillCount, TEXT("BotFill"))));
	MenuProps.Add(MakeShareable(new TAttributePropertyBool(this, &bBalanceTeams, TEXT("BalanceTeams"))));
}

void AUTFlagRunGame::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	if (!UGameplayStatics::HasOption(Options, TEXT("TimeLimit")) || (TimeLimit <= 0))
	{
		AUTWorldSettings* WorldSettings = bUseLevelTiming ? Cast<AUTWorldSettings>(GetWorldSettings()) : nullptr;
		TimeLimit = WorldSettings ? WorldSettings->DefaultRoundLength : TimeLimit;
	}

	// key options are ?RoundLives=xx?Dash=xx?Asymm=xx?PerPlayerLives=xx?OffKillsForPowerup=xx?DefKillsForPowerup=xx?DelayRally=xxx?Boost=xx
	RoundLives = FMath::Max(1, UGameplayStatics::GetIntOption(Options, TEXT("RoundLives"), RoundLives));

	FlagPickupDelay = FMath::Max(1, UGameplayStatics::GetIntOption(Options, TEXT("FlagDelay"), FlagPickupDelay));

	if (!ActivatedPowerupPlaceholderObject.IsNull())
	{
		ActivatedPowerupPlaceholderClass = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), NULL, *ActivatedPowerupPlaceholderObject.ToStringReference().ToString(), NULL, LOAD_NoWarn));
	}

	FString InOpt = UGameplayStatics::ParseOption(Options, TEXT("Boost"));
	bAllowBoosts = EvalBoolOptions(InOpt, bAllowBoosts);

	if (bDevServer)
	{
		FlagPickupDelay = 3.f;
	}

	InOpt = UGameplayStatics::ParseOption(Options, TEXT("SlowFC"));
	bSlowFlagCarrier = EvalBoolOptions(InOpt, bSlowFlagCarrier);

	if (bBasicTrainingGame)
	{
		NumRounds = 2;
	}
}

void AUTFlagRunGame::InitGameState()
{
	Super::InitGameState();

	// Store a cached reference to the GameState
	BlitzGameState = Cast<AUTFlagRunGameState>(GameState);
	BlitzGameState->CTFRound = 1;
	BlitzGameState->NumRounds = NumRounds;
}

void AUTFlagRunGame::PostLogin(APlayerController* NewPlayer)
{
	if (NewPlayer)
	{
		InitPlayerForRound(Cast<AUTPlayerState>(NewPlayer->PlayerState));
		AUTPlayerState* UTPS = Cast<AUTPlayerController>(NewPlayer) ? Cast<AUTPlayerController>(NewPlayer)->UTPlayerState : nullptr;
		if (UTPS && UTPS->Team && IsMatchInProgress() && UTGameState && !UTGameState->IsMatchIntermission())
		{
			NewPlayer->ClientReceiveLocalizedMessage(UUTCTFRoleMessage::StaticClass(), IsTeamOnDefense(UTPS->Team->TeamIndex) ? 2 : 1);
		}
	}
	Super::PostLogin(NewPlayer);
}

void AUTFlagRunGame::BeginGame()
{
	UE_LOG(UT, Log, TEXT("BEGIN GAME GameType: %s"), *GetNameSafe(this));
	UE_LOG(UT, Log, TEXT("Difficulty: %f GoalScore: %i TimeLimit (sec): %i"), GameDifficulty, GoalScore, TimeLimit);

	for (FActorIterator It(GetWorld()); It; ++It)
	{
		AActor* TestActor = *It;
		if (TestActor && !TestActor->IsPendingKill() && TestActor->IsA<AUTPlayerState>())
		{
			Cast<AUTPlayerState>(TestActor)->StartTime = 0;
			Cast<AUTPlayerState>(TestActor)->bSentLogoutAnalytics = false;
		}
		else if (TestActor && !TestActor->IsPendingKill() && TestActor->IsA<AUTProjectile>())
		{
			TestActor->Destroy();
		}
	}
	if (BlitzGameState)
	{
		BlitzGameState->ElapsedTime = 0;
	}

	//Let the game session override the StartMatch function, in case it wants to wait for arbitration
	if (GameSession->HandleStartMatchRequest())
	{
		return;
	}
	if (BlitzGameState)
	{
		BlitzGameState->CTFRound = 1;
		BlitzGameState->NumRounds = NumRounds;
		BlitzGameState->HalftimeScoreDelay = 0.5f;
	}
	for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
	{
		AUTPlayerState* UTPlayerState = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
		if (UTPlayerState)
		{
			UTPlayerState->NotIdle();
		}
	}

	if (GetNetMode() == NM_Standalone)
	{
		IntermissionDuration = 15.f;
	}

	float RealIntermissionDuration = IntermissionDuration;
	IntermissionDuration = bBasicTrainingGame ? 12.f : 6.f;
	SetMatchState(MatchState::MatchIntermission);
	IntermissionDuration = RealIntermissionDuration;
}


float AUTFlagRunGame::GetScoreForXP(AUTPlayerState* PS)
{
	return PS->Kills + PS->KillAssists + 5.f*PS->FlagCaptures;
}

float AUTFlagRunGame::AdjustNearbyPlayerStartScore(const AController* Player, const AController* OtherController, const ACharacter* OtherCharacter, const FVector& StartLoc, const APlayerStart* P)
{
	return 0.f;
}

int32 AUTFlagRunGame::GetFlagCapScore()
{
	if (BlitzGameState)
	{
		int32 BonusTime = BlitzGameState->GetRemainingTime();
		if (BonusTime >= BlitzGameState->GoldBonusThreshold)
		{
			return GoldScore;
		}
		if (BonusTime >= BlitzGameState->SilverBonusThreshold)
		{
			return SilverScore;
		}
	}
	return BronzeScore;
}

void AUTFlagRunGame::AnnounceWin(AUTTeamInfo* WinningTeam, APlayerState* ScoringPlayer, uint8 Reason)
{
	for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
	{
		AUTPlayerController* UTPC = Cast<AUTPlayerController>(*Iterator);
		if (UTPC)
		{
			UTPC->ClientAnnounceRoundScore(WinningTeam, ScoringPlayer, WinningTeam->RoundBonus, Reason);
		}
	}
}

void AUTFlagRunGame::InitGameStateForRound()
{
	if (BlitzGameState)
	{
		BlitzGameState->CTFRound++;
		BlitzGameState->RemainingPickupDelay = FlagPickupDelay;
		BlitzGameState->bRedToCap = !BlitzGameState->bRedToCap;
		BlitzGameState->CurrentRallyPoint = nullptr;
		BlitzGameState->PendingRallyPoint = nullptr;
		BlitzGameState->bEnemyRallyPointIdentified = false;
		BlitzGameState->ScoringPlayerState = nullptr;
	}
}

float AUTFlagRunGame::RatePlayerStart(APlayerStart* P, AController* Player)
{
	// @TODO FIXMESTEVE no need to check enemy traces, just overlaps
	float Result = Super::RatePlayerStart(P, Player);
	if ((Result > 0.f) && Cast<AUTPlayerStart>(P))
	{
		// try to spread out spawns between volumes
		AUTPlayerStart* Start = (AUTPlayerStart*)P;
		AUTPlayerState* PS = Player ? Cast<AUTPlayerState>(Player->PlayerState) : nullptr;
		if (Start && PS && BlitzGameState && PS->Team != NULL)
		{
			const bool bIsAttacker = (BlitzGameState->bRedToCap == (PS->Team->TeamIndex == 0));
			if (Start->PlayerStartGroup != (bIsAttacker ? LastAttackerSpawnGroup : LastDefenderSpawnGroup))
			{
				Result += 20.f;
			}
		}
	}
	return Result;
}

bool AUTFlagRunGame::ChangeTeam(AController* Player, uint8 NewTeamIndex, bool bBroadcast)
{
	AUTPlayerState* PS = Cast<AUTPlayerState>(Player->PlayerState);
	AUTTeamInfo* OldTeam = PS->Team;
	bool bResult = Super::ChangeTeam(Player, NewTeamIndex, bBroadcast);
	if (bResult && (GetMatchState() == MatchState::InProgress))
	{
		HandleTeamChange(PS, OldTeam);
	}
	return bResult;
}

AActor* AUTFlagRunGame::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	AActor* const Best = Super::FindPlayerStart_Implementation(Player, IncomingName);
	if (Best)
	{
		LastStartSpot = Best;
		AUTPlayerStart* Start = (AUTPlayerStart*)Best;
		AUTPlayerState* PS = Player ? Cast<AUTPlayerState>(Player->PlayerState) : nullptr;
		if (Start && PS && BlitzGameState && PS->Team != NULL)
		{
			if (BlitzGameState->bRedToCap == (PS->Team->TeamIndex == 0))
			{
				LastAttackerSpawnGroup = Start->PlayerStartGroup;
			}
			else
			{
				LastDefenderSpawnGroup = Start->PlayerStartGroup;
			}
		}
	}
	return Best;
}

bool AUTFlagRunGame::AvoidPlayerStart(AUTPlayerStart* P)
{
	return P && P->bIgnoreInASymCTF;
}

int32 AUTFlagRunGame::GetDefenseScore()
{
	return DefenseScore;
}

void AUTFlagRunGame::CheckRoundTimeVictory()
{
	int32 RemainingTime = UTGameState ? UTGameState->GetRemainingTime() : 100;
	if (RemainingTime <= 0)
	{
		// Round is over, defense wins.
		ScoreAlternateWin((BlitzGameState && BlitzGameState->bRedToCap) ? 1 : 0, 1);
	}
	else if (RemainingTime - BlitzGameState->EarlyEndTime <= 0)
	{
		// Round is over, defense wins.
		ScoreAlternateWin((BlitzGameState && BlitzGameState->bRedToCap) ? 1 : 0, 2);
	}
	else if (BlitzGameState)
	{
		uint8 OldBonusLevel = BlitzGameState->BonusLevel;
		BlitzGameState->BonusLevel = (RemainingTime >= BlitzGameState->GoldBonusThreshold) ? 3 : 2;
		if (RemainingTime < BlitzGameState->SilverBonusThreshold)
		{
			BlitzGameState->BonusLevel = 1;
		}
		if (OldBonusLevel != BlitzGameState->BonusLevel)
		{
			BlitzGameState->ForceNetUpdate();
		}
	}
}

void AUTFlagRunGame::InitDelayedFlag(AUTCarriedObject* Flag)
{
	if (Flag != nullptr)
	{
		Flag->bFriendlyCanPickup = false;
		if (IsTeamOnOffense(Flag->GetTeamNum()))
		{
			Flag->SetActorHiddenInGame(true);
			FFlagTrailPos NewPosition;
			NewPosition.Location = Flag->GetHomeLocation();
			NewPosition.MidPoints[0] = FVector(0.f);
			Flag->PutGhostFlagAt(NewPosition);
		}
	}
}

void AUTFlagRunGame::InitFlagForRound(AUTCarriedObject* Flag)
{
	if (Flag != nullptr)
	{
		Flag->bSlowsMovement = bSlowFlagCarrier;
		Flag->ClearGhostFlags();
		if (IsTeamOnOffense(Flag->GetTeamNum()))
		{
			Flag->bFriendlyCanPickup = true;
			Flag->SetActorHiddenInGame(false);
			Flag->bWaitingForFirstPickup = true;
			GetWorldTimerManager().SetTimer(Flag->NeedFlagAnnouncementTimer, Flag, &AUTCarriedObject::SendNeedFlagAnnouncement, 5.f, false);
			ActiveFlag = Flag;
		}
		else
		{
			Flag->Destroy();
		}
	}
}

void AUTFlagRunGame::NotifyFirstPickup(AUTCarriedObject* Flag)
{
	if (Flag && Flag->HoldingPawn && StartingArmorClass)
	{
		if (!StartingArmorClass.GetDefaultObject()->HandleGivenTo(Flag->HoldingPawn))
		{
			Flag->HoldingPawn->AddInventory(GetWorld()->SpawnActor<AUTInventory>(StartingArmorClass, FVector(0.0f), FRotator(0.f, 0.f, 0.f)), true);
		}
	}
}

void AUTFlagRunGame::IntermissionSwapSides()
{
	// swap sides, if desired
	BlitzGameState->ChangeTeamSides(1);
}

void AUTFlagRunGame::InitFlags()
{
	if (BlitzGameState && BlitzGameState->FlagSpawner && BlitzGameState->FlagSpawner->MyFlag)
	{
		InitFlagForRound(BlitzGameState->FlagSpawner->MyFlag);

		// check for flag carrier already here waiting
		TArray<AActor*> Overlapping;
		BlitzGameState->FlagSpawner->MyFlag->GetOverlappingActors(Overlapping, AUTCharacter::StaticClass());
		// try humans first, then bots
		for (AActor* A : Overlapping)
		{
			AUTCharacter* Character = Cast<AUTCharacter>(A);
			if (Character != nullptr && Cast<APlayerController>(Character->Controller) != nullptr)
			{
				if (!GetWorld()->LineTraceTestByChannel(Character->GetActorLocation(), BlitzGameState->FlagSpawner->MyFlag->GetActorLocation(), ECC_Pawn, FCollisionQueryParams(), WorldResponseParams))
				{
					BlitzGameState->FlagSpawner->MyFlag->TryPickup(Character);
					if (BlitzGameState->FlagSpawner->MyFlag->ObjectState == CarriedObjectState::Held)
					{
						return;
					}
				}
			}
		}
		for (AActor* A : Overlapping)
		{
			AUTCharacter* Character = Cast<AUTCharacter>(A);
			if (Character != nullptr && Cast<APlayerController>(Character->Controller) == nullptr)
			{
				if (!GetWorld()->LineTraceTestByChannel(Character->GetActorLocation(), BlitzGameState->FlagSpawner->MyFlag->GetActorLocation(), ECC_Pawn, FCollisionQueryParams(), WorldResponseParams))
				{
					BlitzGameState->FlagSpawner->MyFlag->TryPickup(Character);
					if (BlitzGameState->FlagSpawner->MyFlag->ObjectState == CarriedObjectState::Held)
					{
						return;
					}
				}
			}
		}
	}
}

int32 AUTFlagRunGame::PickCheatWinTeam()
{
	AUTFlagRunGameState* FRGS = Cast<AUTFlagRunGameState>(BlitzGameState);
	return (FRGS && FRGS->bRedToCap) ? 0 : 1;
}

bool AUTFlagRunGame::CheckScore_Implementation(AUTPlayerState* Scorer)
{
	CheckForWinner(Scorer->Team);
	return true;
}

bool AUTFlagRunGame::CheckForWinner(AUTTeamInfo* ScoringTeam)
{
	if (ScoringTeam && BlitzGameState && (BlitzGameState->CTFRound >= NumRounds) && (BlitzGameState->CTFRound % 2 == 0))
	{
		AUTTeamInfo* BestTeam = ScoringTeam;
		bool bHaveTie = false;

		// Check if team with highest score has reached goal score
		for (AUTTeamInfo* Team : Teams)
		{
			if (Team->Score > BestTeam->Score)
			{
				BestTeam = Team;
				bHaveTie = false;
				bSecondaryWin = false;
			}
			else if ((Team != BestTeam) && (Team->Score == BestTeam->Score))
			{
				bHaveTie = true;
				AUTFlagRunGameState* GS = Cast<AUTFlagRunGameState>(UTGameState);
				if (GS && (GS->TiebreakValue != 0))
				{
					BestTeam = (GS->TiebreakValue > 0) ? Teams[0] : Teams[1];
					bHaveTie = false;
					bSecondaryWin = true;
				}
			}
		}
		if (!bHaveTie)
		{
			EndTeamGame(BestTeam, FName(TEXT("scorelimit")));
			if (FUTAnalytics::IsAvailable())
			{
				const bool bIsDefenseWin = ScoringTeam ? IsTeamOnDefense(ScoringTeam->GetTeamNum()) : true;
				int WinningTeamNum = 0;
				if (ScoringTeam)
				{
					WinningTeamNum = ScoringTeam->GetTeamNum();
				}
				else
				{
					for (int TeamNumIndex = 0; TeamNumIndex < Teams.Num(); ++TeamNumIndex)
					{
						if (IsTeamOnDefense(Teams.Num()))
						{
							WinningTeamNum = Teams[TeamNumIndex]->GetTeamNum();
						}
					}
				}

				FUTAnalytics::FireEvent_FlagRunRoundEnd(this, bIsDefenseWin, true, WinningTeamNum);
			}
			return true;
		}
	}

	// Check if a team has an insurmountable lead
	// current implementation assumes 6 rounds and 2 teams
	if (BlitzGameState && (BlitzGameState->CTFRound >= NumRounds - 2) && Teams[0] && Teams[1])
	{
		AUTFlagRunGameState* GS = Cast<AUTFlagRunGameState>(BlitzGameState);
		bSecondaryWin = false;
		if (BlitzGameState->CTFRound == NumRounds - 2)
		{
			int32 ScoreDifference = FMath::Abs(Teams[0]->Score - Teams[1]->Score);
			bool bInsurmountableLead = ScoreDifference > DefenseScore + GoldScore;
			if (!bInsurmountableLead && (ScoreDifference == DefenseScore + GoldScore) && (FMath::Abs(GS->TiebreakValue) > 60) && ((GS->TiebreakValue > 0) == (Teams[0]->Score - Teams[1]->Score > 0)))
			{
				bInsurmountableLead = true;
			}
			if (bInsurmountableLead)
			{
				AUTTeamInfo* BestTeam = (Teams[0]->Score > Teams[1]->Score) ? Teams[0] : Teams[1];
				EndTeamGame(BestTeam, FName(TEXT("scorelimit")));

				if (FUTAnalytics::IsAvailable())
				{
					const bool bIsDefenseWin = ScoringTeam ? IsTeamOnDefense(ScoringTeam->GetTeamNum()) : true;
					int WinningTeamNum = 0;
					if (ScoringTeam)
					{
						WinningTeamNum = ScoringTeam->GetTeamNum();
					}
					else
					{
						for (int TeamNumIndex = 0; TeamNumIndex < Teams.Num(); ++TeamNumIndex)
						{
							if (IsTeamOnDefense(Teams.Num()))
							{
								WinningTeamNum = Teams[TeamNumIndex]->GetTeamNum();
							}
						}
					}
					FUTAnalytics::FireEvent_FlagRunRoundEnd(this, bIsDefenseWin, true, WinningTeamNum);
				}
				return true;
			}
		}
		else if (BlitzGameState->CTFRound == NumRounds - 1)
		{
			if (GS && GS->bRedToCap)
			{
				// next round is blue cap
				if ((Teams[0]->Score > Teams[1]->Score + GoldScore) || ((GS->TiebreakValue > 60) && (Teams[0]->Score == Teams[1]->Score + GoldScore)))
				{
					EndTeamGame(Teams[0], FName(TEXT("scorelimit")));

					if (FUTAnalytics::IsAvailable())
					{
						const bool bIsDefenseWin = ScoringTeam ? IsTeamOnDefense(ScoringTeam->GetTeamNum()) : true;
						int WinningTeamNum = 0;
						if (ScoringTeam)
						{
							WinningTeamNum = ScoringTeam->GetTeamNum();
						}
						else
						{
							for (int TeamNumIndex = 0; TeamNumIndex < Teams.Num(); ++TeamNumIndex)
							{
								if (IsTeamOnDefense(Teams.Num()))
								{
									WinningTeamNum = Teams[TeamNumIndex]->GetTeamNum();
								}
							}
						}
						FUTAnalytics::FireEvent_FlagRunRoundEnd(this, bIsDefenseWin, true, WinningTeamNum);
					}
					return true;
				}
				if ((Teams[1]->Score > Teams[0]->Score + DefenseScore) || ((Teams[1]->Score == Teams[0]->Score + DefenseScore) && (GS->TiebreakValue < 0)))
				{
					EndTeamGame(Teams[1], FName(TEXT("scorelimit")));

					if (FUTAnalytics::IsAvailable())
					{
						const bool bIsDefenseWin = ScoringTeam ? IsTeamOnDefense(ScoringTeam->GetTeamNum()) : true;
						int WinningTeamNum = 0;
						if (ScoringTeam)
						{
							WinningTeamNum = ScoringTeam->GetTeamNum();
						}
						else
						{
							for (int TeamNumIndex = 0; TeamNumIndex < Teams.Num(); ++TeamNumIndex)
							{
								if (IsTeamOnDefense(Teams.Num()))
								{
									WinningTeamNum = Teams[TeamNumIndex]->GetTeamNum();
								}
							}
						}
						FUTAnalytics::FireEvent_FlagRunRoundEnd(this, bIsDefenseWin, true, WinningTeamNum);
					}
					return true;
				}
			}
			else
			{
				// next round is red cap
				if ((Teams[1]->Score > Teams[0]->Score + GoldScore) || ((GS->TiebreakValue < -60) && (Teams[1]->Score == Teams[0]->Score + GoldScore)))
				{
					EndTeamGame(Teams[1], FName(TEXT("scorelimit")));

					if (FUTAnalytics::IsAvailable())
					{
						const bool bIsDefenseWin = ScoringTeam ? IsTeamOnDefense(ScoringTeam->GetTeamNum()) : true;
						int WinningTeamNum = 0;
						if (ScoringTeam)
						{
							WinningTeamNum = ScoringTeam->GetTeamNum();
						}
						else
						{
							for (int TeamNumIndex = 0; TeamNumIndex < Teams.Num(); ++TeamNumIndex)
							{
								if (IsTeamOnDefense(Teams.Num()))
								{
									WinningTeamNum = Teams[TeamNumIndex]->GetTeamNum();
								}
							}
						}
						FUTAnalytics::FireEvent_FlagRunRoundEnd(this, bIsDefenseWin, true, WinningTeamNum);
					}
					return true;
				}
				if ((Teams[0]->Score > Teams[1]->Score + DefenseScore) || ((Teams[0]->Score == Teams[1]->Score + DefenseScore) && (GS->TiebreakValue > 0)))
				{
					EndTeamGame(Teams[0], FName(TEXT("scorelimit")));

					if (FUTAnalytics::IsAvailable())
					{
						const bool bIsDefenseWin = ScoringTeam ? IsTeamOnDefense(ScoringTeam->GetTeamNum()) : true;
						int WinningTeamNum = 0;
						if (ScoringTeam)
						{
							WinningTeamNum = ScoringTeam->GetTeamNum();
						}
						else
						{
							for (int TeamNumIndex = 0; TeamNumIndex < Teams.Num(); ++TeamNumIndex)
							{
								if (IsTeamOnDefense(Teams.Num()))
								{
									WinningTeamNum = Teams[TeamNumIndex]->GetTeamNum();
								}
							}
						}
						FUTAnalytics::FireEvent_FlagRunRoundEnd(this, bIsDefenseWin, true, WinningTeamNum);
					}
					return true;
				}
			}
		}
	}

	if (FUTAnalytics::IsAvailable())
	{
		const bool bIsDefenseWin = ScoringTeam ? IsTeamOnDefense(ScoringTeam->GetTeamNum()) : true;
		FUTAnalytics::FireEvent_FlagRunRoundEnd(this, bIsDefenseWin, false);
	}
	return false;
}

float AUTFlagRunGame::GetLineUpTime(LineUpTypes LineUpType)
{
	float TimeDelay = 0.f;
	bool bIsReplayGoingToPlay = (GetNetMode() != NM_Standalone);
	switch (LineUpType)
	{
	case LineUpTypes::Intro:
	{
		TimeDelay = 0.f;
		break;
	}
	case LineUpTypes::Intermission:
	{
		TimeDelay = bIsReplayGoingToPlay ? 9.f : 3.f;
		break;
	}
	case LineUpTypes::PostMatch:
	{
		TimeDelay = bIsReplayGoingToPlay ? 9.f : 3.f;
		break;
	}
	}

	return TimeDelay;
}

void AUTFlagRunGame::DefaultTimer()
{
	Super::DefaultTimer();

	//Super::DefaultTimer will never set the ReplayID because of our override of IsUTHandingReplays, 
	//check Super::IsUTHandlingReplays instead to see if we should set ReplayID.
	if (Super::UTIsHandlingReplays())
	{
		UDemoNetDriver* DemoNetDriver = GetWorld()->DemoNetDriver;
		if (DemoNetDriver != nullptr && DemoNetDriver->ReplayStreamer.IsValid())
		{
			UTGameState->ReplayID = DemoNetDriver->ReplayStreamer->GetReplayID();
		}
	}

	if (UTGameState && UTGameState->IsMatchInProgress() && !UTGameState->IsMatchIntermission())
	{
		AUTFlagRunGameState* RBlitzGameState = Cast<AUTFlagRunGameState>(BlitzGameState);
		if (RBlitzGameState && (RBlitzGameState->RemainingPickupDelay <= 0) && (GetWorld()->GetTimeSeconds() - LastEntryDefenseWarningTime > 12.f))
		{
			// check for uncovered routes - support up to 5 entries for now
			AUTGameVolume* EntryRoutes[MAXENTRYROUTES];
			for (int32 i = 0; i < MAXENTRYROUTES; i++)
			{
				EntryRoutes[i] = nullptr;
			}
			// mark routes that need to be covered
			for (TActorIterator<AUTGameVolume> It(GetWorld()); It; ++It)
			{
				AUTGameVolume* GV = *It;
				if ((GV->RouteID > 0) && (GV->RouteID < MAXENTRYROUTES) && GV->bReportDefenseStatus && (GV->VoiceLinesSet != NAME_None))
				{
					EntryRoutes[GV->RouteID] = GV;
				}
			}
			// figure out where defenders are
			bool bFoundInnerDefender = false;
			AUTPlayerState* Speaker = nullptr;
			FString Why = "";
			for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
			{
				AUTCharacter* UTChar = Cast<AUTCharacter>((*Iterator)->GetPawn());
				AUTPlayerState* UTPS = Cast<AUTPlayerState>((*Iterator)->PlayerState);
				if (UTChar && UTChar->LastGameVolume && UTPS && UTPS->Team && IsTeamOnDefense(UTPS->Team->TeamIndex))
				{
					Speaker = UTPS;
					int32 CoveredRoute = UTChar->LastGameVolume->RouteID;
					if (UTChar->LastGameVolume->bIsDefenderBase)
					{
						bFoundInnerDefender = true;
						break;
					}
					else if ((CoveredRoute > 0) && (CoveredRoute < MAXENTRYROUTES))
					{
						EntryRoutes[CoveredRoute] = nullptr;
					}
					else
					{
						//						UE_LOG(UT, Warning, TEXT("Not in defensive position %s %s routeid %d"), *UTChar->LastGameVolume->GetName(), *UTChar->LastGameVolume->VolumeName.ToString(), UTChar->LastGameVolume->RouteID);
					}
					//Why = Why + FString::Printf(TEXT("%s in position %s routeid %d, "), *UTPS->PlayerName, *UTChar->LastGameVolume->VolumeName.ToString(), UTChar->LastGameVolume->RouteID);
				}
			}
			if (!bFoundInnerDefender && Speaker)
			{
				// warn about any uncovered entries
				for (int32 i = 0; i < MAXENTRYROUTES; i++)
				{
					if (EntryRoutes[i] && (EntryRoutes[i]->VoiceLinesSet != NAME_None))
					{
						LastEntryDefenseWarningTime = GetWorld()->GetTimeSeconds();
						/*
						if (Cast<AUTPlayerController>(Speaker->GetOwner()))
						{
							Cast<AUTPlayerController>(Speaker->GetOwner())->TeamSay(Why);
						}
						else if (Cast<AUTBot>(Speaker->GetOwner()))
						{
							Cast<AUTBot>(Speaker->GetOwner())->Say(Why, true);
						}
						*/
						Speaker->AnnounceLocation(EntryRoutes[i], 3);
					}
				}
			}
		}
	}
}

float AUTFlagRunGame::OverrideRespawnTime(AUTPickupInventory* Pickup, TSubclassOf<AUTInventory> InventoryType)
{
	if (!Pickup || !InventoryType)
	{
		return 0.f;
	}
	AUTWeapon* WeaponDefault = Cast<AUTWeapon>(InventoryType.GetDefaultObject());
	if (WeaponDefault)
	{
		if (WeaponDefault->bMustBeHolstered)
		{
			Pickup->bSpawnOncePerRound = true;
			int32 RoundTime = (TimeLimit == 0) ? 300 : TimeLimit;
			return FMath::Max(20.f, RoundTime - 120.f);
		}
		return 20.f;
	}
	return InventoryType.GetDefaultObject()->RespawnTime;
}

int32 AUTFlagRunGame::GetComSwitch(FName CommandTag, AActor* ContextActor, AUTPlayerController* InInstigator, UWorld* World)
{
	if (World == nullptr) return INDEX_NONE;

	AUTFlagRunGameState* GS = Cast<AUTFlagRunGameState>(BlitzGameState);

	if (Instigator == nullptr || GS == nullptr)
	{
		return Super::GetComSwitch(CommandTag, ContextActor, InInstigator, World);
	}

	AUTPlayerState* UTPlayerState = Cast<AUTPlayerState>(InInstigator->PlayerState);
	AUTCharacter* ContextCharacter = ContextActor != nullptr ? Cast<AUTCharacter>(ContextActor) : nullptr;
	AUTPlayerState* ContextPlayerState = ContextCharacter != nullptr ? Cast<AUTPlayerState>(ContextCharacter->PlayerState) : nullptr;
	
	uint8 OffensiveTeamNum = GS->bRedToCap ? 0 : 1;

	if (ContextCharacter)
	{
		bool bContextOnSameTeam = ContextCharacter != nullptr ? World->GetGameState<AUTGameState>()->OnSameTeam(InInstigator, ContextCharacter) : false;
		bool bContextIsFlagCarrier = ContextPlayerState != nullptr && ContextPlayerState->CarriedObject != nullptr;

		if (bContextIsFlagCarrier)
		{
			if ( bContextOnSameTeam )
			{
				if ( CommandTag == CommandTags::Intent )
				{
					return GOT_YOUR_BACK_SWITCH_INDEX;
				}
				else if (CommandTag == CommandTags::Attack)
				{
					return GOING_IN_SWITCH_INDEX;
				}
				else if (CommandTag == CommandTags::Defend)
				{
					return ATTACK_THEIR_BASE_SWITCH_INDEX;
				}
			}
			else
			{
				if (CommandTag == CommandTags::Intent)
				{
					return ENEMY_FC_HERE_SWITCH_INDEX;
				}
				else if (CommandTag == CommandTags::Attack)
				{
					return GET_FLAG_BACK_SWITCH_INDEX;
				}
				else if (CommandTag == CommandTags::Defend)
				{
					return BASE_UNDER_ATTACK_SWITCH_INDEX;
				}
			}
		}
	}

	AUTCharacter* InstCharacter = Cast<AUTCharacter>(InInstigator->GetCharacter());
	if (InstCharacter != nullptr && !InstCharacter->IsDead())
	{
		// We aren't dead, look to see if we have the flag...
			
		if (UTPlayerState->CarriedObject != nullptr)
		{
			if (CommandTag == CommandTags::Intent)			
			{
				return GOT_FLAG_SWITCH_INDEX;
			}
			if (CommandTag == CommandTags::Attack)			
			{
				return ATTACK_THEIR_BASE_SWITCH_INDEX;
			}
			if (CommandTag == CommandTags::Defend)			
			{
				return DEFEND_FLAG_CARRIER_SWITCH_INDEX;
			}
		}
	}

	if (CommandTag == CommandTags::Intent)
	{
		// Look to see if I'm on offense or defense...

		if (InInstigator->GetTeamNum() == OffensiveTeamNum)
		{
			return ATTACK_THEIR_BASE_SWITCH_INDEX;
		}
		else
		{
			return AREA_SECURE_SWITCH_INDEX;
		}
	}

	if (CommandTag == CommandTags::Attack)
	{
		// Look to see if I'm on offense or defense...

		if (InInstigator->GetTeamNum() == OffensiveTeamNum)
		{
			return ATTACK_THEIR_BASE_SWITCH_INDEX;
		}
		else
		{
			return ON_OFFENSE_SWITCH_INDEX;
		}
	}

	if (CommandTag == CommandTags::Defend)
	{
		// Look to see if I'm on offense or defense...

		if (InInstigator->GetTeamNum() == OffensiveTeamNum)
		{
			return ON_DEFENSE_SWITCH_INDEX;
		}
		else
		{
			return SPREAD_OUT_SWITCH_INDEX;
		}
	}

	if (CommandTag == CommandTags::Distress)
	{
		return UNDER_HEAVY_ATTACK_SWITCH_INDEX;  
	}

	return Super::GetComSwitch(CommandTag, ContextActor, InInstigator, World);
}

bool AUTFlagRunGame::HandleRallyRequest(AController* C)
{
	if (C == nullptr)
	{
		return false;
	}
	AUTCharacter* UTCharacter = Cast<AUTCharacter>(C->GetPawn());
	AUTPlayerState* UTPlayerState = Cast<AUTPlayerState>(C->PlayerState);

	// if can rally, teleport with transloc effect, set last rally time
	AUTFlagRunGameState* GS = GetWorld()->GetGameState<AUTFlagRunGameState>();
	AUTTeamInfo* Team = UTPlayerState ? UTPlayerState->Team : nullptr;
	if (Team && UTCharacter && GS && UTPlayerState && !UTCharacter->GetCarriedObject() && GS->CurrentRallyPoint && UTPlayerState->bCanRally && !GetWorldTimerManager().IsTimerActive(UTPlayerState->RallyTimerHandle) && GS->bAttackersCanRally && IsMatchInProgress() && !GS->IsMatchIntermission() && ((Team->TeamIndex == 0) == GS->bRedToCap))
	{
		UTPlayerState->RallyLocation = GS->CurrentRallyPoint->GetRallyLocation(UTCharacter);
		UTPlayerState->RallyPoint = GS->CurrentRallyPoint;
		UTCharacter->bTriggerRallyEffect = true;
		UTCharacter->bRallyInProgress = true;
		UTCharacter->OnTriggerRallyEffect();
		UTPlayerState->BeginRallyTo(UTPlayerState->RallyPoint, UTPlayerState->RallyLocation, 1.f);
		UTCharacter->SpawnRallyDestinationEffectAt(UTPlayerState->RallyLocation);
		if (UTCharacter->UTCharacterMovement)
		{
			UTCharacter->UTCharacterMovement->StopMovementImmediately();
			UTCharacter->UTCharacterMovement->DisableMovement();
			UTCharacter->DisallowWeaponFiring(true);
		}
		return true;
	}
	return false;
}

void AUTFlagRunGame::FinishRallyRequest(AController *C)
{
	AUTCharacter* UTCharacter = Cast<AUTCharacter>(C->GetPawn());
	AUTFlagRunGameState* GS = GetWorld()->GetGameState<AUTFlagRunGameState>();
	if (UTCharacter)
	{
		UTCharacter->bRallyInProgress = false;
	}
	if (!UTCharacter || !IsMatchInProgress() || !GS || GS->IsMatchIntermission() || UTCharacter->IsPendingKillPending())
	{
		return;
	}
	if (UTCharacter->UTCharacterMovement)
	{
		UTCharacter->UTCharacterMovement->SetDefaultMovementMode();
	}
	UTCharacter->DisallowWeaponFiring(false);
}

bool AUTFlagRunGame::CompleteRallyRequest(AController* C)
{
	if (C == nullptr)
	{
		return false;
	}
	AUTPlayerController* RequestingPC = Cast<AUTPlayerController>(C);
	AUTCharacter* UTCharacter = Cast<AUTCharacter>(C->GetPawn());
	AUTPlayerState* UTPlayerState = Cast<AUTPlayerState>(C->PlayerState);

	// if can rally, teleport with transloc effect, set last rally time
	AUTFlagRunGameState* GS = GetWorld()->GetGameState<AUTFlagRunGameState>();
	AUTTeamInfo* Team = UTPlayerState ? UTPlayerState->Team : nullptr;
	if (!UTCharacter || !IsMatchInProgress() || !GS || GS->IsMatchIntermission() || UTCharacter->IsPendingKillPending())
	{
		if (UTCharacter)
		{
			UTCharacter->bRallyInProgress = false;
		}
		return false;
	}
	UTCharacter->bTriggerRallyEffect = false;
	if (!UTCharacter->bCanRally)
	{
		if (RequestingPC != nullptr)
		{
			RequestingPC->ClientPlaySound(RallyFailedSound);
		}
		return false;
	}

	if (Team && ((Team->TeamIndex == 0) == GS->bRedToCap))
	{
		FVector WarpLocation = FVector::ZeroVector;
		FRotator WarpRotation = UTPlayerState->RallyPoint ? UTPlayerState->RallyPoint->GetActorRotation() : UTCharacter->GetActorRotation();
		WarpRotation.Pitch = 0.f;
		WarpRotation.Roll = 0.f;
		ECollisionChannel SavedObjectType = UTCharacter->GetCapsuleComponent()->GetCollisionObjectType();
		UTCharacter->GetCapsuleComponent()->SetCollisionObjectType(COLLISION_TELEPORTING_OBJECT);

		if (GetWorld()->FindTeleportSpot(UTCharacter, UTPlayerState->RallyLocation, WarpRotation))
		{
			WarpLocation = UTPlayerState->RallyLocation;
		}
		else if (UTPlayerState->RallyPoint)
		{
			WarpLocation = UTPlayerState->RallyPoint->GetRallyLocation(UTCharacter);
		}
		else
		{
			if (RequestingPC != nullptr)
			{
				RequestingPC->ClientPlaySound(RallyFailedSound);
			}
			return false;
		}
		UTCharacter->GetCapsuleComponent()->SetCollisionObjectType(SavedObjectType);

		// teleport
		UPrimitiveComponent* SavedPlayerBase = UTCharacter->GetMovementBase();
		FTransform SavedPlayerTransform = UTCharacter->GetTransform();
		if (UTCharacter->TeleportTo(WarpLocation, WarpRotation))
		{
			if (RequestingPC != nullptr)
			{
				RequestingPC->UTClientSetRotation(WarpRotation);
			}
			UTCharacter->RallyCompleteTime = GetWorld()->GetTimeSeconds();
			UTPlayerState->bRallyActivated = false;
			UTPlayerState->ForceNetUpdate();

			// spawn effects
			FActorSpawnParameters SpawnParams;
			SpawnParams.Instigator = UTCharacter;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			SpawnParams.Owner = UTCharacter;

			// announce
			AActor* RallySpot = nullptr;
			if (UTPlayerState->RallyPoint)
			{
				if (UTPlayerState->RallyPoint->LocationText.IsEmpty())
				{
					RallySpot = UTPlayerState->RallyPoint->MyGameVolume;
				}
				else
				{
					RallySpot = UTPlayerState->RallyPoint;
				}
			}
			if (RallySpot == nullptr)
			{
				UTCharacter->UTCharacterMovement->UpdatedComponent->UpdatePhysicsVolume(true);
				RallySpot = UTCharacter->UTCharacterMovement ? UTCharacter->UTCharacterMovement->GetPhysicsVolume() : nullptr;
				if ((RallySpot == nullptr) || (RallySpot == GetWorld()->GetDefaultPhysicsVolume()))
				{
					AUTBlitzFlag* CarriedFlag = Cast<AUTBlitzFlag>(GS->FlagSpawner->GetCarriedObject());
					if (CarriedFlag)
					{
						RallySpot = CarriedFlag;
					}
				}
			}
			if (UTPlayerState->RallyPoint && GS->CurrentRallyPoint == UTPlayerState->RallyPoint)
			{
				GS->bEnemyRallyPointIdentified = true;
			}
			UTPlayerState->ModifyStatsValue(NAME_Rallies, 1);
			
			for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
			{
				AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
				if (PC)
				{
					if (GS->OnSameTeam(UTPlayerState, PC))
					{
						PC->ClientReceiveLocalizedMessage(UUTCTFMajorMessage::StaticClass(), 27, UTPlayerState);
					}
					else
					{
						PC->ClientReceiveLocalizedMessage(UUTCTFMajorMessage::StaticClass(), 24, UTPlayerState, nullptr, RallySpot); 
					}
				}
			}

			if (FUTAnalytics::IsAvailable())
			{
				FUTAnalytics::FireEvent_PlayerUsedRally(this, UTPlayerState);
			}
		}
		UTCharacter->bSpawnProtectionEligible = true;
		UTCharacter->SpawnProtectionStartTime = GetWorld()->GetTimeSeconds() - GS->SpawnProtectionTime + 0.8f;
		return true;
	}
	return false;
}

void AUTFlagRunGame::HandleMatchIntermission()
{
	if (bFirstRoundInitialized)
	{
		// kick idlers
		if (UTGameState && GameSession && !bIgnoreIdlePlayers && !bIsLANGame)
		{
			for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
			{
				AUTPlayerState* UTPlayerState = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
				if (UTPlayerState && IsPlayerIdle(UTPlayerState) && Cast<APlayerController>(UTPlayerState->GetOwner()))
				{
					Cast<APlayerController>(UTPlayerState->GetOwner())->ClientWasKicked(NSLOCTEXT("General", "IdleKick", "You were kicked for being idle."));
					UTPlayerState->GetOwner()->Destroy();
				}
			}
		}

		// view defender base, with last team to score around it
		int32 TeamToWatch = IntermissionTeamToView(nullptr);

		if ((BlitzGameState == NULL) || (BlitzGameState->DeliveryPoint == nullptr))
		{
			return;
		}

		UTGameState->PrepareForIntermission();

		AActor* IntermissionFocus = SetIntermissionCameras(TeamToWatch);
		// Tell the controllers to look at defender base
		for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
		{
			AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
			if (PC != NULL)
			{
				PC->ClientPrepareForIntermission();
				PC->SetViewTarget(IntermissionFocus);
			}
		}

		UTGameState->CreateLineUp(LineUpTypes::Intermission);
	}

	if (BlitzGameState)
	{
		BlitzGameState->bIsAtIntermission = true;
		BlitzGameState->bStopGameClock = true;
		BlitzGameState->IntermissionTime = IntermissionDuration;
	}

	if (BlitzGameState && BlitzGameState->GetScoringPlays().Num() > 0)
	{
		BlitzGameState->UpdateRoundHighlights();
	}
	if ((BlitzGameState == nullptr) || (BlitzGameState->CTFRound < BlitzGameState->NumRounds - 3) || bBasicTrainingGame)
	{
		return;
	}

	// Update win requirements if last two rounds
	AUTTeamInfo* NextAttacker = (BlitzGameState->bRedToCap == BlitzGameState->IsMatchIntermission()) ? BlitzGameState->Teams[1] : BlitzGameState->Teams[0];
	AUTTeamInfo* NextDefender = (BlitzGameState->bRedToCap == BlitzGameState->IsMatchIntermission()) ? BlitzGameState->Teams[0] : BlitzGameState->Teams[1];
	int32 RequiredTime = (BlitzGameState->bRedToCap == BlitzGameState->IsMatchIntermission()) ? BlitzGameState->TiebreakValue : -1 * BlitzGameState->TiebreakValue;
	RequiredTime = FMath::Max(RequiredTime, 0);
	BlitzGameState->FlagRunMessageTeam = nullptr;
	BlitzGameState->EarlyEndTime = 0;
	if (BlitzGameState->CTFRound == BlitzGameState->NumRounds - 3)
	{
		if (NextAttacker->Score > NextDefender->Score - 1)
		{
			BlitzGameState->FlagRunMessageTeam = NextDefender;
			BlitzGameState->FlagRunMessageSwitch = FMath::Clamp(5 - (NextAttacker->Score - NextDefender->Score), 1, 3);
		}
		else if (NextAttacker->Score < NextDefender->Score - 4)
		{
			BlitzGameState->FlagRunMessageTeam = NextAttacker;
			int32 BonusType = NextDefender->Score - 4 - NextAttacker->Score;
			if (RequiredTime > 60)
			{
				BonusType++;
				RequiredTime = 0;
			}
			BonusType = FMath::Min(BonusType, 3);
			BlitzGameState->FlagRunMessageSwitch = 100 * RequiredTime + BonusType + 3;
			BlitzGameState->EarlyEndTime = 60 * (BonusType - 1) + RequiredTime;
		}
	}
	else if (BlitzGameState->CTFRound == BlitzGameState->NumRounds - 2)
	{
		if (NextAttacker->Score > NextDefender->Score)
		{
			BlitzGameState->FlagRunMessageTeam = NextDefender;
			if (NextAttacker->Score - NextDefender->Score > 2)
			{
				// Defenders must stop attackers to have a chance
				BlitzGameState->FlagRunMessageSwitch = 1;
			}
			else
			{
				int32 BonusType = (NextAttacker->Score - NextDefender->Score == 2) ? 1 : 2;
				BlitzGameState->FlagRunMessageSwitch = BonusType + 1;
			}
		}
		else if (NextDefender->Score > NextAttacker->Score)
		{
			BlitzGameState->FlagRunMessageTeam = NextAttacker;
			
			int32 BonusType = FMath::Max(1, (NextDefender->Score - NextAttacker->Score) - 1);
			if (RequiredTime > 60)
			{
				BonusType++;
				RequiredTime = 0;
			}
			BonusType = FMath::Min(BonusType, 3);
			BlitzGameState->FlagRunMessageSwitch = 100 * RequiredTime + BonusType + 3;
			BlitzGameState->EarlyEndTime = 60 * (BonusType - 1) + RequiredTime;
		}
	}
	else if (BlitzGameState->CTFRound == BlitzGameState->NumRounds - 1)
	{
		bool bNeedTimeThreshold = false;
		BlitzGameState->FlagRunMessageTeam = NextAttacker;
		if (NextDefender->Score <= NextAttacker->Score)
		{
			BlitzGameState->FlagRunMessageSwitch = 8;
		}
		else
		{
			int32 BonusType = NextDefender->Score - NextAttacker->Score;
			if (RequiredTime > 60)
			{
				BonusType++;
				RequiredTime = 0;
			}
			BlitzGameState->FlagRunMessageSwitch = 7 + BonusType + 100 * RequiredTime;
			BlitzGameState->EarlyEndTime = 60 * (BonusType - 1) + RequiredTime;
		}
	}
}

void AUTFlagRunGame::CheatScore()
{
	if ((UE_BUILD_DEVELOPMENT || (GetNetMode() == NM_Standalone)) && !bOfflineChallenge && !bBasicTrainingGame && UTGameState)
	{
		UTGameState->SetRemainingTime(FMath::RandHelper(150));
		IntermissionDuration = 12.f;
		int32 ScoringTeam = PickCheatWinTeam();
		TArray<AController*> Members = Teams[ScoringTeam]->GetTeamMembers();
		if (Members.Num() > 0)
		{
			AUTPlayerState* Scorer = Cast<AUTPlayerState>(Members[FMath::RandHelper(Members.Num())]->PlayerState);
			if (FMath::FRand() < 0.5f)
			{
				FAssistTracker NewAssist;
				NewAssist.Holder = Cast<AUTPlayerState>(Members[FMath::RandHelper(Members.Num())]->PlayerState);
				NewAssist.TotalHeldTime = 0.5f;
				BlitzGameState->FlagSpawner->GetCarriedObject()->AssistTracking.Add(NewAssist);
			}
			if (FMath::FRand() < 0.5f)
			{
				BlitzGameState->FlagSpawner->GetCarriedObject()->HolderRescuers.Add(Members[FMath::RandHelper(Members.Num())]);
			}
			if (FMath::FRand() < 0.5f)
			{
				Cast<AUTPlayerState>(Members[FMath::RandHelper(Members.Num())]->PlayerState)->LastFlagReturnTime = GetWorld()->GetTimeSeconds() - 0.1f;
			}
			ScoreObject(BlitzGameState->FlagSpawner->GetCarriedObject(), Cast<AUTCharacter>(Cast<AController>(Scorer->GetOwner())->GetPawn()), Scorer, FName("FlagCapture"));
		}
	}
}

void AUTFlagRunGame::UpdateSkillRating()
{
	if (bRankedSession)
	{
		ReportRankedMatchResults(GetRankedLeagueName());
	}
	else
	{
		ReportRankedMatchResults(NAME_FlagRunSkillRating.ToString());
	}
}

FString AUTFlagRunGame::GetRankedLeagueName()
{
	return NAME_RankedFlagRunSkillRating.ToString();
}

uint8 AUTFlagRunGame::GetNumMatchesFor(AUTPlayerState* PS, bool bInRankedSession) const
{
	return PS ? PS->FlagRunMatchesPlayed : 0;
}

int32 AUTFlagRunGame::GetEloFor(AUTPlayerState* PS, bool bInRankedSession) const
{
	return PS ? PS->FlagRunRank : Super::GetEloFor(PS, bInRankedSession);
}

void AUTFlagRunGame::SetEloFor(AUTPlayerState* PS, bool bInRankedSession, int32 NewEloValue, bool bIncrementMatchCount)
{
	if (PS)
	{
		if (bInRankedSession)
		{
			PS->RankedFlagRunRank = NewEloValue;
			if (bIncrementMatchCount && (PS->ShowdownMatchesPlayed < 255))
			{
				PS->RankedFlagRunMatchesPlayed++;
			}
		}
		else
		{
			PS->FlagRunRank = NewEloValue;
			if (bIncrementMatchCount && (PS->FlagRunMatchesPlayed < 255))
			{
				PS->FlagRunMatchesPlayed++;
			}
		}
	}
}

void AUTFlagRunGame::HandleTeamChange(AUTPlayerState* PS, AUTTeamInfo* OldTeam)
{
	// If a player doesn't have a valid selected boost powerup, lets go ahead and give them the 1st one available in the Powerup List
	if (PS && UTGameState && bAllowBoosts)
	{
		if (!PS->BoostClass || !UTGameState->IsSelectedBoostValid(PS))
		{
			TSubclassOf<class AUTInventory> SelectedBoost = UTGameState->GetSelectableBoostByIndex(PS, 0);
			PS->BoostClass = SelectedBoost;
		}
	}
	if ((GetWorld()->WorldType == EWorldType::PIE) || bDevServer || !PS || !UTGameState || (UTGameState->GetMatchState() != MatchState::InProgress))
	{
		return;
	}
	if (bSitOutDuringRound)
	{
		PS->RemainingLives = 0;
	}
	if (PS->RemainingLives == 0 && IsPlayerOnLifeLimitedTeam(PS))
	{
		PS->SetOutOfLives(true);
		PS->ForceRespawnTime = 1.f;
	}

	// verify that OldTeam and New team still have live players
	AUTTeamInfo* NewTeam = PS->Team;
	bool bOldTeamHasPlayers = false;
	bool bNewTeamHasPlayers = false;
	for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
	{
		AUTPlayerState* OtherPS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
		if (OtherPS && !OtherPS->bOutOfLives && !OtherPS->bIsInactive)
		{
			if (OldTeam && (OtherPS->Team == OldTeam))
			{
				bOldTeamHasPlayers = true;
			}
			if (NewTeam && (OtherPS->Team == NewTeam))
			{
				bNewTeamHasPlayers = true;
			}
		}
	}
	if (!bOldTeamHasPlayers && OldTeam)
	{
		ScoreAlternateWin((OldTeam->TeamIndex == 0) ? 1 : 0);
	}
	else if (!bNewTeamHasPlayers && NewTeam)
	{
		ScoreAlternateWin((NewTeam->TeamIndex == 0) ? 1 : 0);
	}
}

AActor* AUTFlagRunGame::SetIntermissionCameras(uint32 TeamToWatch)
{
	AUTFlagRunGameState* FRGS = Cast<AUTFlagRunGameState>(BlitzGameState);
	if (FRGS)
	{
		bool bWasCap = false;
		if (IsTeamOnOffense(TeamToWatch))
		{
			// check if annihilation
			bool bFoundDefender = false;
			for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
			{
				AUTPlayerState* TeamPS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
				if (TeamPS && TeamPS->Team && (TeamPS->Team->TeamIndex != TeamToWatch) && !TeamPS->bOutOfLives && !TeamPS->bIsInactive)
				{
					bFoundDefender = true;
					break;
				}
			}
			bWasCap = bFoundDefender;
		}
		if (bWasCap || (FRGS->ScoringPlayerState == nullptr))
		{
			return FRGS->DeliveryPoint;
		}
		return FRGS->ScoringPlayerState;
	}
	return nullptr;
}

bool AUTFlagRunGame::IsTeamOnOffense(int32 TeamNumber) const
{
	AUTFlagRunGameState* FRGS = Cast<AUTFlagRunGameState>(BlitzGameState);
	return FRGS && (FRGS->bRedToCap == (TeamNumber == 0));
}

void AUTFlagRunGame::SendRestartNotifications(AUTPlayerState* PS, AUTPlayerController* PC)
{
	if (PS && PS->Team && IsTeamOnOffense(PS->Team->TeamIndex))
	{
		LastAttackerSpawnTime = GetWorld()->GetTimeSeconds();
	}
	if (PC && (PS->GetRemainingBoosts() > 0))
	{
		PC->ClientReceiveLocalizedMessage(UUTCTFRoleMessage::StaticClass(), 20);
	}
}

void AUTFlagRunGame::ScoreObject_Implementation(AUTCarriedObject* GameObject, AUTCharacter* HolderPawn, AUTPlayerState* Holder, FName Reason)
{
	for (int32 i = 0; i < Teams.Num(); i++)
	{
		if (Teams[i])
		{
			Teams[i]->RoundBonus = 0;
		}
	}
	if (Reason == FName("FlagCapture"))
	{
		if (UTGameState)
		{
			// force replication of server clock time
			UTGameState->SetRemainingTime(UTGameState->GetRemainingTime());
			if (Holder && Holder->Team)
			{
				Holder->Team->RoundBonus = FMath::Min(MaxTimeScoreBonus, UTGameState->GetRemainingTime());
				UpdateTiebreak(Holder->Team->RoundBonus, Holder->Team->TeamIndex);
			}
		}
	}

	if (Holder != NULL && Holder->Team != NULL && !BlitzGameState->HasMatchEnded() && !BlitzGameState->IsMatchIntermission())
	{
		int32 NewFlagCapScore = GetFlagCapScore();
		BlitzScoring->ScoreObject(GameObject, HolderPawn, Holder, Reason, NewFlagCapScore);

		if (BaseMutator != NULL)
		{
			BaseMutator->ScoreObject(GameObject, HolderPawn, Holder, Reason);
		}
		FindAndMarkHighScorer();

		if (Reason == FName("FlagCapture"))
		{
			// Give the team a capture.
			int32 OldScore = Holder->Team->Score;
			Holder->Team->Score += NewFlagCapScore;
			Holder->Team->ForceNetUpdate();
			LastTeamToScore = Holder->Team;
			AnnounceWin(Holder->Team, Holder, 0);
			AddCaptureEventToReplay(Holder, Holder->Team);
			if (Holder->FlagCaptures == 3)
			{
				BroadcastLocalized(this, UUTAssistMessage::StaticClass(), 5, Holder, NULL, Holder->Team);
			}

			for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
			{
				AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
				if (PC)
				{
					if (BlitzGameState->FlagSpawner != nullptr)
					{
						PC->UTClientPlaySound(BlitzGameState->FlagSpawner->FlagScoreRewardSound);
					}

					AUTPlayerState* PS = Cast<AUTPlayerState>((*Iterator)->PlayerState);
					if (PS && PS->bNeedsAssistAnnouncement)
					{
						PC->SendPersonalMessage(UUTAssistMessage::StaticClass(), 2, PS, Holder, NULL);
						PS->bNeedsAssistAnnouncement = false;
					}
				}
			}
			HandleFlagCapture(HolderPawn, Holder);
			if (IsMatchInProgress())
			{
				// tell bots about the cap
				AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
				if (GS != NULL && HolderPawn != nullptr && GameObject != nullptr)
				{
					for (AUTTeamInfo* TeamIter : GS->Teams)
					{
						TeamIter->NotifyObjectiveEvent(GameObject->HomeBase, HolderPawn->Controller, FName(TEXT("FlagCap")));
					}
				}
			}
		}
	}

	if (UTGameState)
	{
		UTGameState->ScoringPlayerState = Cast<AUTPlayerState>(HolderPawn->PlayerState);
	}
}


void AUTFlagRunGame::ScoreRedAlternateWin()
{
	if (IsMatchInProgress() && (GetMatchState() != MatchState::MatchIntermission))
	{
		ScoreAlternateWin(0);
	}
}

void AUTFlagRunGame::ScoreBlueAlternateWin()
{
	if (IsMatchInProgress() && (GetMatchState() != MatchState::MatchIntermission))
	{
		ScoreAlternateWin(1);
	}
}

void AUTFlagRunGame::ScoreAlternateWin(int32 WinningTeamIndex, uint8 Reason /* = 0 */)
{
	// Find last killer on winning team - must do before super call
	if (UTGameState && (Teams.Num() > WinningTeamIndex) && Teams[WinningTeamIndex])
	{
		LastTeamToScore = Teams[WinningTeamIndex];
		float BestScore = 0.f;
		float LastKill = -1000.f;
		UTGameState->ScoringPlayerState = nullptr;
		for (int32 PlayerIdx = 0; PlayerIdx < Teams[WinningTeamIndex]->GetTeamMembers().Num(); PlayerIdx++)
		{
			if (Teams[WinningTeamIndex]->GetTeamMembers()[PlayerIdx] != nullptr)
			{
				AUTPlayerState *PS = Cast<AUTPlayerState>(Teams[WinningTeamIndex]->GetTeamMembers()[PlayerIdx]->PlayerState);
				if ((PS != nullptr) && (PS->LastKillTime >= LastKill))
				{
					PS->Score = PS->RoundKillAssists + PS->RoundKills;
					if ((UTGameState->ScoringPlayerState == nullptr) || (PS->Score > BestScore))
					{
						BestScore = PS->Score;
						LastKill = PS->LastKillTime;
						UTGameState->ScoringPlayerState = PS;
					}
				}
			}
		}
	}

	FindAndMarkHighScorer();
	AUTTeamInfo* WinningTeam = (Teams.Num() > WinningTeamIndex) ? Teams[WinningTeamIndex] : NULL;
	if (WinningTeam)
	{
		if (Reason != 2)
		{
			WinningTeam->Score += IsTeamOnOffense(WinningTeamIndex) ? GetFlagCapScore() : GetDefenseScore();
		}
		if (BlitzGameState)
		{
			for (int32 i = 0; i < Teams.Num(); i++)
			{
				if (Teams[i])
				{
					Teams[i]->RoundBonus = 0;
				}
			}
			if (Reason != 2)
			{
				WinningTeam->RoundBonus = FMath::Min(MaxTimeScoreBonus, BlitzGameState->GetRemainingTime());
				UpdateTiebreak(WinningTeam->RoundBonus, WinningTeam->TeamIndex);
			}

			FCTFScoringPlay NewScoringPlay;
			NewScoringPlay.Team = WinningTeam;
			NewScoringPlay.bDefenseWon = !IsTeamOnOffense(WinningTeamIndex);
			NewScoringPlay.Period = BlitzGameState->CTFRound;
			NewScoringPlay.bAnnihilation = (Reason == 0);
			NewScoringPlay.TeamScores[0] = BlitzGameState->Teams[0] ? BlitzGameState->Teams[0]->Score : 0;
			NewScoringPlay.TeamScores[1] = BlitzGameState->Teams[1] ? BlitzGameState->Teams[1]->Score : 0;
			NewScoringPlay.RemainingTime = BlitzGameState->GetRemainingTime();
			NewScoringPlay.RedBonus = BlitzGameState->Teams[0] ? BlitzGameState->Teams[0]->RoundBonus : 0;
			NewScoringPlay.BlueBonus = BlitzGameState->Teams[1] ? BlitzGameState->Teams[1]->RoundBonus : 0;
			BlitzGameState->AddScoringPlay(NewScoringPlay);

			// force replication of server clock time
			BlitzGameState->SetRemainingTime(BlitzGameState->GetRemainingTime());
		}

		WinningTeam->ForceNetUpdate();
		LastTeamToScore = WinningTeam;
		AnnounceWin(WinningTeam, nullptr, Reason);
		CheckForWinner(LastTeamToScore);
		if (UTGameState->IsMatchInProgress())
		{
			SetMatchState(MatchState::MatchIntermission);
		}

		if (FUTAnalytics::IsAvailable())
		{
			if (GetWorld()->GetNetMode() != NM_Standalone)
			{
				TArray<FAnalyticsEventAttribute> ParamArray;
				ParamArray.Add(FAnalyticsEventAttribute(TEXT("FlagCapScore"), 0));
				FUTAnalytics::SetMatchInitialParameters(this, ParamArray, true);
				FUTAnalytics::GetProvider().RecordEvent(TEXT("RCTFRoundResult"), ParamArray);
			}
		}
	}
}

void AUTFlagRunGame::UpdateTiebreak(int32 Bonus, int32 TeamIndex)
{
	AUTFlagRunGameState* RBlitzGameState = Cast<AUTFlagRunGameState>(BlitzGameState);
	if (RBlitzGameState)
	{
		if (Bonus == MaxTimeScoreBonus)
		{
			Bonus = 60;
		}
		else
		{
			while (Bonus > 59)
			{
				Bonus -= 60;
			}
		}
		if (TeamIndex == 0)
		{
			RBlitzGameState->TiebreakValue += Bonus;
		}
		else
		{
			RBlitzGameState->TiebreakValue -= Bonus;
		}
	}
}

bool AUTFlagRunGame::SupportsInstantReplay() const
{
	return true;
}

void AUTFlagRunGame::FindAndMarkHighScorer()
{
	for (int32 i = 0; i < Teams.Num(); i++)
	{
		int32 BestScore = 0;

		for (int32 PlayerIdx = 0; PlayerIdx < Teams[i]->GetTeamMembers().Num(); PlayerIdx++)
		{
			if (Teams[i]->GetTeamMembers()[PlayerIdx] != nullptr)
			{
				AUTPlayerState *PS = Cast<AUTPlayerState>(Teams[i]->GetTeamMembers()[PlayerIdx]->PlayerState);
				if (PS != nullptr)
				{
					PS->Score = PS->RoundKillAssists + PS->RoundKills;
					if (BestScore == 0 || PS->Score > BestScore)
					{
						BestScore = PS->Score;
					}
				}
			}
		}

		for (int32 PlayerIdx = 0; PlayerIdx < Teams[i]->GetTeamMembers().Num(); PlayerIdx++)
		{
			if (Teams[i]->GetTeamMembers()[PlayerIdx] != nullptr)
			{
				AUTPlayerState *PS = Cast<AUTPlayerState>(Teams[i]->GetTeamMembers()[PlayerIdx]->PlayerState);
				if (PS != nullptr)
				{
					bool bOldHighScorer = PS->bHasHighScore;
					PS->bHasHighScore = (BestScore == PS->Score) && (BestScore > 0);
					if ((bOldHighScorer != PS->bHasHighScore) && (GetNetMode() != NM_DedicatedServer))
					{
						PS->OnRep_HasHighScore();
					}
				}
			}
		}
	}
}

void AUTFlagRunGame::HandleRollingAttackerRespawn(AUTPlayerState* OtherPS)
{
	OtherPS->RespawnWaitTime = RollingAttackerRespawnDelay;
	AUTFlagRunGameState* GS = GetWorld()->GetGameState<AUTFlagRunGameState>();
	int32 RoundTime = (TimeLimit == 0) ? 300 : TimeLimit;
	if (GS && !GS->bAttackersCanRally && GS->bHaveEstablishedFlagRunner && !GS->CurrentRallyPoint && (GS->GetRemainingTime() < RoundTime - 45))
	{
		OtherPS->AnnounceStatus(StatusMessage::NeedRally);
	}
	else if (GS && GS->CurrentRallyPoint && GS->bAttackersCanRally && (GS->CurrentRallyPoint->RallyTimeRemaining > 3.5f))
	{
		OtherPS->RespawnWaitTime = FMath::Min(OtherPS->RespawnWaitTime, GS->CurrentRallyPoint->RallyTimeRemaining - 2.f);
	}
}

void AUTFlagRunGame::PlayEndOfMatchMessage()
{
}

bool AUTFlagRunGame::PlayerWonChallenge()
{
	// make sure player is on best team
	APlayerController* LocalPC = GEngine->GetFirstLocalPlayerController(GetWorld());
	AUTPlayerState* PS = LocalPC ? Cast<AUTPlayerState>(LocalPC->PlayerState) : NULL;
	return PS && PS->Team && (PS->Team == UTGameState->WinningTeam);
}

void AUTFlagRunGame::CheckGameTime()
{
	AUTFlagRunGameState* RBlitzGameState = Cast<AUTFlagRunGameState>(BlitzGameState);
	if (RBlitzGameState && RBlitzGameState->IsMatchIntermission())
	{
		if (RBlitzGameState->IntermissionTime <= 0)
		{
			SetMatchState(MatchState::MatchExitingIntermission);
		}
	}
	else if ((GetMatchState() == MatchState::InProgress) && TimeLimit > 0)
	{
		CheckRoundTimeVictory();
	}
	else
	{
		if (BlitzGameState->IsMatchIntermission())
		{
			if (BlitzGameState->GetIntermissionTime() <= 0)
			{
				SetMatchState(MatchState::MatchExitingIntermission);
			}
		}
	}
}

void AUTFlagRunGame::EndPlayerIntro()
{
	BeginGame();
}

uint8 AUTFlagRunGame::GetWinningTeamForLineUp() const
{
	uint8 Result = Super::GetWinningTeamForLineUp();
	if (Result == 255)
	{
		if (FlagScorer != nullptr)
		{
			Result = FlagScorer->GetTeamNum();
		}
		else if (BlitzGameState != nullptr && BlitzGameState->GetScoringPlays().Num() > 0)
		{
			const TArray<const FCTFScoringPlay>& ScoringPlays = BlitzGameState->GetScoringPlays();
			const FCTFScoringPlay& WinningPlay = ScoringPlays.Last();

			if (WinningPlay.Team)
			{
				Result = WinningPlay.Team->GetTeamNum();
			}
		}
	}
	return Result;
}

void AUTFlagRunGame::RestartPlayer(AController* aPlayer)
{
	if ((!IsMatchInProgress() && bPlacingPlayersAtIntermission) || (GetMatchState() == MatchState::MatchIntermission) || (UTGameState && UTGameState->IsLineUpActive() && UTGameState->ActiveLineUpHelper && UTGameState->ActiveLineUpHelper->bIsPlacingPlayers))
	{
		// placing players during intermission
		if (bPlacingPlayersAtIntermission || (UTGameState && UTGameState->IsLineUpActive()))
		{
			AGameMode::RestartPlayer(aPlayer);
		}
		return;
	}
	AUTPlayerState* PS = Cast<AUTPlayerState>(aPlayer->PlayerState);
	AUTPlayerController* PC = Cast<AUTPlayerController>(aPlayer);
	if (PS && PS->Team && HasMatchStarted())
	{
		if (IsPlayerOnLifeLimitedTeam(PS) && (PS->RemainingLives == 0) && (GetMatchState() == MatchState::InProgress))
		{
			// failsafe for player that leaves match before RemainingLives are set and then rejoins
			PS->SetOutOfLives(true);
		}
		if (PS->bOutOfLives)
		{
			if (PC != NULL)
			{
				PC->ChangeState(NAME_Spectating);
				PC->ClientGotoState(NAME_Spectating);

				for (AController* Member : PS->Team->GetTeamMembers())
				{
					if (Member->GetPawn() != NULL)
					{
						PC->ServerViewPlayerState(Member->PlayerState);
						break;
					}
				}
			}
			return;
		}
		if (IsPlayerOnLifeLimitedTeam(PS))
		{
			if ((PS->RemainingLives > 0) && IsMatchInProgress() && (GetMatchState() != MatchState::MatchIntermission))
			{
				if (PS->RemainingLives == 1)
				{
					if (PC)
					{
						PC->ClientReceiveLocalizedMessage(UUTShowdownStatusMessage::StaticClass(), 5, PS, NULL, NULL);
					}
					PS->AnnounceStatus(StatusMessage::LastLife, 0, true);
					PS->RespawnWaitTime = 0.5f;
					PS->ForceNetUpdate();
					PS->OnRespawnWaitReceived();
				}
				else if (PC)
				{
					PC->ClientReceiveLocalizedMessage(UUTShowdownStatusMessage::StaticClass(), 30 + PS->RemainingLives, PS, NULL, NULL);
				}
			}
			else
			{
				return;
			}
		}
	}
	SendRestartNotifications(PS, PC);
	Super::RestartPlayer(aPlayer);
}

void AUTFlagRunGame::SetPlayerStateInactive(APlayerState* NewPlayerState)
{
	Super::SetPlayerStateInactive(NewPlayerState);
	AUTPlayerState* PS = Cast<AUTPlayerState>(NewPlayerState);
	if (PS && !PS->bOnlySpectator && UTGameState && UTGameState->IsMatchIntermission())
	{
		PS->RemainingLives = RoundLives;
	}
	if (PS)
	{
		PS->ClearRoundStats();
	}
}

void AUTFlagRunGame::BuildServerResponseRules(FString& OutRules)
{
	OutRules += FString::Printf(TEXT("Goal Score\t%i\t"), GoalScore);

	AUTMutator* Mut = BaseMutator;
	while (Mut)
	{
		OutRules += FString::Printf(TEXT("Mutator\t%s\t"), *Mut->DisplayName.ToString());
		Mut = Mut->NextMutator;
	}
}

void AUTFlagRunGame::HandleFlagCapture(AUTCharacter* HolderPawn, AUTPlayerState* Holder)
{
	FlagScorer = Holder;
	CheckScore(Holder);
	if (UTGameState && UTGameState->IsMatchInProgress())
	{
		Holder->AddCoolFactorEvent(400.0f);

		SetMatchState(MatchState::MatchIntermission);
	}
}

int32 AUTFlagRunGame::IntermissionTeamToView(AUTPlayerController* PC)
{
	if (LastTeamToScore)
	{
		return LastTeamToScore->TeamIndex;
	}
	if (PC && !PC->PlayerState->bOnlySpectator && (PC->GetTeamNum() < Teams.Num()))
	{
		return PC->GetTeamNum();
	}

	return (Teams[1]->Score > Teams[0]->Score) ? 1 : 0;
}

void AUTFlagRunGame::HandleExitingIntermission()
{
	BlitzGameState->bStopGameClock = false;
	BlitzGameState->HalftimeScoreDelay = 3.f;
	RemoveAllPawns();

	if (bFirstRoundInitialized)
	{
		IntermissionSwapSides();
	}
	else
	{
		BlitzGameState->CTFRound = 0;
	}

	InitRound();
	if (!bFirstRoundInitialized)
	{
		bFirstRoundInitialized = true;
		if (Super::UTIsHandlingReplays() && GetGameInstance() != nullptr)
		{
			GetGameInstance()->StartRecordingReplay(TEXT(""), GetWorld()->GetMapName());
		}
	}

	//now respawn all the players
	for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
	{
		AController* Controller = Iterator->Get();
		if (Controller->PlayerState != NULL && !Controller->PlayerState->bOnlySpectator)
		{
			RestartPlayer(Controller);

			// Reset group taunt
			AUTPlayerState* PS = Cast<AUTPlayerState>(Controller->PlayerState);
			if (PS)
			{
				PS->ActiveGroupTaunt = nullptr;
				PS->ClearRoundStats();
			}
		}
	}

	// Send all flags home..
	BlitzGameState->ResetFlags();
	BlitzGameState->bIsAtIntermission = false;
	BlitzGameState->SetTimeLimit(TimeLimit);		// Reset the GameClock for the second time.
	SetMatchState(MatchState::InProgress);

	UTGameState->ClearLineUp();
}

void AUTFlagRunGame::ScoreKill_Implementation(AController* Killer, AController* Other, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType)
{
	BlitzScoring->ScoreKill(Killer, Other, KilledPawn, DamageType);
	if (Killer != NULL)
	{
		AUTPlayerState* AttackerPS = Cast<AUTPlayerState>(Killer->PlayerState);
		if (Killer != Other)
		{
			AddKillEventToReplay(Killer, Other, DamageType);
			if (AttackerPS != NULL)
			{
				if (!bFirstBloodOccurred)
				{
					BroadcastLocalized(this, UUTFirstBloodMessage::StaticClass(), 0, AttackerPS, NULL, NULL);
					bFirstBloodOccurred = true;
				}
				AUTPlayerState* OtherPlayerState = Other ? Cast<AUTPlayerState>(Other->PlayerState) : NULL;
				AttackerPS->IncrementKills(DamageType, true, OtherPlayerState);
				TrackKillAssists(Killer, Other, KilledPawn, DamageType, AttackerPS, OtherPlayerState);
			}
		}
		else
		{
			if (AttackerPS != nullptr)
			{
				AttackerPS->ModifyStatsValue(NAME_Suicides, 1);
			}
		}
	}
	if (BaseMutator != NULL)
	{
		BaseMutator->ScoreKill(Killer, Other, DamageType);
	}
	FindAndMarkHighScorer();

	AUTPlayerState* OtherPS = Other ? Cast<AUTPlayerState>(Other->PlayerState) : nullptr;
	if (OtherPS && OtherPS->Team && IsTeamOnOffense(OtherPS->Team->TeamIndex) && bRollingAttackerSpawns)
	{
		HandleRollingAttackerRespawn(OtherPS);
		OtherPS->RespawnWaitTime += 0.01f * FMath::FRand();
		OtherPS->ForceNetUpdate();
		OtherPS->OnRespawnWaitReceived();
	}
	if (OtherPS && IsPlayerOnLifeLimitedTeam(OtherPS) && (OtherPS->RemainingLives > 0))
	{
		OtherPS->RemainingLives--;
		bool bEliminated = false;
		if (OtherPS->RemainingLives == 0)
		{
			// this player is out of lives
			OtherPS->SetOutOfLives(true);
			bEliminated = true;
			bool bFoundTeammate = false;
			for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
			{
				AUTPlayerState* TeamPS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
				if (TeamPS && (OtherPS->Team == TeamPS->Team) && !TeamPS->bOutOfLives && !TeamPS->bIsInactive)
				{
					// found a live teammate, so round isn't over - notify about termination though
					if (IsMatchInProgress() && (GetMatchState() != MatchState::MatchIntermission))
					{
						BroadcastLocalized(NULL, UUTShowdownRewardMessage::StaticClass(), 3, OtherPS);
					}
					bFoundTeammate = true;
					break;
				}
			}
			if (!bFoundTeammate)
			{
				BroadcastLocalized(NULL, UUTShowdownRewardMessage::StaticClass(), 4);
				BlitzGameState->bStopGameClock = true;

				if (OtherPS->Team->TeamIndex == 0)
				{
					FTimerHandle TempHandle;
					GetWorldTimerManager().SetTimer(TempHandle, this, &AUTFlagRunGame::ScoreBlueAlternateWin, 1.f);
				}
				else
				{
					FTimerHandle TempHandle;
					GetWorldTimerManager().SetTimer(TempHandle, this, &AUTFlagRunGame::ScoreRedAlternateWin, 1.f);

				}
			}
		}
		else if (UTGameState && IsMatchInProgress() && (GetMatchState() != MatchState::MatchIntermission))
		{
			bool bFoundLiveTeammate = false;
			int32 TeamCount = 0;
			for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
			{
				AUTPlayerState* TeamPS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
				if (TeamPS && (OtherPS->Team == TeamPS->Team) && !TeamPS->bOutOfLives && !TeamPS->bIsInactive)
				{
					TeamCount++;
					if (TeamPS->GetUTCharacter() && !TeamPS->GetUTCharacter()->IsDead())
					{
						bFoundLiveTeammate = true;
						break;
					}
				}
			}
			if (!bFoundLiveTeammate && (TeamCount == 5) && (GetWorld()->GetTimeSeconds() - LastAceTime > 20.f))
			{
				LastAceTime = GetWorld()->GetTimeSeconds();
				BroadcastLocalized(NULL, UUTCTFRewardMessage::StaticClass(), 8);
			}
		}

		int32 RemainingDefenders = 0;
		int32 RemainingLives = 0;

		// check if just transitioned to last man
		bool bWasAlreadyLastMan = bLastManOccurred;
		AUTPlayerState* LastMan = nullptr;
		for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
			if (PS && (OtherPS->Team == PS->Team) && !PS->bOutOfLives && !PS->bIsInactive)
			{
				RemainingDefenders++;
				RemainingLives += PS->RemainingLives;
				LastMan = PS;
			}
		}
		bLastManOccurred = (RemainingDefenders == 1);
		if (bLastManOccurred && !bWasAlreadyLastMan)
		{
			for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
			{
				AUTPlayerState* PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
				AUTPlayerController* PC = PS ? Cast<AUTPlayerController>(PS->GetOwner()) : nullptr;
				if (PC)
				{
					int32 MessageType = (OtherPS->Team == PS->Team) ? 1 : 0;
					PC->ClientReceiveLocalizedMessage(UUTShowdownRewardMessage::StaticClass(), MessageType, LastMan, NULL, NULL);
				}
			}
		}
		else if (((RemainingDefenders == 3) && bEliminated) || (RemainingLives < 10))
		{
			// find player on other team to speak message
			AUTPlayerState* Speaker = nullptr;
			for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
			{
				AUTPlayerState* TeamPS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
				if (TeamPS && TeamPS->Team && (OtherPS->Team != TeamPS->Team) && !TeamPS->bOutOfLives && !TeamPS->bIsInactive)
				{
					Speaker = TeamPS;
					break;
				}
			}
			if (Speaker != nullptr)
			{
				if ((RemainingDefenders == 3) && bEliminated)
				{
					Speaker->AnnounceStatus(StatusMessage::EnemyThreePlayers);
				}
				else if (RemainingLives == 9)
				{
					Speaker->AnnounceStatus(StatusMessage::EnemyLowLives);
				}
			}
		}

		if (OtherPS->RemainingLives > 0)
		{
			OtherPS->RespawnWaitTime = FMath::Max(1.f, float(RemainingDefenders));
			if (UTGameState && UTGameState->GetRemainingTime() > 150)
			{
				OtherPS->RespawnWaitTime = FMath::Min(OtherPS->RespawnWaitTime, 2.f);
			}
			OtherPS->RespawnWaitTime += 0.01f * FMath::FRand();
			OtherPS->ForceNetUpdate();
			OtherPS->OnRespawnWaitReceived();
		}
	}
}

void AUTFlagRunGame::InitRound()
{
	FlagScorer = nullptr;
	bFirstBloodOccurred = false;
	bLastManOccurred = false;
	InitGameStateForRound();
	ResetFlags();
	if (FlagPickupDelay > 0)
	{
		if (BlitzGameState && BlitzGameState->FlagSpawner && BlitzGameState->FlagSpawner->MyFlag)
		{
			InitDelayedFlag(BlitzGameState->FlagSpawner->MyFlag);
		}
		FTimerHandle TempHandle;
		GetWorldTimerManager().SetTimer(TempHandle, this, &AUTFlagRunGame::FlagCountDown, 1.f*GetActorTimeDilation(), false);
		FTimerHandle RampHandle;
		if (BlitzGameState)
		{
			BlitzGameState->RampStartTime = FMath::Max(int32(RampUpTime[BlitzGameState->CTFRound % RampUpMusic.Num()] + 0.5f), 1);
			GetWorldTimerManager().SetTimer(RampHandle, this, &AUTFlagRunGame::PlayRampUpMusic, FlagPickupDelay - RampUpTime[BlitzGameState->CTFRound % RampUpMusic.Num()], false);
		}
	}
	else
	{
		InitFlags();
	}

	for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
		InitPlayerForRound(PS);
	}
	BlitzGameState->SetTimeLimit(TimeLimit);

	// re-initialize all AI squads, in case objectives have changed sides
	for (AUTTeamInfo* Team : Teams)
	{
		Team->ReinitSquads();
	}
}

void AUTFlagRunGame::PlayRampUpMusic()
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
		if (PC != nullptr)
		{
			PC->UTClientPlaySound(RampUpMusic[BlitzGameState->CTFRound % RampUpMusic.Num()]);
		}
	}
}

void AUTFlagRunGame::PlayEndMusic()
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
		if (PC != nullptr)
		{
			PC->UTClientPlaySound(EndMusic);
		}
	}
}

void AUTFlagRunGame::ResetFlags()
{
	if (BlitzGameState)
	{
		if (BlitzGameState->FlagSpawner && BlitzGameState->FlagSpawner->MyFlag)
		{
			BlitzGameState->FlagSpawner->MyFlag->SetActorHiddenInGame(true);
		}
		if (BlitzGameState->DeliveryPoint)
		{
			BlitzGameState->DeliveryPoint->SpawnDefenseEffect();
		}
	}
}

void AUTFlagRunGame::InitPlayerForRound(AUTPlayerState* PS)
{
	if (PS)
	{
		PS->RoundKills = 0;
		PS->RoundDeaths = 0;
		PS->RoundKillAssists = 0;
		PS->bRallyActivated = false;
		PS->RespawnWaitTime = IsPlayerOnLifeLimitedTeam(PS) ? LimitedRespawnWaitTime : UnlimitedRespawnWaitTime;
		PS->SetRemainingBoosts(0);
		PS->bSpecialTeamPlayer = false;
		PS->bSpecialPlayer = false;
		if (GetNetMode() != NM_DedicatedServer)
		{
			PS->OnRepSpecialPlayer();
			PS->OnRepSpecialTeamPlayer();
		}
		if (PS && (!PS->Team || PS->bOnlySpectator))
		{
			PS->RemainingLives = 0;
			PS->SetOutOfLives(true);
		}
		else if (PS)
		{
			PS->RemainingLives = RoundLives;
			PS->SetOutOfLives(false);
		}
		PS->ForceNetUpdate();
	}
}

bool AUTFlagRunGame::IsTeamOnDefense(int32 TeamNumber) const
{
	return !IsTeamOnOffense(TeamNumber);
}

bool AUTFlagRunGame::IsPlayerOnLifeLimitedTeam(AUTPlayerState* PlayerState) const
{
	return PlayerState && PlayerState->Team && IsTeamOnDefense(PlayerState->Team->TeamIndex);
}

void AUTFlagRunGame::EndTeamGame(AUTTeamInfo* Winner, FName Reason)
{
	// Don't ever end the game in PIE
	if (GetWorld()->WorldType == EWorldType::PIE) return;

	UTGameState->WinningTeam = Winner;
	EndTime = GetWorld()->TimeSeconds;

	APlayerController* LocalPC = GEngine->GetFirstLocalPlayerController(GetWorld());
	UUTLocalPlayer* LP = LocalPC ? Cast<UUTLocalPlayer>(LocalPC->Player) : NULL;
	if (LP)
	{
		LP->EarnedStars = 0;
		LP->RosterUpgradeText = FText::GetEmpty();
		if (bOfflineChallenge && PlayerWonChallenge())
		{
			LP->ChallengeCompleted(ChallengeTag, ChallengeDifficulty + 1);
		}
	}

	if (IsGameInstanceServer() && LobbyBeacon)
	{
		FString MatchStats = FString::Printf(TEXT("%i"), BlitzGameState->ElapsedTime);

		FMatchUpdate MatchUpdate;
		MatchUpdate.GameTime = UTGameState->ElapsedTime;
		MatchUpdate.NumPlayers = NumPlayers;
		MatchUpdate.NumSpectators = NumSpectators;
		MatchUpdate.MatchState = MatchState;
		MatchUpdate.bMatchHasBegun = HasMatchStarted();
		MatchUpdate.bMatchHasEnded = HasMatchEnded();

		UpdateLobbyScore(MatchUpdate);
		LobbyBeacon->EndGame(MatchUpdate);
	}

	// SETENDGAMEFOCUS
	EndMatch();
	AActor* EndMatchFocus = SetIntermissionCameras(Winner->TeamIndex);

	AUTGameObjective* WinningBase = Cast<AUTGameObjective>(EndMatchFocus);
	for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
	{
		AUTPlayerController* Controller = Cast<AUTPlayerController>(*Iterator);
		if (Controller && Controller->UTPlayerState)
		{
			AUTGameObjective* BaseToView = WinningBase;
			// If we don't have a winner, view my base
			if (BaseToView == NULL)
			{
				AUTTeamInfo* MyTeam = Controller->UTPlayerState->Team;
				if (MyTeam)
				{
					BaseToView = BlitzGameState->DeliveryPoint;
				}
			}

			if (BaseToView)
			{
				if (UTGameState->IsLineUpActive())
				{
					Controller->GameHasEnded(Controller->GetPawn(), (Controller->UTPlayerState->Team && (Controller->UTPlayerState->Team->TeamIndex == Winner->TeamIndex)));
				}
				else
				{
					Controller->GameHasEnded(BaseToView, (Controller->UTPlayerState->Team && (Controller->UTPlayerState->Team->TeamIndex == Winner->TeamIndex)));
				}
			}
		}
	}

	// Allow replication to happen before reporting scores, stats, etc.
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, this, &AUTGameMode::HandleMatchHasEnded, 1.5f);
	bGameEnded = true;

	// Setup a timer to continue to the next map.  Need enough time for match summaries
	EndTime = GetWorld()->TimeSeconds;
	float TravelDelay = GetTravelDelay();
	FTimerHandle TempHandle3;
	GetWorldTimerManager().SetTimer(TempHandle3, this, &AUTGameMode::TravelToNextMap, TravelDelay*GetActorTimeDilation());

	FTimerHandle TempHandle4;
	float EndReplayDelay = TravelDelay - 10.f;
	GetWorldTimerManager().SetTimer(TempHandle4, this, &AUTFlagRunGame::StopRCTFReplayRecording, EndReplayDelay);

	SendEndOfGameStats(Reason);
}

void AUTFlagRunGame::StopRCTFReplayRecording()
{
	if (Super::UTIsHandlingReplays() && GetGameInstance() != nullptr)
	{
		GetGameInstance()->StopRecordingReplay();
	}
}

void AUTFlagRunGame::FlagCountDown()
{
	AUTFlagRunGameState* RBlitzGameState = Cast<AUTFlagRunGameState>(BlitzGameState);
	if (RBlitzGameState && IsMatchInProgress() && (MatchState != MatchState::MatchIntermission))
	{
		RBlitzGameState->RemainingPickupDelay--;
		if (RBlitzGameState->RemainingPickupDelay > 0)
		{
			FTimerHandle TempHandle;
			GetWorldTimerManager().SetTimer(TempHandle, this, &AUTFlagRunGame::FlagCountDown, 1.f*GetActorTimeDilation(), false);
		}
		else
		{
			FlagsAreReady();
		}
	}
}

void AUTFlagRunGame::FlagsAreReady()
{
	BroadcastLocalized(this, UUTCTFMajorMessage::StaticClass(), 21, NULL, NULL, NULL);
	InitFlags();
}

void AUTFlagRunGame::AddCaptureEventToReplay(AUTPlayerState* Holder, AUTTeamInfo* Team)
{
	UDemoNetDriver* DemoNetDriver = GetWorld()->DemoNetDriver;
	if (DemoNetDriver != nullptr && DemoNetDriver->ServerConnection == nullptr)
	{
		TArray<uint8> Data;

		FString PlayerName = Holder ? *Holder->PlayerName : TEXT("None");
		PlayerName.ReplaceInline(TEXT(" "), TEXT("%20"));

		FString CapInfo = FString::Printf(TEXT("%s"), *PlayerName);

		FMemoryWriter MemoryWriter(Data);
		MemoryWriter.Serialize(TCHAR_TO_ANSI(*CapInfo), CapInfo.Len() + 1);

		FString MetaTag = FString::FromInt(Team->TeamIndex);

		DemoNetDriver->AddEvent(TEXT("FlagCaps"), MetaTag, Data);
	}
}

void AUTFlagRunGame::AddReturnEventToReplay(AUTPlayerState* Returner, AUTTeamInfo* Team)
{
	UDemoNetDriver* DemoNetDriver = GetWorld()->DemoNetDriver;
	if (Returner && DemoNetDriver != nullptr && DemoNetDriver->ServerConnection == nullptr)
	{
		TArray<uint8> Data;

		FString PlayerName = Returner ? *Returner->PlayerName : TEXT("None");
		PlayerName.ReplaceInline(TEXT(" "), TEXT("%20"));

		FString ReturnInfo = FString::Printf(TEXT("%s"), *PlayerName);

		FMemoryWriter MemoryWriter(Data);
		MemoryWriter.Serialize(TCHAR_TO_ANSI(*ReturnInfo), ReturnInfo.Len() + 1);

		FString MetaTag = Returner->StatsID;
		if (MetaTag.IsEmpty())
		{
			MetaTag = Returner->PlayerName;
		}
		DemoNetDriver->AddEvent(TEXT("FlagReturns"), MetaTag, Data);
	}
}

void AUTFlagRunGame::AddDeniedEventToReplay(APlayerState* KillerPlayerState, AUTPlayerState* Holder, AUTTeamInfo* Team)
{
	UDemoNetDriver* DemoNetDriver = GetWorld()->DemoNetDriver;
	if (DemoNetDriver != nullptr && DemoNetDriver->ServerConnection == nullptr)
	{
		TArray<uint8> Data;

		FString PlayerName = KillerPlayerState ? *KillerPlayerState->PlayerName : TEXT("None");
		PlayerName.ReplaceInline(TEXT(" "), TEXT("%20"));

		FString HolderName = Holder ? *Holder->PlayerName : TEXT("None");
		HolderName.ReplaceInline(TEXT(" "), TEXT("%20"));

		FString DenyInfo = FString::Printf(TEXT("%s %s"), *PlayerName, *HolderName);

		FMemoryWriter MemoryWriter(Data);
		MemoryWriter.Serialize(TCHAR_TO_ANSI(*DenyInfo), DenyInfo.Len() + 1);

		FString MetaTag = FString::FromInt(Team->TeamIndex);

		DemoNetDriver->AddEvent(TEXT("FlagDeny"), MetaTag, Data);
	}
}

void AUTFlagRunGame::ScoreDamage_Implementation(int32 DamageAmount, AUTPlayerState* Victim, AUTPlayerState* Attacker)
{
	Super::ScoreDamage_Implementation(DamageAmount, Victim, Attacker);
	BlitzScoring->ScoreDamage(DamageAmount, Victim, Attacker);
}

void AUTFlagRunGame::GameObjectiveInitialized(AUTGameObjective* Obj)
{
	if (BlitzGameState)
	{
		BlitzGameState->CacheGameObjective(Obj);
	}
}

void AUTFlagRunGame::CallMatchStateChangeNotify()
{
	Super::CallMatchStateChangeNotify();

	if (MatchState == MatchState::MatchIntermission)
	{
		HandleMatchIntermission();
	}
	else if (MatchState == MatchState::MatchExitingIntermission)
	{
		HandleExitingIntermission();
	}
}

void AUTFlagRunGame::EndGame(AUTPlayerState* Winner, FName Reason)
{
	// Dont ever end the game in PIE
	if (GetWorld()->WorldType == EWorldType::PIE) return;

	Super::EndGame(Winner, Reason);

	// Send all of the flags home...
	BlitzGameState->ResetFlags();
}

void AUTFlagRunGame::SetEndGameFocus(AUTPlayerState* Winner)
{
	if (!GetWorld() || !BlitzGameState)
	{
		return;
	}
	int32 WinnerTeamNum = Winner ? Winner->GetTeamNum() : (LastTeamToScore ? LastTeamToScore->TeamIndex : 0);
	AUTGameObjective* WinningBase = NULL;
	WinningBase = BlitzGameState->DeliveryPoint;

	for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
	{
		AUTPlayerController* Controller = Cast<AUTPlayerController>(*Iterator);
		if (Controller && Controller->UTPlayerState)
		{
			AUTGameObjective* BaseToView = WinningBase;
			// If we don't have a winner, view my base
			if (BaseToView == NULL)
			{
				AUTTeamInfo* MyTeam = Controller->UTPlayerState->Team;
				if (MyTeam)
				{
					BaseToView = BlitzGameState->DeliveryPoint;
				}
			}

			if (BaseToView)
			{
				Controller->GameHasEnded(BaseToView, (Controller->UTPlayerState->Team && (Controller->UTPlayerState->Team->TeamIndex == WinnerTeamNum)));
			}
		}
	}
}

