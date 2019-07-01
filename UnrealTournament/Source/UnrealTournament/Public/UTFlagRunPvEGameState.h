// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTFlagRunGameState.h"

#include "UTFlagRunPvEGameState.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTFlagRunPvEGameState : public AUTFlagRunGameState
{
	GENERATED_BODY()
public:
	UPROPERTY(Replicated)
	int32 GameDifficulty;

	/** used to sync up ElapsedTime at game end for scoreboard, since the value is replicated InitialOnly */
	UPROPERTY(ReplicatedUsing = OnRep_EndingElapsedTime)
	int32 EndingElapsedTime;

	UFUNCTION()
	void OnRep_EndingElapsedTime()
	{
		ElapsedTime = EndingElapsedTime;
	}

	virtual void OnRep_MatchState() override
	{
		Super::OnRep_MatchState();

		if (Role == ROLE_Authority && (HasMatchEnded() || IsMatchIntermission()))
		{
			EndingElapsedTime = ElapsedTime;
		}
	}

	/** time next star will be awarded (relative to RemainingTime, not TimeSeconds) */
	UPROPERTY(Replicated)
	int32 NextStarTime;
	UPROPERTY(Replicated)
	int32 KillsUntilExtraLife;
	UPROPERTY()
	int32 ExtraLivesGained;

	virtual FText GetRoundStatusText(bool bForScoreboard) override;

	virtual void UpdateSelectablePowerups() override
	{
		// GameMode sets this via SetSelectablePowerups()
	}
	virtual void AddScoringPlay(const FCTFScoringPlay& NewScoringPlay) override
	{
		// there's no point in a scoring play entry since there's only ever one score per PvE match
	}
	virtual void UpdateTimeMessage() override
	{
		// don't send star notifications
	}

	virtual bool CanSpectate(APlayerController* Viewer, APlayerState* ViewTarget) override
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(ViewTarget);
		return (PS == nullptr || PS->GetTeamNum() != 0) && Super::CanSpectate(Viewer, ViewTarget);
	}
};