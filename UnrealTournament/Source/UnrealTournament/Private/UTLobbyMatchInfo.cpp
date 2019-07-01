// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "GameFramework/PlayerState.h"
#include "UTLobbyGameMode.h"
#include "UTLobbyMatchInfo.h"
#include "Net/UnrealNetwork.h"
#include "UTGameEngine.h"
#include "UTLevelSummary.h"
#include "UTReplicatedMapInfo.h"
#include "UTReplicatedGameRuleset.h"
#include "UTServerBeaconLobbyClient.h"
#include "UTAnalytics.h"

void AUTLobbyMatchInfo::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Kill any associated instance beacon
	if (InstanceBeacon)
	{
		InstanceBeacon->Destroy();
		InstanceBeacon = NULL;
	}

	Super::EndPlay(EndPlayReason);
}

AUTLobbyMatchInfo::AUTLobbyMatchInfo(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer
	.DoNotCreateDefaultSubobject(TEXT("Sprite")))
{
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bAlwaysRelevant = true;
	bReplicateMovement = false;

	// Note: this is very important to set to false. Though all replication infos are spawned at run time, during seamless travel
	// they are held on to and brought over into the new world. In ULevel::InitializeNetworkActors, these PlayerStates may be treated as map/startup actors
	// and given static NetGUIDs. This also causes their deletions to be recorded and sent to new clients, which if unlucky due to name conflicts,
	// may end up deleting the new PlayerStates they had just spaned.
	bNetLoadOnClient = false;

	bSpectatable = true;
	bJoinAnytime = true;
	bMapChanged = false;
	bBeginnerMatch = false;
	TrackedMatchId = -1;
}

void AUTLobbyMatchInfo::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(AUTLobbyMatchInfo, OwnerId);
	DOREPLIFETIME(AUTLobbyMatchInfo, CurrentState);
	DOREPLIFETIME(AUTLobbyMatchInfo, bPrivateMatch);
	DOREPLIFETIME(AUTLobbyMatchInfo, MatchUpdate);
	DOREPLIFETIME(AUTLobbyMatchInfo, CurrentRuleset);
	DOREPLIFETIME(AUTLobbyMatchInfo, Players);
	DOREPLIFETIME(AUTLobbyMatchInfo, InitialMap);
	DOREPLIFETIME(AUTLobbyMatchInfo, PlayersInMatchInstance);
	DOREPLIFETIME(AUTLobbyMatchInfo, bJoinAnytime);
	DOREPLIFETIME(AUTLobbyMatchInfo, bSpectatable);
	DOREPLIFETIME(AUTLobbyMatchInfo, bRankLocked);
	DOREPLIFETIME(AUTLobbyMatchInfo, RankCheck);
	DOREPLIFETIME(AUTLobbyMatchInfo, Redirects);
	DOREPLIFETIME(AUTLobbyMatchInfo, AllowedPlayerList);
	DOREPLIFETIME(AUTLobbyMatchInfo, DedicatedServerMaxPlayers);
	DOREPLIFETIME(AUTLobbyMatchInfo, bDedicatedTeamGame);
	DOREPLIFETIME(AUTLobbyMatchInfo, bBeginnerMatch);

	DOREPLIFETIME(AUTLobbyMatchInfo, CustomGameName);

	DOREPLIFETIME_CONDITION(AUTLobbyMatchInfo, DedicatedServerName, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTLobbyMatchInfo, DedicatedServerDescription, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTLobbyMatchInfo, DedicatedServerGameMode, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTLobbyMatchInfo, bDedicatedMatch, COND_InitialOnly);
	DOREPLIFETIME_CONDITION(AUTLobbyMatchInfo, PrivateKey, COND_InitialOnly);
}

void AUTLobbyMatchInfo::PreInitializeComponents()
{
	Super::PreInitializeComponents();
	SetLobbyMatchState(ELobbyMatchState::Initializing);

	UniqueMatchID = FGuid::NewGuid();
	PrivateKey = FGuid::NewGuid();
}

bool AUTLobbyMatchInfo::CheckLobbyGameState()
{
	LobbyGameState = GetWorld()->GetGameState<AUTLobbyGameState>();
	return LobbyGameState != NULL;
}

