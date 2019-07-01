// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"

#include "GameFramework/GameMode.h"
#include "UTDeathMessage.h"
#include "UTGameMessage.h"
#include "UTVictoryMessage.h"
#include "UTTimedPowerup.h"
#include "UTCountDownMessage.h"
#include "UTFirstBloodMessage.h"
#include "UTSpectatorPickupMessage.h"
#include "UTMutator.h"
#include "UTScoreboard.h"
#include "SlateBasics.h"
#include "UTAnalytics.h"
#include "UTBotPlayer.h"
#include "UTMonsterAI.h"
#include "UTSquadAI.h"
#include "Dialogs/SUTPlayerInfoDialog.h"
#include "Slate/SlateGameResources.h"
#include "Widgets/SUTTabWidget.h"
#include "SNumericEntryBox.h"
#include "UTCharacterContent.h"
#include "UTGameEngine.h"
#include "UTWorldSettings.h"
#include "UTLevelSummary.h"
#include "UTHUD_CastingGuide.h"
#include "UTBotCharacter.h"
#include "UTReplicatedMapInfo.h"
#include "UTReplicatedGameRuleset.h"
#include "StatNames.h"
#include "UTWeap_ImpactHammer.h"
#include "UTWeap_Translocator.h"
#include "UTWeap_Enforcer.h"
#include "Engine/DemoNetDriver.h"
#include "EngineBuildSettings.h"
#include "UTEngineMessage.h"
#include "UTRemoteRedeemer.h"
#include "UTChallengeManager.h"
#include "DataChannel.h"
#include "UTGameInstance.h"
#include "UTDemoRecSpectator.h"
#include "UTMcpUtils.h"
#include "UTGameSessionRanked.h"
#include "UTGameSessionNonRanked.h"
#include "UTPlayerStart.h"
#include "UTPlaceablePowerup.h"
#include "SUTSpawnWindow.h"
#include "AnalyticsEventAttribute.h"
#include "IAnalyticsProvider.h"
#include "UTSupplyChest.h"
#include "UTGameVolume.h"
#include "UTArmor.h"
#include "UTLineUpZone.h"
#include "UTLineUpHelper.h"
#include "UTRewardMessage.h"
#include "UTVoiceChatTokenFeature.h"

DEFINE_LOG_CATEGORY(LogUTGame);

const float IDLE_TIMEOUT_TIME = 120.0f;
const float RETURNING_PLAYER_GRACE_PERIOD = 10.0f;

UUTResetInterface::UUTResetInterface(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{}

namespace MatchState
{
	const FName PlayerIntro = FName(TEXT("PlayerIntro"));
	const FName CountdownToBegin = FName(TEXT("CountdownToBegin"));
	const FName MatchEnteringOvertime = FName(TEXT("MatchEnteringOvertime"));
	const FName MatchIsInOvertime = FName(TEXT("MatchIsInOvertime"));
	const FName MapVoteHappening = FName(TEXT("MapVoteHappening"));
	const FName MatchIntermission = FName(TEXT("MatchIntermission"));
	const FName MatchExitingIntermission = FName(TEXT("MatchExitingIntermission"));
	const FName MatchRankedAbandon = FName(TEXT("MatchRankedAbandon"));
	const FName WaitingTravel = FName(TEXT("WaitingTravel"));
}

AUTGameMode::AUTGameMode(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PlayerPawnObject = FStringAssetReference(TEXT("/Game/RestrictedAssets/Blueprints/DefaultCharacter.DefaultCharacter_C"));

	// use our custom HUD class
	HUDClass = AUTHUD::StaticClass();
	CastingGuideHUDClass = AUTHUD_CastingGuide::StaticClass();

	GameStateClass = AUTGameState::StaticClass();
	PlayerStateClass = AUTPlayerState::StaticClass();
	EngineMessageClass = UUTEngineMessage::StaticClass();

	PlayerControllerClass = AUTPlayerController::StaticClass();
	BotClass = AUTBotPlayer::StaticClass();

	TimeLimit = 15;
	bUseSeamlessTravel = false;
	CountDown = 3;
	MatchIntroTime = 10.f;
	bPauseable = false;
	RespawnWaitTime = 2.f;
	ForceRespawnTime = 5.f;
	StartDelay = 10.f;
	LastMatchNotReady = 0.f;
	DefaultMaxPlayers = 10;
	bNoDefaultLeaderHat = true;

	bHasRespawnChoices = false;
	MaxWaitForPlayers = 120;
	QuickWaitForPlayers = 120;
	ShortWaitForPlayers = 60;
	MatchSummaryDelay = 16.f;
	MatchSummaryTime = 20.f;
	BotFillCount = 0;
	WarmupFillCount = 0;
	bWeaponStayActive = true;
	VictoryMessageClass = UUTVictoryMessage::StaticClass();
	DeathMessageClass = UUTDeathMessage::StaticClass();
	GameMessageClass = UUTGameMessage::StaticClass();
	SquadType = AUTSquadAI::StaticClass();
	MaxSquadSize = 3;
	bClearPlayerInventory = false;
	bDelayedStart = true;
	bDamageHurtsHealth = true;
	bAmmoIsLimited = true;
	bAllowOvertime = true;
	bForceRespawn = false;
	XPMultiplier = 1.0f;
	XPCapPerMin = 20;
	GameDifficulty = 3.f;
	StartPlayTime = 10000000.f;
	bRequireReady = false;
	bRequireFull = false;
	bRemovePawnsAtStart = true;

	DefaultPlayerName = FText::FromString(TEXT("Player"));
	MapPrefix = TEXT("DM");
	LobbyInstanceID = 0;
	DemoFilename = TEXT("%m-%td");
	bDedicatedInstance = false;

	MapVoteTime = 30;
	LastGlobalTauntTime = -1000.f;

	bSpeedHackDetection = false;
	MaxTimeMargin = 0.25f;
	MinTimeMargin = -0.25f;
	TimeMarginSlack = 0.1f;

	bCasterControl = false;
	bCasterReady = false;
	bOfflineChallenge = false;
	bBasicTrainingGame = false;

	bDisableMapVote = false;
	AntiCheatEngine = nullptr;
	EndOfMatchMessageDelay = 1.f;
	bAllowAllArmorPickups = true;
	bTrackKillAssists = true;
	WarmupKills = 0;

	bPlayersStartWithArmor = true;
	StartingArmorObject = FStringAssetReference(TEXT("/Game/RestrictedAssets/Blueprints/Armor_Starting.Armor_Starting_C"));

	ImpactHammerObject = FStringAssetReference(TEXT("/Game/RestrictedAssets/Weapons/ImpactHammer/BP_ImpactHammer.BP_ImpactHammer_C"));
	bGameHasImpactHammer = true;
}

float AUTGameMode::OverrideRespawnTime(AUTPickupInventory* Pickup, TSubclassOf<AUTInventory> InventoryType)
{
	if (bBasicTrainingGame && !bDamageHurtsHealth && (GetNetMode() == NM_Standalone) && InventoryType.GetDefaultObject()->IsA(AUTWeap_Enforcer::StaticClass()))
	{
		return 2.f;
	}
	return (Pickup && InventoryType) ? InventoryType.GetDefaultObject()->RespawnTime : 0.f;
}

void AUTGameMode::NotifySpeedHack(ACharacter* Character)
{
	AUTPlayerController* PC = Character ? Cast < AUTPlayerController>(Character->GetController()) : NULL;
	if (PC)
	{
		PC->ClientReceiveLocalizedMessage(GameMessageClass, 15);
	}
}

void AUTGameMode::BeginPlayMutatorHack(UObject* Context, FFrame& Stack, RESULT_DECL )
{
	P_FINISH;

	// WARNING: 'this' is actually an AActor! Only do AActor things!
    if (!IsA(ALevelScriptActor::StaticClass()) && !IsA(AUTMutator::StaticClass()) &&
		(RootComponent == NULL || RootComponent->Mobility != EComponentMobility::Static || (!IsA(AStaticMeshActor::StaticClass()) && !IsA(ALight::StaticClass()))) )
	{
		AUTGameMode* Game = GetWorld()->GetAuthGameMode<AUTGameMode>();
		// a few type checks being AFTER the CheckRelevance() call is intentional; want mutators to be able to modify, but not outright destroy
		if (Game != NULL && Game != this && !Game->CheckRelevance((AActor*)this) && !IsA(APlayerController::StaticClass()))
		{
			Destroy();
		}
    }
}

bool AUTGameMode::AllowCheats(APlayerController* P)
{
	return (GetNetMode() == NM_Standalone || GIsEditor) && !bOfflineChallenge && !bBasicTrainingGame;
}

void AUTGameMode::Demigod()
{
	bDamageHurtsHealth = !bDamageHurtsHealth;
}

// Parse options for this game...
void AUTGameMode::InitGame( const FString& MapName, const FString& Options, FString& ErrorMessage )
{
	// HACK: workaround to inject CheckRelevance() into the BeginPlay sequence
	UFunction* Func = AActor::GetClass()->FindFunctionByName(FName(TEXT("ReceiveBeginPlay")));
	Func->FunctionFlags |= FUNC_Native;
    //Func->SetNativeFunc(AUTGameMode::BeginPlayMutatorHack);

	if (bGameHasImpactHammer && !ImpactHammerObject.IsNull())
	{
        ImpactHammerClass = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), NULL, *ImpactHammerObject.ToSoftObjectPath().ToString(), NULL, LOAD_NoWarn));
		DefaultInventory.Add(ImpactHammerClass);
	}

	UE_LOG(UT,Log,TEXT("==============="));
	UE_LOG(UT,Log,TEXT("  Init Game Option: %s"), *Options);

	ReturningPlayerGraceCutoff = GetWorld()->GetRealTimeSeconds() + RETURNING_PLAYER_GRACE_PERIOD;

	if (IOnlineSubsystem::Get() != NULL)
	{
		IOnlineEntitlementsPtr EntitlementInterface = IOnlineSubsystem::Get()->GetEntitlementsInterface();
		if (EntitlementInterface.IsValid())
		{
			FOnQueryEntitlementsCompleteDelegate Delegate;
			Delegate.BindUObject(this, &AUTGameMode::EntitlementQueryComplete);
			EntitlementInterface->AddOnQueryEntitlementsCompleteDelegate_Handle(Delegate);
		}
	}

	// Must happen before Super call because GetGameSessionClass() is switched on this
	FString InOpt = UGameplayStatics::ParseOption(Options, TEXT("Ranked"));
	if (!InOpt.IsEmpty())
	{
		bRankedSession = true;
	}
	else
	{
		bRankedSession = false;
	}
	bUseMatchmakingSession = EvalBoolOptions(UGameplayStatics::ParseOption(Options, TEXT("MatchmakingSession")), false);

	bIsQuickMatch = EvalBoolOptions(UGameplayStatics::ParseOption(Options, TEXT("QuickMatch")), false);
	if (bIsQuickMatch)
	{
		MaxWaitForPlayers = QuickWaitForPlayers;
	}
	if (UGameplayStatics::HasOption(Options, TEXT("NextMap")) || UGameplayStatics::HasOption(Options, TEXT("LAN")))
	{
		MaxWaitForPlayers = ShortWaitForPlayers;
	}

	Super::InitGame(MapName, Options, ErrorMessage);

	GameDifficulty = FMath::Max(0, UGameplayStatics::GetIntOption(Options, TEXT("Difficulty"), GameDifficulty));
	RedTeamSkill = GameDifficulty;
	BlueTeamSkill = GameDifficulty;

	HostLobbyListenPort = UGameplayStatics::GetIntOption(Options, TEXT("HostPort"), 14000);
	InOpt = UGameplayStatics::ParseOption(Options, TEXT("ForceRespawn"));
	bForceRespawn = EvalBoolOptions(InOpt, bForceRespawn);

	MaxWaitForPlayers = UGameplayStatics::GetIntOption(Options, TEXT("MaxPlayerWait"), MaxWaitForPlayers);

	TimeLimit = FMath::Max(0, UGameplayStatics::GetIntOption(Options, TEXT("TimeLimit"), TimeLimit));
	TimeLimit *= 60;

	// Set goal score to end match.
	GoalScore = FMath::Max(0, UGameplayStatics::GetIntOption(Options, TEXT("GoalScore"), GoalScore));
	if (bBasicTrainingGame)
	{
		GoalScore = 0;
		bNoTrainingMenu = (UGameplayStatics::GetIntOption(Options, TEXT("NoTutMenu"), bNoTrainingMenu) == 1) ? true : false;
	}

	InOpt = UGameplayStatics::ParseOption(Options, TEXT("RankCheck"));
	if (!InOpt.IsEmpty())
	{
		bRankLocked = true;
		RankCheck = UGameplayStatics::GetIntOption(Options, TEXT("RankCheck"), DEFAULT_RANK_CHECK);
	}
	else
	{
		bRankLocked = false;
	}

	RespawnWaitTime = FMath::Max(0, UGameplayStatics::GetIntOption(Options, TEXT("RespawnWait"), RespawnWaitTime));
	
	InOpt = UGameplayStatics::ParseOption(Options, TEXT("Hub"));
	if (!InOpt.IsEmpty()) HubAddress = InOpt;

	InOpt = UGameplayStatics::ParseOption(Options, TEXT("HubKey"));
	if (!InOpt.IsEmpty()) HubKey = InOpt;

	bRequireReady = (UGameplayStatics::GetIntOption(Options, TEXT("RequireReady"), bRequireReady) == 0) ? false : true;
	bRequireFull = (UGameplayStatics::GetIntOption(Options, TEXT("RequireFull"), bRequireFull) == 0) ? false : true;
	bForceWarmup = (UGameplayStatics::GetIntOption(Options, TEXT("ForceWarmup"), bForceWarmup) == 0) ? false : true;

	bDebugHitScanReplication = UGameplayStatics::HasOption(Options, TEXT("HitScanDebug"));

	if (!UGameplayStatics::HasOption(Options, TEXT("MaxPlayers")))
	{
		GameSession->MaxPlayers = DefaultMaxPlayers;
	}
	else if (UGameplayStatics::HasOption(Options, TEXT("LAN")))
	{
		BotFillCount = GameSession->MaxPlayers;
	}

	// alias for testing convenience
	if (UGameplayStatics::HasOption(Options, TEXT("Bots")))
	{
		BotFillCount = UGameplayStatics::GetIntOption(Options, TEXT("Bots"), BotFillCount) + 1;
		if (BotFillCount > 0)
		{
			GameSession->MaxPlayers = BotFillCount;
		}
	}
	else if (UGameplayStatics::HasOption(Options, TEXT("BotFill")))
	{
		BotFillCount = UGameplayStatics::GetIntOption(Options, TEXT("BotFill"), BotFillCount);
		if (BotFillCount > 0)
		{
			GameSession->MaxPlayers = BotFillCount;
		}
	}
	bForceNoBots = (UGameplayStatics::GetIntOption(Options, TEXT("ForceNoBots"), bForceNoBots) == 0) ? false : true;
	bIsVSAI = (UGameplayStatics::GetIntOption(Options, TEXT("VSAI"), bIsVSAI) == 0) ? false : true;
	bForceNoBots = bForceNoBots && !bIsVSAI;
	if (bForceNoBots)
	{
		BotFillCount = 0;
	}
	bAutoAdjustBotSkill = (bBasicTrainingGame || bIsQuickMatch) && !bIsVSAI;
	InOpt = UGameplayStatics::ParseOption(Options, TEXT("CasterControl"));
	bCasterControl = EvalBoolOptions(InOpt, bCasterControl);

	for (int32 i = 0; i < BuiltInMutators.Num(); i++)
	{
		AddMutatorClass(BuiltInMutators[i]);
	}
	
	for (int32 i = 0; i < ConfigMutators.Num(); i++)
	{
		AddMutator(ConfigMutators[i]);
	}

	InOpt = UGameplayStatics::ParseOption(Options, TEXT("Mutator"));
	if (InOpt.Len() > 0)
	{
		UE_LOG(UT, Log, TEXT("Mutators: %s"), *InOpt);
		while (InOpt.Len() > 0)
		{
			FString LeftOpt;
			int32 Pos = InOpt.Find(TEXT(","));
			if (Pos > 0)
			{
				LeftOpt = InOpt.Left(Pos);
				InOpt = InOpt.Right(InOpt.Len() - Pos - 1);
			}
			else
			{
				LeftOpt = InOpt;
				InOpt.Empty();
			}
			AddMutator(LeftOpt);
		}
	}

	InOpt = UGameplayStatics::ParseOption(Options, TEXT("Demorec"));
	if (InOpt.Len() > 0)
	{
		bRecordDemo = InOpt != TEXT("off") && InOpt != TEXT("false") && InOpt != TEXT("0") && InOpt != TEXT("no") && InOpt != GFalse.ToString() && InOpt != GNo.ToString();
		if (bRecordDemo)
		{
			DemoFilename = InOpt;
		}
	}

	InOpt = UGameplayStatics::ParseOption(Options, TEXT("Dev"));
	bDevServer = EvalBoolOptions(InOpt, bDevServer) || (GetWorld()->WorldType == EWorldType::PIE);

	if (UGameplayStatics::HasOption(Options, TEXT("Challenge")) && (GetNetMode() == NM_Standalone))
	{
		InOpt = UGameplayStatics::ParseOption(Options, TEXT("Challenge"));
		if (!InOpt.IsEmpty())
		{
			TWeakObjectPtr<UUTChallengeManager> ChallengeManager = Cast<UUTGameEngine>(GEngine)->GetChallengeManager();
			if (ChallengeManager.IsValid())
			{
				ChallengeTag = FName(*InOpt);
				ChallengeDifficulty = UGameplayStatics::GetIntOption(Options, TEXT("ChallengeDiff"), 0);
				GameDifficulty = 1.f + 2.5f*ChallengeDifficulty;
				BotFillCount = ChallengeManager->GetNumPlayers(this);
				bOfflineChallenge = ChallengeManager->IsValidChallenge(this, MapName);
				bForceRespawn = true;
				TimeLimit = 480; 
				GoalScore = 0;
			}
		}
	}

	InOpt = UGameplayStatics::ParseOption(Options, TEXT("Proto"));
	bUseProtoTeams = EvalBoolOptions(InOpt, bUseProtoTeams);

	PostInitGame(Options);

	UE_LOG(UT, Log, TEXT("LobbyInstanceID: %i"), LobbyInstanceID);
	UE_LOG(UT, Log, TEXT("=================="));

	// If we are a lobby instance, establish a communication beacon with the lobby.  For right now, this beacon is created on the local host
	// but in time, the lobby's address will have to be passed
	RecreateLobbyBeacon();
	
	static const FName AntiCheatFeatureName("AntiCheat");
	if (IModularFeatures::Get().IsModularFeatureAvailable(AntiCheatFeatureName))
	{
		AntiCheatEngine = &IModularFeatures::Get().GetModularFeature<UTAntiCheatModularFeature>(AntiCheatFeatureName);
	}

	if (bUseMatchmakingSession)
	{
		AUTGameSession* UTGameSession = Cast<AUTGameSession>(GameSession);
		GetWorldTimerManager().SetTimer(ServerRestartTimerHandle, UTGameSession, &AUTGameSession::CheckForPossibleRestart, 60.0f, true);
	}

	InOpt = UGameplayStatics::ParseOption(Options, TEXT("StartArmor"));
	bPlayersStartWithArmor = EvalBoolOptions(InOpt, bPlayersStartWithArmor);
	if (!StartingArmorObject.IsNull())
	{
        StartingArmorClass = Cast<UClass>(StaticLoadObject(UClass::StaticClass(), NULL, *StartingArmorObject.ToSoftObjectPath().ToString(), NULL, LOAD_NoWarn));
	}

	if (UGameplayStatics::HasOption(Options, TEXT("TutorialMask")))
	{
		TutorialMask = UGameplayStatics::GetIntOption(Options, TEXT("TutorialMask"), 0);
	}

	bool bHasAnalyticsLoggedGameStart = EvalBoolOptions(UGameplayStatics::ParseOption(Options, FUTAnalytics::AnalyticsLoggedGameOption), false);
	if (!bHasAnalyticsLoggedGameStart && FUTAnalytics::IsAvailable() && GetWorld() && (GetNetMode() != ENetMode::NM_DedicatedServer))
	{
		FUTAnalytics::FireEvent_EnterMatch(Cast<AUTPlayerController>(GetWorld()->GetFirstPlayerController()), FString::Printf(TEXT("Console - %s"), *DisplayName.ToString()));
	}

	if (((GetNetMode() == NM_Standalone) || bIsLANGame) && (BotFillCount == 0) && !bRankedSession && !bOfflineChallenge && !bForceNoBots && !bDevServer && (GetWorld()->WorldType != EWorldType::PIE))
	{
		// if not competitive match, fill standalone/LAN match with bots
		BotFillCount = FMath::Max(BotFillCount, AdjustedBotFillCount());
	}

	// Handle any associated ruleset
	InOpt = UGameplayStatics::ParseOption(Options, TEXT("ART"));
	if (!InOpt.IsEmpty()) ActiveRuleTag = InOpt;

	if ((GetNetMode() != NM_Standalone) && !bRankedSession && !bForceNoBots && (GetWorld()->WorldType != EWorldType::PIE))
	{
		WarmupFillCount = FMath::Min(GameSession->MaxPlayers, bIsVSAI ? 3 : 6);
	}
}

void AUTGameMode::AddMutator(const FString& MutatorPath)
{
	int32 PeriodIndex = INDEX_NONE;
	if (MutatorPath.Right(2) != FString(TEXT("_C")) && MutatorPath.FindChar(TCHAR('.'), PeriodIndex))
	{
		FName MutatorModuleName = FName(*MutatorPath.Left(PeriodIndex));
		if (!FModuleManager::Get().IsModuleLoaded(MutatorModuleName))
		{
            /*if (!FModuleManager::Get().LoadModule(MutatorModuleName).IsValid())
			{
				UE_LOG(UT, Warning, TEXT("Failed to load module for mutator %s"), *MutatorModuleName.ToString());
            }*/
		}
	}
	TSubclassOf<AUTMutator> MutClass = LoadClass<AUTMutator>(NULL, *MutatorPath, NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
	if (MutClass == NULL && !MutatorPath.Contains(TEXT(".")))
	{
		// use asset registry to try to find shorthand name
		static FName NAME_GeneratedClass(TEXT("GeneratedClass"));
		if (MutatorAssets.Num() == 0)
		{
			GetAllBlueprintAssetData(AUTMutator::StaticClass(), MutatorAssets);

			// create fake asset entries for native classes
			for (TObjectIterator<UClass> It; It; ++It)
			{
				if (It->IsChildOf(AUTMutator::StaticClass()) && It->HasAnyClassFlags(CLASS_Native) && !It->HasAnyClassFlags(CLASS_Abstract))
				{
					FAssetData NewData;
					NewData.AssetName = It->GetFName();

					TMap<FName, FString> TagsAndValuesMap;
					TagsAndValuesMap.Add(NAME_GeneratedClass, It->GetPathName());
                    //NewData.TagsAndValues = MakeSharedMapView(MoveTemp(TagsAndValuesMap)); What is the purpose of MakeSharedMapView?

					MutatorAssets.Add(NewData);
				}
			}
		}
			
		for (const FAssetData& Asset : MutatorAssets)
		{
			if (Asset.AssetName == FName(*MutatorPath) || Asset.AssetName == FName(*FString(TEXT("Mutator_") + MutatorPath)) || Asset.AssetName == FName(*FString(TEXT("UTMutator_") + MutatorPath)))
			{
                const FString* ClassPath = &Asset.TagsAndValues.FindTag(NAME_GeneratedClass).GetValue();
				if (ClassPath != NULL)
				{
					MutClass = LoadObject<UClass>(NULL, **ClassPath);
					if (MutClass != NULL)
					{
						break;
					}
				}
			}
		}
	}
	if (MutClass == NULL)
	{
		UE_LOG(UT, Warning, TEXT("Failed to find or load mutator '%s'"), *MutatorPath);
	}
	else
	{
		AddMutatorClass(MutClass);
	}
}

void AUTGameMode::AddMutatorClass(TSubclassOf<AUTMutator> MutClass)
{
	if (MutClass != NULL && AllowMutator(MutClass))
	{
		AUTMutator* NewMut = GetWorld()->SpawnActor<AUTMutator>(MutClass);
		if (NewMut != NULL)
		{
			NewMut->Init(OptionsString);
			if (BaseMutator == NULL)
			{
				BaseMutator = NewMut;
			}
			else
			{
				BaseMutator->AddMutator(NewMut);
			}
		}
	}
}

bool AUTGameMode::AllowMutator(TSubclassOf<AUTMutator> MutClass)
{
	const AUTMutator* DefaultMut = MutClass.GetDefaultObject();

	for (AUTMutator* Mut = BaseMutator; Mut != NULL; Mut = Mut->NextMutator)
	{
		if (Mut->GetClass() == MutClass)
		{
			// already have this exact mutator
			UE_LOG(UT, Log, TEXT("Rejected mutator %s - already have one"), *MutClass->GetPathName());
			return false;
		}
		for (int32 i = 0; i < Mut->GroupNames.Num(); i++)
		{
			for (int32 j = 0; j < DefaultMut->GroupNames.Num(); j++)
			{
				if (Mut->GroupNames[i] == DefaultMut->GroupNames[j])
				{
					// group conflict
					UE_LOG(UT, Log, TEXT("Rejected mutator %s - already have mutator %s with group %s"), *MutClass->GetPathName(), *Mut->GetPathName(), *Mut->GroupNames[i].ToString());
					return false;
				}
			}
		}
	}
	return true;
}

void AUTGameMode::InitGameState()
{
	Super::InitGameState();

	UTGameState = Cast<AUTGameState>(GameState);
	if (UTGameState != NULL)
	{
		UTGameState->SetGoalScore(GoalScore);
		UTGameState->SetTimeLimit(0);
		UTGameState->SetRespawnWaitTime(RespawnWaitTime);
		UTGameState->ForceRespawnTime = ForceRespawnTime;
		UTGameState->bTeamGame = bTeamGame;
		UTGameState->bRankedSession = bRankedSession;
		UTGameState->bIsQuickMatch = bIsQuickMatch;
		UTGameState->bWeaponStay = bWeaponStayActive;
		UTGameState->bIsInstanceServer = IsGameInstanceServer();
		UTGameState->bDebugHitScanReplication = bDebugHitScanReplication;
		UTGameState->bRequireFull = bRequireFull;
		if (bOfflineChallenge || bRankedSession || bBasicTrainingGame || bIsVSAI)
		{
			UTGameState->bAllowTeamSwitches = false;
		}
		if (bIsVSAI)
		{
			UTGameState->AIDifficulty = 1;
			if (GameDifficulty > 2.f)
			{
				UTGameState->AIDifficulty = (GameDifficulty > 4.f) ? 3 : 2;
			}
		}

		UTGameState->MatchID = FGuid::NewGuid();
	}
	else
	{
		UE_LOG(UT,Error, TEXT("UTGameState is NULL %s"), *GameStateClass->GetFullName());
	}

	if (FUTAnalytics::IsAvailable())
	{
		FUTAnalytics::FireEvent_UTInitMatch(this);
	}

	RegisterServerWithSession();
}

void AUTGameMode::RegisterServerWithSession()
{
	if (GameSession != NULL && GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		AUTGameSessionNonRanked* UTGameSessionNonRanked = Cast<AUTGameSessionNonRanked>(GameSession);
		AUTGameSessionRanked* UTGameSessionRanked = Cast<AUTGameSessionRanked>(GameSession);
		if (UTGameSessionNonRanked)
		{
			if (IsGameInstanceServer())
			{
				// Set the server name to reflect the game mode and map
				UTGameState->ServerName = FString::Printf(TEXT("%s on %s"),*DisplayName.ToString(), *GetWorld()->GetMapName());
			}

			UTGameSessionNonRanked->RegisterServer();
			FTimerHandle TempHandle;
			GetWorldTimerManager().SetTimer(TempHandle, this, &AUTGameMode::UpdateOnlineServer, 60.0f*GetActorTimeDilation());

			if (UTGameSessionNonRanked->bSessionValid)
			{
				NotifyLobbyGameIsReady();
			}
		}
		else if (UTGameSessionRanked)
		{
			FOnlineSessionSettings* SessionSettings = NULL;
			const IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
			if (OnlineSub)
			{
				const IOnlineSessionPtr SessionInt = OnlineSub->GetSessionInterface();
				if (SessionInt.IsValid())
				{
					SessionSettings = SessionInt->GetSessionSettings(GameSessionName);
				}
			}

			if (SessionSettings)
			{
				UTGameSessionRanked->UpdatePlayerNeedsStatus();

				// Init the host beacon again to continue receiving reservation requests
				UTGameSessionRanked->InitHostBeacon(SessionSettings);

				// Update the session settings (to include the new game mode setting)
				UTGameSessionRanked->UpdateSession(GameSessionName, *SessionSettings);

				int32 TeamCount, TeamSize, MaxPartySize;
				UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(GetGameInstance());
				if (UTGameInstance && UTGameInstance->GetPlaylistManager() &&
					UTGameInstance->GetPlaylistManager()->GetMaxTeamInfoForPlaylist(CurrentPlaylistId, TeamCount, TeamSize, MaxPartySize))
				{
					ExpectedPlayerCount = TeamCount * TeamSize;
				}
			}
			else
			{
				UTGameSessionRanked->RegisterServer();
			}
		}
	}
}

void AUTGameMode::UpdateOnlineServer()
{
	if (GameSession &&  GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		AUTGameSession* GS = Cast<AUTGameSession>(GameSession);
		if (GS)
		{
			GS->UpdateGameState();
		}
	}
}

void AUTGameMode::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	// Because of the behavior changes to PostBeginPlay() this really has to go here as PreInitializeCompoennts is sort of the UE4 PBP even
	// though PBP still exists.  It can't go in InitGame() or InitGameState() because team info needed for team locked GameObjectives are not
	// setup at that point.

	for (TActorIterator<AUTGameObjective> ObjIt(GetWorld()); ObjIt; ++ObjIt)
	{
		GameObjectiveInitialized(*ObjIt);
		ObjIt->InitializeObjective();
	}
}

