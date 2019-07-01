// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "ChartCreation.h"

#include "UTGameState.generated.h"


/** used to store a player in recorded match data so that we're guaranteed to have a valid name (use PlayerState if possible, string name if not) */
USTRUCT()
struct FSafePlayerName
{
	GENERATED_USTRUCT_BODY()

		friend uint32 GetTypeHash(const FSafePlayerName& N);

	UPROPERTY()
		AUTPlayerState* PlayerState;
protected:
	UPROPERTY()
		FString PlayerName;
public:
	bool NetSerialize(FArchive& Ar, class UPackageMap* Map, bool& bOutSuccess)
	{
		UObject* Data = PlayerState;
		bool bWriteName = !Map->SerializeObject(Ar, AUTPlayerState::StaticClass(), Data) || Data == NULL;
		PlayerState = Cast<AUTPlayerState>(Data);
		Ar << bWriteName;
		if (bWriteName)
		{
			Ar << PlayerName;
		}
		else if (Ar.IsLoading() && PlayerState != NULL)
		{
			PlayerName = PlayerState->PlayerName;
		}
		return true;
	}

	FSafePlayerName()
		: PlayerState(NULL)
	{}
	FSafePlayerName(AUTPlayerState* InPlayerState)
		: PlayerState(InPlayerState), PlayerName(InPlayerState != NULL ? InPlayerState->PlayerName : TEXT(""))
	{}

	inline bool operator==(const FSafePlayerName& Other) const
	{
		if (PlayerState != NULL || Other.PlayerState != NULL)
		{
			return PlayerState == Other.PlayerState;
		}
		else
		{
			return PlayerName == Other.PlayerName;
		}
	}

	FString GetPlayerName() const
	{
		return (PlayerState != NULL) ? PlayerState->PlayerName : PlayerName;
	}
};
template<>
struct TStructOpsTypeTraits<FSafePlayerName> : public TStructOpsTypeTraitsBase
{
	enum
	{
		WithNetSerializer = true
	};
};
inline uint32 GetTypeHash(const FSafePlayerName& N)
{
	return GetTypeHash(N.PlayerName) + GetTypeHash(N.PlayerState);
}

USTRUCT()
struct FCTFAssist
{
	GENERATED_USTRUCT_BODY()

		UPROPERTY()
		FSafePlayerName AssistName;

	UPROPERTY()
		bool bCarryAssist;

	UPROPERTY()
		bool bDefendAssist;

	UPROPERTY()
		bool bReturnAssist;

	inline bool operator==(const FCTFAssist& Other) const
	{
		return AssistName == Other.AssistName;
	}

};

USTRUCT()
struct FCTFScoringPlay
{
	GENERATED_USTRUCT_BODY()

		/** team that got the cap */
		UPROPERTY()
		AUTTeamInfo* Team;
	UPROPERTY()
		FSafePlayerName ScoredBy;
	UPROPERTY()
		int32 ScoredByCaps;

	UPROPERTY()
		int32 RedBonus;

	UPROPERTY()
		int32 BlueBonus;

	UPROPERTY()
		TArray<FCTFAssist> Assists;
	/** Remaining time in seconds when the cap happened */
	UPROPERTY()
		int32 RemainingTime;
	/** period in which the cap happened (0 : first half, 1 : second half, 2+: OT) */
	UPROPERTY()
		uint8 Period;

	/**For Asymmetric CTF. */
	UPROPERTY()
		bool bDefenseWon;

	UPROPERTY()
		bool bAnnihilation;

	UPROPERTY()
		int32 TeamScores[2];

	FCTFScoringPlay()
		: Team(NULL), RemainingTime(0), Period(0), bDefenseWon(false), bAnnihilation(false)
	{}
	FCTFScoringPlay(const FCTFScoringPlay& Other) = default;