void AUTLobbyMatchInfo::SetLobbyMatchState(FName NewMatchState)
{
	if ((CurrentState != ELobbyMatchState::Recycling || NewMatchState == ELobbyMatchState::Dead) && CurrentState != ELobbyMatchState::Dead)
	{
		CurrentState = NewMatchState;
		if (CurrentState == ELobbyMatchState::Recycling)
		{
			FTimerHandle TempHandle; 
			GetWorldTimerManager().SetTimer(TempHandle, this, &AUTLobbyMatchInfo::RecycleMatchInfo, 120.0, false);
		}
	}
}

void AUTLobbyMatchInfo::RecycleMatchInfo()
{
	if (CheckLobbyGameState())
	{
		LobbyGameState->RemoveMatch(this);
	}
}

TArray<int32> AUTLobbyMatchInfo::GetTeamSizes() const
{
	// get the team sizes;
	TArray<int32> TeamSizes;
	if (CurrentRuleset.IsValid() && CurrentRuleset->Data.bTeamGame)
	{
		TeamSizes.SetNumZeroed(2);

		for (int32 i = 0; i < Players.Num(); i++)
		{
			if (Players[i]->DesiredTeamNum >= 0 && Players[i]->DesiredTeamNum < 255)
			{
				TeamSizes.SetNumZeroed(FMath::Max<int32>(TeamSizes.Num(), Players[i]->DesiredTeamNum));
				TeamSizes[Players[i]->DesiredTeamNum]++;
			}
		}
	}
	return TeamSizes;
}

void AUTLobbyMatchInfo::InitializeMatch(AUTLobbyPlayerState* Creator, const FString& GameName, AUTReplicatedGameRuleset* Ruleset, const FString& StartingMap, bool _bRankLocked, bool _bSpectatable, bool _bPrivateMatch, bool _bBeginner)
{
	bRankLocked = _bRankLocked;
	bPrivateMatch = _bPrivateMatch;
	bSpectatable = _bSpectatable;

	SetOwner(Creator->GetOwner());
	AddPlayer(Creator, true, false);
	SetRules(Ruleset, StartingMap);
	RankCheck = Creator->GetRankCheck(Ruleset->GetDefaultGameModeObject());
	SetRedirects();

	CustomGameName = GameName;
}



void AUTLobbyMatchInfo::AddPlayer(AUTLobbyPlayerState* PlayerToAdd, bool bIsOwner, bool bIsSpectator)
{
	if (bIsOwner && !OwnerId.IsValid())
	{
		OwnerId = PlayerToAdd->UniqueId;
		SetLobbyMatchState(ELobbyMatchState::Setup);
	}
	else
	{
		// Look to see if this player is already in the match

		for (int32 PlayerIdx = 0; PlayerIdx < Players.Num(); PlayerIdx++)
		{
			if (Players[PlayerIdx].IsValid() && Players[PlayerIdx] == PlayerToAdd)
			{
				return;
			}
		}

		if (IsBanned(PlayerToAdd->UniqueId))
		{
			PlayerToAdd->ClientMatchError(NSLOCTEXT("LobbyMessage","Banned","You do not have permission to enter this match."));
			return;
		}
		
		if (CurrentRuleset.IsValid() && CurrentRuleset->Data.bTeamGame)
		{
			TArray<int32> TeamSizes = GetTeamSizes();
			int32 BestTeam = 0;
			for (int32 i = 1; i < TeamSizes.Num(); i++)
			{
				if (TeamSizes[i] < TeamSizes[BestTeam])
				{
					BestTeam = i;
				}
			}
			PlayerToAdd->DesiredTeamNum = BestTeam;
		}
		else
		{
			PlayerToAdd->DesiredTeamNum = 0;
		}
	}
	
	if (PlayerToAdd->PartySize > 1)
	{
		for (int32 i = 0; i < Players.Num(); i++)
		{
			if (Players[i]->PartyLeader == PlayerToAdd->PartyLeader)
			{
				PlayerToAdd->DesiredTeamNum = Players[i]->DesiredTeamNum;
			}
		}
	}

	if (bIsSpectator)
	{
		PlayerToAdd->DesiredTeamNum = 255;
	}
	
	Players.Add(PlayerToAdd);
	PlayerToAdd->AddedToMatch(this);
	PlayerToAdd->ChatDestination = ChatDestinations::Match;
}