void AUTGameMode::GameObjectiveInitialized(AUTGameObjective* Obj)
{
}

APlayerController* AUTGameMode::Login(UPlayer* NewPlayer, ENetRole InRemoteRole, const FString& Portal, const FString& Options, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	bool bCastingView = EvalBoolOptions(UGameplayStatics::ParseOption(Options, TEXT("CastingView")), false);
	if (bCastingView)
	{
		// we allow the casting split views to ignore certain restrictions, so make sure they aren't lying
		UChildConnection* ChildConn = Cast<UChildConnection>(NewPlayer);
		if ( ChildConn == NULL || ChildConn->Parent == NULL || Cast<AUTPlayerController>(ChildConn->Parent->PlayerController) == NULL ||
			!((AUTPlayerController*)ChildConn->Parent->PlayerController)->bCastingGuide )
		{
			ErrorMessage = TEXT("Illegal URL options");
			return NULL;
		}
	}

	FString ModdedPortal = Portal;
	FString ModdedOptions = Options;
	if (BaseMutator != NULL)
	{
		BaseMutator->ModifyLogin(ModdedPortal, ModdedOptions);
	}
	APlayerController* Result = Super::Login(NewPlayer, InRemoteRole, Portal, Options, UniqueId, ErrorMessage);
	if (Result != NULL)
	{
		AUTPlayerController* UTPC = Cast<AUTPlayerController>(Result);
		if (UTPC != NULL)
		{
			UTPC->bCastingGuide = EvalBoolOptions(UGameplayStatics::ParseOption(Options, TEXT("CastingGuide")), false);
			// TODO: check if allowed?
			if (UTPC->bCastingGuide)
			{
				UTPC->PlayerState->bOnlySpectator = true;
				UTPC->CastingGuideViewIndex = 0;
			}

			if (bCastingView)
			{
				UChildConnection* ChildConn = Cast<UChildConnection>(NewPlayer);
				UTPC->CastingGuideViewIndex = (ChildConn != NULL) ? (ChildConn->Parent->Children.Find(ChildConn) + 1) : 0;
			}

			SendVoiceChatLoginToken(UTPC);
		}
		AUTPlayerState* PS = Cast<AUTPlayerState>(Result->PlayerState);
		if (PS != NULL)
		{
			PS->NotIdle();

			if (bUseProtoTeams)
			{
				FString InOpt = UGameplayStatics::ParseOption(Options, TEXT("PlayerCard"));
				if (InOpt.Len() > 0)
				{
					PS->SetPlayerCard(InOpt);
				}
				else if (!PS->ClanName.IsEmpty())
				{
					PS->SetPlayerCard(PS->ClanName);
				}
			}
			else
			{
				FString InOpt = UGameplayStatics::ParseOption(Options, TEXT("Character"));
				if (InOpt.Len() > 0)
				{
					PS->SetCharacter(InOpt);
				}
			}

			// warning: blindly calling this here relies on ValidateEntitlements() defaulting to "allow" if we have not yet obtained this user's entitlement information
			PS->ValidateEntitlements();

			bool bCaster = EvalBoolOptions(UGameplayStatics::ParseOption(Options, TEXT("Caster")), false);
			if (bCaster && bCasterControl)
			{
				PS->bCaster = true;
				PS->bOnlySpectator = true;
			}
			
			// don't consider this player for Elo calculations if he joins too late.
			PS->bSkipELO = bPastELOLimit && Cast<AUTGameSession>(GameSession) && !((AUTGameSession *)(GameSession))->bNoJoinInProgress;

			PS->PartySize = UGameplayStatics::GetIntOption(Options, TEXT("PartySize"), 1);
			PS->PartyLeader = UGameplayStatics::ParseOption(Options, TEXT("PartyLeader"));

            UE_LOG(LogOnlineParty, Display, TEXT("%s joined with Party Leader %s"), *PS->GetPlayerName(), *PS->PartyLeader);
		}
	}

	if (AntiCheatEngine)
	{
		AntiCheatEngine->OnPlayerLogin(Result, Options, UniqueId.GetUniqueNetId());
	}
	
	return Result;
}

void AUTGameMode::SendVoiceChatLoginToken(AUTPlayerController* PC)
{
	static const FName VoiceChatTokenFeatureName("VoiceChatToken");
	if (IModularFeatures::Get().IsModularFeatureAvailable(VoiceChatTokenFeatureName))
	{
		UTVoiceChatTokenFeature* VoiceChatToken = &IModularFeatures::Get().GetModularFeature<UTVoiceChatTokenFeature>(VoiceChatTokenFeatureName);

		AUTPlayerState* PS = Cast<AUTPlayerState>(PC->PlayerState);
		if (PS)
		{
			PC->VoiceChatPlayerName = FString::Printf(TEXT(".%s."), *PS->UniqueId.ToString());

			VoiceChatToken->GenerateClientLoginToken(PC->VoiceChatPlayerName, PC->VoiceChatLoginToken);
#if WITH_EDITOR
			if (PC->IsLocalPlayerController())
			{
				PC->OnRepVoiceChatLoginToken();
			}
#endif
		}
	}
}

void AUTGameMode::EntitlementQueryComplete(bool bWasSuccessful, const FUniqueNetId& UniqueId, const FString& Namespace, const FString& ErrorMessage)
{
	// validate player's custom options
	// note that it is possible that they have not entered the game yet, since this is started via PreLogin() - in that case we'll validate from Login()
	for (APlayerState* PS : GameState->PlayerArray)
	{
		if (PS->UniqueId.IsValid() && *PS->UniqueId.GetUniqueNetId().Get() == UniqueId)
		{
			AUTPlayerState* UTPS = Cast<AUTPlayerState>(PS);
			if (UTPS != NULL)
			{
				UTPS->ValidateEntitlements();
			}
		}
	}
}

UUTBotCharacter* AUTGameMode::ChooseRandomCharacter(uint8 TeamNum)
{
	UUTBotCharacter* ChosenCharacter = NULL;
	if (EligibleBots.Num() > 0)
	{
		int32 BestMatch = FMath::RandHelper(EligibleBots.Num() - 1);
		if (EligibleBots[BestMatch]->MinimumSkill > GameDifficulty + EligibleBots[BestMatch]->SkillAdjust)
		{
			BestMatch = BestMatch = FMath::RandHelper(BestMatch - 1);
		}
		ChosenCharacter = EligibleBots[BestMatch];
		EligibleBots.RemoveAt(BestMatch);
	}
	return ChosenCharacter;
}

AUTBotPlayer* AUTGameMode::AddBot(uint8 TeamNum)
{
	AUTBotPlayer* NewBot = GetWorld()->SpawnActor<AUTBotPlayer>(BotClass);
	if (NewBot != NULL)
	{
		UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
		if (UTEngine)
		{
			if (UTEngine->BotAssets.Num() == 0)
			{
				GetAllAssetData(UUTBotCharacter::StaticClass(), UTEngine->BotAssets);
			}
			if (EligibleBots.Num() == 0)
			{
				for (const FAssetData& Asset : UTEngine->BotAssets)
				{
					UUTBotCharacter* EligibleCharacter = Cast<UUTBotCharacter>(Asset.GetAsset());
					if (EligibleCharacter && (bUseProtoTeams || !EligibleCharacter->bProtoBot))
					{
						EligibleBots.Add(EligibleCharacter);
					}
				}

				EligibleBots.Sort([](const UUTBotCharacter& A, const UUTBotCharacter& B) -> bool
				{
					return A.SkillAdjust < B.SkillAdjust;
				});
			}
		}
		UUTBotCharacter* SelectedCharacter = NULL;
		int32 TotalStars = 0;

		if (UTEngine)
		{
			TWeakObjectPtr<UUTChallengeManager> ChallengeManager = UTEngine->GetChallengeManager();
			if (bOfflineChallenge && ChallengeManager.IsValid())
			{
				APlayerController* LocalPC = GEngine->GetFirstLocalPlayerController(GetWorld());
				UUTLocalPlayer* LP = LocalPC ? Cast<UUTLocalPlayer>(LocalPC->Player) : NULL;
				TotalStars = LP ? LP->GetTotalChallengeStars() : 0;
				SelectedCharacter = ChallengeManager->ChooseBotCharacter(this, TeamNum, TotalStars);
			}
		}
		if (bUseProtoTeams)
		{
			if (UTEngine && (BlueProtoIndex < RedProtoIndex) && (BlueProtoIndex < ProtoBlue.Num()))
			{
				SelectedCharacter = UTEngine->FindBotAsset(ProtoBlue[BlueProtoIndex]);
				BlueProtoIndex++;
			}
			else if (UTEngine && (RedProtoIndex < ProtoRed.Num()))
			{
				SelectedCharacter = UTEngine->FindBotAsset(ProtoRed[RedProtoIndex]);
				RedProtoIndex++;
			}
			AUTPlayerState* PS = Cast<AUTPlayerState>(NewBot->PlayerState);
			if (PS && SelectedCharacter)
			{
				PS->PlayerCard = SelectedCharacter;
			}
		}
		if (SelectedCharacter == NULL)
		{
			SelectedCharacter = ChooseRandomCharacter(TeamNum);
		}

		if (SelectedCharacter != NULL)
		{
			NewBot->InitializeCharacter(SelectedCharacter);
			if (bOfflineChallenge && (TeamNum != 1) && (TotalStars < 6) && (ChallengeDifficulty == 0))
			{
				// make easy bots extra easy till earn 5 stars
				NewBot->InitializeSkill(0.125f * TotalStars);
			}
			else
			{
				NewBot->InitializeSkill(GameDifficulty + SelectedCharacter->SkillAdjust);
			}
		}
		else
		{
			UE_LOG(UT, Warning, TEXT("AddBot(): No BotCharacters defined that are appropriate for game difficulty %f"), GameDifficulty);
			static int32 NameCount = 0;
			NewBot->PlayerState->SetPlayerName(FString(TEXT("TestBot")) + ((NameCount > 0) ? FString::Printf(TEXT("_%i"), NameCount) : TEXT("")));
			NewBot->InitializeSkill(GameDifficulty);
			NameCount++;
		}

		NumBots++;
		ChangeTeam(NewBot, TeamNum);
		GenericPlayerInitialization(NewBot);
	}
	return NewBot;
}

AUTBotPlayer* AUTGameMode::AddNamedBot(const FString& BotName, uint8 TeamNum)
{
	UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
	UUTBotCharacter* BotData = UTEngine ? UTEngine->FindBotAsset(BotName) : nullptr;
	if (BotData == nullptr)
	{
		UE_LOG(UT, Error, TEXT("Character data for bot '%s' not found"), *BotName);
		return NULL;
	}
	else
	{
		AUTBotPlayer* NewBot = GetWorld()->SpawnActor<AUTBotPlayer>(BotClass);
		if (NewBot != NULL)
		{
			NewBot->InitializeCharacter(BotData);
			NewBot->InitializeSkill(GameDifficulty + BotData->SkillAdjust);
			NumBots++;
			ChangeTeam(NewBot, TeamNum);
			GenericPlayerInitialization(NewBot);
		}
		return NewBot;
	}
}

AUTBotPlayer* AUTGameMode::AddAssetBot(const FStringAssetReference& BotAssetPath, uint8 TeamNum)
{
	UUTBotCharacter* BotData = Cast<UUTBotCharacter>(BotAssetPath.TryLoad());
	AUTBotPlayer* NewBot = NULL;
	if (BotData != NULL)
	{
		NewBot = GetWorld()->SpawnActor<AUTBotPlayer>(BotClass);
		if (NewBot != NULL)
		{
			NewBot->InitializeCharacter(BotData);
			NewBot->InitializeSkill(GameDifficulty + BotData->SkillAdjust);
			NumBots++;
			ChangeTeam(NewBot, TeamNum);
			GenericPlayerInitialization(NewBot);
		}
	}
	return NewBot;
}

AUTBotPlayer* AUTGameMode::ForceAddBot(uint8 TeamNum)
{
	if (bOfflineChallenge || bBasicTrainingGame)
	{
		return NULL;
	}
	BotFillCount = FMath::Max<int32>(BotFillCount, NumPlayers + NumBots + 1);
	return AddBot(TeamNum);
}
AUTBotPlayer* AUTGameMode::ForceAddNamedBot(const FString& BotName, uint8 TeamNum)
{
	if (bOfflineChallenge || bBasicTrainingGame)
	{
		return NULL;
	}
	return AddNamedBot(BotName, TeamNum);
}

void AUTGameMode::SetBotCount(uint8 NewCount)
{
	if (bOfflineChallenge || bBasicTrainingGame)
	{
		return;
	}
	BotFillCount = NumPlayers + NewCount;
}

void AUTGameMode::AddBots(uint8 Num)
{
	if (bOfflineChallenge || bBasicTrainingGame)
	{
		return;
	}
	BotFillCount = FMath::Max(NumPlayers, BotFillCount) + Num;
}

void AUTGameMode::KillBots()
{
	if (bOfflineChallenge || bBasicTrainingGame)
	{
		return;
	}
	BotFillCount = 0;
	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		AUTBotPlayer* B = Cast<AUTBotPlayer>(It->Get());
		if (B != NULL)
		{
			B->Destroy();
			It--;
		}
	}
}

bool AUTGameMode::AllowRemovingBot(AUTBotPlayer* B)
{
	if (bOfflineChallenge || bBasicTrainingGame)
	{
		return false;
	}
	AUTPlayerState* PS = Cast<AUTPlayerState>(B->PlayerState);
	// flag carriers should stay in the game until they lose it
	if (PS != NULL && PS->CarriedObject != NULL)
	{
		return false;
	}
	else
	{
		// score leader should stay in the game unless it's the last bot
		if (NumBots > 1 && PS != NULL)
		{
			bool bHighScore = true;
			for (APlayerState* OtherPS : GameState->PlayerArray)
			{
				if (OtherPS != PS && OtherPS->Score >= PS->Score)
				{
					bHighScore = false;
					break;
				}
			}
			if (bHighScore)
			{
				return false;
			}
		}

		// remove as soon as dead or out of combat
		// TODO: if this isn't getting them out fast enough we can restrict to only human players
		return B->GetPawn() == NULL || B->GetEnemy() == NULL || B->LostContact(5.0f) || !HasMatchStarted();
	}
}

void AUTGameMode::RemoveExtraBots()
{
	int32 FailsafeCount = 0;
	int32 BotsNeeded = (UTGameState && (UTGameState->GetMatchState() == MatchState::WaitingToStart) && (GetNetMode() != NM_Standalone)) ? WarmupFillCount : BotFillCount;
	while ((NumPlayers + NumBots > BotsNeeded) && (FailsafeCount < 10))
	{
		for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			AUTBotPlayer* B = Cast<AUTBotPlayer>(It->Get());
			if (B != NULL)
			{
				B->Destroy();
				break;
			}
		}
		FailsafeCount++;
	}
}

void AUTGameMode::CheckBotCount()
{
	int32 BotsNeeded = (UTGameState && (UTGameState->GetMatchState() == MatchState::WaitingToStart) && (GetNetMode() != NM_Standalone)) ? WarmupFillCount : BotFillCount;
	if (NumPlayers + NumBots > BotsNeeded)
	{
		for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			AUTBotPlayer* B = Cast<AUTBotPlayer>(It->Get());
			if (B != NULL && AllowRemovingBot(B))
			{
				B->Destroy();
				break;
			}
		}
	}
	else while (NumPlayers + NumBots < BotsNeeded)
	{
		AddBot();
	}
}

void AUTGameMode::BeginLineUp(const FString& LineUpTypeName)
{
	const UEnum* LineUpTypeEnum = FindObject<UEnum>(ANY_PACKAGE, TEXT("LineUpTypes"));
	if (UTGameState && LineUpTypeEnum)
	{
		for (int EnumIndex = 0; EnumIndex < LineUpTypeEnum->NumEnums(); ++EnumIndex)
		{
            if (LineUpTypeEnum->GetNameStringByIndex(EnumIndex).Equals(LineUpTypeName, ESearchCase::IgnoreCase))
			{
				LineUpTypes TypeToCreate = static_cast<LineUpTypes>(EnumIndex);
				
				AUTLineUpZone* ZoneToUse = UTGameState->GetAppropriateSpawnList(TypeToCreate);
				if (ZoneToUse)
				{
					//Spawn bots to fill up to the spawn locations count
					BotFillCount = FMath::Max(ZoneToUse->SpawnLocations.Num(), BotFillCount);
					WarmupFillCount = BotFillCount;
					CheckBotCount();

					UTGameState->CreateLineUp(TypeToCreate);
					break;
				}
			}
		}
	}
}

void AUTGameMode::EndLineUp()
{
	if (UTGameState && UTGameState->ActiveLineUpHelper)
	{
		UTGameState->ActiveLineUpHelper->CleanUp();
	}
}

void AUTGameMode::RecreateLobbyBeacon()
{
	UE_LOG(UT,Log,TEXT("RecreateLobbyBeacon: %s %i"), HubAddress.IsEmpty() ? TEXT("none") : *HubAddress, LobbyInstanceID);

	// If we have an instance id, or if we have a defined HUB address, then attempt to connect.
	if (LobbyInstanceID > 0 || !HubAddress.IsEmpty())
	{
		if (LobbyBeacon)
		{
			// Destroy the existing beacon first
			LobbyBeacon->DestroyBeacon();
			LobbyBeacon = nullptr;
		}

		LobbyBeacon = GetWorld()->SpawnActor<AUTServerBeaconLobbyClient>(AUTServerBeaconLobbyClient::StaticClass());
		if (LobbyBeacon)
		{
			FString IP = HubAddress.IsEmpty() ? TEXT("127.0.0.1") : HubAddress;
			FURL LobbyURL(nullptr, *IP, TRAVEL_Absolute);
			LobbyURL.Port = HostLobbyListenPort;
			LobbyBeacon->InitLobbyBeacon(LobbyURL, LobbyInstanceID, ServerInstanceGUID, HubKey);
			UE_LOG(UT, Verbose, TEXT("..... Connecting back to lobby on port %i!"), HostLobbyListenPort);
		}
	}
}

/**
 *	DefaultTimer is called once per second and is useful for consistent timed events that don't require to be 
 *  done every frame.
 **/
void AUTGameMode::DefaultTimer()
{
	// preview world is for blueprint editing, don't try to play
	if (GetWorld()->WorldType == EWorldType::EditorPreview)
	{
		return;
	}

	if (!bGameEnded && GetWorld()->GetRealTimeSeconds() > ReturningPlayerGraceCutoff)
	{
		ReturningPlayerGraceCutoff = 0.0f;
		UUTGameEngine* UTGameEngine = Cast<UUTGameEngine>(GEngine);
		if (UTGameEngine && UTGameEngine->PlayerReservations.Num() > 0)
		{
			UTGameEngine->PlayerReservations.Empty();
		}
	}

 	if (LobbyBeacon && LobbyBeacon->GetNetConnection()->State == EConnectionState::USOCK_Closed)
	{
		// if the server is empty and would be asking the hub to kill it, just kill ourselves rather than waiting for reconnection
		// this relies on there being good monitoring and cleanup code in the hub, but it's better than some kind of network port failure leaving an instance spamming connection attempts forever
		// also handles the hub itself failing
		if (!bDedicatedInstance && NumPlayers <= 0 && MatchState != MatchState::WaitingToStart)
		{
			FPlatformMisc::RequestExit(false);
			return;
		}

		// Lost connection with the beacon. Recreate it.
		UE_LOG(UT, Verbose, TEXT("Beacon %s lost connection. Attempting to recreate."), *GetNameSafe(this));
		RecreateLobbyBeacon();
	}

	// Let the game see if it's time to end the match
	CheckGameTime();

	if (bForceRespawn)
	{
		for (auto It = GetWorld()->GetControllerIterator(); It; ++It)
		{
			AController* Controller = It->Get();
			if (Controller->IsInState(NAME_Inactive))
			{
				AUTPlayerState* PS = Cast<AUTPlayerState>(Controller->PlayerState);
				if (PS != NULL && PS->ForceRespawnTime <= 0.0f)
				{
					RestartPlayer(Controller);
				}
			}
		}
	}

	CheckBotCount();

	int32 CurrentNumPlayers = GetNumPlayers();

	if (IsGameInstanceServer() && LobbyBeacon)
	{
		if (GetWorld()->GetTimeSeconds() - LastLobbyUpdateTime >= 10.0f) // MAKE ME CONIFG!!!!
		{
			UpdateLobbyMatchStats();
		}

		if (!bDedicatedInstance)
		{
			if (!HasMatchStarted())
			{
				if (GetWorld()->GetRealTimeSeconds() > LobbyInitialTimeoutTime && CurrentNumPlayers <= 0 && (GetNetDriver() == NULL || GetNetDriver()->ClientConnections.Num() == 0))
				{
					ShutdownGameInstance();
				}
			}
			else 
			{
				if (CurrentNumPlayers <= 0)
				{
					ShutdownGameInstance();
				}
			}

			// Check to see if everyone is idle.
			if (!bIgnoreIdlePlayers && UTGameState != nullptr && UTGameState->IsMatchInProgress())
			{
				bool bAllPlayersAreIdle = true;
				for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
				{
					AUTPlayerState* UTPlayerState = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
					if (UTPlayerState && !IsPlayerIdle(UTPlayerState))
					{
						bAllPlayersAreIdle = false;
						break;
					}
				}

				if (bAllPlayersAreIdle)
				{
					ShutdownGameInstance();
				}
			}
		}
	}
	else
	{
		// Look to see if we should restart the game due to server inactivity
		if (GetNumPlayers() <= 0 && NumSpectators <= 0 && HasMatchStarted())
		{
			EmptyServerTime++;
			if (EmptyServerTime >= AutoRestartTime)
			{
				TravelToNextMap();
			}
		}
		else
		{
			EmptyServerTime = 0;
		}
	}

	if (MatchState == MatchState::MapVoteHappening )
	{
		if (GetWorld()->GetNetMode() != NM_Standalone)
		{
			UTGameState->VoteTimer--;
			if (UTGameState->VoteTimer<=0)
			{
				UTGameState->VoteTimer = 0;
			}
		}
		
		// Scan the maps and see if we have 
		TArray<AUTReplicatedMapInfo*> Best;
		for (int32 i=0; i< UTGameState->MapVoteList.Num(); i++)
		{
			if (UTGameState->MapVoteList[i]->VoteCount > 0)
			{
				if (Best.Num() == 0 || Best[0]->VoteCount < UTGameState->MapVoteList[i]->VoteCount)
				{
					Best.Empty();
					Best.Add(UTGameState->MapVoteList[i]);
				}
			}
		}
		if ( Best.Num() > 0 )
		{
			int32 Target = int32( float(GetNumPlayers()) * 0.5);
			if ( Best[0]->VoteCount > Target)
			{
				TallyMapVotes();
			}
		}
	}

	// Update the Replay id.
	if ( UTIsHandlingReplays() )
	{
		UDemoNetDriver* DemoNetDriver = GetWorld()->DemoNetDriver;
		if (DemoNetDriver != nullptr && DemoNetDriver->ReplayStreamer.IsValid())
		{
			UTGameState->ReplayID = DemoNetDriver->ReplayStreamer->GetReplayID();
		}
	}
}