	inline bool operator==(const FCTFScoringPlay& Other) const
	{
		return (Team == Other.Team && ScoredBy == Other.ScoredBy && Assists == Other.Assists && RemainingTime == Other.RemainingTime && Period == Other.Period && bDefenseWon == Other.bDefenseWon);
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTeamSideSwapDelegate, uint8, Offset);

class AUTGameMode;
class AUTReplicatedMapInfo;
class AUTLineUpHelper;

enum class LineUpTypes : uint8;

UCLASS(Config = Game)
class UNREALTOURNAMENT_API AUTGameState : public AGameState
{
	GENERATED_UCLASS_BODY()

	/** server settings */
	UPROPERTY(Replicated, GlobalConfig, EditAnywhere, BlueprintReadWrite, Category = ServerInfo, replicatedUsing = OnRep_ServerName)
	FString ServerName;
	
	// The message of the day
	UPROPERTY(Replicated, GlobalConfig, EditAnywhere, BlueprintReadWrite, Category = ServerInfo, replicatedUsing = OnRep_ServerMOTD)
	FString ServerMOTD;

	UFUNCTION()
	virtual void OnRep_ServerName();

	UFUNCTION()
	virtual void OnRep_ServerMOTD();

	// A quick string field for the scoreboard and other browsers that contains description of the server
	UPROPERTY(Replicated, GlobalConfig, EditAnywhere, BlueprintReadWrite, Category = ServerInfo)
	FString ServerDescription;

	UPROPERTY()
	float MusicVolume;

	/** objects game class requested client pre-loading for via PreloadClientAssets()
	 * this array prevents those items from GC'ing prior to usage being replicated and is only used on clients
	 */
	UPROPERTY()
	TArray<UObject*> PreloadedAssets;

	UPROPERTY()
	TArray<UObject*> AsyncLoadedAssets;

	/** Allow game states to react to asset packages being loaded asynchronously */
	virtual void AsyncPackageLoaded(UObject* Package) override;

	virtual void OnRep_GameModeClass() override;

	/** teams, if the game type has them */
	UPROPERTY(BlueprintReadOnly, Category = GameState)
	TArray<AUTTeamInfo*> Teams;

