// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once
#include "UTATypes.h"
#include "UTResetInterface.h"
#include "UTGameVolume.generated.h"

/**
* Type of physics volume that has UT specific gameplay effects/data.
*/

UCLASS(BlueprintType)
class UNREALTOURNAMENT_API AUTGameVolume : public APhysicsVolume, public IUTTeamInterface, public IUTResetInterface
{
	GENERATED_UCLASS_BODY()

	/** Displayed volume name, @TODO FIXMESTEVE should be localized. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
		FText VolumeName;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
		bool bShowOnMinimap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
		FVector2D MinimapOffset;

	UPROPERTY(BlueprintReadWrite, Category = "Gameplay")
		TArray<class AUTSupplyChest*> TeamLockers;

	UPROPERTY(BlueprintReadWrite, Category = "Gameplay")
		TArray<class AUTRallyPoint*> RallyPoints;

	/** If team safe volume, associated team members are invulnerable and everyone else is killed entering this volume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
	bool bIsTeamSafeVolume;

	/** OBSOLETE */
	UPROPERTY()
		bool bIsNoRallyZone;

	/** Set true if this volume is part of the core defender base. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
		bool bIsDefenderBase;

	/** If true, play incoming warning when enemy enters this zone. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
		bool bPlayIncomingWarning;

	/** Character entering this volume immediately triggers teleporter in this volume. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
		bool bIsTeleportZone;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
		bool bIsWarningZone;

	/** Alarm sound played if this is bNoRallyZone and enemy flag carrier enters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
		USoundBase* AlarmSound;

	UPROPERTY()
		class AUTTeleporter* AssociatedTeleporter;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
		uint8 TeamIndex;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
		FName VoiceLinesSet;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
		bool bReportDefenseStatus;

	/** Set when volume is entered for the first time. */
	UPROPERTY(BlueprintReadWrite, Category = "Gameplay")
		bool bHasBeenEntered;

	UPROPERTY(BlueprintReadWrite, Category = "Gameplay")
		bool bHasFCEntry;

	/** minimum interval between non-FC enemy in base warnings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = GameState)
		float MinEnemyInBaseInterval;

	/** Used to identify unique routes/entries to enemy base.  Default -1, inner base 0, entries each have own value. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gameplay")
		int32 RouteID;

	virtual void ActorEnteredVolume(class AActor* Other) override;
	virtual void ActorLeavingVolume(class AActor* Other) override;

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "GetTeamNum"))
		uint8 ScriptGetTeamNum();

	/** return team number the object is on, or 255 for no team */
	virtual uint8 GetTeamNum() const { return TeamIndex; };

	virtual void SetTeamForSideSwap_Implementation(uint8 NewTeamNum) override;
	virtual void Reset_Implementation() override;
	virtual void PostInitializeComponents() override;

	/* Play incoming direction message for attackers, plus display debug for easier testing of entry direction orientation.  Use for testing, clear this flag before checking in. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
		bool bTestBaseEntry;

	// Determine which direction volume is being entered relative to defensive base
	virtual int32 DetermineEntryDirection(class AUTCharacter* EnteringCharacter, class AUTFlagRunGameState* GS);

	virtual void PostLoad() override;

	/** Pick best enemy team player to play announcement. */
	class AUTPlayerState* GetBestWarner(class AUTCharacter* StatusEnemy);

	/** Warn that enemy FC is incoming. */
	virtual void WarnFCIncoming(AUTCharacter* FlagCarrier);
};


