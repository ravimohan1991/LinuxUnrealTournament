// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTCarriedObject.h"
#include "UTBlitzFlag.h"
#include "UTFlagRunGameState.h"
#include "UTFlagRunGame.h"
#include "UTBlitzFlagSpawner.h"
#include "UTBlitzDeliveryPoint.h"
#include "UTCTFRewardMessage.h"
#include "UTFlagRunGameMessage.h"

static FName NAME_Wipe(TEXT("Wipe"));

AUTBlitzFlag::AUTBlitzFlag(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bGradualAutoReturn = true;
	AutoReturnTime = 8.f;
	bDisplayHolderTrail = true;
	bShouldPingFlag = true;
	bSendHomeOnScore = false;
	MessageClass = UUTFlagRunGameMessage::StaticClass();
	bEnemyCanPickup = false;
	bFriendlyCanPickup = true;
	bTeamPickupSendsHome = false;
	bEnemyPickupSendsHome = false;
}

void AUTBlitzFlag::SendHomeWithNotify()
{
	if ((Role == ROLE_Authority) && bWaitForNearbyPlayer)
	{
		// if team member nearby, wait a bit longer
		AUTFlagRunGameState* GameState = GetWorld()->GetGameState<AUTFlagRunGameState>();
		for (FConstPawnIterator It = GetWorld()->GetPawnIterator(); It; ++It)
		{
			AUTCharacter* TeamChar = Cast<AUTCharacter>(*It);
			if (TeamChar && !TeamChar->IsDead() && GameState && GameState->OnSameTeam(TeamChar, this) && IsNearTeammate(TeamChar))
			{
				GetWorldTimerManager().SetTimer(SendHomeWithNotifyHandle, this, &AUTBlitzFlag::SendHomeWithNotify, 0.2f, false);
				return;
			}
		}
	}

	AUTBlitzFlagSpawner* FlagBase = Cast<AUTBlitzFlagSpawner>(HomeBase);
	if (FlagBase)
	{
		UUTGameplayStatics::UTPlaySound(GetWorld(), FlagBase->FlagReturnedSound, this);
	}
	SendHome();
}

void AUTBlitzFlag::Drop(AController* Killer)
{
	UUTGameplayStatics::UTPlaySound(GetWorld(), DropSound, (HoldingPawn != NULL) ? (AActor*)HoldingPawn : (AActor*)this);

	bool bDelayDroppedMessage = false;
	AUTPlayerState* KillerState = Killer ? Cast<AUTPlayerState>(Killer->PlayerState) : NULL;
	if (KillerState && KillerState->Team && (KillerState != Holder))
	{
		// see if this is a last second save
		AUTFlagRunGameState* GameState = GetWorld()->GetGameState<AUTFlagRunGameState>();
		if (GameState && GameState->DeliveryPoint && GameState->DeliveryPoint->ActorIsNearMe(this))
		{
			AUTFlagRunGame* GM = GetWorld()->GetAuthGameMode<AUTFlagRunGame>();
			if (GM)
			{
				bDelayDroppedMessage = true;
				GM->BroadcastLocalized(this, UUTCTFRewardMessage::StaticClass(), 6, Killer->PlayerState, Holder, NULL);
				GM->AddDeniedEventToReplay(Killer->PlayerState, Holder, Holder->Team);
				KillerState->AddCoolFactorEvent(100.0f);
				KillerState->ModifyStatsValue(NAME_FlagDenials, 1);
			}
		}
	}

	FlagDropTime = GetWorld()->GetTimeSeconds();
	if (bDelayDroppedMessage)
	{
		FTimerHandle TempHandle;
		GetWorldTimerManager().SetTimer(TempHandle, this, &AUTBlitzFlag::DelayedDropMessage, 0.8f, false);
	}
	else
	{
		SendGameMessage(3, Holder, NULL);
		LastDroppedMessageTime = GetWorld()->GetTimeSeconds();
	}
	NoLongerHeld();

	if (HomeBase != NULL)
	{
		HomeBase->ObjectWasDropped(LastHoldingPawn);
	}
	ChangeState(CarriedObjectState::Dropped);

	// Toss is out
	TossObject(LastHoldingPawn);

	if (bGradualAutoReturn && (Holder == nullptr))
	{
		RemoveInvalidPastPositions();
		if (PastPositions.Num() > 0)
		{
			PutGhostFlagAt(PastPositions[PastPositions.Num() - 1]);
		}
		else if (HomeBase)
		{
			FFlagTrailPos BasePosition;
			BasePosition.Location = GetHomeLocation();
			PutGhostFlagAt(BasePosition);
		}
	}
}