	/** If TRUE, then we weapon pick ups to stay on their base */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = GameState)
	uint32 bWeaponStay:1;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = GameState)
	uint32 bTeamGame : 1;
	
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = GameState)
	uint32 bRankedSession : 1;

	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = GameState)
	uint32 bIsQuickMatch : 1;

	/** True if players are allowed to switch teams (if team game). */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = GameState)
	uint32 bAllowTeamSwitches : 1;

	/** If true, we will stop the game clock */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = GameState)
	uint32 bStopGameClock : 1;

	/** True if TeamDamagePct>0p, so projectiles and hitscan impact teammates. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = GameState)
		uint32 bTeamProjHits : 1;

	/** If true, teammates block each others movement. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = GameState)
		uint32 bTeamCollision : 1;

	/** If true, teammates play status announcements */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		uint32 bPlayStatusAnnouncements : 1;
	
	/** If true, kill icon messages persist through a round/ */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		uint32 bPersistentKillIconMessages : 1;

	/** If true, hitscan replication debugging is enabled. */
	UPROPERTY(Replicated, BlueprintReadWrite, Category = GameState)
		uint32 bDebugHitScanReplication : 1;

	/** If true, have match host controlling match start. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = GameState)
		uint32 bHaveMatchHost : 1;

	/** If true, match must be full to start. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = GameState)
		uint32 bRequireFull : 1;

	/** Replicated only for vs AI matches, 0 means not vs AI. */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = GameState)
		uint8 AIDifficulty;

	/** If a single player's (or team's) score hits this limited, the game is over */
	UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = GameState)
	int32 GoalScore;

	/** The maximum amount of time the game will be */
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = GameState)
	int32 TimeLimit;

	/** amount of time after a player spawns where they are immune to damage from enemies */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Replicated, Category = GameState)
	float SpawnProtectionTime;

	/** Whether can display minimap. */
	virtual bool AllowMinimapFor(AUTPlayerState* PS);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
	TSubclassOf<UUTLocalMessage> MultiKillMessageClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
	TSubclassOf<UUTLocalMessage> SpreeMessageClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		FText GoalScoreText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		FText GameOverStatus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		FText MapVoteStatus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		FText PreGameStatus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		FText NeedPlayersStatus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		FText OvertimeStatus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		FText HostStatus;

	/** amount of time between kills to qualify as a multikill */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
	float MultiKillDelay;

	// Tell clients if more players are needed before match starts
	UPROPERTY(Replicated)
	int32 PlayersNeeded;
	
	/** Used to limit frequency of enemy entering base messages. */
	UPROPERTY()
		float LastEnemyEnteringBaseTime;

	/** Used to limit frequency of enemy FC entering base messages. */
	UPROPERTY()
		float LastEnemyFCEnteringBaseTime;

	/** Used to limit frequency of entering enemy base messages. */
	UPROPERTY()
		float LastEnteringEnemyBaseTime;

	UPROPERTY()
		float LastFriendlyLocationReportTime;

	UPROPERTY()
		float LastEnemyLocationReportTime;

	UPROPERTY()
		float LastRedSniperWarningTime;

	UPROPERTY()
		float LastBlueSniperWarningTime;

	UPROPERTY(replicatedusing = OnUpdateFriendlyLocation)
		uint8 FCFriendlyLocCount;

	UPROPERTY(replicatedusing = OnUpdateEnemyLocation)
		uint8 FCEnemyLocCount;

	UFUNCTION()
		void OnUpdateFriendlyLocation();

	UFUNCTION()
		void OnUpdateEnemyLocation();

	virtual void UpdateFCFriendlyLocation(AUTPlayerState* AnnouncingPlayer, class AUTGameVolume* LocationVolume);
	virtual void UpdateFCEnemyLocation(AUTPlayerState* AnnouncingPlayer, class AUTGameVolume* LocationVolume);

	UPROPERTY()
		float LastIncomingWarningTime;

	UPROPERTY()
		FName LastFriendlyLocationName;

	UPROPERTY()
		FName LastEnemyLocationName;

	protected:
	/** How much time is remaining in this match. */
	UPROPERTY(BlueprintReadOnly, Category = GameState)
	int32 RemainingTime;

	// Used to sync the time on clients to the server. Updated at a lower frequency to reduce bandwidth cost -- See DefaultTimer()
	UPROPERTY(ReplicatedUsing=OnRepRemainingTime)
		int32 ReplicatedRemainingTime;

	UPROPERTY()
		float LastTimerMessageTime;

	UPROPERTY()
		int32 LastTimerMessageIndex;

	UFUNCTION()
		virtual void OnRepRemainingTime();

	public:
	int32 GetRemainingTime() { return RemainingTime; };
	virtual void SetRemainingTime(int32 NewRemainingTime);

	/** local world time that game ended (i.e. relative to World->TimeSeconds) */
	UPROPERTY(BlueprintReadOnly, Category = GameState)
	float MatchEndTime;

