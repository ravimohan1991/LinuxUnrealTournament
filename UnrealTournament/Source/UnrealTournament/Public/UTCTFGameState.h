// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTCTFFlagBase.h"
#include "UTGameState.h"

#include "UTCTFGameState.generated.h"


UCLASS()
class UNREALTOURNAMENT_API AUTCTFGameState: public AUTGameState
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(BlueprintReadOnly,Replicated,Category = CTF)
	uint32 bSecondHalf : 1;

	UPROPERTY(BlueprintReadOnly,Replicated, Category = CTF)
	uint32 bIsAtIntermission : 1;

	/** The Elapsed time at which Overtime began */
	UPROPERTY(BlueprintReadOnly, Category = CTF)
	int32 OvertimeStartTime;

	UPROPERTY(Replicated)
		int32 IntermissionTime;

	/** Will be true if the game is playing advantage going in to half-time */
	UPROPERTY(Replicated)
		uint32 bPlayingAdvantage : 1;

	/** Delay before bringing up scoreboard at halftime. */
	UPROPERTY(BlueprintReadOnly, Replicated, Category = CTF)
		float HalftimeScoreDelay;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = CTF)
	TArray<AUTCTFFlagBase*> FlagBases;

	UPROPERTY(Replicated)
	uint8 AdvantageTeamIndex;

	UPROPERTY(BlueprintReadOnly, Replicated)
	int32 CTFRound;

	UPROPERTY(BlueprintReadOnly, Replicated)
	int32 NumRounds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		FText RedAdvantageStatus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		FText BlueAdvantageStatus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		FText RoundInProgressStatus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		FText FullRoundInProgressStatus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		FText IntermissionStatus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		FText HalftimeStatus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		FText ExtendedOvertimeStatus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		FText FirstHalfStatus;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		FText SecondHalfStatus;

	/** Sets the # of teams.  This will also Pre-seed FlagsBases */
	virtual void SetMaxNumberOfTeams(int32 TeamCount);

	/** Cache a flag by in the FlagBases array */
	virtual void CacheFlagBase(AUTCTFFlagBase* BaseToCache);

	/** Returns the current state of a given flag */
	virtual FName GetFlagState(uint8 TeamNum);

	UFUNCTION(BlueprintCallable, Category = GameState)
	virtual AUTPlayerState* GetFlagHolder(uint8 TeamNum);
	virtual AUTCTFFlagBase* GetFlagBase(uint8 TeamNum);

	virtual void ResetFlags();

	virtual float GetClockTime() override;
	virtual bool IsMatchInProgress() const override;
	virtual bool IsMatchInOvertime() const override;
	virtual bool IsMatchIntermission() const override;
	virtual void OnRep_MatchState() override;
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
	void SpawnLineUpZoneOnFlagBase(AUTCTFFlagBase* BaseToSpawnOn, LineUpTypes TypeToSpawn);

public:
	inline const TArray<const FCTFScoringPlay>& GetScoringPlays() const
	{
		return *(const TArray<const FCTFScoringPlay>*)&ScoringPlays;
	}

	virtual AUTLineUpZone* GetAppropriateSpawnList(LineUpTypes ZoneType);

	/** Checks that all line up types have a valid line up zone for each line up type. If not, it creates a default to use for each type **/
	virtual void SpawnDefaultLineUpZones();

	AUTCTFFlagBase* GetLeadTeamFlagBase();

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = Scoring)
	virtual void AddScoringPlay(const FCTFScoringPlay& NewScoringPlay);

	virtual FText GetGameStatusText(bool bForScoreboard) override;

	virtual float ScoreCameraView(AUTPlayerState* InPS, AUTCharacter *Character) override;

	virtual uint8 NearestTeamSide(AActor* InActor) override;

	bool GetImportantPickups_Implementation(TArray<AUTPickup*>& PickupList);
};