void AUTGameMode::ShutdownGameInstance()
{
	SendEveryoneBackToLobby();
	LobbyBeacon->Empty();			
}

void AUTGameMode::ForceLobbyUpdate()
{
	LastLobbyUpdateTime = -10.0;
}

void AUTGameMode::RestartGame()
{
	if (HasMatchStarted())
	{
		Super::RestartGame();
	}
}

bool AUTGameMode::IsEnemy(AController * First, AController* Second)
{
	return First && Second && !UTGameState->OnSameTeam(First, Second);
}

void AUTGameMode::UpdateSkillAdjust(const AUTPlayerState* KillerPlayerState, const AUTPlayerState* KilledPlayerState)
{
	if (KillerPlayerState && (KillerPlayerState->bIsABot != KilledPlayerState->bIsABot))
	{
		// a bot vs player kill occurred, so adjust skill accordingly
		bool bKillerOnBlue = KillerPlayerState->Team && (KillerPlayerState->Team->TeamIndex == 1);
		if (bKillerOnBlue)
		{
			if (KillerPlayerState->bIsABot)
			{
				BlueTeamKills++;
			}
			else
			{
				BlueTeamDeaths++;
			}
		}
		else
		{
			if (KillerPlayerState->bIsABot)
			{
				RedTeamKills++;
			}
			else
			{
				RedTeamDeaths++;
			}
		}
		if (KillerPlayerState->bIsABot)
		{
			// decrease skill level for bot team
			float MinSkill = bBasicTrainingGame ? 0.f : 1.f;
			if (bKillerOnBlue)
			{
				// if more deaths still, less reduction
				float Adjust = (BlueTeamDeaths > BlueTeamKills) ? 0.5f : 1.25f;
				if (bBasicTrainingGame)
				{
					Adjust *= 1.3f;
				}
				BlueTeamSkill = FMath::Max(MinSkill, BlueTeamSkill - Adjust / FMath::Min(5.f, 0.7f*float(BlueTeamKills + BlueTeamDeaths)));
			}
			else
			{
				float Adjust = (RedTeamDeaths > RedTeamKills) ? 0.5f : 1.25f;
				if (bBasicTrainingGame)
				{
					Adjust *= 1.3f;
				}
				RedTeamSkill = FMath::Max(MinSkill, RedTeamSkill - Adjust / FMath::Min(4.f, 0.7f*float(RedTeamKills + RedTeamDeaths)));
			}
			AUTBot* Bot = Cast<AUTBot>(KillerPlayerState->GetOwner());
			if (Bot)
			{
				Bot->AutoUpdateSkillFor(this);
			}
		}
		else
		{
			// increase skill level for bot team capped
			float MaxSkill = bBasicTrainingGame ? GameDifficulty : 5.f;
			if (bKillerOnBlue || !bTeamGame)
			{
				float Adjust = (RedTeamKills > RedTeamDeaths) ? 0.5f : 1.f;
				RedTeamSkill = FMath::Min(MaxSkill, RedTeamSkill + Adjust / FMath::Min(4.f, 0.7f*float(RedTeamKills + RedTeamDeaths)));
			}
			else
			{
				// if more deaths still, less reduction
				float Adjust = (BlueTeamKills > BlueTeamDeaths) ? 0.5f : 1.f;
				BlueTeamSkill = FMath::Min(MaxSkill, BlueTeamSkill + Adjust / FMath::Min(5.f, 0.7f*float(BlueTeamKills + BlueTeamDeaths)));
			}
		}
		if (bBasicTrainingGame)
		{
			BlueTeamSkill = RedTeamSkill;
		}
	}
}

void AUTGameMode::Killed(AController* Killer, AController* KilledPlayer, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType)
{
	// Ignore all killing when entering overtime as we kill off players and don't want it affecting their score.
	if ((GetMatchState() != MatchState::MatchEnteringOvertime) && (GetMatchState() != MatchState::WaitingPostMatch) && (GetMatchState() != MatchState::MapVoteHappening))
	{
		AUTPlayerState* const KillerPlayerState = Killer ? Cast<AUTPlayerState>(Killer->PlayerState) : NULL;
		AUTPlayerState* const KilledPlayerState = KilledPlayer ? Cast<AUTPlayerState>(KilledPlayer->PlayerState) : NULL;

		//UE_LOG(UT, Log, TEXT("Player Killed: %s killed %s"), (KillerPlayerState != NULL ? *KillerPlayerState->PlayerName : TEXT("NULL")), (KilledPlayerState != NULL ? *KilledPlayerState->PlayerName : TEXT("NULL")));
		if (KilledPlayerState != NULL)
		{
			bool const bEnemyKill = IsEnemy(Killer, KilledPlayer);
			KilledPlayerState->LastKillerPlayerState = KillerPlayerState;

			if (HasMatchStarted())
			{
				if (bAutoAdjustBotSkill && bEnemyKill)
				{
					UpdateSkillAdjust(KillerPlayerState, KilledPlayerState);
				}
				KilledPlayerState->IncrementDeaths(DamageType, KillerPlayerState);
				TSubclassOf<UUTDamageType> UTDamage(*DamageType);
				if (UTDamage && bEnemyKill)
				{
					UTDamage.GetDefaultObject()->ScoreKill(KillerPlayerState, KilledPlayerState, KilledPawn);

					if (EnemyKillsByDamageType.Contains(UTDamage))
					{
						EnemyKillsByDamageType[UTDamage] = EnemyKillsByDamageType[UTDamage] + 1;
					}
					else
					{
						EnemyKillsByDamageType.Add(UTDamage, 1);
					}
				}

				if (!bEnemyKill && (Killer != KilledPlayer) && (Killer != NULL))
				{
					ScoreTeamKill(Killer, KilledPlayer, KilledPawn, DamageType);
				}
				else
				{
					ScoreKill(Killer, KilledPlayer, KilledPawn, DamageType);
				}
			}
			else if (KilledPlayerState)
			{
				if (bEnemyKill)
				{
					WarmupKills++;
				}
				TSubclassOf<UUTDamageType> UTDamage(*DamageType);

				if (KillerPlayerState && UTDamage && UTDamage.GetDefaultObject()->RewardAnnouncementClass)
				{
					AUTPlayerController* KillerPC = Cast<AUTPlayerController>(KillerPlayerState->GetOwner());
					if (KillerPC != nullptr)
					{
						KillerPC->SendPersonalMessage(UTDamage.GetDefaultObject()->RewardAnnouncementClass, 0, KillerPlayerState, KilledPlayerState);
					}
				}
				KilledPlayerState->RespawnWaitTime = bIsInstagib ? 1.f : 2.f;
				KilledPlayerState->OnRespawnWaitReceived();
			}
			BroadcastDeathMessage(Killer, KilledPlayer, DamageType);

			if (bHasRespawnChoices)
			{
				KilledPlayerState->RespawnChoiceA = nullptr;
				KilledPlayerState->RespawnChoiceB = nullptr;
				KilledPlayerState->RespawnChoiceA = Cast<APlayerStart>(ChoosePlayerStart(KilledPlayer));
				KilledPlayerState->RespawnChoiceB = Cast<APlayerStart>(ChoosePlayerStart(KilledPlayer));
				KilledPlayerState->bChosePrimaryRespawnChoice = true;
			}
		}

		DiscardInventory(KilledPawn, Killer);
	}
	NotifyKilled(Killer, KilledPlayer, KilledPawn, DamageType);
}

void AUTGameMode::NotifyKilled(AController* Killer, AController* Killed, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType)
{
	// update AI data
	if (Killer != NULL && Killer != Killed)
	{
		AUTRecastNavMesh* NavData = GetUTNavData(GetWorld());
		if (NavData != NULL)
		{
			{
				UUTPathNode* Node = NavData->FindNearestNode(KilledPawn->GetNavAgentLocation(), KilledPawn->GetSimpleCollisionCylinderExtent());
				if (Node != NULL)
				{
					Node->NearbyDeaths++;
				}
			}
			if (Killer->GetPawn() != NULL)
			{
				// it'd be better to get the node from which the shot was fired, but it's probably not worth it
				UUTPathNode* Node = NavData->FindNearestNode(Killer->GetPawn()->GetNavAgentLocation(), Killer->GetPawn()->GetSimpleCollisionCylinderExtent());
				if (Node != NULL)
				{
					Node->NearbyKills++;
				}
			}
		}
	}

	for (FConstControllerIterator It = GetWorld()->GetControllerIterator(); It; ++It)
	{
		if (It->IsValid())
		{
			AUTBot* B = Cast<AUTBot>(It->Get());
			if (B != NULL && !B->IsPendingKillPending())
			{
				B->UTNotifyKilled(Killer, Killed, KilledPawn, DamageType.GetDefaultObject());
			}
		}
	}
}

void AUTGameMode::ScorePickup_Implementation(AUTPickup* Pickup, AUTPlayerState* PickedUpBy, AUTPlayerState* LastPickedUpBy)
{
}

void AUTGameMode::ScoreDamage_Implementation(int32 DamageAmount, AUTPlayerState* Victim, AUTPlayerState* Attacker)
{
	if (Victim && Attacker && UTGameState && !UTGameState->OnSameTeam(Victim, Attacker))
	{
		Attacker->IncrementDamageDone(DamageAmount);
	}
	if (BaseMutator != NULL)
	{
		BaseMutator->ScoreDamage(DamageAmount, Victim, Attacker);
	}
}

void AUTGameMode::ScoreTeamKill_Implementation(AController* Killer, AController* Other, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType)
{
	AddKillEventToReplay(Killer, Other, DamageType);

	if (BaseMutator != NULL)
	{
		BaseMutator->ScoreKill(Killer, Other, DamageType);
	}
}

void AUTGameMode::ScoreKill_Implementation(AController* Killer, AController* Other, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType)
{
	AUTPlayerState* OtherPlayerState = Other ? Cast<AUTPlayerState>(Other->PlayerState) : NULL;
	if( (Killer == Other) || (Killer == NULL) )
	{
		// If it's a suicide, subtract a kill from the player...
		if (OtherPlayerState)
		{
			OtherPlayerState->AdjustScore(-1);
		}
		if (Killer != nullptr)
		{
			AUTPlayerState * KillerPlayerState = Cast<AUTPlayerState>(Killer->PlayerState);
			if (KillerPlayerState != nullptr)
			{
				KillerPlayerState->ModifyStatsValue(NAME_Suicides, 1);
			}
		}
	}
	else 
	{
		AUTPlayerState * KillerPlayerState = Cast<AUTPlayerState>(Killer->PlayerState);
		if ( KillerPlayerState != NULL )
		{
			KillerPlayerState->AdjustScore(+1);
			KillerPlayerState->IncrementKills(DamageType, true, OtherPlayerState);
			CheckScore(KillerPlayerState);

			if (!bFirstBloodOccurred)
			{
				BroadcastLocalized(this, UUTFirstBloodMessage::StaticClass(), 0, KillerPlayerState, NULL, NULL);
				bFirstBloodOccurred = true;
			}
			TrackKillAssists(Killer, Other, KilledPawn, DamageType, KillerPlayerState, OtherPlayerState);
		}
	}

	AddKillEventToReplay(Killer, Other, DamageType);

	if (BaseMutator != NULL)
	{
		BaseMutator->ScoreKill(Killer, Other, DamageType);
	}
	FindAndMarkHighScorer();
}

void AUTGameMode::TrackKillAssists(AController* Killer, AController* Other, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType, AUTPlayerState* KillerPlayerState, AUTPlayerState* OtherPlayerState)
{
	if (bTrackKillAssists)
	{
		AUTCharacter* Char = Cast<AUTCharacter>(KilledPawn);
		if (Char)
		{
			TArray<AUTPlayerState*> TrackedAssists;
			for (int32 i = 0; i < Char->HealthRemovalAssists.Num(); i++)
			{
				if (Char->HealthRemovalAssists[i] && !Char->HealthRemovalAssists[i]->IsPendingKillPending() && (Char->HealthRemovalAssists[i] != KillerPlayerState))
				{
					TrackedAssists.AddUnique(Char->HealthRemovalAssists[i]);
				}
			}
			for (int32 i = 0; i < Char->ArmorRemovalAssists.Num(); i++)
			{
				if (Char->ArmorRemovalAssists[i] && !Char->ArmorRemovalAssists[i]->IsPendingKillPending() && (Char->ArmorRemovalAssists[i] != KillerPlayerState))
				{
					TrackedAssists.AddUnique(Char->ArmorRemovalAssists[i]);
				}
			}
			for (int32 i = 0; i < TrackedAssists.Num(); i++)
			{
				TrackedAssists[i]->IncrementKillAssists(DamageType, true, OtherPlayerState);
			}
		}
	}
}

void AUTGameMode::AddKillEventToReplay(AController* Killer, AController* Other, TSubclassOf<UDamageType> DamageType)
{
	// Could be a suicide
	if (Killer == nullptr)
	{
		return;
	}

	// Shouldn't happen, but safety first
	if (Other == nullptr)
	{
		return;
	}
	
	UDemoNetDriver* DemoNetDriver = GetWorld()->DemoNetDriver;
	if (DemoNetDriver != nullptr && DemoNetDriver->ServerConnection == nullptr)
	{
		AUTPlayerState* KillerPlayerState = Cast<AUTPlayerState>(Killer->PlayerState);
		AUTPlayerState* OtherPlayerState = Cast<AUTPlayerState>(Other->PlayerState);

        FString KillerName = KillerPlayerState ? *KillerPlayerState->GetPlayerName() : TEXT("None");
        FString VictimName = OtherPlayerState ? *OtherPlayerState->GetPlayerName() : TEXT("None");

		KillerName.ReplaceInline(TEXT(" "), TEXT("%20"));
		VictimName.ReplaceInline(TEXT(" "), TEXT("%20"));

		FString DamageName = DamageType ? DamageType->GetName() : TEXT("Unknown");
		
		TArray<uint8> Data;
		FString KillInfo = FString::Printf(TEXT("%s %s %s"), *KillerName, *VictimName, *DamageName);

		FMemoryWriter MemoryWriter(Data);
		MemoryWriter.Serialize(TCHAR_TO_ANSI(*KillInfo), KillInfo.Len() + 1);

		FString MetaTag;
		if (KillerPlayerState != nullptr)
		{
			MetaTag = KillerPlayerState->StatsID;
			if (MetaTag.IsEmpty())
			{
                MetaTag = KillerPlayerState->GetPlayerName();
			}
		}
		DemoNetDriver->AddEvent(TEXT("Kills"), MetaTag, Data);
	}
}

void AUTGameMode::AddMultiKillEventToReplay(AController* Killer, int32 MultiKillLevel)
{
	UDemoNetDriver* DemoNetDriver = GetWorld()->DemoNetDriver;
	if (Killer && DemoNetDriver != nullptr && DemoNetDriver->ServerConnection == nullptr)
	{
		AUTPlayerState* KillerPlayerState = Cast<AUTPlayerState>(Killer->PlayerState);
        FString KillerName = KillerPlayerState ? *KillerPlayerState->GetPlayerName() : TEXT("None");
		KillerName.ReplaceInline(TEXT(" "), TEXT("%20"));

		TArray<uint8> Data;
		FString KillInfo = FString::Printf(TEXT("%s %d"), *KillerName, MultiKillLevel);

		FMemoryWriter MemoryWriter(Data);
		MemoryWriter.Serialize(TCHAR_TO_ANSI(*KillInfo), KillInfo.Len() + 1);

		FString MetaTag;
		if (KillerPlayerState != nullptr)
		{
			MetaTag = KillerPlayerState->StatsID;
			if (MetaTag.IsEmpty())
			{
                MetaTag = KillerPlayerState->GetPlayerName();
			}
		}
		DemoNetDriver->AddEvent(TEXT("MultiKills"), MetaTag, Data);
	}
}

void AUTGameMode::AddSpreeKillEventToReplay(AController* Killer, int32 SpreeLevel)
{
	UDemoNetDriver* DemoNetDriver = GetWorld()->DemoNetDriver;
	if (Killer && DemoNetDriver != nullptr && DemoNetDriver->ServerConnection == nullptr)
	{
		AUTPlayerState* KillerPlayerState = Cast<AUTPlayerState>(Killer->PlayerState);
        FString KillerName = KillerPlayerState ? *KillerPlayerState->GetPlayerName() : TEXT("None");
		KillerName.ReplaceInline(TEXT(" "), TEXT("%20"));

		TArray<uint8> Data;
		FString KillInfo = FString::Printf(TEXT("%s %d"), *KillerName, SpreeLevel);

		FMemoryWriter MemoryWriter(Data);
		MemoryWriter.Serialize(TCHAR_TO_ANSI(*KillInfo), KillInfo.Len() + 1);

		FString MetaTag;
		if (KillerPlayerState != nullptr)
		{
			MetaTag = KillerPlayerState->StatsID;
			if (MetaTag.IsEmpty())
			{
                MetaTag = KillerPlayerState->GetPlayerName();
			}
		}
		DemoNetDriver->AddEvent(TEXT("SpreeKills"), MetaTag, Data);
	}
}

bool AUTGameMode::OverridePickupQuery_Implementation(APawn* Other, TSubclassOf<AUTInventory> ItemClass, AActor* Pickup, bool& bAllowPickup)
{
	return (BaseMutator != NULL && BaseMutator->OverridePickupQuery(Other, ItemClass, Pickup, bAllowPickup));
}

void AUTGameMode::DiscardInventory(APawn* Other, AController* Killer)
{
	AUTCharacter* UTC = Cast<AUTCharacter>(Other);
	if (UTC != NULL)
	{
		if (!UTGameState || !UTGameState->IsLineUpActive())
		{
			// toss weapon
			if (UTC->GetWeapon() != NULL)
			{
				if (UTC->GetWeapon()->ShouldDropOnDeath())
				{
					AUTWeapon* OldWeapon = UTC->GetWeapon();
					UTC->TossInventory(UTC->GetWeapon());
					if (OldWeapon && !OldWeapon->IsPendingKillPending() && OldWeapon->bShouldAnnounceDrops && (OldWeapon->Ammo > 0) && (OldWeapon->PickupAnnouncementName != NAME_None) && bAllowPickupAnnouncements && IsMatchInProgress())
					{
                        AUTPlayerState* UTPS = Cast<AUTPlayerState>(Other->GetPlayerState());
						if (UTPS)
						{
							UTPS->AnnounceStatus(OldWeapon->PickupAnnouncementName, 2);
						}
					}
				}
				else
				{
					// drop default weapon instead @TODO FIXMESTEVE - this should go through default items array
					AUTWeapon* Enforcer = UTC->FindInventoryType<AUTWeapon>(AUTWeap_Enforcer::StaticClass());
					if (Enforcer && !Enforcer->IsPendingKillPending())
					{
						UTC->TossInventory(Enforcer);
					}
				}
			}
			// toss all powerups
			for (TInventoryIterator<> It(UTC); It; ++It)
			{
				if (It->bAlwaysDropOnDeath)
				{
					UTC->TossInventory(*It, FVector(FMath::FRandRange(0.0f, 200.0f), FMath::FRandRange(-400.0f, 400.0f), FMath::FRandRange(0.0f, 200.0f)));
					if (It->bShouldAnnounceDrops && (It->PickupAnnouncementName != NAME_None) && bAllowPickupAnnouncements && IsMatchInProgress())
					{
                        AUTPlayerState* UTPS = Cast<AUTPlayerState>(Other->GetPlayerState());
						AUTWeapon* Weap = Cast<AUTWeapon>(*It);
						if (UTPS && (!Weap || Weap->Ammo > 0))
						{
							UTPS->AnnounceStatus(It->PickupAnnouncementName, 2);
						}
					}
				}
			}
		}
		// delete the rest
		UTC->DiscardAllInventory();
	}
}

void AUTGameMode::FindAndMarkHighScorer()
{
	int32 BestScore = 0;
	for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
	{
		AUTPlayerState *PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
		if (PS && !PS->bOnlySpectator)
		{
			if (BestScore == 0 || PS->Score > BestScore)
			{
				BestScore = PS->Score;
			}
		}
	}

	for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
		if (PS != nullptr)
		{
			bool bOldHighScorer = PS->bHasHighScore;
			PS->bHasHighScore = (BestScore == PS->Score);
			if ((bOldHighScorer != PS->bHasHighScore) && (GetNetMode() != NM_DedicatedServer))
			{
				PS->OnRep_HasHighScore();
			}
		}
	}
}

//Special markup for Analytics event so they show up properly in grafana. Should be eventually moved to UTAnalytics.
/*
* @EventName NewMatch
*
* @Trigger Sent when a player begins a new match
*
* @Type Sent by the Server
*
* @EventParam MapName string Name of the Map started
* @EventParam GameName string Name of the game mode
* @EventParam GoalScore int32 Goal that the match ends on
* @EventParam TimeLimit int32 Time that the match ends on
* @EventParam CustomContent string Custom content used, or 0 if none

* @Comments
*/

//Special markup for Analytics event so they show up properly in grafana. Should be eventually moved to UTAnalytics.
/*
* @EventName NewOfflineChallengeMatch
*
* @Trigger Sent when a player begins a new offline challenge
*
* @Type Sent by the Server
*
* @EventParam OfflineChallenge bool Is the challenge online or off
* @EventParam ChallengeDifficulty int32 Difficulty of challenge
* @EventParam ChallengeTag string Name of the Challenge Tag
*
* @Comments
*/

//Special markup for Analytics event so they show up properly in grafana. Should be eventually moved to UTAnalytics.
/*
* @EventName MatchWithCustomContent
*
* @Trigger Sent when a player begins a match with custom content that is a standalone client
*
* @Type Sent by the Server
*
* @EventParam CustomContent string Checksum maps of all the custom content used
*
* @Comments
*/
void AUTGameMode::StartMatch()
{
	if (HasMatchStarted())
	{
		// Already started
		return;
	}

	//Fire new match analytics
	if (FUTAnalytics::IsAvailable())
	{
		if (GetWorld() && (TutorialMask != 0))
		{
			FUTAnalytics::FireEvent_UTTutorialStarted(Cast<AUTPlayerController>(GetWorld()->GetFirstPlayerController()), GetWorld()->GetMapName());
		}

		FUTAnalytics::FireEvent_UTStartMatch(this);
	}

	if (GetWorld()->IsPlayInEditor() || !bDelayedStart)
	{
		SetMatchState(MatchState::InProgress);
	}
	else
	{
		SetMatchState(MatchState::PlayerIntro);
	}
	
	// clear any PlayerState values that could be modified during warmup
	for (int32 i = 0; i < GameState->PlayerArray.Num(); i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(GameState->PlayerArray[i]);
		if (PS != nullptr)
		{
			PS->BoostRechargePct = 0.0f;
			PS->SetRemainingBoosts(0);
		}
	}

	// Any player that join pre-StartMatch is given a free pass to quit
	// Rejoining will not copy this flag to their new playerstate, so if they rejoin the server post-StartMatch and drop, they will get a quit notice
	for (int32 i = 0; i < InactivePlayerArray.Num(); i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(InactivePlayerArray[i]);
		if (PS)
		{
			PS->bAllowedEarlyLeave = true;
		}
	}

	if (FUTAnalytics::IsAvailable())
	{
		if (GetWorld()->GetNetMode() != NM_Standalone)
		{
			TArray<FAnalyticsEventAttribute> ParamArray;
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("MapName"), GetWorld()->GetMapName()));
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("GameName"), GetNameSafe(GetClass())));
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("GoalScore"), GoalScore));
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("TimeLimit"), TimeLimit));
			UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
			if (UTEngine)
			{
				ParamArray.Add(FAnalyticsEventAttribute(TEXT("CustomContent"), UTEngine->LocalContentChecksums.Num() + UTEngine->MountedDownloadedContentChecksums.Num()));
			}
			else
			{
				ParamArray.Add(FAnalyticsEventAttribute(TEXT("CustomContent"), 0));
			}
			FUTAnalytics::SetMatchInitialParameters(this, ParamArray, false);

			FUTAnalytics::GetProvider().RecordEvent( TEXT("NewMatch"), ParamArray );
		}
		else if (bOfflineChallenge)
		{
			TArray<FAnalyticsEventAttribute> ParamArray;
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("OfflineChallenge"), bOfflineChallenge));
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("ChallengeDifficulty"), ChallengeDifficulty));
			ParamArray.Add(FAnalyticsEventAttribute(TEXT("ChallengeTag"), ChallengeTag.ToString()));
			FUTAnalytics::SetMatchInitialParameters(this, ParamArray, false);

			FUTAnalytics::GetProvider().RecordEvent(TEXT("NewOfflineChallengeMatch"), ParamArray);
		}
		else
		{
			UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
			if (UTEngine && ((UTEngine->LocalContentChecksums.Num() + UTEngine->MountedDownloadedContentChecksums.Num()) > 0))
			{
				TArray<FAnalyticsEventAttribute> ParamArray;
				ParamArray.Add(FAnalyticsEventAttribute(TEXT("CustomContent"), UTEngine->LocalContentChecksums.Num() + UTEngine->MountedDownloadedContentChecksums.Num()));
				FUTAnalytics::SetMatchInitialParameters(this, ParamArray, false);
				FUTAnalytics::GetProvider().RecordEvent(TEXT("MatchWithCustomContent"), ParamArray);
			}
		}
	}
}

void AUTGameMode::RemoveAllPawns()
{
	// remove any warm up pawns
	for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
	{
		// Detach all controllers from their pawns
		AController* Controller = Iterator->Get();
		AUTPlayerState* NewPlayerState = Cast<AUTPlayerState>(Controller->PlayerState);
		if (NewPlayerState)
		{
			NewPlayerState->bIsWarmingUp = false;
		}
		if (Controller->GetPawn() != NULL)
		{
			Controller->PawnPendingDestroy(Controller->GetPawn());
			Controller->UnPossess();
		}
	}

	TArray<APawn*> PawnsToDestroy;
	for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
	{
		if (It->IsValid() && !Cast<ASpectatorPawn>((*It).Get()))
		{
			PawnsToDestroy.Add(It->Get());
		}
	}

	for (int32 i = 0; i<PawnsToDestroy.Num(); i++)
	{
		APawn* Pawn = PawnsToDestroy[i];
		if (Pawn != NULL && !Pawn->IsPendingKill())
		{
			Pawn->Destroy();
		}
	}

	// also get rid of projectiles left over
	for (FActorIterator It(GetWorld()); It; ++It)
	{
		AActor* TestActor = *It;
		if (TestActor && !TestActor->IsPendingKill() && TestActor->IsA<AUTProjectile>())
		{
			TestActor->Destroy();
		}
	}
}