// deprecated, not called
	UFUNCTION()
	virtual void OnRep_RemainingTime() {};

	/** Returns time in seconds that should be displayed on game clock. */
	virtual float GetClockTime();

	UPROPERTY()
		bool bNeedToClearIntermission;

	/** Return remaining intermission time. */
	virtual float GetIntermissionTime();

	// How long a player can wait before being forced respawned (added to RespawnWaitTime).  Set to 0 for no delay.
	UPROPERTY(Replicated, EditAnywhere, BlueprintReadWrite, Category = GameState)
	float ForceRespawnTime;

	/** offset to level placed team IDs for the purposes of swapping/rotating sides
	 * i.e. if this value is 1 and there are 4 teams, team 0 objects become owned by team 1, team 1 objects become owned by team 2... team 3 objects become owned by team 0
	 */
	UPROPERTY(ReplicatedUsing = OnTeamSideSwap, BlueprintReadOnly, Category = GameState)
	uint8 TeamSwapSidesOffset;
	/** previous value, so we know how much we're changing by */
	UPROPERTY()
	uint8 PrevTeamSwapSidesOffset;

	/** changes team sides; generally offset should be 1 unless it's a 3+ team game and you want to rotate more than one spot */
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = GameState)
	virtual void ChangeTeamSides(uint8 Offset = 1);

	UFUNCTION()
	virtual void OnTeamSideSwap();

	UPROPERTY(BlueprintAssignable)
	FTeamSideSwapDelegate TeamSideSwapDelegate;

	UPROPERTY(Replicated, BlueprintReadOnly, ReplicatedUsing = OnWinnerReceived, Category = GameState)
	AUTPlayerState* WinnerPlayerState;

	/** Holds the team of the winning team */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = GameState)
	AUTTeamInfo* WinningTeam;

	/** Identifies who capped the flag */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = GameState)
	AUTPlayerState* ScoringPlayerState;

	UFUNCTION()
	virtual void OnWinnerReceived();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = GameState)
	virtual void SetTimeLimit(int32 NewTimeLimit);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = GameState)
	virtual void SetGoalScore(int32 NewGoalScore);

	UFUNCTION()
	virtual void SetWinner(AUTPlayerState* NewWinner);

	/** Called once per second (or so depending on TimeDilation) after RemainingTime() has been replicated */
	virtual void DefaultTimer();

	/** called to check for time message broadcasts ("3 minutes remain", etc) */
	virtual void CheckTimerMessage();

	/** Determines if a player is on the same team */
	UFUNCTION(BlueprintCallable, Category = GameState)
	virtual bool OnSameTeam(const AActor* Actor1, const AActor* Actor2);

	/** Determines if 2 PlayerStates are in score order */
	virtual bool InOrder( class AUTPlayerState* P1, class AUTPlayerState* P2 );

	/** Sorts the Player State Array */
	virtual void SortPRIArray();

	/** Find the current team that is in the lead */
	virtual AUTTeamInfo* FindLeadingTeam();

	/** Returns true if the match state is InProgress or later */
	UFUNCTION(BlueprintCallable, Category = GameState)
	virtual bool HasMatchStarted() const;

	UFUNCTION(BlueprintCallable, Category = GameState)
	virtual bool IsMatchInProgress() const;

	UFUNCTION(BlueprintCallable, Category = GameState)
	virtual bool IsMatchInOvertime() const;

	UFUNCTION(BlueprintCallable, Category = GameState)
	virtual bool IsMatchInCountdown() const;

	UFUNCTION(BlueprintCallable, Category = GameState)
	virtual bool IsMatchIntermission() const;

	virtual void BeginPlay() override;

	/** Return largest SpectatingId value in current PlayerArray. */
	virtual int32 GetMaxSpectatingId();

	/** Return largest SpectatingIdTeam value in current PlayerArray. */
	virtual int32 GetMaxTeamSpectatingId(int32 TeamNum);

	virtual bool CanSpectate(APlayerController* Viewer, APlayerState* ViewTarget);

	/** add an overlay to the OverlayMaterials list */
	UFUNCTION(Meta = (DeprecatedFunction, DeprecationMessage = "Use AddOverlayEffect"), BlueprintCallable, BlueprintAuthorityOnly, Category = Effects)
	virtual void AddOverlayMaterial(UMaterialInterface* NewOverlay, UMaterialInterface* NewOverlay1P = NULL);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Effects)
	virtual void AddOverlayEffect(const FOverlayEffect& NewOverlay, const FOverlayEffect& NewOverlay1P
#if CPP // UHT is dumb
	= FOverlayEffect()
#endif
	);
	/** find an overlay in the OverlayMaterials list, return its index */
	int32 FindOverlayMaterial(UMaterialInterface* TestOverlay) const
	{
		for (int32 i = 0; i < ARRAY_COUNT(OverlayEffects); i++)
		{
			if (OverlayEffects[i].Material == TestOverlay)
			{
				return i;
			}
		}
		return INDEX_NONE;
	}
	int32 FindOverlayEffect(const FOverlayEffect& TestEffect) const
	{
		for (int32 i = 0; i < ARRAY_COUNT(OverlayEffects); i++)
		{
			if (OverlayEffects[i] == TestEffect)
			{
				return i;
			}
		}
		return INDEX_NONE;
	}
	/** get overlay material from index */
	FOverlayEffect GetOverlayMaterial(int32 Index, bool bFirstPerson)
	{
		if (Index >= 0 && Index < ARRAY_COUNT(OverlayEffects))
		{
			return (bFirstPerson && OverlayEffects1P[Index].IsValid()) ? OverlayEffects1P[Index] : OverlayEffects[Index];
		}
		else
		{
			return FOverlayEffect();
		}
	}
	/** returns first active overlay material given the passed in flags */
	FOverlayEffect GetFirstOverlay(uint16 Flags, bool bFirstPerson)
	{
		// early out
		if (Flags == 0)
		{
			return FOverlayEffect();
		}
		else
		{
			for (int32 i = 0; i < ARRAY_COUNT(OverlayEffects); i++)
			{
				if (Flags & (1 << i))
				{
					return (bFirstPerson && OverlayEffects1P[i].IsValid()) ? OverlayEffects1P[i] : OverlayEffects[i];
				}
			}
			return FOverlayEffect();
		}
	}

	/**
	 *	This is called from the UTPlayerCameraManage to allow the game to force an override to the current player camera to make it easier for
	 *  Presentation to be controlled by the server.
	 **/
	
	virtual FName OverrideCameraStyle(APlayerController* PCOwner, FName CurrentCameraStyle);

	// Returns the rules for this server.
	virtual FText ServerRules();

	/** used on clients to know when all TeamInfos have been received */
	UPROPERTY(Replicated)
	uint8 NumTeams;

	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

	virtual void ReceivedGameModeClass() override;

	virtual void StartRecordingReplay();

	virtual FText GetGameStatusText(bool bForScoreboard = false);

	virtual FLinearColor GetGameStatusColor();

	virtual void OnRep_MatchState() override;

	virtual void AddPlayerState(class APlayerState* PlayerState) override;

	virtual void Tick(float DeltaTime) override;

	/** rearrange any players' SpectatingID so that the list of values is continuous starting from 1
	 * generally should not be called during gameplay as reshuffling this list unnecessarily defeats the point
	 */
	virtual void CompactSpectatingIDs();

	UPROPERTY()
	FName SecondaryAttackerStat;


	UFUNCTION(BlueprintCallable, Category = GameState)
		virtual float GetRespawnWaitTimeFor(AUTPlayerState* PS);

	UFUNCTION(BlueprintCallable, Category = GameState)
		virtual void SetRespawnWaitTime(float NewWaitTime);

	UFUNCTION(BlueprintCallable, Category = GameState)
		virtual TSubclassOf<class AUTInventory> GetSelectableBoostByIndex(AUTPlayerState* PlayerState, int Index) const;

	UFUNCTION(BlueprintCallable, Category = GameState)
		virtual bool IsSelectedBoostValid(AUTPlayerState* PlayerState) const;

