// Copyright 1998-2013 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTLobbyGameState.h"
#include "UTLobbyPlayerState.h"
#include "OnlineSubsystemTypes.h"
#include "TAttributeProperty.h"
#include "UTGameMode.h"
#include "UTServerBeaconClient.h"
#include "UTLobbyMatchInfo.generated.h"


DECLARE_DELEGATE(FOnMatchInfoUpdated);
DECLARE_DELEGATE(FOnRulesetUpdated);

const uint32 MATCH_FLAG_InProgress = 0x0001;
const uint32 MATCH_FLAG_Ranked = 0x0002;
const uint32 MATCH_FLAG_Private = 0x0004;
const uint32 MATCH_FLAG_NoJoinInProgress = 0x0008;
const uint32 MATCH_FLAG_NoSpectators = 0x0010;
const uint32 MATCH_FLAG_Beginner = 0x0020;

const int32 RANK_CHECK_MIN = -400;
const int32 RANK_CHECK_MAX =  400;

class AUTServerBeaconLobbyClient;

UCLASS(notplaceable)
class UNREALTOURNAMENT_API AUTLobbyMatchInfo : public AInfo
{
	GENERATED_UCLASS_BODY()
public:
	// We use  the FUniqueNetID of the owner to be the Anchor point for this object.  This way we can reassociated the MatchInfo with the player when they reenter a server from travel.
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_Update)
	FUniqueNetIdRepl OwnerId;

	// The current state of this match.  
	UPROPERTY(Replicated)
	FName CurrentState;

	// if true, the owner will have to accept people joining this lobby
	UPROPERTY(Replicated)
	uint32 bPrivateMatch:1;

	// if true (defaults to true) then this match can be joined as a spectator.
	UPROPERTY(Replicated)
	uint32 bSpectatable:1;

	// if true (defaults to true) then people can join this match at any time
	UPROPERTY(Replicated)
	uint32 bJoinAnytime:1;

	// If true, this match is locked by rank
	UPROPERTY(Replicated)
	uint32 bRankLocked : 1;

	UPROPERTY(Replicated)
	bool bBeginnerMatch;

	// Holds data about the match.  In matches that are not started yet, it holds the description of the match.  In matches in progress, it's 
	// replicated data from the instance about the state of the match.  NOTE: Player information is not replicated from the instance to the server here
	// it's replicated in the PlayersInMatchInstance array.  But this contains important information regarding the match in ?Key=Value form.

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_MatchUpdate)
	FMatchUpdate MatchUpdate;

	UPROPERTY(Replicated, ReplicatedUsing = OnRep_InitialMap)
	FString InitialMap;

	// This will be looked up when the inital map is set.
	UPROPERTY()
	TWeakObjectPtr<AUTReplicatedMapInfo> InitialMapInfo;

	// Set by OnRep_InitialMap
	bool bMapChanged;

	// The current ruleset the governs this match
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_CurrentRuleset)
	TWeakObjectPtr<AUTReplicatedGameRuleset> CurrentRuleset;

	// A list of players in this lobby
	UPROPERTY(Replicated)
	TArray<TWeakObjectPtr<AUTLobbyPlayerState>> Players;

	// This is the process handle of the game instance that is running.
	FProcHandle GameInstanceProcessHandle;

	// This value is set client-side and holds the id that is beinig used in the MatchList.
	uint32 TrackedMatchId;

	// This is the lobby server generated instance id
	UPROPERTY()
	uint32 GameInstanceID;

	// The GUID for this game instance for spectating and join in progress
	UPROPERTY()
	FString GameInstanceGUID;

	// This is the key that will be used to lock this match
	UPROPERTY(Replicated)
	FGuid PrivateKey;

	// Holds a list of Unique IDs of players who are currently in the match.  When a player returns to lobby if their ID is in this list, they will be re-added to the match.
	UPROPERTY(Replicated)
	TArray<FRemotePlayerInfo> PlayersInMatchInstance;

	// Cache some data
	virtual void PreInitializeComponents() override;

	virtual void InitializeMatch(AUTLobbyPlayerState* Creator, const FString& GameName, AUTReplicatedGameRuleset* Ruleset, const FString& StartingMap, bool _bRankLocked, bool _bSpectatable, bool _bPrivateMatch, bool _bBeginner);
	virtual void AddPlayer(AUTLobbyPlayerState* PlayerToAdd, bool bIsOwner = false, bool bIsSpectator = false);
	virtual bool RemovePlayer(AUTLobbyPlayerState* PlayerToRemove);
	virtual FText GetActionText();

	FOnMatchInfoUpdated OnMatchInfoUpdatedDelegate;
	FOnRulesetUpdated OnRulesetUpdatedDelegate;

	UPROPERTY()
	TArray<FUniqueNetIdRepl> BannedIDs;

	virtual void LaunchMatch(bool bAllowBots, int32 BotDifficulty, bool bRequireFilled, bool bHostControl);

	virtual void SetLobbyMatchState(FName NewMatchState);
	virtual void GameInstanceReady(FGuid inGameInstanceGUID);

	/**
	 *	Removes the player from 
	 **/
	void RemoveFromMatchInstance(AUTLobbyPlayerState* PlayerState);


	/**
	 *	returns true if this match is in progress already.
	 **/
	virtual bool IsInProgress();
	virtual bool ShouldShowInDock();

	// Called from clients.  This will check to make sure all of the needed replicated information has arrived and that the player is ready to join.
	virtual bool MatchIsReadyToJoin(AUTLobbyPlayerState* Joiner);

	// This will be true if this match is a dedicated match and shouldn't ever go down
	UPROPERTY(Replicated)
	bool bDedicatedMatch;

	UPROPERTY(Replicated)
	bool bDedicatedTeamGame;

	// The Key used to associated this match with a dedicated instance
	UPROPERTY()
	FString AccessKey;

	// The name for this server
	UPROPERTY(Replicated)
	FString DedicatedServerName;

	UPROPERTY(Replicated)
	FString DedicatedServerGameMode;

	UPROPERTY(Replicated)
	FString DedicatedServerDescription;

	UPROPERTY(Replicated)
	int32 DedicatedServerMaxPlayers;

	FText GetDebugInfo();

	bool SkillTest(int32 PlayerRankCheck, bool bForceLock=false);

	/**
	 *	Returns the Owner's UTLobbyPlayerState
	 **/
	TWeakObjectPtr<AUTLobbyPlayerState> GetOwnerPlayerState()
	{
		for (int32 i=0;i<Players.Num();i++)
		{
			if (Players[i].IsValid() && Players[i]->UniqueId == OwnerId)
			{
				return Players[i];
			}
		}

		return NULL;
	}