void AUTGameMode::HandleMatchHasStarted()
{
	if (bRemovePawnsAtStart && (GetNetMode() != NM_Standalone) && !GetWorld()->IsPlayInEditor())
	{
		RemoveAllPawns();
	}

	// reset things, relevant for any kind of warmup mode and to make sure placed Actors like pickups are in their initial state no matter how much time has passed in pregame
	for (FActorIterator It(GetWorld()); It; ++It)
	{
		if (It->GetClass()->ImplementsInterface(UUTResetInterface::StaticClass()))
		{
			IUTResetInterface::Execute_Reset(*It);
		}
	}

	if (UTGameState != NULL)
	{
		UTGameState->CompactSpectatingIDs();
	}

	UpdatePlayersPresence();

	Super::HandleMatchHasStarted();

	if (UTIsHandlingReplays() && GetGameInstance() != nullptr)
	{
		GetGameInstance()->StartRecordingReplay(TEXT(""), GetWorld()->GetMapName());
	}

	UTGameState->SetTimeLimit(TimeLimit);
	bFirstBloodOccurred = false;
	AnnounceMatchStart();

	//Make sure we aren't still in a line up.
	if (UTGameState)
	{
		UTGameState->ClearLineUp();
	}

	if (IsGameInstanceServer() && LobbyBeacon)
	{
		FString MatchStats = FString::Printf(TEXT("%i"), UTGameState->ElapsedTime);

		FMatchUpdate MatchUpdate;
		MatchUpdate.GameTime = UTGameState->ElapsedTime;
		MatchUpdate.NumPlayers = NumPlayers;
		MatchUpdate.NumSpectators = NumSpectators;
		MatchUpdate.MatchState = MatchState;
		MatchUpdate.bMatchHasBegun = HasMatchStarted();
		MatchUpdate.bMatchHasEnded = HasMatchEnded();
		LobbyBeacon->StartGame(MatchUpdate);
	}

}

void AUTGameMode::AnnounceMatchStart()
{
	BroadcastLocalized(this, UUTGameMessage::StaticClass(), 0, NULL, NULL, NULL);
}

void AUTGameMode::BeginGame()
{
	UE_LOG(UT,Log,TEXT("BEGIN GAME GameType: %s"), *GetNameSafe(this));
	UE_LOG(UT,Log,TEXT("Difficulty: %f GoalScore: %i TimeLimit (sec): %i"), GameDifficulty, GoalScore, TimeLimit);

	for (FActorIterator It(GetWorld()); It; ++It)
	{
		AActor* TestActor = *It;
		if (TestActor && !TestActor->IsPendingKill() && TestActor->IsA<AUTPlayerState>())
		{
			Cast<AUTPlayerState>(TestActor)->StartTime = 0;
			Cast<AUTPlayerState>(TestActor)->bSentLogoutAnalytics = false;
		}
	}
	UTGameState->ElapsedTime = 0;

	//Let the game session override the StartMatch function, in case it wants to wait for arbitration
	if (GameSession->HandleStartMatchRequest())
	{
		return;
	}
	SetMatchState(MatchState::InProgress);
}

void AUTGameMode::EndMatch()
{
	Super::EndMatch();

	if (FUTAnalytics::IsAvailable())
	{
		if (GetWorld() && (TutorialMask != 0))
		{
			FUTAnalytics::FireEvent_UTTutorialCompleted(Cast<AUTPlayerController>(GetWorld()->GetFirstPlayerController()), GetWorld()->GetMapName());
		}
	}

	APlayerController* LocalPC = GEngine->GetFirstLocalPlayerController(GetWorld());
	UUTLocalPlayer* LP = LocalPC ? Cast<UUTLocalPlayer>(LocalPC->Player) : NULL;
	if (LP && UTGameState)
	{
		LP->EarnedStars = 0;
		LP->RosterUpgradeText = FText::GetEmpty();
		if (bOfflineChallenge && PlayerWonChallenge())
		{
			LP->ChallengeCompleted(ChallengeTag, ChallengeDifficulty + 1);
		}

		if (TutorialMask > 0)
		{
			LP->SetTutorialFinished(TutorialMask);
		}
	}

	//Log weapon kills in analytics
	FUTAnalytics::FireEvent_UTServerWeaponKills(this, &EnemyKillsByDamageType);

	UTGameState->UpdateMatchHighlights();

	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, this, &AUTGameMode::PlayEndOfMatchMessage, EndOfMatchMessageDelay * GetActorTimeDilation());

	UTGameState->PrepareForIntermission();

	// Tell the controllers to look at own team flag
	for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
		if (PC != NULL)
		{
			PC->ClientPrepareForIntermission();
		}
	}
}

bool AUTGameMode::AllowPausing(APlayerController* PC)
{
	AUTPlayerState* PS = (PC != NULL) ? Cast<AUTPlayerState>(PC->PlayerState) : NULL;
	if (PS != NULL && PS->bIsRconAdmin)
	{
		return true;
	}
	else
	{
		// allow pausing even in listen server mode if no remote players are connected
		return (Super::AllowPausing(PC) || GetWorld()->GetNetDriver() == NULL || GetWorld()->GetNetDriver()->ClientConnections.Num() == 0);
	}
}

void AUTGameMode::UpdateSkillRating()
{

}

void AUTGameMode::SendEndOfGameStats(FName Reason)
{
	if (FUTAnalytics::IsAvailable())
	{
		if (GetWorld()->GetNetMode() != NM_Standalone)
		{
			FUTAnalytics::FireEvent_UTEndMatch(this, Reason);
		}

		for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
		{
			SendLogoutAnalytics(Cast<AUTPlayerState>(UTGameState->PlayerArray[i]));
		}
	}

	if (!bDisableCloudStats)
	{
		AwardXP();

		UpdateSkillRating();

		const double CloudStatsStartTime = FPlatformTime::Seconds();
		for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
			if (PS != NULL)
			{
				PS->SetStatsValue(NAME_MatchesPlayed, 1);
				PS->SetStatsValue(NAME_TimePlayed, PS->ElapsedTime);
				PS->AddMatchToStats(GetWorld()->GetMapName(), GetClass()->GetPathName(), nullptr, &UTGameState->PlayerArray, &InactivePlayerArray);
				PS->WriteStatsToCloud();
			}
		}

		for (int32 i = 0; i < InactivePlayerArray.Num(); i++)
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(InactivePlayerArray[i]);
			if (PS && !PS->HasWrittenStatsToCloud())
			{
				if (!PS->bAllowedEarlyLeave)
				{
					PS->SetStatsValue(NAME_MatchesQuit, 1);
				}

				PS->SetStatsValue(NAME_MatchesPlayed, 1);
				PS->SetStatsValue(NAME_TimePlayed, PS->ElapsedTime);

				PS->AddMatchToStats(GetWorld()->GetMapName(), GetClass()->GetPathName(), nullptr, &UTGameState->PlayerArray, &InactivePlayerArray);
				if (PS != nullptr)
				{
					PS->WriteStatsToCloud();
				}
			}
		}

		const double CloudStatsTime = FPlatformTime::Seconds() - CloudStatsStartTime;
		UE_LOG(UT, Verbose, TEXT("Cloud stats write time %.3f"), CloudStatsTime);
	}
}

float AUTGameMode::GetScoreForXP(AUTPlayerState* PS)
{
	return PS->Score;
}

void AUTGameMode::AwardXP()
{
	static const bool bXPCheatEnabled = FParse::Param(FCommandLine::Get(), TEXT("XPGiveaway"));
	for (APlayerState* PS : GameState->PlayerArray)
	{
		AUTPlayerState* UTPS = Cast<AUTPlayerState>(PS);
		if (UTPS != NULL && UTPS->UniqueId.IsValid())
		{
			AUTPlayerController* PC = Cast<AUTPlayerController>(UTPS->GetOwner());
			if (PC != NULL)
			{
				// TODO: move some of this to the backend
				UTPS->GiveXP(FNewScoreXP(FMath::Max<int32>(0, FMath::TruncToInt(GetScoreForXP(UTPS)))));
				if (XPCapPerMin > 0)
				{
					UTPS->ClampXP(XPCapPerMin * (((UTGameState->ElapsedTime - PS->StartTime) / 60) + 1));
				}
				if (!bIsQuickMatch && GameSession->MaxPlayers > 2 && (NumPlayers == 1 || NumPlayers < NumBots))
				{
					UTPS->ApplyBotXPPenalty(GameDifficulty);
				}
				if (bOfflineChallenge && (XPMultiplier > 0.f))
				{
					UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(PC->Player);
					TWeakObjectPtr<UUTChallengeManager> ChallengeManager = Cast<UUTGameEngine>(GEngine)->GetChallengeManager();
					if (LP && ChallengeManager.IsValid())
					{
						UTPS->GiveXP(FNewChallengeXP(ChallengeManager->XPBonus * float(LP->EarnedStars) / XPMultiplier));
					}
				}
				if (bXPCheatEnabled)
				{
					UTPS->GiveXP(FNewKillAwardXP(250000));
				}
#if WITH_PROFILE
				if (UTPS->GetMcpProfile() != NULL)
				{
					UTPS->GetMcpProfile()->GrantXP(UTPS->GetXP().Total());
				}
				else
				{
					UE_LOG(UT, Warning, TEXT("Player %s missing profile for gaining XP! Id: %s"), *UTPS->PlayerName, *UTPS->UniqueId.ToString());
				}
#endif
			}
		}
	}
}

bool AUTGameMode::PlayerWonChallenge()
{
	AUTPlayerState* Winner = NULL;
	for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
	{
		AController* Controller = Iterator->Get();
		AUTPlayerState* CPS = Cast<AUTPlayerState>(Controller->PlayerState);
		if (CPS && ((Winner == NULL) || (CPS->Score >= Winner->Score)))
		{
			Winner = CPS;
		}
	}
	return Winner && Cast<AUTPlayerController>(Winner->GetOwner());
}

void AUTGameMode::EndGame(AUTPlayerState* Winner, FName Reason )
{
	// Dont ever end the game in PIE
	if (GetWorld()->WorldType == EWorldType::PIE) return;

	// If we don't have a winner, then go and find one
	if (Winner == NULL)
	{
		for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
		{
			AController* Controller = Iterator->Get();
			AUTPlayerState* CPS = Cast<AUTPlayerState>(Controller->PlayerState);
			if ( CPS && !CPS->bOnlySpectator )
			{
				if ((Winner == NULL) || (CPS->Score >= Winner->Score))
				{
					Winner = CPS;
				}

			}
		}
	}

	UTGameState->SetWinner(Winner);
	EndTime = GetWorld()->TimeSeconds;

	if (IsGameInstanceServer() && LobbyBeacon)
	{
		FString MatchStats = FString::Printf(TEXT("%i"), UTGameState->ElapsedTime);

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
	GetWorldTimerManager().SetTimer(TempHandle4, this, &AUTGameMode::StopReplayRecording, EndReplayDelay);

	SendEndOfGameStats(Reason);
	UnlockSession();
	EndMatch();

	//PickMostCoolMoments();

	// This should happen after EndMatch()
	SetEndGameFocus(Winner);
}

// Keep cool moments spread out a bit
bool IsValidCoolMomentTime(float TimeToCheck, TArray<float>& UsedTimes)
{
	if (UsedTimes.Num() == 0)
	{
		return true;
	}

	for (int32 i = 0; i < UsedTimes.Num(); i++)
	{
		if (FMath::Abs(UsedTimes[i] - TimeToCheck) < 5.0f)
		{
			return false;
		}
	}

	return true;
}

void AUTGameMode::PickMostCoolMoments(bool bClearCoolMoments, int32 CoolMomentsToShow)
{
	UE_LOG(UT, Log, TEXT("PickMostCoolMoments"));

	TArray<AUTPlayerState*> CoolestPlayers;
	TArray<float> HighestCoolFactors;
	TArray<float> CoolPlayTimes;

	TArray<AUTPlayerState*> PlayerStates;
	for (int32 i = 0; i < GameState->PlayerArray.Num(); i++)
	{
		AUTPlayerState* UTPS = Cast<AUTPlayerState>(GameState->PlayerArray[i]);
		if (UTPS && UTPS->CoolFactorHistory.Num() > 0 && UTPS->UniqueId.IsValid())
		{
			PlayerStates.Add(UTPS);
		}
	}

	AUTPlayerState* CoolestPlayer = nullptr;
	float MostCoolFactor = 0;
	float BestTimeOccurred = 0;
	while (CoolestPlayers.Num() < CoolMomentsToShow && PlayerStates.Num() > 0)
	{
		CoolestPlayer = nullptr;
		MostCoolFactor = 0;
		BestTimeOccurred = 0;

		for (int32 PlayerIdx = 0; PlayerIdx < PlayerStates.Num(); PlayerIdx++)
		{
			for (int32 CoolMomentIdx = 0; CoolMomentIdx < PlayerStates[PlayerIdx]->CoolFactorHistory.Num(); CoolMomentIdx++)
			{
				if (PlayerStates[PlayerIdx]->CoolFactorHistory[CoolMomentIdx].CoolFactorAmount > MostCoolFactor)
				{
					if (IsValidCoolMomentTime(PlayerStates[PlayerIdx]->CoolFactorHistory[CoolMomentIdx].TimeOccurred, CoolPlayTimes))
					{
						MostCoolFactor = PlayerStates[PlayerIdx]->CoolFactorHistory[CoolMomentIdx].CoolFactorAmount;
						BestTimeOccurred = PlayerStates[PlayerIdx]->CoolFactorHistory[CoolMomentIdx].TimeOccurred;
						CoolestPlayer = PlayerStates[PlayerIdx];
					}
				}
			}
		}

		if (CoolestPlayer == nullptr)
		{
			break;
		}

		CoolestPlayers.Add(CoolestPlayer);
		HighestCoolFactors.Add(MostCoolFactor);
		CoolPlayTimes.Add(BestTimeOccurred);

		PlayerStates.Remove(CoolestPlayer);

        UE_LOG(UT, Log, TEXT("PickMostCoolMoments found cool moment #%d at %f by %s"), CoolestPlayers.Num(), BestTimeOccurred, *CoolestPlayer->GetPlayerName());

		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			AUTPlayerController* PC = Cast<AUTPlayerController>(It->Get());
			if (PC)
			{
				PC->ClientQueueCoolMoment(CoolestPlayer->UniqueId, GetWorld()->TimeSeconds - BestTimeOccurred);
			}
		}
	}

	if (bClearCoolMoments)
	{
		for (int32 i = 0; i < GameState->PlayerArray.Num(); i++)
		{
			AUTPlayerState* UTPS = Cast<AUTPlayerState>(GameState->PlayerArray[i]);
			if (UTPS)
			{
				UTPS->CoolFactorHistory.Empty();
			}
		}
	}
}

float AUTGameMode::GetTravelDelay()
{
	return MatchSummaryDelay + MatchSummaryTime;
}

void AUTGameMode::StopReplayRecording()
{
	if (UTIsHandlingReplays() && GetGameInstance() != nullptr)
	{
		GetGameInstance()->StopRecordingReplay();
	}
}

bool AUTGameMode::UTIsHandlingReplays()
{
	// If we're running in PIE, don't record demos
	if (GetWorld() != nullptr && GetWorld()->IsPlayInEditor())
	{
		return false;
	}
	
#if !(UE_BUILD_SHIPPING)
	//Ignore bRecordReplays for non-shipping builds and always record replays on servers.
	return GetNetMode() == ENetMode::NM_DedicatedServer;
#endif

	return bRecordReplays && GetNetMode() == ENetMode::NM_DedicatedServer;
}

void AUTGameMode::TravelToNextMap_Implementation()
{
	if (FUTAnalytics::IsAvailable())
	{
		FUTAnalytics::FireEvent_UTMapTravel(this);
	}

	// Handle tutorial games first.

	if (GetWorld()->GetNetMode() == NM_Standalone)
	{
		APlayerController* LocalPC = GEngine->GetFirstLocalPlayerController(GetWorld());
		UUTLocalPlayer* LP = LocalPC ? Cast<UUTLocalPlayer>(LocalPC->Player) : NULL;
		if (LP)
		{
			// If the player has a pending quickmatch, then it will be triggered here.
			if ( LP->LaunchPendingQuickmatch() )
			{
				return;
			}

			// Otherwise, display the tutorial finished screen so that the user can determine his destination
			else if (TutorialMask > 0)
			{
				if (bNoTrainingMenu)
				{
					TutorialMask = 0;
					GetWorld()->URL.RemoveOption(TEXT("tutorialmask"));
					GetWorld()->URL.RemoveOption(TEXT("tutorialmenu"));
					LP->ReturnToMainMenu();
					return;
				}
				else
				{
					UClass* UMGWidgetClass = LoadClass<UUserWidget>(NULL, TEXT("/Game/RestrictedAssets/Tutorials/Blueprints/TutFinishScreenWidget.TutFinishScreenWidget_C"), NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
					if (UMGWidgetClass)
					{
						UUserWidget* UMGWidget = CreateWidget<UUserWidget>(GetWorld(), UMGWidgetClass);
						if (UMGWidget)
						{
							UMGWidget->AddToViewport(1000);
							return;
						}
					}
				}
			}
		}	
	}

	// Next check for off line challenges.
	FString CurrentMapName = GetWorld()->GetMapName();
	if (bOfflineChallenge)
	{
		// Return to offline challenge menu
		FWorldContext* WorldContext = GEngine->GetWorldContextFromWorld(GetWorld());
		if (WorldContext)
		{
			UE_LOG(UT, Warning, TEXT("ADD showchallenge"));
			APlayerController* LocalPC = GEngine->GetFirstLocalPlayerController(GetWorld());
			UUTLocalPlayer* LP = LocalPC ? Cast<UUTLocalPlayer>(LocalPC->Player) : NULL;
			if (LP)
			{
				LP->HideMenu();
				LP->CloseReplayWindow();
			}
			GEngine->SetClientTravel(GetWorld(),TEXT("/Game/RestrictedAssets/Maps/UT-Entry?ShowChallenge"), TRAVEL_Absolute);
			return;
		}
	}

	UE_LOG(UT,Log,TEXT("TravelToNextMap: %i %i"),bDedicatedInstance,IsGameInstanceServer());

	KickIdlePlayers();


	// Let all remaining players know this game has ended so that they can clear
	// their reconnect data.
	if (bUseMatchmakingSession)
	{
		GetWorldTimerManager().ClearTimer(ServerRestartTimerHandle);

		// Make sure no one tried to reconnect to this matchmaking session
		for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
		{
			AUTPlayerController* Controller = Cast<AUTPlayerController>(*Iterator);
			if (Controller)
			{
				Controller->ClientMatchmakingGameComplete();
			}
		}
	}

	// This is a ranked session, so restart the matchmaking server to it's empty state
	if (bRankedSession)
	{
		SendEveryoneBackToLobby();

		AUTGameSessionRanked* RankedGameSession = Cast<AUTGameSessionRanked>(GameSession);
		if (RankedGameSession)
		{
			RankedGameSession->Restart();
		}
	}
	// This is a quickmatch session.  If there is noone left in the session, then return to matchmaking server to it's empty state
	else if (bUseMatchmakingSession && NumPlayers == 0)
	{
		// Everyone left this quick match
		AUTGameSessionRanked* RankedGameSession = Cast<AUTGameSessionRanked>(GameSession);
		if (RankedGameSession)
		{
			RankedGameSession->Restart();
		}
	}
	// This is a dedicated server.  Everyone left, just restart game per JIRA UT-7376
	else if (NumPlayers == 0)
	{
		// If we are an instance server, notify the hub we are done.
		if (IsGameInstanceServer())
		{
			ShutdownGameInstance();		
		}
		else
		{
			RestartGame();
		}
	}
	else
	{
		// Handle the rcon next map command
		if (!RconNextMapName.IsEmpty())
		{
			FString TravelMapName = RconNextMapName;
			if ( FPackageName::IsShortPackageName(RconNextMapName) )
			{
				FPackageName::SearchForPackageOnDisk(RconNextMapName, &TravelMapName); 
			}

			GetWorld()->ServerTravel(TravelMapName + TEXT("?NextMap=1"), false);
			return;
		}


		if ( PrepareMapVote() )
		{
			SetMatchState(MatchState::MapVoteHappening);
		}
		else
		{
			SendEveryoneBackToLobby();
			RestartGame();
		}
	}
}

void AUTGameMode::KickIdlePlayers()
{

	// LAN games shouldn't kick idle players.
	if (bIsLANGame) return;

	for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
	{
		AUTPlayerState* UTPlayerState = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
		if (!bIgnoreIdlePlayers && UTPlayerState != nullptr && IsPlayerIdle(UTPlayerState) && !UTPlayerState->bIsABot)
		{
			AUTPlayerController* Controller = Cast<AUTPlayerController>(UTPlayerState->GetOwner());
			if (Controller)
			{
				if ( IsGameInstanceServer() )
				{
					Controller->ClientReturnToLobby(true, true);
				}
				else if (GameSession != nullptr)
				{
					GameSession->KickPlayer(Controller, NSLOCTEXT("General", "IdleKick", "You were kicked for being idle."));
				}
			}
		}
		else if (UTPlayerState != nullptr && !UTPlayerState->bIsABot && bGameEnded)
		{
			// Collect non-idle players for the next round
			UUTGameEngine* UTGameEngine = Cast<UUTGameEngine>(GEngine);
			if (UTGameEngine)
			{
				UTGameEngine->PlayerReservations.Add(UTPlayerState->UniqueId.ToString());
			}
		}
	}
}

bool AUTGameMode::PrepareMapVote()
{
	// First, we need a list of possible maps to vote from.  
	
	TArray<FString> MapPrefixList;
	TArray<FAssetData> AllMaps;
	TArray<FString> MapList;	

	MapPrefixList.Add(MapPrefix);
	UTGameState->ScanForMaps(MapPrefixList, AllMaps);


	// If there is an active ruleset, then get a list of allowed maps
	if (!ActiveRuleTag.IsEmpty())
	{
		// Get a pointer to the active ruleset.

		UUTGameInstance* UTGameInstance= Cast<UUTGameInstance>(GetWorld()->GetGameInstance());
		if (UTGameInstance)
		{
			FUTGameRuleset* ActiveRuleset = UTGameInstance->GetRuleset(ActiveRuleTag);
			if (ActiveRuleset)
			{
				ActiveRuleset->GetCompleteMapList(MapList);

				// Now, ensure full names
				for (FString& Map : MapList)
				{
					if (FPackageName::IsShortPackageName(Map))
					{
						for (const FAssetData& MapAsset : AllMaps)
						{
							FString PackageName = MapAsset.PackageName.ToString();
							int32 Pos = PackageName.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd);
							int32 Cnt = PackageName.Len() - Pos - 1;
							PackageName = PackageName.Right(Cnt);
							if (Map == PackageName)
							{
								Map = MapAsset.PackageName.ToString();
								break;
							}
						}
					}
				}
			
			}
		}
	}

	// If we don't have any maps, just add all of the maps to the list
	if (MapList.Num() == 0 )
	{
		for (const FAssetData& MapAsset : AllMaps)
		{
			MapList.Add(MapAsset.PackageName.ToString());
		}
	}

	// Now, go through all of the maps and 
	for (const FAssetData& MapAsset : AllMaps)
	{
		FString PackageName = MapAsset.PackageName.ToString();
		if (MapList.Find(PackageName) != INDEX_NONE)
		{
            const FString* Title = &MapAsset.TagsAndValues.FindTag(NAME_MapInfo_Title).GetValue();
            const FString* Screenshot = &MapAsset.TagsAndValues.FindTag(NAME_MapInfo_ScreenshotReference).GetValue();
			UTGameState->CreateMapVoteInfo(PackageName, (Title != NULL && !Title->IsEmpty()) ? *Title : *MapAsset.AssetName.ToString(), Screenshot != nullptr ? *Screenshot : TEXT(""));
		}
	}

	return (UTGameState->MapVoteList.Num() > 0);
}

void AUTGameMode::SetEndGameFocus(AUTPlayerState* Winner)
{
	if (!Winner)
	{
		return; // It's possible to call this with Winner == NULL if timelimit is hit with noone on the server
	}
	EndGameFocus = Cast<AController>(Winner->GetOwner()) ?  Cast<AController>(Winner->GetOwner())->GetPawn() : nullptr;
	AUTRemoteRedeemer* Missile = Cast<AUTRemoteRedeemer>(EndGameFocus);
	if (Missile && Missile->Driver)
	{
		EndGameFocus = Missile->Driver;
	}
	if ( (EndGameFocus == NULL) && (Cast<AController>(Winner->GetOwner()) != NULL) )
	{
		// If the controller of the winner does not have a pawn, give him one.
		RestartPlayer(Cast<AController>(Winner->GetOwner()));
		EndGameFocus = Cast<AController>(Winner->GetOwner())->GetPawn();
	}

	if ( EndGameFocus != NULL )
	{
		EndGameFocus->bAlwaysRelevant = true;
	}

	for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
		if (PC != NULL)
		{
			PC->SetViewTarget(EndGameFocus);
			PC->ClientGameEnded(EndGameFocus, (PC->PlayerState != NULL) && (PC->PlayerState == Winner));
		}
	}
}

void AUTGameMode::BroadcastDeathMessage(AController* Killer, AController* Other, TSubclassOf<UDamageType> DamageType)
{
	if (DeathMessageClass != NULL)
	{
		if ( (Killer == Other) || (Killer == NULL) )
		{
			// Message index 1 reserved for suicides
			BroadcastLocalized(this, DeathMessageClass, 1, NULL, Other->PlayerState, DamageType);
		}
		else
		{
			// MessageIndex 10s digit represents multikill level
			// 100s plus represents spree level
			int32 MessageIndex = 0;
			AUTPlayerState* KillerPS = Cast<AUTPlayerState>(Killer->PlayerState);
			if (KillerPS)
			{
				MessageIndex = 10 * FMath::Clamp(KillerPS->MultiKillLevel, 0, 9);
				if (KillerPS->Spree % 5 == 0)
				{
					MessageIndex += 100 * KillerPS->Spree / 5;
				}
				if (KillerPS->bAnnounceWeaponSpree)
				{
					MessageIndex += 1000;
				}
				if (KillerPS->bAnnounceWeaponReward)
				{
					MessageIndex += 10000;
				}
			}
			BroadcastLocalized(this, DeathMessageClass, MessageIndex, Killer->PlayerState, Other->PlayerState, DamageType);
		}
	}
}

void AUTGameMode::PlayEndOfMatchMessage()
{
	if (!UTGameState || !UTGameState->WinnerPlayerState || (bBasicTrainingGame && !bDamageHurtsHealth && (GetNetMode() == NM_Standalone)))
	{
		return;
	}
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
		if (PC && (PC->PlayerState != NULL) && !PC->PlayerState->bOnlySpectator)
		{
			PC->ClientReceiveLocalizedMessage(VictoryMessageClass, ((UTGameState->WinnerPlayerState == PC->PlayerState) ? 1 : 0), UTGameState->WinnerPlayerState, PC->PlayerState, NULL);
		}
	}
}