protected:
	virtual void UpdateTimeMessage();

	// How long must a player wait before respawning
	UPROPERTY(Replicated, EditAnywhere, Category = GameState)
		float RespawnWaitTime;

	static const uint8 MAX_OVERLAY_MATERIALS = 16;
	/** overlay materials, mapped to bits in UTCharacter's OverlayFlags/WeaponOverlayFlags and used to efficiently handle character/weapon overlays
	 * only replicated at startup so set any used materials via BeginPlay()
	 */
	UPROPERTY(ReplicatedUsing = OnRep_OverlayEffects)
	FOverlayEffect OverlayEffects[MAX_OVERLAY_MATERIALS];
	UPROPERTY(ReplicatedUsing = OnRep_OverlayEffects)
	FOverlayEffect OverlayEffects1P[MAX_OVERLAY_MATERIALS];

	virtual void HandleMatchHasStarted() override;
	virtual void HandleMatchHasEnded() override;

	UFUNCTION()
	virtual void OnRep_OverlayEffects();

public:
	// Will be true if this is an instanced server from a lobby.
	UPROPERTY(Replicated)
	bool bIsInstanceServer;

	// the GUID of the hub the player should return when they leave.  
	UPROPERTY(Replicated, ReplicatedUsing=OnReceiveHubGuid)
	FGuid HubGuid;

	/** Game specific rating of a player as a desireable camera focus for spectators. */
	virtual float ScoreCameraView(AUTPlayerState* InPS, AUTCharacter *Character)
	{
		return 0.f;
	};