bool AUTLobbyMatchInfo::RemovePlayer(AUTLobbyPlayerState* PlayerToRemove)
{
	// Owners remove everyone and kill the match
	if (OwnerId == PlayerToRemove->UniqueId)
	{
		// The host is removing this match, notify everyone.
		for (int32 i=0;i<Players.Num();i++)
		{
			if (Players[i].IsValid())
			{
				Players[i]->RemovedFromMatch(this);
			}
		}
		Players.Empty();

		// We are are not launching, kill this lobby otherwise keep it around
		if ( CurrentState == ELobbyMatchState::Launching || !IsInProgress() )
		{
			SetLobbyMatchState(ELobbyMatchState::Dead);
			return true;
		}

		return false;
	}

	// Else just remove this one player
	else
	{
		Players.Remove(PlayerToRemove);
		PlayerToRemove->RemovedFromMatch(this);
	}

	return false;
}

bool AUTLobbyMatchInfo::MatchIsReadyToJoin(AUTLobbyPlayerState* Joiner)
{
	if (Joiner && CheckLobbyGameState())
	{
		if (	CurrentState == ELobbyMatchState::WaitingForPlayers || 
		   (    CurrentState == ELobbyMatchState::Setup && OwnerId == Joiner->UniqueId) ||
		   (	CurrentState == ELobbyMatchState::Launching && (Joiner->CurrentMatch == this || bJoinAnytime || OwnerId == Joiner->UniqueId) )
		   )
		{
			return ( OwnerId.IsValid() );
		}
	}

	return false;
}

FText AUTLobbyMatchInfo::GetActionText()
{
	if (CurrentState == ELobbyMatchState::Dead)
	{
		return NSLOCTEXT("SUTMatchPanel","Dead","!! DEAD - BUG !!");
	}
	else if (CurrentState == ELobbyMatchState::Setup)
	{
		return NSLOCTEXT("SUTMatchPanel","Setup","Initializing...");
	}
	else if (CurrentState == ELobbyMatchState::WaitingForPlayers)
	{
		if (MatchHasRoom())
		{
			return NSLOCTEXT("SUTMatchPanel","ClickToJoin","Click to Join");
		}
		else
		{
			return NSLOCTEXT("SUTMatchPanel","Full","Match is Full");
		}
	}
	else if (CurrentState == ELobbyMatchState::Launching)
	{
		return NSLOCTEXT("SUTMatchPanel","Launching","Launching...");
	}
	else if (CurrentState == ELobbyMatchState::InProgress)
	{
		if (bJoinAnytime)
		{
			return NSLOCTEXT("SUTMatchPanel","ClickToJoin","Click to Join");
		}
		else if (bSpectatable)
		{
			return NSLOCTEXT("SUTMatchPanel","Spectate","Click to Spectate");
		}
		else 
		{
			return NSLOCTEXT("SUTMatchPanel","InProgress","In Progress...");
		}
	}
	else if (CurrentState == ELobbyMatchState::Returning)
	{
		return NSLOCTEXT("SUTMatchPanel","MatchOver","!! Match is over !!");
	}

	return FText::GetEmpty();
}



void AUTLobbyMatchInfo::LaunchMatch(bool bAllowBots, int32 BotDifficulty, bool bRequireFilled, bool bHostControl)
{
	if (CheckLobbyGameState() && CurrentRuleset.IsValid() && InitialMapInfo.IsValid())
	{

		// build all of the data needed to launch the map.
		FString GameURL = CurrentRuleset->GenerateURL(InitialMap, bAllowBots, BotDifficulty, bRequireFilled);

		if (!bSpectatable) GameURL += TEXT("?MaxSpectators=0");
		if (!bJoinAnytime && GameURL.Find(TEXT("NoJIP"), ESearchCase::IgnoreCase) == INDEX_NONE) GameURL += TEXT("?NoJIP");
		if (bHostControl) 
		{
			GameURL += FString::Printf(TEXT("?HostId=%s"), *OwnerId.ToString());
		}

		LobbyGameState->LaunchGameInstance(this, GameURL);
	}
}

void AUTLobbyMatchInfo::GameInstanceReady(FGuid inGameInstanceGUID)
{
	GameInstanceGUID = inGameInstanceGUID.ToString();
	UWorld* World = GetWorld();
	if (World == NULL) return;

	AUTLobbyGameMode* GM = World->GetAuthGameMode<AUTLobbyGameMode>();
	if (GM)
	{
		for (int32 i=0;i<Players.Num();i++)
		{
			if (Players[i].IsValid() && !Players[i]->IsPendingKillPending())	//  Just in case.. they shouldn't be here anyway..
			{
				// Tell the client to connect to the instance

				Players[i]->ClientConnectToInstance(GameInstanceGUID, Players[i]->DesiredTeamNum, Players[i]->DesiredTeamNum == 255);
			}
		}
	}

	SetLobbyMatchState(ELobbyMatchState::InProgress);
}

