// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "UTTeamGameMode.h"
#include "UTATypes.h"
#include "UTFlagRunGame.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTFlagRunGame : public AUTTeamGameMode
{
	GENERATED_UCLASS_BODY()

public:

	/** Cached reference to the CTF game state */
	UPROPERTY(BlueprintReadOnly, Category = Blitz)
		AUTFlagRunGameState* BlitzGameState;

	/** Class of GameState associated with this GameMode. */
	UPROPERTY(EditAnywhere, noclear, BlueprintReadWrite, Category = Classes)
		TSubclassOf<class AUTBaseScoring> BlitzScoringClass;

	/** Handles individual player scoring */
	UPROPERTY(BlueprintReadOnly, Category = Blitz)
		AUTBaseScoring* BlitzScoring;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Blitz)
		int32 IntermissionDuration;

	UPROPERTY(BlueprintReadOnly, Category = Blitz)
		AUTTeamInfo* LastTeamToScore;

	UPROPERTY(transient)
		bool bPlacingPlayersAtIntermission;

	/** Used for periodic defense warnings about unguarded routes. */
	UPROPERTY()
		float LastEntryDefenseWarningTime;

	/** Amount of score earned for 3 star cap. */
	UPROPERTY()
		int32 GoldScore;

	/** Amount of score earned for 2 star cap. */
	UPROPERTY()
		int32 SilverScore;

	/** Amount of score earned for 1 star cap. */
	UPROPERTY()
		int32 BronzeScore;

	// Score for a successful defense
	UPROPERTY()
		int32 DefenseScore;

	UPROPERTY()
		bool bAllowBoosts;

	/*  If true, slow flag carrier */
	UPROPERTY(BlueprintReadOnly, Category = Blitz)
		bool bSlowFlagCarrier;

	/** Active carried object for this round. */
	UPROPERTY(BlueprintReadOnly, Category = Blitz)
		class AUTCarriedObject* ActiveFlag;

	TAssetSubclassOf<class AUTInventory> ActivatedPowerupPlaceholderObject;

	UPROPERTY()
		TSubclassOf<class AUTInventory> ActivatedPowerupPlaceholderClass;

	virtual TSubclassOf<class AUTInventory> GetActivatedPowerupPlaceholderClass() { return ActivatedPowerupPlaceholderClass; };

	/** Sound to play when a rally attempt fails. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Message)
		USoundBase* RallyFailedSound;

	/** Sound to play during countdown to flag available. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Intro)
		TArray<USoundBase*> RampUpMusic;

	/** Sound to play during countdown to flag available. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Intro)
		TArray<float> RampUpTime;

	/** Sound to play during countdown to flag available. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Intro)
		USoundBase* EndMusic;

	/** Sound to play during countdown to flag available. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = Intro)
		float EndMusicTime;

	UFUNCTION()
		virtual void PlayRampUpMusic();

	UFUNCTION()
		virtual void PlayEndMusic();

	/** Last spawn group used by attackers (used to spread out spawns). */
	UPROPERTY()
		FName LastAttackerSpawnGroup;

	/** Last spawn group used by defenders (used to spread out spawns). */
	UPROPERTY()
		FName LastDefenderSpawnGroup;

	/** Alternate round victory condition - get this many kills. */
	UPROPERTY(BlueprintReadWrite, Category = Blitz)
		int32 RoundLives;

	/* If true, use world settings round times */
	UPROPERTY(BlueprintReadOnly, Category = Blitz)
		bool bUseLevelTiming;

	/** Respawn wait time for team with no life limit. */
	UPROPERTY(BlueprintReadOnly, Category = Blitz)
		float UnlimitedRespawnWaitTime;

	/** Limited Respawn wait time. */
	UPROPERTY(BlueprintReadOnly, Category = Blitz)
		float LimitedRespawnWaitTime;

	UPROPERTY(BlueprintReadOnly, Category = Blitz)
		int32 NumRounds;

	/** Last time Ace reward was announced (kill results in all five member of enemy team dead simultaneously). */
	UPROPERTY(BlueprintReadWrite, Category = Blitz)
		float LastAceTime;

	/** Set once first round has been initialized. */
	UPROPERTY()
		bool bFirstRoundInitialized;

	/*  Victory due to secondary score (best total capture time) */
	UPROPERTY(BlueprintReadOnly, Category = Blitz)
		bool bSecondaryWin;

	/** Delay at start of round before attackers can pick up flag. */
	UPROPERTY(BlueprintReadOnly)
		int32 FlagPickupDelay;

	/** Maximum time remaining considered for tiebreak bonus. */
	UPROPERTY()
		int32 MaxTimeScoreBonus;

	UPROPERTY()
		bool bRollingAttackerSpawns;

	UPROPERTY()
		float RollingAttackerRespawnDelay;

	UPROPERTY()
		float LastAttackerSpawnTime;

	UPROPERTY()
		float RollingSpawnStartTime;

	UPROPERTY()
		bool bLastManOccurred;

	UPROPERTY()
		AUTPlayerState* FlagScorer;

protected:
	// If true, players who join during the round, or switch teams during the round will be forced to
	// sit out and wait for the next round/
	UPROPERTY()
		bool bSitOutDuringRound;

