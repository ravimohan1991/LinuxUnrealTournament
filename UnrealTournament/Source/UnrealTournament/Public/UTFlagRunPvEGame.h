// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTFlagRunGame.h"
#include "UTMonster.h"

#include "UTFlagRunPvEGame.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTFlagRunPvEGame : public AUTFlagRunGame
{
	GENERATED_BODY()
public:
	AUTFlagRunPvEGame(const FObjectInitializer& OI);

	UPROPERTY(EditDefaultsOnly, Meta = (MetaClass = "UTMonster"))
	TArray<FStringClassReference> EditableMonsterTypes;
	UPROPERTY(BlueprintReadWrite)
	TArray<TSubclassOf<AUTMonster>> MonsterTypesPeon;
	UPROPERTY(BlueprintReadWrite)
	TArray<TSubclassOf<AUTMonster>> MonsterTypesElite;

	// temp vial replacement for energy pickups
	UPROPERTY()
	FStringClassReference VialReplacement;
	UPROPERTY(BlueprintReadWrite)
	bool bLevelHasEnergyPickups;

	/** number of kills to gain an extra life (multiplied by number of lives gained so far) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int32 BaseKillsForExtraLife;

	/** amount of points available to spawn monsters */
	UPROPERTY(BlueprintReadWrite)
	int32 ElitePointsRemaining;
	/** most expensive monster that can spawn at this point in the round */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int32 EliteCostLimit;

	UPROPERTY(EditDefaultsOnly, Meta = (MetaClass = "UTInventory"))
	TArray<FStringClassReference> BoostPowerupTypes;

	virtual void PreloadClientAssets(TArray<UObject*>& ObjList) const override;
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void InitGameState() override;
	virtual void InitBoostTypes();
	virtual bool CheckRelevance_Implementation(AActor* Other) override;
	virtual void StartMatch() override;

	virtual void HandleMatchHasStarted() override;
	virtual void CheckRoundTimeVictory() override;
	virtual void HandleMatchHasEnded() override;
	virtual bool ShouldBalanceTeams(bool bInitialTeam) const override
	{
		return false;
	}
	virtual bool CheckForWinner(AUTTeamInfo* ScoringTeam) override;
	virtual AUTBotPlayer* AddBot(uint8 TeamNum) override;
	virtual bool ChangeTeam(AController* Player, uint8 NewTeam, bool bBroadcast) override;
	virtual void GiveDefaultInventory(APawn* PlayerPawn) override;
	virtual void AddEliteMonsters(int32 MaxNum);
	virtual void AddPeonMonsters(int32 Num);
	UFUNCTION()
	virtual void EscalateMonsters();
	virtual bool ModifyDamage_Implementation(int32& Damage, FVector& Momentum, APawn* Injured, AController* InstigatedBy, const FHitResult& HitInfo, AActor* DamageCauser, TSubclassOf<UDamageType> DamageType) override;
	virtual void ScoreKill_Implementation(AController* Killer, AController* Other, APawn* KilledPawn, TSubclassOf<UDamageType> DamageType) override;
	virtual void NotifyFirstPickup(AUTCarriedObject* Flag) override
	{}
	virtual void FindAndMarkHighScorer() override;
	virtual void HandleRollingAttackerRespawn(AUTPlayerState* OtherPS) override;
	virtual int32 GetFlagCapScore() override;

	virtual void CreateGameURLOptions(TArray<TSharedPtr<TAttributePropertyBase>>& MenuProps) override;
#if !UE_SERVER
	virtual void CreateConfigWidgets(TSharedPtr<class SVerticalBox> MenuSpace, bool bCreateReadOnly, TArray< TSharedPtr<TAttributePropertyBase> >& ConfigProps, int32 MinimumPlayers) override;
#endif

	virtual uint8 GetWinningTeamForLineUp() const override
	{
		// always show player team in endgame line-up, nobody cares about the monsters' stats
		return 1;
	}

protected:
	virtual void SpawnMonster(TSubclassOf<AUTMonster> MonsterClass);
};