bool AUTGameMode::AllowSuicideBy(AUTPlayerController* PC)
{
	if (GetMatchState() != MatchState::InProgress)
	{
		return false;
	}
	if (GetWorld()->WorldType == EWorldType::PIE || GetNetMode() == NM_Standalone)
	{
		return true;
	}
	return (PC->GetPawn() != nullptr) && (GetWorld()->TimeSeconds - PC->GetPawn()->CreationTime > 10.0f);
}

void AUTGameMode::RestartPlayer(AController* aPlayer)
{
    if ((aPlayer == NULL) || (aPlayer->PlayerState == NULL) || aPlayer->PlayerState->GetPlayerName().IsEmpty())
	{
		UE_LOG(UT, Warning, TEXT("RestartPlayer with a bad player, bad playerstate, or empty player name"));
		return;
	}
	if (aPlayer->PlayerState->bOnlySpectator)
	{
		return;
	}
	AUTPlayerController* UTPC = Cast<AUTPlayerController>(aPlayer);
	bool bWantsToWarmUp = UTPC ? UTPC->UTPlayerState  && UTPC->UTPlayerState->bIsWarmingUp : true;
	if (!IsMatchInProgress() && ((GetMatchState() != MatchState::WaitingToStart) || !bWantsToWarmUp || (GetNetMode() == NM_Standalone)) && (!UTGameState || !UTGameState->ActiveLineUpHelper || !UTGameState->ActiveLineUpHelper->bIsPlacingPlayers))
	{
		return;
	}

	{
		TGuardValue<bool> FlagGuard(bSetPlayerDefaultsNewSpawn, true);
		Super::RestartPlayer(aPlayer);
	
		// apply any health changes
		AUTCharacter* UTC = Cast<AUTCharacter>(aPlayer->GetPawn());
		if (UTC != NULL && UTC->GetClass()->GetDefaultObject<AUTCharacter>()->Health == 0)
		{
			UTC->SetInitialHealth();
		}
	}

	AUTBot* Bot = Cast<AUTBot>(aPlayer);
	if (Bot)
	{
		if (GetMatchState() == MatchState::WaitingToStart)
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(Bot->PlayerState);
			if (PS)
			{
				PS->bIsWarmingUp = true;
			}
		}
		Bot->LastRespawnTime = GetWorld()->TimeSeconds;
		if (bAutoAdjustBotSkill)
		{
			Bot->AutoUpdateSkillFor(this);
		}
	}

	if (UTPC)
	{
		// forced camera cut is a good time to GC
		UTPC->ClientForceGarbageCollection();
		if (!UTPC->IsLocalController())
		{
			UTPC->ClientSwitchToBestWeapon();
		}
		UTPC->ClientStopKillcam();
	}

	AUTPlayerState* UTPS = Cast<AUTPlayerState>(aPlayer->PlayerState);
	if (UTPS)
	{
		// clear spawn choices
		UTPS->RespawnChoiceA = nullptr;
		UTPS->RespawnChoiceB = nullptr;

		// clear multikill in progress
		UTPS->LastKillTime = -100.f;

		// clear in life stats
		UTPS->ThisLifeDamageDone = 0;
		UTPS->ThisLifeKills = 0;

		//TODO: Talk to Matt O. about merging this functionality of this and the bot's LastRespawnTime.
		UTPS->LastSpawnTime = GetWorld()->TimeSeconds;
	}
}

void AUTGameMode::GiveDefaultInventory(APawn* PlayerPawn)
{
	AUTCharacter* UTCharacter = Cast<AUTCharacter>(PlayerPawn);
	if (UTCharacter != nullptr)
	{
		if (bClearPlayerInventory)
		{
			UTCharacter->DefaultCharacterInventory.Empty();
		}
		if (bPlayersStartWithArmor && StartingArmorClass)
		{
			if (!StartingArmorClass.GetDefaultObject()->HandleGivenTo(UTCharacter))
			{
				UTCharacter->AddInventory(GetWorld()->SpawnActor<AUTInventory>(StartingArmorClass, FVector(0.0f), FRotator(0.f, 0.f, 0.f)), true);
			}
		}
		UTCharacter->AddDefaultInventory(DefaultInventory);
        AUTPlayerState * PS = Cast<AUTPlayerState>(UTCharacter->GetPlayerState());
		if (StartingArmorClass && PS && PS->PlayerCard && (PS->PlayerCard->ExtraArmor > 0))
		{
			UTCharacter->SetArmorAmount(StartingArmorClass.GetDefaultObject(), FMath::Max(FMath::Max(UTCharacter->GetArmorAmount(), PS->PlayerCard->ExtraArmor), FMath::Min(100, UTCharacter->GetArmorAmount() + PS->PlayerCard->ExtraArmor)));
		}
		AUTGameVolume* GV = UTCharacter->UTCharacterMovement ? Cast<AUTGameVolume>(UTCharacter->UTCharacterMovement->GetPhysicsVolume()) : nullptr;
		if (GV && GV->bIsTeamSafeVolume)
		{
			if ((GV->TeamLockers.Num() > 0) && GV->TeamLockers[0])
			{
				GV->TeamLockers[0]->GiveAmmo(UTCharacter);
			}
		}
	}
}

/* 
  Make sure pawn properties are back to default
  Also add default inventory
*/
void AUTGameMode::SetPlayerDefaults(APawn* PlayerPawn)
{
	Super::SetPlayerDefaults(PlayerPawn);

	if (BaseMutator != NULL)
	{
		BaseMutator->ModifyPlayer(PlayerPawn, bSetPlayerDefaultsNewSpawn);
	}

	if (bSetPlayerDefaultsNewSpawn)
	{
		GiveDefaultInventory(PlayerPawn);
		NotifyPlayerDefaultsSet(PlayerPawn);
	}
}

bool AUTGameMode::ShouldSpawnAtStartSpot(AController* Player)
{
	if ( Player && Cast<APlayerStartPIE>(Player->StartSpot.Get()) )
	{
		return true;
	}

	return false;
}


AActor* AUTGameMode::FindPlayerStart_Implementation(AController* Player, const FString& IncomingName)
{
	AActor* const Best = Super::FindPlayerStart_Implementation(Player, IncomingName);
	if (Best)
	{
		LastStartSpot = Best;
	}

	return Best;
}

FString AUTGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	FString ErrorMessage = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	AUTPlayerState* NewPlayerState = Cast<AUTPlayerState>(NewPlayerController->PlayerState);
	if (bHasRespawnChoices && NewPlayerState && !NewPlayerState->bIsSpectator)
	{
		bCheckAgainstPotentialStarts = true;
		NewPlayerState->RespawnChoiceA = nullptr;
		NewPlayerState->RespawnChoiceB = nullptr;
		NewPlayerState->RespawnChoiceA = Cast<APlayerStart>(ChoosePlayerStart(NewPlayerController));
		NewPlayerState->RespawnChoiceB = Cast<APlayerStart>(ChoosePlayerStart(NewPlayerController));
		NewPlayerState->bChosePrimaryRespawnChoice = true;
		bCheckAgainstPotentialStarts = false;
	}

	return ErrorMessage;
}

AActor* AUTGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	AUTPlayerState* UTPS = Player != NULL ? Cast<AUTPlayerState>(Player->PlayerState) : NULL;
	if (bHasRespawnChoices && UTPS && UTPS->RespawnChoiceA != nullptr && UTPS->RespawnChoiceB != nullptr)
	{
		AUTBot* B = Cast<AUTBot>(Player);
		if (B != NULL)
		{
			// give bot selection now
			TArray<APlayerStart*> Choices;
			Choices.Add(UTPS->RespawnChoiceA);
			Choices.Add(UTPS->RespawnChoiceB);
			APlayerStart* Pick = B->PickSpawnPoint(Choices);
			return (Pick != NULL) ? Pick : UTPS->RespawnChoiceA;
		}
		else if (UTPS->bChosePrimaryRespawnChoice)
		{
			return UTPS->RespawnChoiceA;
		}
		else
		{
			return UTPS->RespawnChoiceB;
		}
	}
	else if (!bHasRespawnChoices && UTPS && UTPS->RespawnChoiceA != nullptr)
	{
		return UTPS->RespawnChoiceA;
	}

	if (PlayerStarts.Num() == 0)
	{
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			PlayerStarts.Add(*It);
		}
		if (PlayerStarts.Num() == 0)
		{
			return Super::ChoosePlayerStart_Implementation(Player);
		}

		// Pre-randomize
		for (int32 i = 0; i < PlayerStarts.Num() / 2; i++)
		{
			APlayerStart* Swap = PlayerStarts[i];
			int32 RandIndex = FMath::Min(PlayerStarts.Num() - 1, PlayerStarts.Num() / 2 + FMath::RandHelper(PlayerStarts.Num() / 2 - 1));
			PlayerStarts[i] = PlayerStarts[RandIndex];
			PlayerStarts[RandIndex] = Swap;
		}
	}

	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		for (TActorIterator<APlayerStart> It(GetWorld()); It; ++It)
		{
			// Always prefer the first "Play from Here" PlayerStart, if we find one while in PIE mode
			if (Cast<APlayerStartPIE>(*It))
			{
				return *It;
			}
		}
	}

	// Randomize each time
	int32 NumToRandomize = FMath::Clamp(PlayerStarts.Num()/4, 0, 4);
	for (int32 i = 0; i < NumToRandomize; i++)
	{
		APlayerStart* Swap = PlayerStarts[i];
		int32 RandIndex = FMath::RandHelper(PlayerStarts.Num()/2);
		PlayerStarts[i] = PlayerStarts[RandIndex];
		PlayerStarts[RandIndex] = Swap;
	}

	float BestRating = -20.f;
	APlayerStart* BestStart = NULL;
	int32 BestStartIndex = 0;
	int32 AvoidedStarts = PlayerStarts.Num() / 2;
	for ( int32 i=0; i<PlayerStarts.Num()-AvoidedStarts; i++ )
	{
		APlayerStart* P = PlayerStarts[i];
		float NewRating = RatePlayerStart(P,Player);

		if ( NewRating > BestRating )
		{
			BestRating = NewRating;
			BestStart = P;
			BestStartIndex = i;
			if (NewRating >= 30.0f)
			{
				// this PlayerStart is good enough
				break;
			}
		}
	}
	if ((AvoidedStarts > 0) && (BestRating < 0.f))
	{
		for (int32 i = PlayerStarts.Num() - AvoidedStarts; i<PlayerStarts.Num(); i++)
		{
			APlayerStart* P = PlayerStarts[i];

			float NewRating = RatePlayerStart(P, Player);

			if (NewRating > BestRating)
			{
				BestRating = NewRating;
				BestStart = P;
				BestStartIndex = i;
			}
		}
	}
	if (BestStart)
	{
		// move to back to avoid picking again
		check(BestStart == PlayerStarts[BestStartIndex]);
		PlayerStarts.RemoveAt(BestStartIndex);
		PlayerStarts.Add(BestStart);
	}
	return (BestStart != NULL) ? BestStart : Super::ChoosePlayerStart_Implementation(Player);
}

float AUTGameMode::RatePlayerStart(APlayerStart* P, AController* Player)
{
	float Score = 29.0f + 1.3f*FMath::FRand();

	AActor* LastSpot = (Player != NULL && Player->StartSpot.IsValid()) ? Player->StartSpot.Get() : NULL;
	AUTPlayerState *UTPS = Player ? Cast<AUTPlayerState>(Player->PlayerState) : NULL;
	if (P == LastStartSpot || (LastSpot != NULL && P == LastSpot) || AvoidPlayerStart(Cast<AUTPlayerStart>(P)))
	{
		// avoid re-using starts
		return -8.0f;
	}
	FVector StartLoc = P->GetActorLocation() + AUTCharacter::StaticClass()->GetDefaultObject<AUTCharacter>()->BaseEyeHeight;
	if (UTPS && UTPS->RespawnChoiceA)
	{
		if (P == UTPS->RespawnChoiceA)
		{
			// make sure to have two choices
			return -5.f;
		}
		// try to get far apart choices
		float Dist = (UTPS->RespawnChoiceA->GetActorLocation() - StartLoc).Size();
		if (Dist < 5000.0f)
		{
			Score -= 5.f;
		}
	}

	if (Player != NULL)
	{
		bool bTwoPlayerGame = (NumPlayers + NumBots == 2);
		for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
		{
			AController* OtherController = Iterator->Get();
			ACharacter* OtherCharacter = Cast<ACharacter>( OtherController->GetPawn());

            if ( OtherCharacter && OtherCharacter->GetPlayerState() && (!UTGameState || !UTGameState->IsLineUpActive()) )
			{
				if (FMath::Abs(StartLoc.Z - OtherCharacter->GetActorLocation().Z) < P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() + OtherCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()
					&& (StartLoc - OtherCharacter->GetActorLocation()).Size2D() < P->GetCapsuleComponent()->GetScaledCapsuleRadius() + OtherCharacter->GetCapsuleComponent()->GetScaledCapsuleRadius())
				{
					// overlapping - would telefrag
					return -10.f;
				}
				Score += AdjustNearbyPlayerStartScore(Player, OtherController, OtherCharacter, StartLoc, P);
			}
			if ((bCheckAgainstPotentialStarts || !OtherCharacter) && OtherController->PlayerState && !OtherController->PlayerState->bOnlySpectator)
			{
				// make sure no one else has this start as a pending choice
				AUTPlayerState* OtherUTPS = Cast<AUTPlayerState>(OtherController->PlayerState);
				if (OtherUTPS)
				{
					if (P == OtherUTPS->RespawnChoiceA || P == OtherUTPS->RespawnChoiceB)
					{
						return -5.f;
					}
					if (bTwoPlayerGame)
					{
						// avoid choosing starts near a pending start
						if (OtherUTPS->RespawnChoiceA)
						{
							float Dist = (OtherUTPS->RespawnChoiceA->GetActorLocation() - StartLoc).Size();
							Score -= 7.f * FMath::Max(0.f, (5000.f - Dist) / 5000.f);
						}
						if (OtherUTPS->RespawnChoiceB)
						{
							float Dist = (OtherUTPS->RespawnChoiceB->GetActorLocation() - StartLoc).Size();
							Score -= 7.f * FMath::Max(0.f, (5000.f - Dist) / 5000.f);
						}
					}
				}
			}

		}
	}
	return FMath::Max(Score, 0.2f);
}

float AUTGameMode::AdjustNearbyPlayerStartScore(const AController* Player, const AController* OtherController, const ACharacter* OtherCharacter, const FVector& StartLoc, const APlayerStart* P)
{
	float ScoreAdjust = 0.f;
	float NextDist = (OtherCharacter->GetActorLocation() - StartLoc).Size();
	bool bTwoPlayerGame = (NumPlayers + NumBots == 2);
	if (((NextDist < 5000.0f) || bTwoPlayerGame) && !UTGameState->OnSameTeam(Player, OtherController))
	{
		static FName NAME_RatePlayerStart = FName(TEXT("RatePlayerStart"));
        bool bIsLastKiller = (OtherCharacter->GetPlayerState() == Cast<AUTPlayerState>(Player->PlayerState)->LastKillerPlayerState);
		if (bIsInstagib && !bIsLastKiller)
		{
			return 0.f;
		}
		if (!GetWorld()->LineTraceTestByChannel(StartLoc, OtherCharacter->GetActorLocation() + FVector(0.f, 0.f, OtherCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), COLLISION_TRACE_WEAPONNOCHARACTER, FCollisionQueryParams(NAME_RatePlayerStart, false)))
		{
			// Avoid the last person that killed me
			if (bIsLastKiller)
			{
				ScoreAdjust -= 7.f;
			}

			ScoreAdjust -= (5.f - 0.0003f * NextDist);
		}
	}
	return ScoreAdjust;
}

bool AUTGameMode::AvoidPlayerStart(AUTPlayerStart* P)
{
	return P && (!bTeamGame && P->bIgnoreInNonTeamGame);
}

bool AUTGameMode::PlayerCanRestart_Implementation(APlayerController* Player)
{
	if (Player == NULL || Player->IsPendingKillPending())
	{
		return false;
	}
	if (!IsMatchInProgress())
	{
		AUTPlayerController* UTPC = Cast<AUTPlayerController>(Player);
		if (!UTPC || (GetMatchState() != MatchState::WaitingToStart) || !UTPC->UTPlayerState || !UTPC->UTPlayerState->bIsWarmingUp)
		{
			return false;
		}
	}

	// Ask the player controller if it's ready to restart as well
	return Player->CanRestartPlayer();
}

void AUTGameMode::InitializeHUDForPlayer_Implementation(APlayerController* NewPlayer)
{
	AUTPlayerController* UTNewPlayer = Cast<AUTPlayerController>(NewPlayer);
	if (UTNewPlayer != NULL)
	{
		UTNewPlayer->HUDClass = HUDClass;
		if (Cast<UNetConnection>(UTNewPlayer->Player) == NULL)
		{
			UTNewPlayer->OnRep_HUDClass();
		}
	}
	else
	{
		Super::InitializeHUDForPlayer_Implementation(NewPlayer);
	}
}

UClass* AUTGameMode::GetDefaultPawnClassForController_Implementation(AController* InController)
{
	AUTMonsterAI* MonsterAI = Cast<AUTMonsterAI>(InController);
	return (MonsterAI != nullptr && MonsterAI->PawnClass != nullptr) ? *MonsterAI->PawnClass : Super::GetDefaultPawnClassForController_Implementation(InController);
}

bool AUTGameMode::ReadyToStartMatch_Implementation()
{
	if (GetWorld()->IsPlayInEditor() || !bDelayedStart)
	{
		// starting on first frame has side effects in PIE because of differences in ordering; components haven't been initialized/registered yet...
		if (GetWorld()->TimeSeconds == 0.0f)
		{
			GetWorldTimerManager().SetTimerForNextTick(this, &AUTGameMode::StartMatch);
			return false;
		}
		else
		{
			// PIE is always ready to start.
			return true;
		}
	}

	if (GetMatchState() == MatchState::WaitingToStart)
	{
		if (bRankedSession)
		{
			if (ExpectedPlayerCount != 0 && ExpectedPlayerCount == NumPlayers)
			{
				// Clear this to avoid penalizing anyone that quit before game started
				InactivePlayerArray.Empty();

				LockSession();
			}
			else
			{
				// Ranked doesn't need to run a countdown unless all the players are in
				return false;
			}
		}

		int32 NeededPlayers = GameSession ? GameSession->MaxPlayers : DefaultMaxPlayers;
		UTGameState->PlayersNeeded = FMath::Max(0, NeededPlayers - NumPlayers);
		UTGameState->bHaveMatchHost = false;
		bool bHostIsReady = false;
		if (!HostIdString.IsEmpty())
		{
			for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
			{
				AUTPlayerState* PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
				if (PS != NULL && !PS->bIsInactive && HostIdString.Equals(PS->UniqueId.ToString(), ESearchCase::IgnoreCase))
				{
					UTGameState->bHaveMatchHost = true;
					bHostIsReady = bCasterReady && (!bRequireFull || (UTGameState->PlayersNeeded == 0));
					PS->bIsMatchHost = true;
				}
				else if (PS)
				{
					PS->bIsMatchHost = false;
				}
			}
		}
		StartPlayTime = (NumPlayers > 0) ? FMath::Min(StartPlayTime, GetWorld()->GetTimeSeconds()) : 10000000.f;
		float ElapsedWaitTime = FMath::Max(0.f, GetWorld()->GetTimeSeconds() - StartPlayTime);

		if (!bRequireReady && !bRequireFull && !bRankedSession && (GetWorld()->GetTimeSeconds() - StartPlayTime > MaxWaitForPlayers))
		{
			int32 MinPlayersToStart = 1;
			UTGameState->PlayersNeeded = FMath::Max(0, MinPlayersToStart - NumPlayers);
		}
		if (((GetNetMode() == NM_Standalone) || bDevServer || bHostIsReady || (UTGameState->PlayersNeeded == 0)) && (NumPlayers + NumSpectators > 0))
		{
			bool bReadyFulfilled = true;
			if (UTGameState->bHaveMatchHost)
			{
				bReadyFulfilled = bHostIsReady;
			}
			else if (bRequireReady || (GetNetMode() == NM_Standalone) || bDevServer)
			{
				// Count how many ready players we have
				int32 WarmupCount = 0;
				int32 AllCount = 0;
				for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
				{
					AUTPlayerState* PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
					if (PS != NULL && !PS->bOnlySpectator && !PS->bIsInactive)
					{
						if (PS->bIsWarmingUp || PS->bIsABot)
						{
							WarmupCount++;
						}
						AllCount++;
					}
				}
				bReadyFulfilled = bCasterControl ? bCasterReady : (WarmupCount == AllCount);
			}
			float RemainingStartDelay = StartDelay;
			if (bReadyFulfilled)
			{
				RemainingStartDelay = UTGameState->bHaveMatchHost ? 0 : StartDelay - (GetWorld()->GetTimeSeconds() - LastMatchNotReady);
				if (GetNetMode() == NM_Standalone)
				{
					RemainingStartDelay = 0.f;
				}
				UTGameState->SetRemainingTime(FMath::Max(0.f, RemainingStartDelay));
				if (!bRankedSession && !bOfflineChallenge && (RemainingStartDelay < 2) && !bForceNoBots && !bDevServer && (GetWorld()->WorldType != EWorldType::PIE))
				{
					// if not competitive match, fill with bots if have waited long enough
					BotFillCount = FMath::Max(BotFillCount, AdjustedBotFillCount());
					CheckBotCount();
				}
			}
			return (RemainingStartDelay <= 0.f);
		}
		LastMatchNotReady = GetWorld()->GetTimeSeconds();
		bool bUpdateWaitCountdown = UTGameState && (ElapsedWaitTime > 0);
		int32 WaitCountDown = MaxWaitForPlayers;
		WaitCountDown -= ElapsedWaitTime;
		UTGameState->SetRemainingTime(FMath::Max(0, WaitCountDown));
	}
	return false;
}

/**
 *	Overwriting all of these functions to work the way I think it should.  Really, the match state should
 *  only be in 1 place otherwise it's prone to mismatch errors.  I'm chosen the GameState because it's
 *  replicated and will be available on clients.
 **/
bool AUTGameMode::HasMatchStarted() const
{
	return UTGameState->HasMatchStarted();
}

bool AUTGameMode::IsMatchInProgress() const
{
	return UTGameState->IsMatchInProgress();
}

bool AUTGameMode::HasMatchEnded() const
{
	return UTGameState->HasMatchEnded();
}

bool AUTGameMode::IsPlayerIdle(AUTPlayerState* PS)
{
	return PS && !PS->bIsABot && !PS->bOnlySpectator && !PS->bOutOfLives && !PS->bIsInactive && (GetWorld()->GetTimeSeconds() - PS->LastActiveTime > IDLE_TIMEOUT_TIME) && HasMatchStarted();
}

/**	I needed to rework the ordering of SetMatchState until it can be corrected in the engine. **/
void AUTGameMode::SetMatchState(FName NewState)
{
	// Reset the idle state on all players
	if (NewState == MatchState::InProgress)
	{
		for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
		{
			AUTPlayerState* UTPlayerState = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
			if (UTPlayerState)
			{
				UTPlayerState->NotIdle();
			}
		}
	}

	if (MatchState == NewState)
	{
		return;
	}

	MatchState = NewState;
	if (UTGameState)
	{
		UTGameState->SetMatchState(NewState);
	}

	CallMatchStateChangeNotify();
	K2_OnSetMatchState(NewState);

	if (BaseMutator != NULL)
	{
		BaseMutator->NotifyMatchStateChange(MatchState);
	}
}

void AUTGameMode::CallMatchStateChangeNotify()
{
	// Call change callbacks

	if (MatchState == MatchState::WaitingToStart)
	{
		HandleMatchIsWaitingToStart();
	}
	else if (MatchState == MatchState::CountdownToBegin)
	{
		HandleCountdownToBegin();
	}
	else if (MatchState == MatchState::PlayerIntro)
	{
		HandlePlayerIntro();
	}
	else if (MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if (MatchState == MatchState::WaitingPostMatch)
	{
		HandleMatchHasEnded();
	}
	else if (MatchState == MatchState::LeavingMap)
	{
		HandleLeavingMap();
	}
	else if (MatchState == MatchState::Aborted)
	{
		HandleMatchAborted();
	}
	else if (MatchState == MatchState::MatchEnteringOvertime)
	{
		HandleEnteringOvertime();
	}
	else if (MatchState == MatchState::MatchIsInOvertime)
	{
		HandleMatchInOvertime();
	}
	else if (MatchState == MatchState::MapVoteHappening)
	{
		HandleMapVote();
	}
}

void AUTGameMode::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();

	// save AI data only after completed matches
	AUTRecastNavMesh* NavData = GetUTNavData(GetWorld());
	if (NavData != NULL)
	{
		NavData->SaveMapLearningData();
	}

	UTGameState->CreateLineUp(LineUpTypes::PostMatch);
}

void AUTGameMode::HandleEnteringOvertime()
{
	SetMatchState(MatchState::MatchIsInOvertime);
}

void AUTGameMode::HandleMatchInOvertime()
{
	// Send the overtime message....
	BroadcastLocalized( this, UUTGameMessage::StaticClass(), 1, NULL, NULL, NULL);
}

void AUTGameMode::HandlePlayerIntro()
{
	RemoveExtraBots();
	CheckBotCount();
	if (!UTGameState || !UTGameState->IsLineUpActive())
	{
		RemoveAllPawns();
	}
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(It->Get());
		if (PC && PC->UTPlayerState)
		{
			PC->UTPlayerState->bIsWarmingUp = false;
			PC->UTPlayerState->RespawnChoiceA = nullptr;
			PC->UTPlayerState->RespawnChoiceB = nullptr;
		}
	}

	if (UTGameState)
	{
		UTGameState->CreateLineUp(LineUpTypes::Intro);
	}

	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, this, &AUTGameMode::EndPlayerIntro, MatchIntroTime*GetActorTimeDilation(), false);
}