void AUTLobbyMatchInfo::RemoveFromMatchInstance(AUTLobbyPlayerState* PlayerState)
{
	for (int32 i=0; i<PlayersInMatchInstance.Num();i++)
	{
		if (PlayersInMatchInstance[i].PlayerID == PlayerState->UniqueId)
		{
			PlayersInMatchInstance.RemoveAt(i);

			AUTLobbyGameMode* LobbyGameMode = GetWorld()->GetAuthGameMode<AUTLobbyGameMode>();
			// Update the Game state.
			if (LobbyGameMode)
			{
				AUTGameSession* UTGameSession = Cast<AUTGameSession>(LobbyGameMode->GameSession);
				if (UTGameSession) UTGameSession->UpdateGameState();
			}
			break;
		}
	}
}

bool AUTLobbyMatchInfo::IsInProgress()
{
	return CurrentState == ELobbyMatchState::InProgress || CurrentState == ELobbyMatchState::Launching || CurrentState == ELobbyMatchState::Completed;
}

bool AUTLobbyMatchInfo::ShouldShowInDock()
{
	if (bDedicatedMatch)
	{
		// dedicated instances always show unless they are dead
		return (CurrentState != ELobbyMatchState::Aborting && CurrentState != ELobbyMatchState::Dead && CurrentState != ELobbyMatchState::Recycling);
	}
	else
	{
		return (OwnerId.IsValid() && CurrentRuleset.IsValid() && (CurrentState == ELobbyMatchState::InProgress || CurrentState == ELobbyMatchState::WaitingForPlayers || CurrentState == ELobbyMatchState::Launching));
	}
}


FText AUTLobbyMatchInfo::GetDebugInfo()
{
	FText OwnerText = NSLOCTEXT("UTLobbyMatchInfo","NoOwner","NONE");
	if (OwnerId.IsValid())
	{
        if (Players.Num() > 0 && Players[0].IsValid()) OwnerText = FText::FromString(Players[0]->GetPlayerName());
		else OwnerText = FText::FromString(OwnerId.ToString());
	}

	FFormatNamedArguments Args;
	Args.Add(TEXT("OwnerName"), OwnerText);
	Args.Add(TEXT("CurrentState"), FText::FromName(CurrentState));
	Args.Add(TEXT("CurrentRuleSet"), FText::FromString(CurrentRuleset.IsValid() ? CurrentRuleset->Data.Title : TEXT("None")));
	Args.Add(TEXT("ShouldShowInDock"), FText::AsNumber(ShouldShowInDock()));
	Args.Add(TEXT("InProgress"), FText::AsNumber(IsInProgress()));

	return FText::Format(NSLOCTEXT("UTLobbyMatchInfo","DebugFormat","Owner [{OwnerName}] State [{CurrentState}] RuleSet [{CurrentRuleSet}] Flags [{ShouldShowInDock}, {InProgress}]  Stats: {MatchStats}"), Args);
}

void AUTLobbyMatchInfo::OnRep_CurrentRuleset()
{
	OnRep_Update();
	OnRulesetUpdatedDelegate.ExecuteIfBound();
}

void AUTLobbyMatchInfo::OnRep_Update()
{
	// Let the UI know
	OnMatchInfoUpdatedDelegate.ExecuteIfBound();
}

void AUTLobbyMatchInfo::OnRep_InitialMap()
{
	bMapChanged = true;
	GetMapInformation();
}