public:
	// These IDs are banned for the remainder of the match
	TArray<FUniqueNetIdRepl> TempBans;

	// Returns true if this player has been temp banned from this server/instance
	bool IsTempBanned(const FUniqueNetIdRepl& UniqueId);

	// Registers a vote for temp banning a player.  If the player goes above the threashhold, they will be banned for the remainder of the match
	bool VoteForTempBan(AUTPlayerState* BadGuy, AUTPlayerState* Voter);

	UPROPERTY(Config)
	float KickThreshold;

	/** Returns which team side InActor is closest to.   255 = no team. */
	virtual uint8 NearestTeamSide(AActor* InActor)
	{
		return 255;
	};

	// Used to get a list of game modes and maps that can be choosen from the menu.  Typically, this just pulls all of 
	// available local content however, in hubs it will be from data replicated from the server.
	virtual void GetAvailableGameData(TArray<UClass*>& GameModes, TArray<UClass*>& MutatorList);

	virtual void ScanForMaps(const TArray<FString>& AllowedMapPrefixes, TArray<FAssetData>& MapList);

	// Create a replicated map info from a map's asset registry data
	virtual AUTReplicatedMapInfo* CreateMapInfo(const FAssetData& MapAsset);

	/** Used to translate replicated FName refs to highlights into text. */
	TMap< FName, FText> HighlightMap;

	/** Used to translate replicated FName refs to highlights into text. */
	TMap< FName, FText> ShortHighlightMap;

	/** Used to prioritize which highlights to show (lower value = higher priority). */
	TMap< FName, float> HighlightPriority;

	/** Clear highlights array. */
	UFUNCTION(BlueprintCallable, Category = "Game")
		virtual void ClearHighlights();

	UFUNCTION(BlueprintCallable, Category = "Game")
		virtual bool PreventWeaponFire();

	virtual void UpdateMatchHighlights();
	virtual void UpdateRoundHighlights();

	/** On server side - generate a list of highlights for each player.  Every UTPlayerStates' MatchHighlights array will have been cleared when this is called. */
	UFUNCTION(BlueprintNativeEvent, Category = "Game")
		void UpdateHighlights();

	/** On client side, returns an array of text based on the PlayerStates Highlights. */
	UFUNCTION(BlueprintNativeEvent, Category = "Game")
		TArray<FText> GetPlayerHighlights(AUTPlayerState* PlayerState);

	/** After all major highlights added, fill in some minor ones if there is space left. */
	UFUNCTION(BlueprintNativeEvent, Category = "Game")
		void AddMinorHighlights(AUTPlayerState* PS);

	virtual int32 NumHighlightsNeeded();

	virtual FText FormatPlayerHighlightText(AUTPlayerState* PS, int32 Index);

	/** Return short version of top highlight for that player. */
	virtual FText ShortPlayerHighlightText(AUTPlayerState* PS, int32 Index=0);

	/** Return a score value for the "impressiveness" of the Match highlights for PS. */
	virtual float MatchHighlightScore(AUTPlayerState* PS);
	
	UPROPERTY()
		TArray<FName> GameScoreStats;

	UPROPERTY()
		TArray<FName> TeamStats;

	UPROPERTY()
		TArray<FName> WeaponStats;

	UPROPERTY()
		TArray<FName> RewardStats;

	UPROPERTY()
		TArray<FName> MovementStats;

	UPROPERTY(Replicated)
	TArray<AUTReplicatedMapInfo*> MapVoteList;

	UPROPERTY(Replicated)
	int32 MapVoteListCount;

	virtual void CreateMapVoteInfo(const FString& MapPackage,const FString& MapTitle, const FString& MapScreenshotReference);

	// The # of seconds left for voting for a map.
	UPROPERTY(Replicated)
	int32 VoteTimer;

	/** Returns a list of important pickups for this gametype
	*	Used to gather pickups for the spectator slideout
	*	For now, do gamytype specific team sorting here
	*   NOTE: return value is a workaround for blueprint bugs involving ref parameters and is not used
	*/
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = GameState)
	bool GetImportantPickups(UPARAM(ref) TArray<class AUTPickup*>& PickupList);

	/**
	 *	The server will replicate it's session id.  Clients, upon recieving it, will check to make sure they are in that session
	 *  and if not, add themselves.  If at all possible, clients should add themselves to the session before joining a server
	 *  however there are times where that isn't possible (join via IP) and this will act as a catch all.
	 **/
	UPROPERTY(Replicated, ReplicatedUsing = OnRep_ServerSessionId)
	FString ServerSessionId;