void AUTGameMode::EndPlayerIntro()
{
	bCheckAgainstPotentialStarts = true;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(It->Get());
		if (PC && PC->UTPlayerState)
		{
			PC->UTPlayerState->bIsWarmingUp = false;
			PC->UTPlayerState->RespawnChoiceA = nullptr;
			PC->UTPlayerState->RespawnChoiceB = nullptr;
			if (!PC->UTPlayerState->bIsSpectator)
			{
				PC->UTPlayerState->RespawnChoiceA = Cast<APlayerStart>(ChoosePlayerStart(PC));
				if (bHasRespawnChoices)
				{
					PC->UTPlayerState->RespawnChoiceB = Cast<APlayerStart>(ChoosePlayerStart(PC));
				}
				PC->UTPlayerState->bChosePrimaryRespawnChoice = true;
				if (PC->GetSpectatorPawn() && PC->UTPlayerState->RespawnChoiceA)
				{
					PC->GetSpectatorPawn()->SetActorLocationAndRotation(PC->UTPlayerState->RespawnChoiceA->GetActorLocation(), PC->UTPlayerState->RespawnChoiceA->GetActorRotation());
					PC->SetControlRotation(PC->UTPlayerState->RespawnChoiceA->GetActorRotation());
				}
			}
		}
	}

	//Make sure we aren't still in a line up.
	UTGameState->ClearLineUp();

	bCheckAgainstPotentialStarts = false;
	SetMatchState(MatchState::CountdownToBegin);
}

void AUTGameMode::HandleCountdownToBegin()
{
	// Currently broken by replay streaming
	/*
	if (bRecordDemo)
	{
		FString MapName = GetOutermost()->GetName();
		GetWorld()->Exec(GetWorld(), *FString::Printf(TEXT("Demorec %s"), *DemoFilename.Replace(TEXT("%m"), *MapName.RightChop(MapName.Find(TEXT("/"), ESearchCase::IgnoreCase, ESearchDir::FromEnd) + 1))));
	}*/
	CountDown = 3;
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, this, &AUTGameMode::CheckCountDown, 1.f*GetActorTimeDilation(), false);
}

void AUTGameMode::CheckCountDown()
{
	if (CountDown >0)
	{
		// Broadcast the localized message saying when the game begins.
		BroadcastLocalized( this, UUTCountDownMessage::StaticClass(), CountDown, NULL, NULL, NULL);
		FTimerHandle TempHandle;
		GetWorldTimerManager().SetTimer(TempHandle, this, &AUTGameMode::CheckCountDown, 1.f*GetActorTimeDilation(), false);
		CountDown--;
	}
	else
	{
		BeginGame();
	}
}

void AUTGameMode::CheckGameTime()
{
	if (IsMatchInProgress() && !HasMatchEnded() && TimeLimit > 0 && UTGameState->GetRemainingTime() <= 0)
	{
		// Game should be over.. look to see if we need to go in to overtime....	

		bool bTied = false;
		AUTPlayerState* Winner = IsThereAWinner(bTied);

		if (!bAllowOvertime || !bTied)
		{
			EndGame(Winner, FName(TEXT("TimeLimit")));			
		}
		else if (bAllowOvertime)
		{
			if (!UTGameState->IsMatchInOvertime())
			{
				// add 60 seconds for overtime
				if (bTeamGame)
				{
					UTGameState->SetRemainingTime(120);
				}
				else
				{
					UTGameState->bStopGameClock = true;
				}
				SetMatchState(MatchState::MatchEnteringOvertime);
			}
			else if (bTeamGame)
			{
				UTGameState->SetRemainingTime(120);
			}
		}
	}
}

AUTPlayerState* AUTGameMode::IsThereAWinner_Implementation(bool& bTied)
{
	AUTPlayerState* BestPlayer = NULL;
	float BestScore = 0.0;

	for (int32 PlayerIdx=0; PlayerIdx < UTGameState->PlayerArray.Num();PlayerIdx++)
	{
		if ((UTGameState->PlayerArray[PlayerIdx] != NULL) && !UTGameState->PlayerArray[PlayerIdx]->bOnlySpectator)
		{
			if (BestPlayer == NULL || UTGameState->PlayerArray[PlayerIdx]->Score > BestScore)
			{
				BestPlayer = Cast<AUTPlayerState>(UTGameState->PlayerArray[PlayerIdx]);
				BestScore = BestPlayer->Score;
				bTied = 0;
			}
			else if (UTGameState->PlayerArray[PlayerIdx]->Score == BestScore)
			{
				bTied = 1;
			}
		}
	}
	return BestPlayer;
}

bool AUTGameMode::CheckScore_Implementation(AUTPlayerState* Scorer)
{
	if (Scorer != NULL)
	{
		if ((GoalScore > 0) && (Scorer->Score >= GoalScore))
		{
			EndGame(Scorer, FName(TEXT("fraglimit")));
		}
	}
	return true;
}

void AUTGameMode::OverridePlayerState(APlayerController* PC, APlayerState* OldPlayerState)
{
	Super::OverridePlayerState(PC, OldPlayerState);

	// if we're in this function GameMode swapped PlayerState objects so we need to update the precasted copy
	AUTPlayerController* UTPC = Cast<AUTPlayerController>(PC);
	if (UTPC != NULL)
	{
		UTPC->UTPlayerState = Cast<AUTPlayerState>(UTPC->PlayerState);
	}
}

void AUTGameMode::GenericPlayerInitialization(AController* C)
{
	Super::GenericPlayerInitialization(C);

	if (BaseMutator != NULL)
	{
		BaseMutator->PostPlayerInit(C);
	}

	UpdatePlayersPresence();

	if (IsGameInstanceServer() && LobbyBeacon)
	{
		if (C && Cast<AUTPlayerController>(C) && C->PlayerState)
		{
			AUTPlayerState* PlayerState = Cast<AUTPlayerState>(C->PlayerState);
			if (PlayerState && !PlayerState->bIsDemoRecording && !PlayerState->bIsABot)
			{
				LobbyBeacon->UpdatePlayer(this, PlayerState, false);
			}
		}
	}
}

void AUTGameMode::PostLogin( APlayerController* NewPlayer )
{
	TSubclassOf<AHUD> SavedHUDClass = HUDClass;

	AUTPlayerController* UTPC = Cast<AUTPlayerController>(NewPlayer);
	bool bIsCastingGuidePC = UTPC != NULL && UTPC->CastingGuideViewIndex >= 0 && GameState->PlayerArray.IsValidIndex(UTPC->CastingGuideViewIndex);
	if (bIsCastingGuidePC)
	{
		HUDClass = CastingGuideHUDClass;
	}

	Super::PostLogin(NewPlayer);
	
	// in case PlayerState got replaced with an inactive PlayerState
	AUTPlayerState* PS = Cast<AUTPlayerState>(NewPlayer->PlayerState);
	if (PS != NULL)
	{
		PS->NotIdle();
	}
	
	NewPlayer->ClientSetLocation(NewPlayer->GetFocalLocation(), NewPlayer->GetControlRotation());
	if (GameSession != NULL)
	{
		AUTGameSession* UTGameSession = Cast<AUTGameSession>(GameSession);
		if (UTGameSession != NULL)
		{
			UTGameSession->UpdateGameState();
		}
	}
	
	if (bIsCastingGuidePC)
	{
		// TODO: better choice of casting views
		UTPC->ServerViewPlayerState(Cast<AUTPlayerState>(GameState->PlayerArray[UTPC->CastingGuideViewIndex]));
	}

	CheckBotCount();

	// Check if a (re)joining player is the leader
	FindAndMarkHighScorer();

	HUDClass = SavedHUDClass;

	if (bIsQuickMatch && bUseMatchmakingSession)
	{
		GetWorldTimerManager().ClearTimer(ServerRestartTimerHandle);
	}

	if (UTGameState)
	{
		UTGameState->SetRemainingTime(UTGameState->GetRemainingTime());
		if (UTGameState->ActiveLineUpHelper)
		{
			UTGameState->ActiveLineUpHelper->ServerOnPlayerChange(Cast<AUTPlayerState>(NewPlayer->PlayerState));
		}
	}
	if (UTPC && FUTAnalytics::IsAvailable() && (GetNetMode() == NM_DedicatedServer))
	{
		FUTAnalytics::FireEvent_UTServerPlayerJoin(this, UTPC->UTPlayerState);
	}

	if (bForceWarmup && UTPC && PS && UTGameState && (UTGameState->GetMatchState() == MatchState::WaitingToStart))
	{
		PS->bIsWarmingUp = true;
		PS->ForceNetUpdate();
		UTPC->ClientUpdateWarmup(PS->bIsWarmingUp);
		RestartPlayer(UTPC);
	}
}

void AUTGameMode::SwitchToCastingGuide(AUTPlayerController* NewCaster)
{
	// TODO: check if allowed
	if (NewCaster != NULL && !NewCaster->bCastingGuide && NewCaster->PlayerState->bOnlySpectator)
	{
		NewCaster->bCastingGuide = true;
		NewCaster->CastingGuideViewIndex = 0;
		NewCaster->ClientSetHUD(CastingGuideHUDClass);
	}
}

//Special markup for Analytics event so they show up properly in grafana. Should be eventually moved to UTAnalytics.
/*
* @EventName PlayerLogoutStat
*
* @Trigger Sent when a player logs out of a match
*
* @Type Sent by the Server
*
* @EventParam ID string Stats ID for this player
* @EventParam PlayerName string Player name
* @EventParam TimeOnline float How long this player was in game
* @EventParam Kills int32 Amount of kills
* @EventParam Deaths int32 Amount of Deaths
* @EventParam Score int32 Score
* @EventParam GameName string Name of this game mode
* @EventParam PlayerXP int64 Players total XP
* @EventParam PlayerLevel int32 Player's level
* @EventParam PlayerStars int32 Number of Stars a player has earned 
*
* @Comments
*/
void AUTGameMode::SendLogoutAnalytics(AUTPlayerState* PS)
{
	if ((PS != nullptr) && !PS->bSentLogoutAnalytics && FUTAnalytics::IsAvailable() && (Cast<AUTPlayerController>(PS->GetOwner()) != nullptr) && (Cast<AUTDemoRecSpectator>(PS->GetOwner()) == nullptr))
	{
		PS->bSentLogoutAnalytics = true;
		float TotalTimeOnline = UTGameState->ElapsedTime - PS->StartTime;
		TArray<FAnalyticsEventAttribute> ParamArray;
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("ID"), PS->StatsID));
        ParamArray.Add(FAnalyticsEventAttribute(TEXT("PlayerName"), PS->GetPlayerName()));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("TimeOnline"), TotalTimeOnline));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("Kills"), PS->Kills));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("Deaths"), PS->Deaths));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("Score"), PS->Score));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("GameName"), GetNameSafe(GetClass())));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("PlayerXP"), PS->GetXP().Total()));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("PlayerXP"), PS->GetXP().Total()));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("PlayerLevel"), GetLevelForXP(PS->GetXP().Total())));
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("PlayerStars"), PS->TotalChallengeStars));
		FUTAnalytics::SetMatchInitialParameters(this, ParamArray, false);
		FUTAnalytics::GetProvider().RecordEvent(TEXT("PlayerLogoutStat"), ParamArray);
	}
}

void AUTGameMode::Logout(AController* Exiting)
{
	if (BaseMutator != NULL)
	{
		BaseMutator->NotifyLogout(Exiting);
	}

	// Lets Analytics know how long this player has been online....
	AUTPlayerState* PS = Cast<AUTPlayerState>(Exiting->PlayerState);
	SendLogoutAnalytics(PS);
	if (PS != NULL)
	{
		PS->RespawnChoiceA = NULL;
		PS->RespawnChoiceB = NULL;

		// If this is a lan game, and this is the host, kick everyone....

		if (bIsLANGame && !HostIdString.IsEmpty())
		{
			if ( HostIdString.Equals(PS->UniqueId.ToString(), ESearchCase::IgnoreCase) )
			{
				SendEveryoneBackToLobby();
				// Now exit in about a second to make sure everyone get's told to exit.
				FTimerHandle TempHandle;
				GetWorldTimerManager().SetTimer(TempHandle, this, &AUTGameMode::ForceEndServer, 0.75f);
			}
		}
	}
	if (AntiCheatEngine)
	{
		AntiCheatEngine->OnPlayerLogout(Cast<APlayerController>(Exiting));
	}

	if (Cast<AUTBotPlayer>(Exiting) != NULL)
	{
		NumBots--;
	}

	// the demorec spectator doesn't count as a real player or spectator and we need to avoid the Super call that will incorrectly decrement the spectator count
	if (Cast<AUTDemoRecSpectator>(Exiting) == NULL)
	{
		Super::Logout(Exiting);
	}

	if (GameSession != NULL)
	{
		AUTGameSession* UTGameSession = Cast<AUTGameSession>(GameSession);
		if (UTGameSession != NULL)
		{
			UTGameSession->UpdateGameState();
		}
	}

	UpdatePlayersPresence();

	if (IsGameInstanceServer() && LobbyBeacon)
	{
		if ( PS && !PS->bIsABot && !PS->bIsDemoRecording )
		{
			LobbyBeacon->UpdatePlayer(this, PS, true);
		}
	}

	if (NumPlayers == 0 && bIsQuickMatch && bUseMatchmakingSession)
	{
		AUTGameSession* UTGameSession = Cast<AUTGameSession>(GameSession);
		GetWorldTimerManager().SetTimer(ServerRestartTimerHandle, UTGameSession, &AUTGameSession::CheckForPossibleRestart, 5.0f, true);
		UTGameSession->CheckForPossibleRestart();
	}

	if (UTGameState && UTGameState->ActiveLineUpHelper)
	{
		//UTGameState->ActiveLineUpHelper->OnPlayerChange();
	}
}

void AUTGameMode::ForceEndServer()
{
	FPlatformMisc::RequestExit(false);
}


bool AUTGameMode::ModifyDamage_Implementation(int32& Damage, FVector& Momentum, APawn* Injured, AController* InstigatedBy, const FHitResult& HitInfo, AActor* DamageCauser, TSubclassOf<UDamageType> DamageType)
{
	AUTCharacter* InjuredChar = Cast<AUTCharacter>(Injured);
	if (InjuredChar != NULL && InjuredChar->bSpawnProtectionEligible && InstigatedBy != NULL && InstigatedBy != Injured->Controller)
	{
		Damage = 0;
	}
	if (HasMatchStarted() && (!IsMatchInProgress() || (UTGameState && UTGameState->IsMatchIntermission())))
	{
		Damage = 0;
	}
	else if (BaseMutator != NULL)
	{
		BaseMutator->ModifyDamage(Damage, Momentum, Injured, InstigatedBy, HitInfo, DamageCauser, DamageType);
	}
	return true;
}

bool AUTGameMode::PreventDeath_Implementation(APawn* KilledPawn, AController* Killer, TSubclassOf<UDamageType> DamageType, const FHitResult& HitInfo)
{
	return (BaseMutator != NULL && BaseMutator->PreventDeath(KilledPawn, Killer, DamageType, HitInfo));
}

bool AUTGameMode::CheckRelevance_Implementation(AActor* Other)
{
	if (BaseMutator == NULL)
	{
		return true;
	}
	else
	{
		bool bPreventModify = false;
		bool bForceKeep = BaseMutator->AlwaysKeep(Other, bPreventModify);
		if (bForceKeep && bPreventModify)
		{
			return true;
		}
		else
		{
			return (BaseMutator->CheckRelevance(Other) || bForceKeep);
		}
	}
}

void AUTGameMode::SetWorldGravity(float NewGravity)
{
	AWorldSettings* Settings = GetWorld()->GetWorldSettings();
	Settings->bWorldGravitySet = true;
	Settings->WorldGravityZ = NewGravity;
}

bool AUTGameMode::ChangeTeam(AController* Player, uint8 NewTeam, bool bBroadcast)
{
	// By default, we don't do anything.
	return true;
}

TSubclassOf<AGameSession> AUTGameMode::GetGameSessionClass() const
{
	if (bUseMatchmakingSession)
	{
		return AUTGameSessionRanked::StaticClass();
	}

	return AUTGameSessionNonRanked::StaticClass();
}

void AUTGameMode::ScoreObject_Implementation(AUTCarriedObject* GameObject, AUTCharacter* HolderPawn, AUTPlayerState* Holder, FName Reason)
{
	if (BaseMutator != NULL)
	{
		BaseMutator->ScoreObject(GameObject, HolderPawn, Holder, Reason);
	}
}

void AUTGameMode::GetSeamlessTravelActorList(bool bToEntry, TArray<AActor*>& ActorList)
{
	Super::GetSeamlessTravelActorList(bToEntry, ActorList);

	for (AUTMutator* Mut = BaseMutator; Mut != NULL; Mut = Mut->NextMutator)
	{
		Mut->GetSeamlessTravelActorList(bToEntry, ActorList);
	}
}

FString AUTGameMode::GetGameRulesDescription()
{
	TArray<TSharedPtr<TAttributePropertyBase>> MenuProps;
	TArray<FString> OptionsList;
	int32 PlayerCount;

	CreateGameURLOptions(MenuProps);
	GetGameURLOptions(MenuProps, OptionsList, PlayerCount);

	FString Description = FString::Printf(TEXT("A dedicated %s match!"), *DisplayName.ToString());			
	
	if (BotFillCount > 0)
	{
		OptionsList.Add(FString::Printf(TEXT("Difficulty=%i"), GameDifficulty));
	}

	FString Mutators = TEXT("");
	AUTMutator* NextMutator = BaseMutator;
	while (NextMutator != NULL)
	{
		Mutators = Mutators == TEXT("") ? NextMutator->DisplayName.ToString() : TEXT(", ") + NextMutator->DisplayName.ToString();
		NextMutator = NextMutator->NextMutator;
	}

	if (Mutators != TEXT("")) OptionsList.Add(FString::Printf(TEXT("Mutators=%s"), *Mutators));

	for (int32 i = 0; i < OptionsList.Num(); i++)
	{
		Description += FString::Printf(TEXT("\n%s"), *OptionsList[i]);
	}

	return Description;
}

void AUTGameMode::GetGameURLOptions(const TArray<TSharedPtr<TAttributePropertyBase>>& MenuProps, TArray<FString>& OptionsList, int32& DesiredPlayerCount)
{
	for (TSharedPtr<TAttributePropertyBase> Prop : MenuProps)
	{
		if (Prop.IsValid() && Prop->GetURLKey() != TEXT("BotFill"))
		{
			OptionsList.Add(Prop->GetURLString());
		}
	}

	DesiredPlayerCount = BotFillCount;
}

int32 AUTGameMode::AdjustedBotFillCount()
{
	if (GameSession && ((GetNetMode() == NM_Standalone) || bIsLANGame))
	{
		return GameSession->MaxPlayers;
	}
	int32 AdjustedBotFillCount = bTeamGame ? DefaultMaxPlayers : FMath::Max(BotFillCount, 5);
	return (bIsVSAI || !GameSession) ? AdjustedBotFillCount : FMath::Min(GameSession->MaxPlayers, AdjustedBotFillCount);
}

void AUTGameMode::CreateGameURLOptions(TArray<TSharedPtr<TAttributePropertyBase>>& MenuProps)
{
	MenuProps.Empty();
	MenuProps.Add(MakeShareable(new TAttributeProperty<int32>(this, &TimeLimit, TEXT("TimeLimit"))));
	MenuProps.Add(MakeShareable(new TAttributeProperty<int32>(this, &GoalScore, TEXT("GoalScore"))));

	if (BotFillCount == 0)
	{
		BotFillCount = AdjustedBotFillCount();
	}
	MenuProps.Add(MakeShareable(new TAttributeProperty<int32>(this, &BotFillCount, TEXT("BotFill"))));
}

TSharedPtr<TAttributePropertyBase> AUTGameMode::FindGameURLOption(TArray<TSharedPtr<TAttributePropertyBase>>& MenuProps, const FString& SearchKey)
{
	for (int32 i = 0; i < MenuProps.Num(); i++)
	{
		if (MenuProps[i].IsValid() && MenuProps[i]->GetURLKey().Equals(SearchKey,ESearchCase::IgnoreCase))
		{
			return MenuProps[i];
		}
	}

	return nullptr;
}

#if !UE_SERVER
void AUTGameMode::CreateConfigWidgets(TSharedPtr<class SVerticalBox> MenuSpace, bool bCreateReadOnly, TArray< TSharedPtr<TAttributePropertyBase> >& ConfigProps, int32 MinimumPlayers)
{
	CreateGameURLOptions(ConfigProps);

	TSharedPtr< TAttributeProperty<int32> > TimeLimitAttr = StaticCastSharedPtr<TAttributeProperty<int32>>(FindGameURLOption(ConfigProps,TEXT("TimeLimit")));
	TSharedPtr< TAttributeProperty<int32> > GoalScoreAttr = StaticCastSharedPtr<TAttributeProperty<int32>>(FindGameURLOption(ConfigProps, TEXT("GoalScore")));
	TSharedPtr< TAttributePropertyBool > ForceRespawnAttr = StaticCastSharedPtr<TAttributePropertyBool>(FindGameURLOption(ConfigProps, TEXT("ForceRespawn")));
	TSharedPtr< TAttributeProperty<int32> > CombatantsAttr = StaticCastSharedPtr<TAttributeProperty<int32>>(FindGameURLOption(ConfigProps, TEXT("BotFill")));

	// FIXME: temp 'ReadOnly' handling by creating new widgets; ideally there would just be a 'disabled' or 'read only' state in Slate...
	if (CombatantsAttr.IsValid())
	{
		MenuSpace->AddSlot()
		.AutoHeight()
		.VAlign(VAlign_Top)
		.Padding(0.0f,0.0f,0.0f,5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(0.0f, 5.0f, 0.0f, 0.0f)
			[
				SNew(SBox)
				.WidthOverride(350)
				[
					SNew(STextBlock)
					.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Tween")
					.Text(NSLOCTEXT("UTGameMode", "NumCombatants", "Number of Combatants"))
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding(20.0f,0.0f,0.0f,0.0f)
			[
				SNew(SBox)
				.WidthOverride(300)
				[
					bCreateReadOnly ?
					StaticCastSharedRef<SWidget>(
						SNew(STextBlock)
						.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Tween.Bold")
						.Text(CombatantsAttr.ToSharedRef(), &TAttributeProperty<int32>::GetAsText)
					) :
					StaticCastSharedRef<SWidget>(
						SNew(SNumericEntryBox<int32>)
						.Value(CombatantsAttr.ToSharedRef(), &TAttributeProperty<int32>::GetOptional)
						.OnValueChanged(CombatantsAttr.ToSharedRef(), &TAttributeProperty<int32>::Set)
						.AllowSpin(true)
						.Delta(1)
						.MinValue(MinimumPlayers)
						.MaxValue(24)
						.MinSliderValue(MinimumPlayers)
						.MaxSliderValue(24)
						.EditableTextBoxStyle(SUWindowsStyle::Get(), "UT.Common.NumEditbox.White")

					)
				]
			]
		];
	}
	if (GoalScoreAttr.IsValid())
	{
		MenuSpace->AddSlot()
		.AutoHeight()
		.VAlign(VAlign_Top)
		.Padding(0.0f,0.0f,0.0f,5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(350)
				[
					SNew(STextBlock)
					.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Tween")
					.Text(NSLOCTEXT("UTGameMode", "GoalScore", "Goal Score"))
				]
			]
			+ SHorizontalBox::Slot()
			.Padding(20.0f,0.0f,0.0f,0.0f)
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(300)
				[
					bCreateReadOnly ?
					StaticCastSharedRef<SWidget>(
						SNew(STextBlock)
						.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Tween.Bold")
						.Text(GoalScoreAttr.ToSharedRef(), &TAttributeProperty<int32>::GetAsText)
					) :
					StaticCastSharedRef<SWidget>(
						SNew(SNumericEntryBox<int32>)
						.Value(GoalScoreAttr.ToSharedRef(), &TAttributeProperty<int32>::GetOptional)
						.OnValueChanged(GoalScoreAttr.ToSharedRef(), &TAttributeProperty<int32>::Set)
						.AllowSpin(true)
						.Delta(1)
						.MinValue(0)
						.MaxValue(999)
						.MinSliderValue(0)
						.MaxSliderValue(99)
						.EditableTextBoxStyle(SUWindowsStyle::Get(), "UT.Common.NumEditbox.White")
					)
				]
			]
		];
	}
	if (TimeLimitAttr.IsValid())
	{
		MenuSpace->AddSlot()
		.AutoHeight()
		.VAlign(VAlign_Top)
		.Padding(0.0f,0.0f,0.0f,5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(350)
				[
					SNew(STextBlock)
					.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Tween")
					.Text(NSLOCTEXT("UTGameMode", "TimeLimit", "Time Limit"))
				]
			]
			+ SHorizontalBox::Slot()
			.Padding(20.0f,0.0f,0.0f,0.0f)
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(300)
				[
					bCreateReadOnly ?
					StaticCastSharedRef<SWidget>(
					SNew(STextBlock)
					.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Tween.Bold")
					.Text(TimeLimitAttr.ToSharedRef(), &TAttributeProperty<int32>::GetAsText)
					) :
					StaticCastSharedRef<SWidget>(
					SNew(SNumericEntryBox<int32>)
					.Value(TimeLimitAttr.ToSharedRef(), &TAttributeProperty<int32>::GetOptional)
					.OnValueChanged(TimeLimitAttr.ToSharedRef(), &TAttributeProperty<int32>::Set)
					.AllowSpin(true)
					.Delta(1)
					.MinValue(0)
					.MaxValue(999)
					.MinSliderValue(0)
					.MaxSliderValue(60)
					.EditableTextBoxStyle(SUWindowsStyle::Get(), "UT.Common.NumEditbox.White")
					)
				]
			]
		];
	}
	if (ForceRespawnAttr.IsValid())
	{
		MenuSpace->AddSlot()
		.AutoHeight()
		.VAlign(VAlign_Top)
		.Padding(0.0f,0.0f,0.0f,5.0f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.VAlign(VAlign_Center)
			[
				SNew(SBox)
				.WidthOverride(350)
				[
					SNew(STextBlock)
					.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Tween")
					.Text(NSLOCTEXT("UTGameMode", "ForceRespawn", "Force Respawn"))
				]
			]
			+ SHorizontalBox::Slot()
			.Padding(20.0f, 0.0f, 0.0f, 10.0f)
			.AutoWidth()
			[
				SNew(SBox)
				.WidthOverride(300)
				[
					bCreateReadOnly ?
					StaticCastSharedRef<SWidget>(
						SNew(SCheckBox)
						.IsChecked(ForceRespawnAttr.ToSharedRef(), &TAttributePropertyBool::GetAsCheckBox)
						.Style(SUWindowsStyle::Get(), "UT.Common.CheckBox")
						.ForegroundColor(FLinearColor::White)
						.Type(ESlateCheckBoxType::CheckBox)
					) :
					StaticCastSharedRef<SWidget>(
						SNew(SCheckBox)
						.IsChecked(ForceRespawnAttr.ToSharedRef(), &TAttributePropertyBool::GetAsCheckBox)
						.OnCheckStateChanged(ForceRespawnAttr.ToSharedRef(), &TAttributePropertyBool::SetFromCheckBox)
						.Style(SUWindowsStyle::Get(), "UT.Common.CheckBox")
						.ForegroundColor(FLinearColor::White)
						.Type(ESlateCheckBoxType::CheckBox)
					)
				]
			]
		];
	}
}