public:
	virtual void ScoreRedAlternateWin();
	virtual void ScoreBlueAlternateWin();

	virtual void FlagCountDown();
	virtual void FlagsAreReady();

	virtual void HandleTeamChange(AUTPlayerState* PS, AUTTeamInfo* OldTeam);

	virtual bool CheckForWinner(AUTTeamInfo* ScoringTeam);

	/** Initialize for new round. */
	virtual void InitRound();

	/** Initialize a player for the new round. */
	virtual void InitPlayerForRound(AUTPlayerState* PS);

	virtual void ResetFlags();

	virtual void EndTeamGame(AUTTeamInfo* Winner, FName Reason);

	virtual bool UTIsHandlingReplays() override { return false; }
	virtual void StopRCTFReplayRecording();
	void AddCaptureEventToReplay(AUTPlayerState* Holder, AUTTeamInfo* Team);
	void AddReturnEventToReplay(AUTPlayerState* Returner, AUTTeamInfo* Team);
	virtual void AddDeniedEventToReplay(APlayerState* KillerPlayerState, AUTPlayerState* Holder, AUTTeamInfo* Team) override;
	virtual float GetLineUpTime(LineUpTypes LineUpType) override;

	virtual void PreInitializeComponents() override;
	virtual class AActor* FindPlayerStart_Implementation(AController* Player, const FString& IncomingName = TEXT("")) override;
	virtual void AnnounceWin(AUTTeamInfo* WinningTeam, APlayerState* ScoringPlayer, uint8 Reason);
	virtual void NotifyFirstPickup(AUTCarriedObject* Flag) override;
	virtual float GetScoreForXP(class AUTPlayerState* PS) override;
	virtual void BeginGame() override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual float OverrideRespawnTime(AUTPickupInventory* Pickup, TSubclassOf<AUTInventory> InventoryType) override;
	virtual bool HandleRallyRequest(AController* C) override;
	virtual bool CompleteRallyRequest(AController* C) override;
	virtual void FinishRallyRequest(AController *C) override;
	virtual int32 PickCheatWinTeam();
	virtual bool AvoidPlayerStart(class AUTPlayerStart* P) override;
	virtual void InitDelayedFlag(class AUTCarriedObject* Flag);
	virtual void InitFlagForRound(class AUTCarriedObject* Flag);
	virtual void IntermissionSwapSides();
	virtual int32 GetFlagCapScore();
	virtual int32 GetDefenseScore();
	virtual void CheckRoundTimeVictory();
	virtual void InitGameStateForRound();
	virtual bool IsTeamOnOffense(int32 TeamNumber) const;
	virtual AActor* SetIntermissionCameras(uint32 TeamToWatch);
	virtual void SendRestartNotifications(AUTPlayerState* PS, AUTPlayerController* PC);
	virtual bool PlayerWonChallenge() override;
	virtual bool ChangeTeam(AController* Player, uint8 NewTeam, bool bBroadcast) override;
	virtual bool CheckScore_Implementation(AUTPlayerState* Scorer);
	virtual void CheckGameTime() override;
	virtual void EndPlayerIntro() override;
	virtual uint8 GetWinningTeamForLineUp() const override;
	virtual void RestartPlayer(AController* aPlayer) override;
	virtual void SetPlayerStateInactive(APlayerState* NewPlayerState) override;
	virtual void BuildServerResponseRules(FString& OutRules) override;
	virtual float AdjustNearbyPlayerStartScore(const AController* Player, const AController* OtherController, const ACharacter* OtherCharacter, const FVector& StartLoc, const APlayerStart* P) override;
	virtual void PlayEndOfMatchMessage() override;
	virtual void CreateGameURLOptions(TArray<TSharedPtr<TAttributePropertyBase>>& MenuProps);
	virtual void ScoreKill_Implementation(AController* Killer, AController* Other, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType) override;
	virtual void InitGameState() override;

	virtual int32 GetComSwitch(FName CommandTag, AActor* ContextActor, AUTPlayerController* Instigator, UWorld* World);
	virtual void InitFlags();
	virtual void HandleMatchIntermission();
	virtual void CheatScore();
	virtual void DefaultTimer() override;
	virtual void HandleFlagCapture(AUTCharacter* HolderPawn, AUTPlayerState* Holder);
	virtual int32 IntermissionTeamToView(AUTPlayerController* PC);
	virtual void HandleExitingIntermission();
	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void UpdateSkillRating() override;

	virtual FString GetRankedLeagueName() override;

	virtual float RatePlayerStart(APlayerStart* P, AController* Player) override;

	virtual uint8 GetNumMatchesFor(AUTPlayerState* PS, bool bRankedSession) const override;
	virtual int32 GetEloFor(AUTPlayerState* PS, bool bRankedSession) const override;
	virtual void SetEloFor(AUTPlayerState* PS, bool bRankedSession, int32 NewELoValue, bool bIncrementMatchCount) override;

	virtual void ScoreObject_Implementation(AUTCarriedObject* GameObject, AUTCharacter* HolderPawn, AUTPlayerState* Holder, FName Reason) override;
	virtual void ScoreAlternateWin(int32 WinningTeamIndex, uint8 Reason = 0);

	virtual bool SupportsInstantReplay() const override;
	virtual void FindAndMarkHighScorer() override;
	virtual void HandleRollingAttackerRespawn(AUTPlayerState* OtherPS);

	/** Update tiebreaker value based on new round bonus. Tiebreaker is positive if Red is ahead, negative if blue is ahead. */
	virtual void UpdateTiebreak(int32 Bonus, int32 TeamIndex);

	virtual void ScoreDamage_Implementation(int32 DamageAmount, AUTPlayerState* Victim, AUTPlayerState* Attacker);
	virtual void GameObjectiveInitialized(AUTGameObjective* Obj);

	virtual void CallMatchStateChangeNotify() override;
	virtual void EndGame(AUTPlayerState* Winner, FName Reason) override;
	virtual void SetEndGameFocus(AUTPlayerState* Winner) override;

protected:
	virtual bool IsTeamOnDefense(int32 TeamNumber) const;
	virtual bool IsPlayerOnLifeLimitedTeam(AUTPlayerState* PlayerState) const;
};