protected:
	UFUNCTION()
	virtual void OnRep_ServerSessionId();


	/** map of additional stats to hold match total stats*/
	TMap< FName, float > StatsData;

public:
	UPROPERTY()
	float LastScoreStatsUpdateTime;

	/** Accessors for StatsData. */
	float GetStatsValue(FName StatsName);
	void SetStatsValue(FName StatsName, float NewValue);
	void ModifyStatsValue(FName StatsName, float Change);

	/** Returns true if all players are ready */
	UFUNCTION(BlueprintCallable, Category = GameState)
	bool AreAllPlayersReady();

	/** returns whether the player can choose to spawn at the passed in start point (for game modes that allow players to pick)
	 * valid on both client and server
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = GameState)
	bool IsAllowedSpawnPoint(AUTPlayerState* Chooser, APlayerStart* DesiredStart) const;


	/** Current index to use as basis for next selection in Taunt list. */
	UPROPERTY()
		int32 TauntSelectionIndex;

	virtual void FillOutRconPlayerList(TArray<FRconPlayerData>& PlayerList);

	/** hook for blueprints */
	UFUNCTION(BlueprintCallable, Category = GameState)
	TSubclassOf<AGameMode> GetGameModeClass() const
	{
		return TSubclassOf<AGameMode>(*GameModeClass);
	}

	virtual void MakeJsonReport(TSharedPtr<FJsonObject> JsonObject);

	/** if > 0 and BoostRechargeMaxCharges > 0 then player's activatable boost recharges after this many seconds */
	UPROPERTY(BlueprintReadWrite, Replicated)
	float BoostRechargeTime;
	/** maximum number of boost charges that can be recharged through the timer */
	UPROPERTY(BlueprintReadWrite, Replicated)
	int32 BoostRechargeMaxCharges;

	// Flags that this match no longer allows for a party join
	UPROPERTY(BlueprintReadOnly, Replicated)
	bool bRestrictPartyJoin;

	virtual bool CanShowBoostMenu(AUTPlayerController* Target);
	
	UPROPERTY(Replicated, GlobalConfig, EditAnywhere, BlueprintReadWrite, Category = GameState)
	bool bOnlyTeamCanVoteKick;
	
	UPROPERTY(Replicated, GlobalConfig, EditAnywhere, BlueprintReadWrite, Category = GameState)
	bool bDisableVoteKick;

	virtual void PrepareForIntermission();

	//User Info Query for all players in the match
public:

	FText GetEpicAccountNameForAccount(TSharedRef<const FUniqueNetId> UserId);

	/** Informs the player controller that it might need to do a new UserInfoQuery as UserInfo may have changed*/
	virtual void AddUserInfoQuery(TSharedRef<const FUniqueNetId> UserId);
	virtual void AddAllUsersToInfoQuery();

