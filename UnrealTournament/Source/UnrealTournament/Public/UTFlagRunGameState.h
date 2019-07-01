// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTGameState.h"
#include "UTFlagRunGameState.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTFlagRunGameState : public AUTGameState
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintReadOnly, Replicated, Category = Blitz)
		uint32 bIsAtIntermission : 1;

	UPROPERTY(Replicated)
		int32 IntermissionTime;

	/** Delay before bringing up scoreboard at halftime. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = Blitz)
		float HalftimeScoreDelay;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = Blitz)
		class AUTBlitzDeliveryPoint* DeliveryPoint;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = Blitz)
		class AUTBlitzFlagSpawner* FlagSpawner;

	UPROPERTY(BlueprintReadOnly, Replicated)
		int32 CTFRound;

	UPROPERTY(BlueprintReadOnly, Replicated)
		int32 NumRounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		FText RoundInProgressStatus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		FText FullRoundInProgressStatus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		FText IntermissionStatus;

	virtual FText GetGameStatusText(bool bForScoreboard) override;

	virtual float ScoreCameraView(AUTPlayerState* InPS, AUTCharacter *Character) override;

	virtual uint8 NearestTeamSide(AActor* InActor) override;

	UPROPERTY(Replicated, BlueprintReadOnly)
		uint32 bRedToCap : 1;

	UPROPERTY()
		int32 GoldBonusThreshold;

	UPROPERTY()
		int32 SilverBonusThreshold;

	UPROPERTY()
		FText GoldBonusText;

	UPROPERTY()
		FText SilverBonusText;

	UPROPERTY()
		FText BronzeBonusText;

	UPROPERTY()
		FText GoldBonusTimedText;

	UPROPERTY()
		FText SilverBonusTimedText;

	UPROPERTY()
		FText BronzeBonusTimedText;

	// Early ending time for this round
	UPROPERTY(Replicated)
		int32 EarlyEndTime;

	UPROPERTY(Replicated)
		bool bAttackersCanRally;

	UPROPERTY()
		bool bHaveEstablishedFlagRunner;

	UPROPERTY(Replicated)
		uint8 BonusLevel;

	UPROPERTY(Replicated)
		bool bAllowBoosts;

	UPROPERTY(Replicated)
		int32 FlagRunMessageSwitch;

	UPROPERTY(Replicated)
		int32 TiebreakValue;

	UPROPERTY(Replicated)
		int32 RemainingPickupDelay;

	UPROPERTY(Replicated)
		int32 RampStartTime;

	UPROPERTY(Replicated)
		class AUTTeamInfo* FlagRunMessageTeam;

	UPROPERTY(BlueprintReadOnly, Replicated)
		class AUTRallyPoint* CurrentRallyPoint;

	UPROPERTY(BlueprintReadOnly)
		class AUTRallyPoint* PendingRallyPoint;

	UPROPERTY(BlueprintReadOnly, Replicated)
		bool bEnemyRallyPointIdentified;

	/** Used during highlight generation. */
	UPROPERTY(BlueprintReadWrite)
		bool bHaveRallyHighlight;

	UPROPERTY(BlueprintReadWrite)
		bool bHaveRallyPoweredHighlight;

	UPROPERTY(BlueprintReadWrite)
		int32 HappyCount;

	UPROPERTY(BlueprintReadWrite)
		int32 HiredGunCount;

	UPROPERTY(BlueprintReadWrite)
		int32 BobLifeCount;

	UPROPERTY(BlueprintReadWrite)
		int32 BubbleGumCount;

	UPROPERTY(BlueprintReadWrite)
		int32 NaturalKillerCount;

	UPROPERTY(BlueprintReadWrite)
		int32 DestroyerCount;

	virtual void PlayTimeWarningSound();

	virtual void Tick(float DeltaTime) override;
	virtual void BeginPlay() override;

	virtual void UpdateSelectablePowerups();

	virtual void CheckTimerMessage() override;


	UFUNCTION(BlueprintCallable, Category = Team)
		virtual bool IsTeamOnOffense(int32 TeamNumber) const;

	UFUNCTION(BlueprintCallable, Category = Team)
		virtual bool IsTeamOnDefense(int32 TeamNumber) const;

	UFUNCTION(BlueprintCallable, Category = Team)
		virtual bool IsTeamOnDefenseNextRound(int32 TeamNumber) const;

	UFUNCTION(BlueprintCallable, Category = GameState)
		FString GetPowerupSelectWidgetPath(int32 TeamNumber);

	UFUNCTION(BlueprintCallable, Category = GameState)
		virtual TSubclassOf<class AUTInventory> GetSelectableBoostByIndex(AUTPlayerState* PlayerState, int Index) const override;

	UFUNCTION(BlueprintCallable, Category = GameState)
		virtual bool IsSelectedBoostValid(AUTPlayerState* PlayerState) const override;

	UFUNCTION(BlueprintCallable, Category = Team)
		virtual AUTBlitzFlag* GetOffenseFlag();

	//Handles precaching all game announcement sounds for the local player
	virtual void PrecacheAllPowerupAnnouncements(class UUTAnnouncer* Announcer) const;

	virtual FText GetRoundStatusText(bool bForScoreboard);

	virtual FLinearColor GetGameStatusColor() override;
	virtual void UpdateRoundHighlights() override;

	virtual void AddMinorRoundHighlights(AUTPlayerState* PS);

	virtual int32 NumHighlightsNeeded() override;
	virtual bool InOrder(class AUTPlayerState* P1, class AUTPlayerState* P2) override;