void AUTLobbyMatchInfo::SetRedirects()
{
	// Copy any required redirects in to match info.  The UI will pickup on the replication and pull them.
	AUTBaseGameMode* BaseGame = GetWorld()->GetAuthGameMode<AUTBaseGameMode>();
	if (BaseGame != NULL)
	{
		Redirects.Empty();
		for (int32 i = 0; i < CurrentRuleset->Data.RequiredPackages.Num(); i++)
		{
			FPackageRedirectReference Redirect;
			if (BaseGame->FindRedirect(CurrentRuleset->Data.RequiredPackages[i], Redirect))
			{
				Redirects.Add(Redirect);
			}
		}
		// automatically add redirects for the map, game mode and mutator pak files (if any)
		FPackageRedirectReference Redirect;
		FString MapFullName;
		if (FPackageName::SearchForPackageOnDisk(InitialMap + FPackageName::GetMapPackageExtension(), &MapFullName) && BaseGame->FindRedirect(GetModPakFilenameFromPkg(MapFullName), Redirect))
		{
			Redirects.Add(Redirect);
		}
		if (BaseGame->FindRedirect(GetModPakFilenameFromPath(CurrentRuleset->Data.GameMode), Redirect))
		{
			Redirects.Add(Redirect);
		}
		FString AllMutators = UGameplayStatics::ParseOption(CurrentRuleset->Data.GameOptions, TEXT("Mutator"));
		while (AllMutators.Len() > 0)
		{
			FString MutPath;
			int32 Pos = AllMutators.Find(TEXT(","));
			if (Pos > 0)
			{
				MutPath = AllMutators.Left(Pos);
				AllMutators = AllMutators.Right(AllMutators.Len() - Pos - 1);
			}
			else
			{
				MutPath = AllMutators;
				AllMutators.Empty();
			}
			if (BaseGame->FindRedirect(GetModPakFilenameFromPath(MutPath), Redirect))
			{
				Redirects.Add(Redirect);
			}
		}

		if (InitialMapInfo.IsValid() && InitialMapInfo->Redirect.PackageName != TEXT(""))
		{
			Redirects.Add(InitialMapInfo->Redirect);
		}
	}
}

void AUTLobbyMatchInfo::AssignTeams()
{
	if (CurrentRuleset.IsValid())
	{
		for (int32 i = 0 ; i < Players.Num(); i++)
		{
			if (!Players[i]->bIsSpectator)
			{
				if (CurrentRuleset->Data.bTeamGame)
				{
					// If player is in a party, they are most likely already on the correct team
					if (Players[i]->PartySize == 1)
					{
						Players[i]->DesiredTeamNum = i % 2;
					}
				}
				else 
				{
					Players[i]->DesiredTeamNum = 0;
				}
			}
		}
	}
}

void AUTLobbyMatchInfo::SetRules(TWeakObjectPtr<AUTReplicatedGameRuleset> NewRuleset, const FString& StartingMap)
{
	bool bOldTeamGame = CurrentRuleset.IsValid() ? CurrentRuleset->Data.bTeamGame : false;
	CurrentRuleset = NewRuleset;

	if (bOldTeamGame != CurrentRuleset->Data.bTeamGame)
	{
		AssignTeams();
	}

	InitialMap = StartingMap;
	GetMapInformation();
	SetRedirects();
	bMapChanged = true;
}


void AUTLobbyMatchInfo::ProcessMatchUpdate(const FMatchUpdate& NewMatchUpdate)
{
	LastInstanceCommunicationTime = GetWorld()->GetRealTimeSeconds();
	MatchUpdate = NewMatchUpdate;
	OnRep_MatchUpdate();
}

void AUTLobbyMatchInfo::OnRep_MatchUpdate()
{
}


bool AUTLobbyMatchInfo::IsBanned(FUniqueNetIdRepl Who)
{
	for (int32 i=0;i<BannedIDs.Num();i++)
	{
		if (Who == BannedIDs[i])
		{
			return true;
		}
	}

	return false;
}

void AUTLobbyMatchInfo::GetMapInformation()
{
	if ( CheckLobbyGameState() )
	{
		InitialMapInfo = LobbyGameState->GetMapInfo(InitialMap);
		if (InitialMapInfo.IsValid()) return;
	}

	// We need to keep trying this until we get a valid map info.
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, this, &AUTLobbyMatchInfo::GetMapInformation, 0.25);
}

int32 AUTLobbyMatchInfo::NumPlayersInMatch()
{
	int32 ActualPlayerCount = 0;
	for (int32 i = 0; i < Players.Num(); i++)
	{
		if (Players[i].IsValid() && Players[i]->DesiredTeamNum != 255) ActualPlayerCount++;
	}

	if (CurrentState == ELobbyMatchState::Launching || CurrentState == ELobbyMatchState::WaitingForPlayers)
	{
		return ActualPlayerCount;
	}
	else if (CurrentState == ELobbyMatchState::InProgress)
	{
		int32 Cnt = ActualPlayerCount;
		for (int32 i=0; i < PlayersInMatchInstance.Num(); i++)
		{
			if (!PlayersInMatchInstance[i].bIsSpectator)
			{
				Cnt++;
			}
		}

		return Cnt;
	}
	return 0;
}