protected:

	// Only available on the server, this holds a cached reference to the GameState.
	UPROPERTY()
	TWeakObjectPtr<AUTLobbyGameState> LobbyGameState;

	UFUNCTION()
	virtual void OnRep_CurrentRuleset();


	UFUNCTION()
	virtual void OnRep_Update();

	UFUNCTION()
	virtual void OnRep_InitialMap();

	// This match info is done.  Kill it.
	void RecycleMatchInfo();

	bool CheckLobbyGameState();

	UFUNCTION()
	virtual void OnRep_MatchUpdate();

	/** return current size of teams for team game */
	TArray<int32> GetTeamSizes() const;
public:

	// Returns true if the match has room for a new player to join it
	virtual bool MatchHasRoom(bool bForSpectator=false);

	/** set redirect list on CurrentRuleset, including those explicitly specified and those automatically detected by the game settings */
	virtual void SetRedirects();

	virtual void SetRules(TWeakObjectPtr<AUTReplicatedGameRuleset> NewRuleset, const FString& StartingMap);

	// Processing an update to the match coming from an instance
	virtual void ProcessMatchUpdate(const FMatchUpdate& NewMatchUpdate);

	// Processing an update to the match coming from an instance
	virtual void ProcessStartMatch(const FMatchUpdate& NewMatchUpdate);

	bool IsBanned(FUniqueNetIdRepl Who);
	void GetMapInformation();

public:
	// Unique for each match on this hub.  This will be used to lookup data regarding a given instance.
	FGuid UniqueMatchID;

	int32 NumPlayersInMatch();
	int32 NumSpectatorsInMatch();

	// When the hub receives the notice that the instance for this match is ready, notify any beacons in this array.
	UPROPERTY()
	TArray<AUTServerBeaconClient*> NotifyBeacons;

	// Holds the average rank of the players in this match
	UPROPERTY(replicated)
	int32 RankCheck;

	// Will be true if the host of this match is a beginner
	UPROPERTY(replicated)
	uint32 bHostIsBeginner:1;

	// Updates the rank variables based on an event
	void UpdateRank();

public:
	/**
	 *	This holds the list of required packages to play this current match.  
	 **/
	UPROPERTY(replicated, ReplicatedUsing = OnRep_RedirectsChanged)
	TArray<FPackageRedirectReference> Redirects;

	// This will be true if the redirects have changed.  When true, the UI should attempt to download any of the objects within the redirects.
	bool bRedirectsHaveChanged;

	void FillPlayerColumnsForDisplay(TArray<FMatchPlayerListStruct>& FirstColumn, TArray<FMatchPlayerListStruct>& SecondColumn, FString& Spectators);
	void GetPlayerData(TArray<FMatchPlayerListStruct>& PlayerData);

	int32 CountFriendsInMatch(const TArray<FUTFriend>& Friends);

	uint32 GetMatchFlags();

	bool IsPrivateMatch()
	{
		return ((GetMatchFlags() & MATCH_FLAG_Private) > 0);
	}

	UFUNCTION(Server, Reliable, WithValidation)
	void ServerInvitePlayer(AUTLobbyPlayerState* Who, bool bInvite);

protected:
	UFUNCTION()
	void OnRep_RedirectsChanged();

	// Moves players to set teams depending on the ruleset.
	void AssignTeams();

public:
	UPROPERTY(Replicated)
	TArray<FString> AllowedPlayerList;

	// When was this server launched.
	float InstanceLaunchTime;

	FString GetOwnerName();

	// A reference to the beacon client for communication to this instance..
	UPROPERTY()
	AUTServerBeaconLobbyClient* InstanceBeacon;

	virtual void MakeJsonReport(TSharedPtr<FJsonObject> JsonObject);

	// Used internally, this is the stat's port that this instance will listen to.
	int32 StatsPort;

	// The time when we last received a beacon communication from this instance
	float LastInstanceCommunicationTime;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);

	UPROPERTY(Replicated)
	FString CustomGameName;

};