protected:
	IOnlineUserPtr OnlineUserInterface;
	FOnQueryUserInfoCompleteDelegate OnUserInfoCompleteDelegate;
	virtual void OnQueryUserInfoComplete(int32 LocalPlayer, bool bWasSuccessful, const TArray< TSharedRef<const FUniqueNetId> >& UserIds, const FString& ErrorStr);
	virtual void RunAllUserInfoQuery();

	void StartFPSCharts();
	void StopFPSCharts();

	void OnHitchDetected(float DurationInSeconds);

	bool bRunFPSChart;

	TSharedPtr<FPerformanceTrackingChart> GameplayFPSChart;

	/** Handle to the delegate bound for hitch detection */
	FDelegateHandle OnHitchDetectedHandle;

	/** How many unplayable hitches we have had during this match. */
	int32 UnplayableHitchesDetected;

	/** How much time we spent hitching above unplayable threshold, in milliseconds. */
	double UnplayableTimeInMs;

	/** Threshold after which a hitch is considered unplayable (hitch must be >= the threshold) */
	UPROPERTY(Config)
	float UnplayableHitchThresholdInMs;

	/** Threshold after which we consider that the server is unplayable and report that. */
	UPROPERTY(Config)
	int32 MaxUnplayableHitchesToTolerate;
	
protected:
	float UserInfoQueryRetryTime;
	FTimerHandle UserInfoQueryRetryHandle;

	/**Used to determine if we have gotten new User Data and thus need to perform a new Query*/
	bool bIsUserQueryNeeded;

	/**Used  to determine if a UserQuery is in progress*/
	bool bIsAlreadyPendingUserQuery;

	/** Array holding net ids to query*/
	TArray<TSharedRef<const FUniqueNetId>> CurrentUsersToQuery;

	/** Allows the game to change the client's music volume based on the state of the game */
	virtual void ManageMusicVolume(float DeltaTime);

	/** Holds a list of any line ups that have been spawned. 
		Line Ups are only spawned when the level is missing them. **/
	UPROPERTY()
	TArray<AUTLineUpZone*> SpawnedLineUps;

public:
	// This is the GUID if the current servers.  See UTBaseGameMode for more information
	UPROPERTY(Replicated)
	FGuid ServerInstanceGUID;

	UPROPERTY(Replicated)
	AUTPlayerState* LeadLineUpPlayer;

	UPROPERTY(Replicated)
	AUTLineUpHelper* ActiveLineUpHelper;

	virtual bool IsLineUpActive();

	virtual AUTLineUpZone* GetAppropriateSpawnList(LineUpTypes ZoneType);

	virtual void SpawnDefaultLineUpZones();

	virtual void CreateLineUp(LineUpTypes LineUpType);

	virtual void ClearLineUp();

	virtual UCameraComponent* GetCameraComponentForLineUp(LineUpTypes ZoneType);
	
	// Returns true if the replication of the MapVote list is completed
	bool IsMapVoteListReplicationCompleted();

	virtual bool HasMatchEnded() const;

	UPROPERTY(Replicated)
	FString ReplayID;

	UPROPERTY(Replicated)
	FGuid MatchID;

	// Will be set true on a client if their menu is opened.  We use this to override the
	// code that fades out the level music while in a game
	bool bLocalMenusAreActive;

	virtual void RulesetsAreLoaded() {}

	// Holds the Unique id of the host who started this match
	UPROPERTY(Replicated)
	FString HostIdString; 

	virtual FText OverrideRoleText(AUTPlayerState* PS) { return FText::GetEmpty(); };

protected:
	virtual AUTLineUpZone* CreateLineUpAtPlayerStart(LineUpTypes LineUpType, class APlayerStart* PlayerSpawn);
	void TrackGame();

	UFUNCTION()
	void OnReceiveHubGuid();

};