int32 AUTLobbyMatchInfo::NumSpectatorsInMatch()
{
	int32 ActualPlayerCount = 0;
	for (int32 i = 0; i < Players.Num(); i++)
	{
		if (Players[i].IsValid() && Players[i]->DesiredTeamNum == 255) ActualPlayerCount++;
	}

	if (CurrentState == ELobbyMatchState::Launching || CurrentState == ELobbyMatchState::WaitingForPlayers)
	{
		return ActualPlayerCount;
	}
	else if (CurrentState == ELobbyMatchState::InProgress)
	{
		int32 Cnt = Players.Num();
		for (int32 i=0; i < PlayersInMatchInstance.Num(); i++)
		{
			if (PlayersInMatchInstance[i].bIsSpectator)
			{
				Cnt++;
			}
		}

		return Cnt;
	}

	return 0;
}

bool AUTLobbyMatchInfo::MatchHasRoom(bool bForSpectator)
{
	if (CurrentRuleset.IsValid())
	{
		if (CurrentState == MatchState::InProgress)	
		{
			if (bForSpectator && CheckLobbyGameState())
			{
				return NumSpectatorsInMatch() < LobbyGameState->MaxSpectatorsInInstance;
			}

			return NumPlayersInMatch() < CurrentRuleset->Data.MaxPlayers;
		}
	
	}
	return true;
}

bool AUTLobbyMatchInfo::SkillTest(int32 PlayerRankCheck, bool bForceLock)
{
	if (bRankLocked || bForceLock)
	{
		return AUTPlayerState::CheckRank(PlayerRankCheck,RankCheck);
	}

	return true;
}


void AUTLobbyMatchInfo::OnRep_RedirectsChanged()
{
	bRedirectsHaveChanged = true;
}

void AUTLobbyMatchInfo::FillPlayerColumnsForDisplay(TArray<FMatchPlayerListStruct>& FirstColumn, TArray<FMatchPlayerListStruct>& SecondColumn, FString& Spectators)
{
	bool bIsTeamGame = CurrentRuleset.IsValid() ? CurrentRuleset->Data.bTeamGame : (bDedicatedMatch ? bDedicatedTeamGame : false);

	if (bIsTeamGame)
	{
		for (int32 i=0; i < Players.Num(); i++)
		{
			if (Players[i].IsValid())
			{
                if (Players[i]->GetTeamNum() == 0) FirstColumn.Add( FMatchPlayerListStruct(Players[i]->GetPlayerName(), Players[i]->UniqueId.ToString(), TEXT("0"),0) );
                else if (Players[i]->GetTeamNum() == 1) SecondColumn.Add( FMatchPlayerListStruct(Players[i]->GetPlayerName(), Players[i]->UniqueId.ToString() ,TEXT("0"),1) );
				else 
				{
                    Spectators = Spectators.IsEmpty() ? Players[i]->GetPlayerName() : FString::Printf(TEXT(", %s"), *Players[i]->GetPlayerName());
				}
			}

		}

		for (int32 i=0; i < PlayersInMatchInstance.Num(); i++)
		{
			if (PlayersInMatchInstance[i].TeamNum == 0) FirstColumn.Add( FMatchPlayerListStruct(PlayersInMatchInstance[i].PlayerName, PlayersInMatchInstance[i].PlayerID.ToString(), FString::Printf(TEXT("%i"),PlayersInMatchInstance[i].PlayerScore),0) );
			else if (PlayersInMatchInstance[i].TeamNum == 1) SecondColumn.Add(FMatchPlayerListStruct(PlayersInMatchInstance[i].PlayerName, PlayersInMatchInstance[i].PlayerID.ToString(), FString::Printf(TEXT("%i"),PlayersInMatchInstance[i].PlayerScore),1) );
			else 
			{
				Spectators = Spectators.IsEmpty() ? PlayersInMatchInstance[i].PlayerName : FString::Printf(TEXT(", %s"), *PlayersInMatchInstance[i].PlayerName);
			}
		}
	}
	else
	{
		int32 cnt=0;
		for (int32 i=0; i < Players.Num(); i++)
		{
			if (Players[i].IsValid())
			{
				if (Players[i]->bIsSpectator) 
				{
                    Spectators = Spectators.IsEmpty() ? Players[i]->GetPlayerName() : FString::Printf(TEXT("%s, %s"),*Spectators, *Players[i]->GetPlayerName());
				}
				else 
				{
					if (cnt % 2 == 0) 
					{
                        FirstColumn.Add( FMatchPlayerListStruct(Players[i]->GetPlayerName(), Players[i]->UniqueId.ToString(), TEXT("0"),0));
					}
					else
					{
                        SecondColumn.Add( FMatchPlayerListStruct(Players[i]->GetPlayerName(), Players[i]->UniqueId.ToString(), TEXT("0"),0));
					}
					cnt++;
				}
			}
		}

		for (int32 i=0; i < PlayersInMatchInstance.Num(); i++)
		{
			if (PlayersInMatchInstance[i].bIsSpectator) 
			{
				Spectators = Spectators.IsEmpty() ? PlayersInMatchInstance[i].PlayerName : FString::Printf(TEXT("%s, %s"), *Spectators , *PlayersInMatchInstance[i].PlayerName);
			}
			else
			{
				if (cnt % 2 == 0) 
				{
					FirstColumn.Add( FMatchPlayerListStruct(PlayersInMatchInstance[i].PlayerName, PlayersInMatchInstance[i].PlayerID.ToString(), FString::Printf(TEXT("%i"),PlayersInMatchInstance[i].PlayerScore),PlayersInMatchInstance[i].TeamNum));
				}
				else
				{
					SecondColumn.Add( FMatchPlayerListStruct(PlayersInMatchInstance[i].PlayerName, PlayersInMatchInstance[i].PlayerID.ToString(), FString::Printf(TEXT("%i"),PlayersInMatchInstance[i].PlayerScore),PlayersInMatchInstance[i].TeamNum));
				}
				cnt++;
			}
		}
	}

	if (FirstColumn.Num() > 0) FirstColumn.Sort(FMatchPlayerListCompare());
	if (SecondColumn.Num() > 0) SecondColumn.Sort(FMatchPlayerListCompare());

}