#endif



void AUTGameMode::ProcessServerTravel(const FString& URL, bool bAbsolute)
{
	if (GameSession != NULL)
	{
		AUTGameSession* UTGameSession = Cast<AUTGameSession>(GameSession);
		if (UTGameSession != NULL)
		{
			UTGameSession->UnRegisterServer(false);
		}
	}
	Super::ProcessServerTravel(URL, bAbsolute);
}

FText AUTGameMode::BuildServerRules(AUTGameState* InGameState)
{
	return FText::Format(NSLOCTEXT("UTGameMode", "GameRules", "{0} - GoalScore: {1}  Time Limit: {2}"), DisplayName, FText::AsNumber(InGameState->GoalScore), (InGameState->TimeLimit > 0) ? FText::Format(NSLOCTEXT("UTGameMode", "TimeMinutes", "{0} min"), FText::AsNumber(uint32(InGameState->TimeLimit / 60))) : NSLOCTEXT("General", "None", "None"));
}

void AUTGameMode::BuildServerResponseRules(FString& OutRules)
{
	// TODO: need to rework this so it can be displayed in the client's local language
	OutRules += FString::Printf(TEXT("Goal Score\t%i\t"), GoalScore);
	OutRules += FString::Printf(TEXT("Time Limit\t%i\t"), int32(TimeLimit/60.0));
	OutRules += FString::Printf(TEXT("Allow Overtime\t%s\t"), bAllowOvertime ? TEXT("True") : TEXT("False"));

	AUTMutator* Mut = BaseMutator;
	while (Mut)
	{
		OutRules += FString::Printf(TEXT("Mutator\t%s\t"), *Mut->DisplayName.ToString());
		Mut = Mut->NextMutator;
	}
}

void AUTGameMode::BlueprintBroadcastLocalized( AActor* Sender, TSubclassOf<ULocalMessage> Message, int32 Switch, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject)
{
	BroadcastLocalized(Sender, Message, Switch, RelatedPlayerState_1, RelatedPlayerState_2, OptionalObject);
}

void AUTGameMode::BlueprintSendLocalized( AActor* Sender, AUTPlayerController* Receiver, TSubclassOf<ULocalMessage> Message, int32 Switch, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject)
{
	Receiver->ClientReceiveLocalizedMessage(Message, Switch, RelatedPlayerState_1, RelatedPlayerState_2, OptionalObject);
}

void AUTGameMode::BroadcastSpectator(AActor* Sender, TSubclassOf<ULocalMessage> Message, int32 Switch, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject)
{
	for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	{
		APlayerController* PC = Iterator->Get();
		if (PC->PlayerState != nullptr && PC->PlayerState->bOnlySpectator)
		{
			PC->ClientReceiveLocalizedMessage(Message, Switch, RelatedPlayerState_1, RelatedPlayerState_2, OptionalObject);
		}
	}
}

void AUTGameMode::BroadcastSpectatorPickup(AUTPlayerState* PS, FName StatsName, UClass* PickupClass)
{
	if (PS != nullptr && PickupClass != nullptr && StatsName != NAME_None)
	{
		//0 will not show the pickup count numbers
		int32 Switch = 0;
		BroadcastSpectator(nullptr, UUTSpectatorPickupMessage::StaticClass(), Switch, PS, nullptr, PickupClass);
	}
}

void AUTGameMode::PrecacheAnnouncements(UUTAnnouncer* Announcer) const
{
	// slow but fairly reliable base implementation that looks up all local messages
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(UUTLocalMessage::StaticClass()))
		{
			It->GetDefaultObject<UUTLocalMessage>()->PrecacheAnnouncements(Announcer);
		}
		if (It->IsChildOf(UUTDamageType::StaticClass()))
		{
			It->GetDefaultObject<UUTDamageType>()->PrecacheAnnouncements(Announcer);
		}
	}
}

void AUTGameMode::AssignDefaultSquadFor(AController* C)
{
	if (C != NULL)
	{
		if (SquadType == NULL)
		{
			UE_LOG(UT, Warning, TEXT("Game mode %s missing SquadType"), *GetName());
			SquadType = AUTSquadAI::StaticClass();
		}
		AUTPlayerState* PS = Cast<AUTPlayerState>(C->PlayerState);
		if (PS != NULL && PS->Team != NULL)
		{
			PS->Team->AssignDefaultSquadFor(C);
		}
		else
		{
			// default is to just spawn a squad for each individual
			AUTBot* B = Cast<AUTBot>(C);
			if (B != NULL)
			{
				B->SetSquad(GetWorld()->SpawnActor<AUTSquadAI>(SquadType));
			}
		}
	}
}

void AUTGameMode::NotifyLobbyGameIsReady()
{
	if (IsGameInstanceServer() && LobbyBeacon)
	{
		UE_LOG(UT,Verbose,TEXT("Calling Lobby_NotifyInstanceIsReady with %s"), * GetWorld()->GetMapName());
		LobbyBeacon->Lobby_NotifyInstanceIsReady(LobbyInstanceID, ServerInstanceGUID, GetWorld()->GetMapName());
	}
}

void AUTGameMode::UpdateLobbyMatchStats()
{
	// Update the players

	UpdateLobbyPlayerList();

	if (ensure(LobbyBeacon) && UTGameState)
	{
		FMatchUpdate MatchUpdate;
		MatchUpdate.TimeLimit = TimeLimit;
		MatchUpdate.GoalScore = GoalScore;
		MatchUpdate.GameTime = TimeLimit > 0 ? UTGameState->GetRemainingTime() : UTGameState->ElapsedTime;
		MatchUpdate.NumPlayers = NumPlayers;
		MatchUpdate.NumSpectators = NumSpectators;
		MatchUpdate.MatchState = MatchState;
		MatchUpdate.bMatchHasBegun = HasMatchStarted();
		MatchUpdate.bMatchHasEnded = HasMatchEnded();
		MatchUpdate.RankCheck = RankCheck;

		UpdateLobbyScore(MatchUpdate);
		LobbyBeacon->UpdateMatch(MatchUpdate);
	}

	LastLobbyUpdateTime = GetWorld()->GetTimeSeconds();
}

void AUTGameMode::UpdateLobbyScore(FMatchUpdate& MatchUpdate)
{
}

void AUTGameMode::UpdateLobbyPlayerList()
{
	if (ensure(LobbyBeacon))
	{
		for (int32 i=0;i<UTGameState->PlayerArray.Num();i++)
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
			if ( PS && !PS->bIsABot && !PS->bIsDemoRecording )
			{
				LobbyBeacon->UpdatePlayer(this, PS, false);
			}
		}
	}
}

void AUTGameMode::SendEveryoneBackToLobbyGameAbandoned()
{
	// Game Instance Servers just tell everyone to just return to the lobby.
	for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
	{
		AUTPlayerController* Controller = Cast<AUTPlayerController>(*Iterator);
		if (Controller)
		{
			Controller->ClientRankedGameAbandoned();
		}
	}
}

void AUTGameMode::SendEveryoneBackToLobby()
{
	// Game Instance Servers just tell everyone to just return to the lobby.
	for (FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator)
	{
		AUTPlayerController* Controller = Cast<AUTPlayerController>(*Iterator);
		if (Controller)
		{
			Controller->ClientReturnToLobby();
		}
	}
}

#if !UE_SERVER
FString AUTGameMode::GetHUBPregameFormatString()
{
	return FString::Printf(TEXT("<UWindows.Standard.MatchBadge.Header>%s</>\n\n<UWindows.Standard.MatchBadge.Small>Host</>\n<UWindows.Standard.MatchBadge>{Player0Name}</>\n\n<UWindows.Standard.MatchBadge.Small>({NumPlayers} Players)</>"), *DisplayName.ToString());
}
#endif

void AUTGameMode::UpdatePlayersPresence()
{
	bool bAllowJoin = GameSession ? (NumPlayers < GameSession->MaxPlayers) : false;

	AUTGameSession* UTGameSession = Cast<AUTGameSession>(GameSession);
	bool bNoJoinInProgress = UTGameSession ? UTGameSession->bNoJoinInProgress : false;

	bool bAllowInvites = !bPrivateMatch && (!bNoJoinInProgress || !UTGameState->HasMatchStarted());
	bool bAllowJoinInProgress = !bNoJoinInProgress || !UTGameState->HasMatchStarted();

	UE_LOG(UT,Verbose,TEXT("UpdatePlayersPresence: AllowJoin: %i %i %i"), bAllowJoin, bAllowInvites, bAllowJoinInProgress);

	if (GameSession)
	{
		const auto OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			const auto SessionInterface = OnlineSub->GetSessionInterface();
			if (SessionInterface.IsValid())
			{
				EOnlineSessionState::Type State = SessionInterface->GetSessionState(GameSessionName);
				if (State == EOnlineSessionState::Pending ||
					State == EOnlineSessionState::Starting ||
					State == EOnlineSessionState::InProgress)
				{
					GameSession->UpdateSessionJoinability(GameSessionName, true, bAllowInvites, true, false);
				}
			}
		}
	}


	UTGameState->bRestrictPartyJoin =  !bAllowJoinInProgress;
	FString PresenceString = FText::Format(NSLOCTEXT("UTGameMode","PlayingPresenceFormatStr","Playing {0} on {1}"), DisplayName, FText::FromString(*GetWorld()->GetMapName())).ToString();
	for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
	{
		AUTPlayerController* Controller = Cast<AUTPlayerController>(*Iterator);
		if (Controller)
		{
			Controller->ClientSetPresence(PresenceString, bAllowInvites, bAllowJoinInProgress, bAllowJoinInProgress, false);
		}
	}
}

#if !UE_SERVER
void AUTGameMode::NewPlayerInfoLine(TSharedPtr<SVerticalBox> VBox, FText InDisplayName, TSharedPtr<TAttributeStat> Stat, TArray<TSharedPtr<TAttributeStat> >& StatList)
{
	//Add stat in here for layout convenience
	StatList.Add(Stat);

	VBox->AddSlot()
	.AutoHeight()
	[
		SNew(SOverlay)
		+SOverlay::Slot()
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			[
				SNew(STextBlock)
				.Text(InDisplayName)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
				.ColorAndOpacity(FLinearColor::Gray)
			]
		]
		+ SOverlay::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Text(Stat.ToSharedRef(), &TAttributeStat::GetValueText)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
			]
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Right)
			.FillWidth(0.35f)
		]
	];
}

void AUTGameMode::NewWeaponInfoLine(TSharedPtr<SVerticalBox> VBox, FText InDisplayName, TSharedPtr<TAttributeStat> KillStat, TSharedPtr<TAttributeStat> DeathStat, TSharedPtr<struct TAttributeStat> AccuracyStat, TArray<TSharedPtr<TAttributeStat> >& StatList)
{
	//Add stat in here for layout convenience
	StatList.Add(KillStat);
	StatList.Add(DeathStat);
	StatList.Add(AccuracyStat);

	VBox->AddSlot()
	.AutoHeight()
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Left)
			[
				SNew(STextBlock)
				.Text(InDisplayName)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
				.ColorAndOpacity(FLinearColor::Gray)
			]
		]
		+ SOverlay::Slot()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(0.6f)
			.HAlign(HAlign_Fill)

			+ SHorizontalBox::Slot()
			.FillWidth(0.3f)
			.HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Text(KillStat.ToSharedRef(), &TAttributeStat::GetValueText)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
				.ColorAndOpacity(FLinearColor(0.6f, 1.0f, 0.6f))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.3f)
			.HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Text(DeathStat.ToSharedRef(), &TAttributeStat::GetValueText)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
				.ColorAndOpacity(FLinearColor(1.0f,0.6f,0.6f))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.4f)
			.HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Text(AccuracyStat.ToSharedRef(), &TAttributeStat::GetValueText)
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
			]
		]
	];
}

void AUTGameMode::BuildPaneHelper(TSharedPtr<SHorizontalBox>& HBox, TSharedPtr<SVerticalBox>& LeftPane, TSharedPtr<SVerticalBox>& RightPane)
{
	SAssignNew(HBox, SHorizontalBox)
	+ SHorizontalBox::Slot()
	.HAlign(HAlign_Fill)
	.Padding(20.0f, 20.0f, 20.0f, 10.0f)
	[
		SAssignNew(LeftPane, SVerticalBox)
	]
	+ SHorizontalBox::Slot()
	.HAlign(HAlign_Fill)
	.Padding(20.0f, 20.0f, 20.0f, 10.0f)
	[
		SAssignNew(RightPane, SVerticalBox)
	];
}

void AUTGameMode::BuildScoreInfo(AUTPlayerState* PlayerState, TSharedPtr<class SUTTabWidget> TabWidget, TArray<TSharedPtr<TAttributeStat> >& StatList)
{
	TAttributeStat::StatValueTextFunc TwoDecimal = [](const AUTPlayerState* PS, const TAttributeStat* Stat) -> FText
	{
		return FText::FromString(FString::Printf(TEXT("%8.2f"), Stat->GetValue()));
	};

	TSharedPtr<SVerticalBox> LeftPane;
	TSharedPtr<SVerticalBox> RightPane;
	TSharedPtr<SHorizontalBox> HBox;
	BuildPaneHelper(HBox, LeftPane, RightPane);

	TabWidget->AddTab(NSLOCTEXT("AUTGameMode", "Score", "Score"), HBox);

	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "Kills", "Kills"), MakeShareable(new TAttributeStat(PlayerState, NAME_None, [](const AUTPlayerState* PS, const TAttributeStat* Stat) -> float { return PS->Kills;	})), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "Deaths", "Deaths"), MakeShareable(new TAttributeStat(PlayerState, NAME_None, [](const AUTPlayerState* PS, const TAttributeStat* Stat) -> float {	return PS->Deaths; })), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "Suicides", "Suicides"), MakeShareable(new TAttributeStat(PlayerState, NAME_Suicides)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "ScorePM", "Score Per Minute"), MakeShareable(new TAttributeStat(PlayerState, NAME_None, [](const AUTPlayerState* PS, const TAttributeStat* Stat) -> float
	{
		return (PS->StartTime < PS->GetWorld()->GetGameState<AUTGameState>()->ElapsedTime) ? PS->Score * 60.f / (PS->GetWorld()->GetGameState<AUTGameState>()->ElapsedTime - PS->StartTime) : 0.f;
	}, TwoDecimal)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "KDRatio", "K/D Ratio"), MakeShareable(new TAttributeStat(PlayerState, NAME_None, [](const AUTPlayerState* PS, const TAttributeStat* Stat) -> float
	{
		return (PS->Deaths > 0) ? float(PS->Kills) / PS->Deaths : 0.f;
	}, TwoDecimal)), StatList);

	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "BeltPickups", "Shield Belt Pickups"), MakeShareable(new TAttributeStat(PlayerState, NAME_ShieldBeltCount)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "LargeArmorPickups", "Large Armor Pickups"), MakeShareable(new TAttributeStat(PlayerState, NAME_ArmorVestCount)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "MediumArmorPickups", "Medium Armor Pickups"), MakeShareable(new TAttributeStat(PlayerState, NAME_ArmorPadsCount)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "SmallArmorPickups", "Small Armor Pickups"), MakeShareable(new TAttributeStat(PlayerState, NAME_HelmetCount)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "JumpBootJumps", "JumpBoot Jumps"), MakeShareable(new TAttributeStat(PlayerState, NAME_BootJumps)), StatList);

	LeftPane->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(40.0f)];
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "UDamagePickups", "UDamage Pickups"), MakeShareable(new TAttributeStat(PlayerState, NAME_UDamageCount)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "BerserkPickups", "Berserk Pickups"), MakeShareable(new TAttributeStat(PlayerState, NAME_BerserkCount)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "InvisibilityPickups", "Invisibility Pickups"), MakeShareable(new TAttributeStat(PlayerState, NAME_InvisibilityCount)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "KegPickups", "Keg Pickups"), MakeShareable(new TAttributeStat(PlayerState, NAME_KegCount)), StatList);
	
	TAttributeStat::StatValueTextFunc ToTime = [](const AUTPlayerState* PS, const TAttributeStat* Stat) -> FText
	{
		int32 Seconds = (int32)Stat->GetValue();
		int32 Mins = Seconds / 60;
		Seconds -= Mins * 60;
		return FText::FromString(FString::Printf(TEXT("%d:%02d"), Mins, Seconds));
	};

	RightPane->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(40.0f)];
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "UDamageControl", "UDamage Control"), MakeShareable(new TAttributeStat(PlayerState, NAME_UDamageTime, nullptr, ToTime)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "BerserkControl", "Berserk Control"), MakeShareable(new TAttributeStat(PlayerState, NAME_BerserkTime, nullptr, ToTime)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "InvisibilityControl", "Invisibility Control"), MakeShareable(new TAttributeStat(PlayerState, NAME_InvisibilityTime, nullptr, ToTime)), StatList);
}

void AUTGameMode::BuildRewardInfo(AUTPlayerState* PlayerState, TSharedPtr<class SUTTabWidget> TabWidget, TArray<TSharedPtr<TAttributeStat> >& StatList)
{
	TSharedPtr<SVerticalBox> LeftPane;
	TSharedPtr<SVerticalBox> RightPane;
	TSharedPtr<SHorizontalBox> HBox;
	BuildPaneHelper(HBox, LeftPane, RightPane);

	TabWidget->AddTab(NSLOCTEXT("AUTGameMode", "Rewards", "Rewards"), HBox);

	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "DoubleKills", "Double Kill"), MakeShareable(new TAttributeStat(PlayerState, NAME_MultiKillLevel0)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "MultiKills", "Multi Kill"), MakeShareable(new TAttributeStat(PlayerState, NAME_MultiKillLevel1)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "UltraKills", "Ultra Kill"), MakeShareable(new TAttributeStat(PlayerState, NAME_MultiKillLevel2)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "MonsterKills", "Monster Kill"), MakeShareable(new TAttributeStat(PlayerState, NAME_MultiKillLevel3)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "KillingSprees", "Killing Spree"), MakeShareable(new TAttributeStat(PlayerState, NAME_SpreeKillLevel0)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "RampageSprees", "Rampage"), MakeShareable(new TAttributeStat(PlayerState, NAME_SpreeKillLevel1)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "DominatingSprees", "Dominating"), MakeShareable(new TAttributeStat(PlayerState, NAME_SpreeKillLevel2)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "UnstoppableSprees", "Unstoppable"), MakeShareable(new TAttributeStat(PlayerState, NAME_SpreeKillLevel3)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "GodlikeSprees", "Godlike"), MakeShareable(new TAttributeStat(PlayerState, NAME_SpreeKillLevel4)), StatList);
}

void AUTGameMode::BuildWeaponInfo(AUTPlayerState* PlayerState, TSharedPtr<class SUTTabWidget> TabWidget, TArray<TSharedPtr<TAttributeStat> >& StatList)
{
	TSharedPtr<SVerticalBox> TopLeftPane;
	TSharedPtr<SVerticalBox> TopRightPane;
	TSharedPtr<SVerticalBox> BotLeftPane;
	TSharedPtr<SVerticalBox> BotRightPane;
	TSharedPtr<SHorizontalBox> TopBox;
	TSharedPtr<SHorizontalBox> BotBox;
	BuildPaneHelper(TopBox, TopLeftPane, TopRightPane);
	BuildPaneHelper(BotBox, BotLeftPane, BotRightPane);

	//Add the header
	TSharedPtr<SVerticalBox> VBox;
	VBox = TopLeftPane;
	VBox->AddSlot()
		.AutoHeight()
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(0.6f)
			.HAlign(HAlign_Fill)

			+ SHorizontalBox::Slot()
			.FillWidth(0.3f)
			.HAlign(HAlign_Right)
			[
				SNew(STextBlock)
				.Text(NSLOCTEXT("AUTGameMode", "KillsAbbrev", "K"))
				.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
				.ColorAndOpacity(FLinearColor(0.6f, 1.0f, 0.6f))
			]
			+ SHorizontalBox::Slot()
				.FillWidth(0.3f)
				.HAlign(HAlign_Right)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("AUTGameMode", "DeathsAbbrev", "D"))
					.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
					.ColorAndOpacity(FLinearColor(1.0f, 0.6f, 0.6f))
				]
			+ SHorizontalBox::Slot()
				.FillWidth(0.4f)
				.HAlign(HAlign_Right)
				[
					SNew(STextBlock)
					.Text(NSLOCTEXT("AUTGameMode", "AccuracyAbbrev", "Acc"))
					.TextStyle(SUWindowsStyle::Get(), "UT.Common.NormalText")
				]
		];

	//4x4 panes
	TSharedPtr<SVerticalBox> MainBox = SNew(SVerticalBox)
	+ SVerticalBox::Slot()
	.AutoHeight()
	[
		TopBox.ToSharedRef()
	]
	+ SVerticalBox::Slot()
	.AutoHeight()
	[
		BotBox.ToSharedRef()
	];
	TabWidget->AddTab(NSLOCTEXT("AUTGameMode", "Weapons", "Weapons"), MainBox);

	//Get the weapons used in this map
	TArray<AUTWeapon *> StatsWeapons;
	{
		// add default weapons - needs to be automated
		if (bGameHasImpactHammer)
		{
			StatsWeapons.AddUnique(AUTWeap_ImpactHammer::StaticClass()->GetDefaultObject<AUTWeapon>());
		}
		StatsWeapons.AddUnique(AUTWeap_Enforcer::StaticClass()->GetDefaultObject<AUTWeapon>());

		//Get the rest of the weapons from the pickups in the map
		for (FActorIterator It(PlayerState->GetWorld()); It; ++It)
		{
			AUTPickupWeapon* Pickup = Cast<AUTPickupWeapon>(*It);
			if (Pickup && Pickup->GetInventoryType())
			{
				StatsWeapons.AddUnique(Pickup->GetInventoryType()->GetDefaultObject<AUTWeapon>());
			}
		}
		StatsWeapons.AddUnique(AUTWeap_Translocator::StaticClass()->GetDefaultObject<AUTWeapon>());
	}

	//Add weapons to the panes
	for (int32 i = 0; i < StatsWeapons.Num();i++)
	{
		TSharedPtr<SVerticalBox> Pane = TopLeftPane;
		NewWeaponInfoLine(Pane, StatsWeapons[i]->DisplayName,
			MakeShareable(new TAttributeStatWeapon(PlayerState, StatsWeapons[i], TAttributeStatWeapon::WS_KillStat)),
			MakeShareable(new TAttributeStatWeapon(PlayerState, StatsWeapons[i], TAttributeStatWeapon::WS_DeathStat)),
			MakeShareable(new TAttributeStatWeapon(PlayerState, StatsWeapons[i], TAttributeStatWeapon::WS_AccuracyStat)),
			StatList);
	}

	TopRightPane->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(40.0f)];
	NewPlayerInfoLine(TopRightPane, NSLOCTEXT("AUTGameMode", "ShockComboKills", "Shock Combo Kills"), MakeShareable(new TAttributeStat(PlayerState, NAME_ShockComboKills)), StatList);
	NewPlayerInfoLine(TopRightPane, NSLOCTEXT("AUTGameMode", "AmazingCombos", "Amazing Combos"), MakeShareable(new TAttributeStat(PlayerState, NAME_AmazingCombos)), StatList);
	NewPlayerInfoLine(TopRightPane, NSLOCTEXT("AUTGameMode", "HeadShots", "Sniper Headshots"), MakeShareable(new TAttributeStat(PlayerState, NAME_SniperHeadshotKills)), StatList);
	NewPlayerInfoLine(TopRightPane, NSLOCTEXT("AUTGameMode", "AirRox", "Air Rocket Kills"), MakeShareable(new TAttributeStat(PlayerState, NAME_AirRox)), StatList);
	NewPlayerInfoLine(TopRightPane, NSLOCTEXT("AUTGameMode", "FlakShreds", "Flak Shreds"), MakeShareable(new TAttributeStat(PlayerState, NAME_FlakShreds)), StatList);
	NewPlayerInfoLine(TopRightPane, NSLOCTEXT("AUTGameMode", "AirSnot", "Air Snot Kills"), MakeShareable(new TAttributeStat(PlayerState, NAME_AirSnot)), StatList);
}

void AUTGameMode::BuildMovementInfo(AUTPlayerState* PlayerState, TSharedPtr<class SUTTabWidget> TabWidget, TArray<TSharedPtr<TAttributeStat> >& StatList)
{
	TAttributeStat::StatValueFunc ConvertToMeters = [](const AUTPlayerState* PS, const TAttributeStat* Stat) -> float
	{
		return 0.01f * PS->GetStatsValue(Stat->StatName);
	};

	TAttributeStat::StatValueTextFunc OneDecimal = [](const AUTPlayerState* PS, const TAttributeStat* Stat) -> FText
	{
		return FText::FromString(FString::Printf(TEXT("%8.1fm"), Stat->GetValue()));
	};

	TSharedPtr<SVerticalBox> LeftPane;
	TSharedPtr<SVerticalBox> RightPane;
	TSharedPtr<SHorizontalBox> HBox;
	BuildPaneHelper(HBox, LeftPane, RightPane);

	TabWidget->AddTab(NSLOCTEXT("AUTGameMode", "Movement", "Movement"), HBox);

	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "RunDistance", "Run Distance"), MakeShareable(new TAttributeStat(PlayerState, NAME_RunDist, ConvertToMeters, OneDecimal)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "SlideDistance", "Slide Distance"), MakeShareable(new TAttributeStat(PlayerState, NAME_SlideDist, ConvertToMeters, OneDecimal)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "WallRunDistance", "WallRun Distance"), MakeShareable(new TAttributeStat(PlayerState, NAME_WallRunDist, ConvertToMeters, OneDecimal)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "FallDistance", "Fall Distance"), MakeShareable(new TAttributeStat(PlayerState, NAME_InAirDist, ConvertToMeters, OneDecimal)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "SwimDistance", "Swim Distance"), MakeShareable(new TAttributeStat(PlayerState, NAME_SwimDist, ConvertToMeters, OneDecimal)), StatList);
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "TranslocDistance", "Teleport Distance"), MakeShareable(new TAttributeStat(PlayerState, NAME_TranslocDist, ConvertToMeters, OneDecimal)), StatList);
	LeftPane->AddSlot().AutoHeight()[SNew(SBox).HeightOverride(40.0f)];
	NewPlayerInfoLine(LeftPane, NSLOCTEXT("AUTGameMode", "Total", "Total"), MakeShareable(new TAttributeStat(PlayerState, NAME_None, [](const AUTPlayerState* PS, const TAttributeStat* Stat) -> float
	{
		float Total = 0.0f;
		Total += PS->GetStatsValue(NAME_RunDist);
		Total += PS->GetStatsValue(NAME_SlideDist);
		Total += PS->GetStatsValue(NAME_WallRunDist);
		Total += PS->GetStatsValue(NAME_InAirDist);
		Total += PS->GetStatsValue(NAME_TranslocDist);
		return 0.01f * Total;
	}, OneDecimal)), StatList);

	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "NumJumps", "Jumps"), MakeShareable(new TAttributeStat(PlayerState, NAME_NumJumps)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "NumDodges", "Dodges"), MakeShareable(new TAttributeStat(PlayerState, NAME_NumDodges)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "NumWallDodges", "Wall Dodges"), MakeShareable(new TAttributeStat(PlayerState, NAME_NumWallDodges)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "NumLiftJumps", "Lift Jumps"), MakeShareable(new TAttributeStat(PlayerState, NAME_NumLiftJumps)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "NumFloorSlides", "Floor Slides"), MakeShareable(new TAttributeStat(PlayerState, NAME_NumFloorSlides)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "NumWallRuns", "Wall Runs"), MakeShareable(new TAttributeStat(PlayerState, NAME_NumWallRuns)), StatList);
	NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "NumImpactJumps", "Impact Jumps"), MakeShareable(new TAttributeStat(PlayerState, NAME_NumImpactJumps)), StatList);
	//NewPlayerInfoLine(RightPane, NSLOCTEXT("AUTGameMode", "NumRocketJumps", "Rocket Jumps"), MakeShareable(new TAttributeStat(PlayerState, NAME_NumRocketJumps)), StatList);
}