protected:
	virtual void UpdateTimeMessage() override;
	virtual void ManageMusicVolume(float DeltaTime) override;

	virtual void CachePowerupAnnouncement(class UUTAnnouncer* Announcer, TSubclassOf<AUTInventory> PowerupClass) const;

	virtual void AddModeSpecificOverlays();

	// FIXME: Replication is temp
	UPROPERTY(Replicated)
	TArray<TSubclassOf<class AUTInventory>> DefenseSelectablePowerups;
	UPROPERTY(Replicated)
	TArray<TSubclassOf<class AUTInventory>> OffenseSelectablePowerups;

public:
	virtual void SetSelectablePowerups(const TArray<TSubclassOf<AUTInventory>>& OffenseList, const TArray<TSubclassOf<AUTInventory>>& DefenseList);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoleStrings")
		FText AttackText;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RoleStrings")
		FText DefendText;

	virtual FText OverrideRoleText(AUTPlayerState* PS) override;

	virtual void CacheGameObjective(class AUTGameObjective* BaseToCache);

	UFUNCTION(BlueprintCallable, Category = GameState)
		virtual AUTPlayerState* GetFlagHolder();

	virtual class AUTGameObjective* GetFlagBase(uint8 TeamNum);

	virtual void ResetFlags();

	virtual float GetClockTime() override;
	virtual bool IsMatchInProgress() const override;
	virtual bool IsMatchIntermission() const override;
	virtual float GetIntermissionTime() override;
	virtual void DefaultTimer() override;

	virtual FName OverrideCameraStyle(APlayerController* PCOwner, FName CurrentCameraStyle);

	virtual void UpdateHighlights_Implementation() override;
	virtual void AddMinorHighlights_Implementation(AUTPlayerState* PS) override;

private:
	/** list of scoring plays
	* replicating dynamic arrays is dangerous for bandwidth and performance, but the alternative in this case is some painful code so we're as safe as possible by tightly restricting access
	*/
	UPROPERTY(Replicated)
		TArray<FCTFScoringPlay> ScoringPlays;

protected:
	void SpawnLineUpZoneOnFlagBase(class AUTGameObjective* BaseToSpawnOn, LineUpTypes TypeToSpawn);

public:
	inline const TArray<const FCTFScoringPlay>& GetScoringPlays() const
	{
		return *(const TArray<const FCTFScoringPlay>*)&ScoringPlays;
	}

	virtual AUTLineUpZone* GetAppropriateSpawnList(LineUpTypes ZoneType);

	/** Checks that all line up types have a valid line up zone for each line up type. If not, it creates a default to use for each type **/
	virtual void SpawnDefaultLineUpZones();

	class AUTGameObjective* GetLeadTeamFlagBase();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Scoring)
		virtual void AddScoringPlay(const FCTFScoringPlay& NewScoringPlay);

};