void AUTLobbyMatchInfo::GetPlayerData(TArray<FMatchPlayerListStruct>& PlayerData)
{
	TArray<FMatchPlayerListStruct> ColumnA;
	TArray<FMatchPlayerListStruct> ColumnB;
	FString Specs;

	FillPlayerColumnsForDisplay(ColumnA, ColumnB, Specs);
	int32 Max = FMath::Max<int32>(ColumnA.Num(), ColumnB.Num());

	for (int32 i=0; i < Max; i++)
	{
		if (i < ColumnA.Num()) PlayerData.Add(FMatchPlayerListStruct(ColumnA[i].PlayerName, ColumnA[i].PlayerId, ColumnA[i].PlayerScore, ColumnA[i].TeamNum));
		if (i < ColumnB.Num()) PlayerData.Add(FMatchPlayerListStruct(ColumnB[i].PlayerName, ColumnB[i].PlayerId, ColumnB[i].PlayerScore, ColumnB[i].TeamNum));
	}
}

int32 AUTLobbyMatchInfo::CountFriendsInMatch(const TArray<FUTFriend>& Friends)
{
	int32 NumFriends = 0;

	for (int32 i=0; i < Players.Num(); i++)
	{
		for (int32 j = 0 ; j < Friends.Num(); j++)
		{
			if (Players[i].IsValid() && Players[i]->UniqueId.ToString() == Friends[j].UserId)
			{
				NumFriends++;
				break;
			}
		}
	}

	for (int32 i=0; i < PlayersInMatchInstance.Num(); i++)
	{
		for (int32 j = 0 ; j < Friends.Num(); j++)
		{
			if (PlayersInMatchInstance[i].PlayerID.ToString() == Friends[j].UserId)
			{
				NumFriends++;
				break;
			}
		}
	}

	return NumFriends;
}

uint32 AUTLobbyMatchInfo::GetMatchFlags()
{
	uint32 Flags = 0x00;
	if (CurrentState == ELobbyMatchState::Launching || CurrentState == ELobbyMatchState::InProgress)
	{
		Flags = Flags | MATCH_FLAG_InProgress;
	}

	if (bPrivateMatch)
	{
		Flags = Flags | MATCH_FLAG_Private;
	}

	if (bRankLocked)
	{
		Flags = Flags | MATCH_FLAG_Ranked;
	}

	if (!bJoinAnytime)
	{
		Flags = Flags | MATCH_FLAG_NoJoinInProgress;
	}

	if (!bSpectatable)
	{
		Flags = Flags | MATCH_FLAG_NoSpectators;
	}

	if (bBeginnerMatch)
	{
		Flags = Flags | MATCH_FLAG_Beginner;
	}

	return Flags;
}