void AUTGameMode::BuildPlayerInfo(AUTPlayerState* PlayerState, TSharedPtr<SUTTabWidget> TabWidget, TArray<TSharedPtr<TAttributeStat> >& StatList)
{
	//These need to be in the same order as they are in the scoreboard. Replication of stats are done per tab
	BuildScoreInfo(PlayerState, TabWidget, StatList);
	BuildWeaponInfo(PlayerState, TabWidget, StatList);
	BuildRewardInfo(PlayerState, TabWidget, StatList);
	BuildMovementInfo(PlayerState, TabWidget, StatList);
}
#endif

bool AUTGameMode::ValidateHat(AUTPlayerState* HatOwner, const FString& HatClass)
{
	if (HatClass.IsEmpty())
	{
		return true;
	}

	// Load the hat and make sure it's not override only.
	UClass* HatClassObject = LoadClass<AUTHat>(NULL, *HatClass, NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
	if (HatClassObject)
	{
		AUTHat* DefaultHat = HatClassObject->GetDefaultObject<AUTHat>();
		if (DefaultHat && !DefaultHat->bOverrideOnly)
		{
			return true;
		}
	}
	return false;
}

bool AUTGameMode::PlayerCanAltRestart_Implementation( APlayerController* Player )
{
	return PlayerCanRestart(Player);
}

void AUTGameMode::BecomeDedicatedInstance(FGuid HubGuid, int32 InstanceID)
{
	UTGameState->bIsInstanceServer = true;
	LobbyInstanceID = InstanceID;
	UTGameState->HubGuid = HubGuid;
	bDedicatedInstance = true;

	NotifyLobbyGameIsReady();

	UE_LOG(UT,Verbose,TEXT("Becoming a Dedicated Instance HubGuid=%s InstanceID=%i"), *HubGuid.ToString(), InstanceID);
}

void AUTGameMode::HandleMapVote()
{

	// Force at least 20 seconds of map vote time.

	MapVoteTime = FMath::Max(MapVoteTime, 20) * GetActorTimeDilation();

	UTGameState->MapVoteListCount = UTGameState->MapVoteList.Num();

	// In stand alone, we don't time out on map voting
	if (GetWorld()->GetNetMode() != NM_Standalone)
	{
		UTGameState->VoteTimer = MapVoteTime;
		FTimerHandle TempHandle;
		GetWorldTimerManager().SetTimer(TempHandle, this, &AUTGameMode::TallyMapVotes, MapVoteTime + GetActorTimeDilation());
		FTimerHandle TempHandle2;
		GetWorldTimerManager().SetTimer(TempHandle2, this, &AUTGameMode::CullMapVotes, MapVoteTime - 10 * GetActorTimeDilation());
	}

	for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
		if (PC != NULL && !PC->PlayerState->bOnlySpectator)
		{
			PC->ClientShowMapVote();
		}
	}
}

/**
 *	With 10 seconds to go in map voting, cull the list of voteable maps to no more than 6.  
 **/
void AUTGameMode::CullMapVotes()
{
	TArray<AUTReplicatedMapInfo*> Sorted;

	for (int32 i=0; i< UTGameState->MapVoteList.Num(); i++)
	{
		int32 InsertIndex = 0;
		while (InsertIndex < Sorted.Num() && Sorted[InsertIndex]->VoteCount >= UTGameState->MapVoteList[i]->VoteCount)
		{
			InsertIndex++;
		}
		
		Sorted.Insert(UTGameState->MapVoteList[i], InsertIndex);
	}

	UTGameState->MapVoteList.Empty();

	if (Sorted.Num() > 0)
	{
		if (Sorted[0]->VoteCount == 0)	// Top map has 0 votes so no one has voted
		{
			// Randomly pick up to 3 maps from the list and add them to the final

			for (int32 i = 0; i < 3; i++)
			{
				if ( Sorted.Num() > 0 )
				{
					int32 RandIdx = FMath::RandRange(i, Sorted.Num()-1);
					if (Sorted.IsValidIndex(RandIdx) && UTGameState->MapVoteList.Find(Sorted[RandIdx]) == INDEX_NONE)
					{
						UTGameState->MapVoteList.Add(Sorted[RandIdx]);
						Sorted.RemoveAt(RandIdx);
					}
				}
			}
		}
		else 
		{
			for (int32 i=0; i < 3; i++)
			{
				if (Sorted.Num() > 0)
				{
					if (Sorted[0]->VoteCount > 0)
					{
						UTGameState->MapVoteList.Add(Sorted[0]);
						Sorted.RemoveAt(0);
					}
					else
					{
						int32 RandIdx = FMath::RandRange(i, Sorted.Num()-1);
						if (Sorted.IsValidIndex(RandIdx) && UTGameState->MapVoteList.Find(Sorted[RandIdx]) == INDEX_NONE)
						{
							UTGameState->MapVoteList.Add(Sorted[RandIdx]);
							Sorted.RemoveAt(RandIdx);
						}

					}
				}
			}
		}
	}

	UTGameState->MapVoteListCount = UTGameState->MapVoteList.Num();

	for (int32 i=0; i<Sorted.Num(); i++)
	{
		if (UTGameState->MapVoteList.Find(Sorted[i]) == INDEX_NONE)
		{
			// Kill the actor
			Sorted[i]->Destroy();
		}
	}
}

void AUTGameMode::TallyMapVotes()
{
	for( FConstControllerIterator Iterator = GetWorld()->GetControllerIterator(); Iterator; ++Iterator )
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(*Iterator);
		if (PC != NULL)
		{
			PC->ClientHideMapVote();
		}
	}

	TArray<AUTReplicatedMapInfo*> Best;
	for (int32 i=0; i< UTGameState->MapVoteList.Num(); i++)
	{
		if (Best.Num() == 0 || Best[0]->VoteCount < UTGameState->MapVoteList[i]->VoteCount)
		{
			Best.Empty();
			Best.Add(UTGameState->MapVoteList[i]);
		}
	}

	if (Best.Num() > 0)
	{
		int32 Idx = FMath::RandRange(0, Best.Num() - 1);
		GetWorld()->ServerTravel(Best[Idx]->MapPackageName  + TEXT("?NextMap=1"), false);
	}
	else
	{
		SendEveryoneBackToLobby();
	}
}

void AUTGameMode::SetPlayerStateInactive(APlayerState* NewPlayerState)
{
	// bIsInactive needs to be set now as we are replicating right away
	NewPlayerState->bIsInactive = true;
}

//Same as AGameMode except we replicate the new inactive PS
void AUTGameMode::AddInactivePlayer(APlayerState* PlayerState, APlayerController* PC)
{
	// don't store if it's an old PlayerState from the previous level or if it's a spectator
	if (!PlayerState->bFromPreviousLevel && !PlayerState->bOnlySpectator)
	{
		APlayerState* NewPlayerState = PlayerState->Duplicate();
		if (NewPlayerState)
		{
			GameState->RemovePlayerState(NewPlayerState);

			//AUTGameMode begin 
			SetPlayerStateInactive(NewPlayerState);
			//AUTGameMode end

			// delete after some time
			NewPlayerState->SetLifeSpan(InactivePlayerStateLifeSpan);

			// On console, we have to check the unique net id as network address isn't valid
			bool bIsConsole = GEngine->IsConsoleBuild();

			// make sure no duplicates
			for (int32 i = 0; i<InactivePlayerArray.Num(); i++)
			{
				APlayerState* CurrentPlayerState = InactivePlayerArray[i];
				if ((CurrentPlayerState == NULL) || CurrentPlayerState->IsPendingKill() ||
					(!bIsConsole && (CurrentPlayerState->SavedNetworkAddress == NewPlayerState->SavedNetworkAddress)))
				{
					InactivePlayerArray.RemoveAt(i, 1);
					i--;
				}
			}
			InactivePlayerArray.Add(NewPlayerState);

			// cap at 16 saved PlayerStates
			if (InactivePlayerArray.Num() > 16)
			{
				InactivePlayerArray.RemoveAt(0, InactivePlayerArray.Num() - 16);
			}

			if (FUTAnalytics::IsAvailable() && (GetNetMode() == NM_DedicatedServer))
			{
				FUTAnalytics::FireEvent_UTServerPlayerDisconnect(this, Cast<AUTPlayerState>(PlayerState));
			}
		}
	}

	PlayerState->Destroy();
}

//Same as AGameMode except we duplicate the inactive PS
bool AUTGameMode::FindInactivePlayer(APlayerController* PC)
{
	// don't bother for spectators
	if (!PC || !PC->PlayerState || PC->PlayerState->bOnlySpectator)
	{
		return false;
	}

	// On console, we have to check the unique net id as network address isn't valid
	bool bIsConsole = GEngine->IsConsoleBuild();

	FString NewNetworkAddress = PC->PlayerState->SavedNetworkAddress;
    FString NewName = PC->PlayerState->GetPlayerName();
	for (int32 i = 0; i<InactivePlayerArray.Num(); i++)
	{
		APlayerState* CurrentPlayerState = InactivePlayerArray[i];
		if ((CurrentPlayerState == NULL) || CurrentPlayerState->IsPendingKill())
		{
			InactivePlayerArray.RemoveAt(i, 1);
			i--;
		}
		else if ((bIsConsole && (CurrentPlayerState->UniqueId == PC->PlayerState->UniqueId)) ||
            (!bIsConsole && (FCString::Stricmp(*CurrentPlayerState->SavedNetworkAddress, *NewNetworkAddress) == 0) && (FCString::Stricmp(*CurrentPlayerState->GetPlayerName(), *NewName) == 0)))
		{
			// found it!
			APlayerState* OldPlayerState = PC->PlayerState;

			//AUTGameMode begin - Since bIsInactive is COND_InitialOnly we need to create a new PS
			PC->PlayerState = CurrentPlayerState->Duplicate();
			CurrentPlayerState->Destroy();
			//AUTGameMode end

			PC->PlayerState->SetOwner(PC);
			OverridePlayerState(PC, OldPlayerState);
			GameState->AddPlayerState(PC->PlayerState);
			InactivePlayerArray.RemoveAt(i, 1);
			OldPlayerState->bIsInactive = true;
			// Set the uniqueId to NULL so it will not kill the player's registration 
			// in UnregisterPlayerWithSession()
			OldPlayerState->SetUniqueId(NULL);
			OldPlayerState->Destroy();
			return true;
		}
	}
	return false;
}

void AUTGameMode::GatherRequiredRedirects(TArray<FPackageRedirectReference>& Redirects)
{
	Super::GatherRequiredRedirects(Redirects);

	FPackageRedirectReference Redirect;

	// mutator paks
	for (TActorIterator<AUTMutator> It(GetWorld()); It; ++It)
	{	
		if (FindRedirect(GetModPakFilenameFromPkg(It->GetClass()->GetOutermost()->GetName()), Redirect))
		{
			Redirects.AddUnique(Redirect);
		}
	}
	// item paks (might be replaced via generic mutator that doesn't reference it directly)
	for (TObjectIterator<UClass> It; It; ++It)
	{
		if (It->IsChildOf(AUTInventory::StaticClass()) && !It->HasAnyClassFlags(CLASS_Native) && FindRedirect(GetModPakFilenameFromPkg(It->GetOutermost()->GetName()), Redirect))
		{
			Redirects.AddUnique(Redirect);
		}
	}
}

void AUTGameMode::GetGood()
{
#if !(UE_BUILD_SHIPPING)
	if (GetNetMode() == NM_Standalone)
	{
		UTGameState->SetRemainingTime(1);
		TimeLimit = 2;

		for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
		{
			AUTPlayerState *PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
			if (PS)
			{
				PS->Score = Cast<APlayerController>(PS->GetOwner()) ? 100 : 1;
			}
		}
	}
#endif
}

void AUTGameMode::GetRankedTeamInfo(int32 TeamId, FRankedTeamInfo& RankedTeamInfoOut)
{
	// report the social party initial size
	RankedTeamInfoOut.SocialPartySize = 1;

	for (int32 PlayerIdx = 0; PlayerIdx < UTGameState->PlayerArray.Num(); PlayerIdx++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(UTGameState->PlayerArray[PlayerIdx]);
		if (PS && !PS->bOnlySpectator && PS->GetTeamNum() == TeamId)
		{
			FRankedTeamMemberInfo RankedMemberInfo;
			RankedMemberInfo.AccountId = PS->StatsID;
			RankedMemberInfo.IsBot = PS->bIsABot;
			if (PS->bIsABot)
			{
                RankedMemberInfo.AccountId = PS->GetPlayerName();
			}
			RankedTeamInfoOut.Members.Add(RankedMemberInfo);
			RankedTeamInfoOut.SocialPartySize = FMath::Max(RankedTeamInfoOut.SocialPartySize, PS->PartySize);
		}
	}

	for (int32 PlayerIdx = 0; PlayerIdx < InactivePlayerArray.Num(); PlayerIdx++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(InactivePlayerArray[PlayerIdx]);
		if (PS && !PS->bOnlySpectator && PS->GetTeamNum() == TeamId)
		{
			FRankedTeamMemberInfo RankedMemberInfo;
			RankedMemberInfo.AccountId = PS->StatsID;
			RankedMemberInfo.IsBot = PS->bIsABot;
			if (PS->bIsABot)
			{
                RankedMemberInfo.AccountId = PS->GetPlayerName();
			}
			RankedTeamInfoOut.Members.Add(RankedMemberInfo);
			RankedTeamInfoOut.SocialPartySize = FMath::Max(RankedTeamInfoOut.SocialPartySize, PS->PartySize);
		}
	}
}

void AUTGameMode::PrepareRankedMatchResultGameCustom(FRankedMatchResult& MatchResult)
{
	// get the winner
	if (UTGameState == nullptr || UTGameState->WinningTeam == nullptr)
	{
		MatchResult.MatchInfo.RedScore = 0.5f;
	}
	else if (UTGameState->WinningTeam->TeamIndex == 0)
	{
		// red team wins
		MatchResult.MatchInfo.RedScore = 1.0f;
	}
	else
	{
		// red team loses
		MatchResult.MatchInfo.RedScore = 0.0f;
	}
	
	// add team member info
	GetRankedTeamInfo(0, MatchResult.RedTeam);
	GetRankedTeamInfo(1, MatchResult.BlueTeam);
}

void AUTGameMode::ReportRankedMatchResults(const FString& MatchRatingType)
{
	if (GetWorld()->GetNetMode() != NM_DedicatedServer)
	{
		return;
	}

	if (bSkipReportingMatchResults)
	{
		return;
	}

	// get MCP Utils
	UUTMcpUtils* McpUtils = UUTMcpUtils::Get(GetWorld(), TSharedPtr<const FUniqueNetId>());
	if (McpUtils == nullptr)
	{
		UE_LOG(UT, Warning, TEXT("Unable to load McpUtils. Will not be able to report the results of this ranked match"));
		return;
	}

	// prepare a MatchResult structure
	FRankedMatchResult MatchResult;
	MatchResult.RatingType = MatchRatingType;
	
	// get the session ID
	IOnlineSessionPtr SessionInt = IOnlineSubsystem::Get()->GetSessionInterface();
	FNamedOnlineSession* SessionPtr = SessionInt.IsValid() ? SessionInt->GetNamedSession(GameSessionName) : nullptr;
	TSharedPtr<FOnlineSessionInfo> SessionInfo = SessionPtr ? SessionPtr->SessionInfo : TSharedPtr<FOnlineSessionInfo>();
	if (SessionInfo.IsValid())
	{
		MatchResult.MatchInfo.SessionId = SessionInfo->GetSessionId().ToString();
		UE_LOG(UT, Log, TEXT("Reporting ranked results for session %s"), *MatchResult.MatchInfo.SessionId);
	}
	else
	{
		// just make a new one so we can submit
		MatchResult.MatchInfo.SessionId = FGuid::NewGuid().ToString();
		UE_LOG(UT, Warning, TEXT("Unable to get match session ID. Will report the results of this ranked match with sessionID = %s"), *MatchResult.MatchInfo.SessionId);
	}

	// get the length of the match (in seconds)
	if (ensure(GameState))
	{
		MatchResult.MatchInfo.MatchLengthSeconds = EndTime - StartPlayTime;
	}

	PrepareRankedMatchResultGameCustom(MatchResult);

	// tell MCP about the match to update players' MMRs
	McpUtils->ReportRankedMatchResult(MatchResult, [this, MatchRatingType](const FOnlineError& Result) {
		if (!Result.bSucceeded)
		{
			// best we can do is log an error
            UE_LOG(UT, Warning, TEXT("Failed to report Match Results to the server. User ELOs will not be updated. %s %s"), /*Result.HttpResult,*/ *Result.ErrorCode, *Result.ErrorMessage.ToString());
		}
		else
		{
			UE_LOG(UT, Display, TEXT("Ranked match reported to backend"));
		}

		for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
		{
			if (UTGameState->PlayerArray[i])
			{
				AUTPlayerController* UTPC = Cast<AUTPlayerController>(UTGameState->PlayerArray[i]->GetOwner());
				if (UTPC)
				{
					UTPC->ClientUpdateSkillRating(MatchRatingType);
				}
			}
		}
	});
}

void AUTGameMode::SendLobbyMessage(const FString& Message, AUTPlayerState* Sender)
{
	if (LobbyBeacon && Sender)
	{
        LobbyBeacon->Lobby_ReceiveUserMessage(Message, Sender->GetPlayerName());
	}
}

#if !UE_SERVER
TSharedPtr<SUTHUDWindow> AUTGameMode::CreateSpawnWindow(TWeakObjectPtr<UUTLocalPlayer> PlayerOwner)
{
	TSharedPtr<SUTSpawnWindow> SpawnWindow;
	SAssignNew(SpawnWindow, SUTSpawnWindow, PlayerOwner);

	return SpawnWindow;
}
#endif

void AUTGameMode::LockSession()
{
	if (bRankedSession)
	{
		AUTGameSessionRanked* UTGameSession = Cast<AUTGameSessionRanked>(GameSession);
		if (UTGameSession)
		{
			UTGameSession->LockPlayersToSession(true);
		}
	}
}

void AUTGameMode::UnlockSession()
{
	if (bRankedSession)
	{
		AUTGameSessionRanked* UTGameSession = Cast<AUTGameSessionRanked>(GameSession);
		if (UTGameSession)
		{
			UTGameSession->LockPlayersToSession(false);
		}
	}
}

bool AUTGameMode::CanBoost(AUTPlayerState* Who)
{
	if (Who != nullptr && (IsMatchInProgress() || GetMatchState() == MatchState::WaitingToStart) && (GetMatchState() != MatchState::MatchIntermission))
	{
		return Who->GetRemainingBoosts() > 0;
	}
	else
	{
		return false;
	}
}

bool AUTGameMode::TriggerBoost(AUTPlayerState* Who)
{
	if (BaseMutator != NULL)
	{
		BaseMutator->TriggerBoost(Who);
	}
	return CanBoost(Who);
}

bool AUTGameMode::AttemptBoost(AUTPlayerState* Who)
{
	bool bCanBoost = CanBoost(Who);
	if (bCanBoost)
	{
		Who->SetRemainingBoosts(Who->GetRemainingBoosts() - 1);
	}
	return bCanBoost;
}

void AUTGameMode::SendComsMessage( AUTPlayerController* Sender, AUTPlayerState* Target, int32 Switch)
{
	AUTPlayerState* UTPlayerState = Sender ? Cast<AUTPlayerState>(Sender->PlayerState) : nullptr;

	if (UTPlayerState != nullptr)
	{
		if (Target != nullptr)
		{
			// This is a targeting com message.  Send it only to the sender and the target
			AUTPlayerController* TargetPC = Cast<AUTPlayerController>(Target->GetOwner());
			if (TargetPC != nullptr)
			{
				TargetPC->ClientReceiveLocalizedMessage(UTPlayerState->GetCharacterVoiceClass(), Switch, UTPlayerState, nullptr, UTPlayerState->LastKnownLocation);
			}
			else
			{
				AUTBotPlayer* Bot = Cast<AUTBotPlayer>(Target->GetOwner());
				if (Bot != nullptr)
				{
					SendBotVoiceOrder(Sender, Bot, Switch);
				}
			}

			Sender->ClientReceiveLocalizedMessage(UTPlayerState->GetCharacterVoiceClass(), Switch, UTPlayerState, nullptr, UTPlayerState->LastKnownLocation);
		}
		else
		{
			AUTTeamInfo* CommTeam = UTPlayerState->Team;
			for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
			{
				AUTPlayerController* UTPlayerController = Cast<AUTPlayerController>(It->Get());
				AUTPlayerState* PS = UTPlayerController ? Cast<AUTPlayerState>(UTPlayerController->PlayerState) : nullptr;
				if (UTPlayerController && (!bTeamGame || (PS && PS->Team && (PS->Team == CommTeam))))
				{
					UTPlayerController->ClientReceiveLocalizedMessage(UTPlayerState->GetCharacterVoiceClass(), Switch, UTPlayerState, nullptr, UTPlayerState->LastKnownLocation);
				}
			}
		}
	}
}
void AUTGameMode::SendBotVoiceOrder(AUTPlayerController* Sender, AUTBot* Target, int32 Switch)
{
	AUTPlayerState* BotPS = Cast<AUTPlayerState>(Target->PlayerState);
	if (BotPS != nullptr && BotPS->Team != nullptr)
	{
		switch (Switch)
		{
			case ATTACK_THEIR_BASE_SWITCH_INDEX:
			case DEFEND_FLAG_CARRIER_SWITCH_INDEX:
				BotPS->Team->AssignToSquad(Target, NAME_Attack);
				break;
			case DEFEND_FLAG_SWITCH_INDEX:
				BotPS->Team->AssignToSquad(Target, NAME_Defend);
				break;
			case DROP_FLAG_SWITCH_INDEX:
				if (Target->GetUTChar() != nullptr && Target->GetUTChar()->GetCarriedObject() != nullptr)
				{
					Target->GetUTChar()->DropFlag();
					BotPS->Team->BotIgnoreFlagUntil = GetWorld()->TimeSeconds + 5.0f;
				}
				break;
			default:
				break;
		}
	}
}

int32 AUTGameMode::GetComSwitch(FName CommandTag, AActor* ContextActor, AUTPlayerController* InInstigator, UWorld* World)
{
	if (CommandTag == CommandTags::Yes)
	{
		return ACKNOWLEDGE_SWITCH_INDEX;
	}

	if (CommandTag == CommandTags::No)
	{
		return NEGATIVE_SWITCH_INDEX;
	}

	return INDEX_NONE;
}

void AUTGameMode::PreLogin(const FString& Options, const FString& Address, const FUniqueNetIdRepl& UniqueId, FString& ErrorMessage)
{
	if (UTGameState)
	{
		for (int32 i=0; i < UTGameState->PlayerArray.Num(); i++)
		{
			if (UTGameState->PlayerArray[i]->UniqueId == UniqueId)
			{
				UE_LOG(UT,Warning,TEXT("Rejecting Preloging for a client already in game."));
				return;
			}
		}
	}

	int32 SpectatorOnly = 0;
	SpectatorOnly = UGameplayStatics::GetIntOption(Options, TEXT("SpectatorOnly"), SpectatorOnly);

	UUTGameEngine* UTGameEngine = Cast<UUTGameEngine>(GEngine);
	if ( !SpectatorOnly && UTGameEngine && GameSession && GetWorld()->GetRealTimeSeconds() <= ReturningPlayerGraceCutoff )
	{
		// Look to see if this player is returning...
		if (UTGameEngine->PlayerReservations.Find(UniqueId.ToString()) == INDEX_NONE)
		{
			// Player isn't in the list... so look to see if there is room...
			
			if (UTGameEngine->PlayerReservations.Num() + 1 > GameSession->MaxPlayers )
			{
				// No room, return server is full
				ErrorMessage = TEXT("Server full.");
				return;
			}

			// Add this player to the reservation list.
			UTGameEngine->PlayerReservations.Add(UniqueId.ToString());
		}
	}

	Super::PreLogin(Options, Address, UniqueId, ErrorMessage);
}

void AUTGameMode::PreloadClientAssets(TArray<UObject*>& ObjList) const
{
	for (const FStringClassReference& Item : AssetsToPreloadOnClients)
	{
		TSubclassOf<UObject> NewType = Item.TryLoadClass<UObject>();
		if (NewType != nullptr)
		{
			ObjList.Add(NewType);
		}
	}
}

float AUTGameMode::GetLineUpTime(LineUpTypes LineUpType)
{
	return 3.f;
}

bool AUTGameMode::AllowTextMessage_Implementation(FString& Msg, bool bIsTeamMessage, AUTBasePlayerController* Sender)
{
	if (BaseMutator && !BaseMutator->AllowTextMessage(Msg, bIsTeamMessage, Sender))
	{
		return false;
	}

	return Super::AllowTextMessage_Implementation(Msg, bIsTeamMessage, Sender);
}