bool AUTLobbyMatchInfo::ServerInvitePlayer_Validate(AUTLobbyPlayerState* Who, bool bInvite) { return true; }
void AUTLobbyMatchInfo::ServerInvitePlayer_Implementation(AUTLobbyPlayerState* Who, bool bInvite)
{
    UE_LOG(UT,Verbose,TEXT("ServerInvitePlayer: %s -  %s [%s] to the match"), (bInvite ? TEXT("Inviting") : TEXT("Uninviting")), (Who ? *Who->GetPlayerName() : TEXT("[nullptr]")), (Who ? *Who->UniqueId.ToString() : TEXT("[nullptr]")));

	if (!Who)
	{
		return;
	}

	if (bInvite)
	{
		if (AllowedPlayerList.Find(Who->UniqueId.ToString()) == INDEX_NONE)
		{
			UE_LOG(UT,Verbose,TEXT("ServerInvitePlayer: Found the player and sending the client an invite"));

			AllowedPlayerList.Add(Who->UniqueId.ToString());
			Who->InviteToMatch(this);
		}
	}
	else
	{
		if (AllowedPlayerList.Find(Who->UniqueId.ToString()) != INDEX_NONE)
		{
			UE_LOG(UT,Verbose,TEXT("ServerInvitePlayer: Found the player and sending the client an uninvite"));

			AllowedPlayerList.Remove(Who->UniqueId.ToString());
			Who->UninviteFromMatch(this);
		}
	}
}

FString AUTLobbyMatchInfo::GetOwnerName()
{
	TWeakObjectPtr<AUTPlayerState> PS = GetOwnerPlayerState();
    return PS.IsValid() ? PS->GetPlayerName() : TEXT("N/A");
}

void AUTLobbyMatchInfo::MakeJsonReport(TSharedPtr<FJsonObject> JsonObject)
{
	JsonObject->SetStringField(TEXT("State"), CurrentState.ToString());
	JsonObject->SetStringField(TEXT("OwnerId"), OwnerId.ToString());

	JsonObject->SetNumberField(TEXT("GameInstanceID"), GameInstanceID);
	JsonObject->SetStringField(TEXT("GameInstanceGUID"), GameInstanceGUID);

#if PLATFORM_LINUX 
	JsonObject->SetNumberField(TEXT("ProcessId"), GameInstanceProcessHandle.IsValid() ? (int32) GameInstanceProcessHandle.GetProcessInfo()->GetProcessId() : -1);
#endif
	JsonObject->SetNumberField(TEXT("TimeSinceLastBeaconUpdate"), GetWorld()->GetRealTimeSeconds() - LastInstanceCommunicationTime);

	if (CurrentRuleset.IsValid())
	{
		TSharedPtr<FJsonObject> MatchJson = MakeShareable(new FJsonObject);
		CurrentRuleset->MakeJsonReport(MatchJson);
		JsonObject->SetObjectField(TEXT("CurrentRuleset"), MatchJson);
	}

	JsonObject->SetBoolField(TEXT("bPrivateMatch"),	bPrivateMatch);
	JsonObject->SetBoolField(TEXT("bJoinAnyTime"), bJoinAnytime);
	JsonObject->SetBoolField(TEXT("bRankLocked"), bRankLocked);
	JsonObject->SetBoolField(TEXT("bBeginnerMatch"), bBeginnerMatch );

	TArray<TSharedPtr<FJsonValue>> APArray;
	for (int32 i=0; i < Players.Num(); i++)
	{
		TSharedPtr<FJsonObject> PJson = MakeShareable(new FJsonObject);
		Players[i]->MakeJsonReport(PJson);
		APArray.Add( MakeShareable( new FJsonValueObject( PJson )) );			
	}

	JsonObject->SetArrayField(TEXT("Players"),  APArray);

	TArray<TSharedPtr<FJsonValue>> IPArray;
	for (int32 i=0; i < PlayersInMatchInstance.Num(); i++)
	{
		TSharedPtr<FJsonObject> PJson = MakeShareable(new FJsonObject);
		PlayersInMatchInstance[i].MakeJsonReport(PJson);
		IPArray.Add( MakeShareable( new FJsonValueObject( PJson )) );			
	}

	JsonObject->SetArrayField(TEXT("InstancePlayers"),  IPArray);

}
void AUTLobbyMatchInfo::ProcessStartMatch(const FMatchUpdate& NewMatchUpdate)
{
	if (CurrentRuleset.IsValid() && CurrentRuleset->Data.bCompetitiveMatch)
	{
		bPrivateMatch = true;		
	}
}

