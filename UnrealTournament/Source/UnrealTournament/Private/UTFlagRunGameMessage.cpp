// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTCTFGameMessage.h"
#include "UTFlagRunGameMessage.h"
#include "UTFlagRunGameState.h"
#include "UTTeamInfo.h"

UUTFlagRunGameMessage::UUTFlagRunGameMessage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	KilledMessagePostfix = NSLOCTEXT("CTFGameMessage", "KilledMessage", " killed the flag carrier!");
	NoPowerRallyMessage = NSLOCTEXT("FlagRunGameMessage", "NoPowerRally", "You need the flag to power a rally point.");
	NoDefenderRallyMessage = NSLOCTEXT("FlagRunGameMessage", "NoDefenderRally", "Only attackers can power a rally point.");
	MaxAnnouncementDelay = 2.5f;
}

float UUTFlagRunGameMessage::GetMaxAnnouncementDelay(const FAnnouncementInfo AnnouncementInfo)
{ 
	return ((AnnouncementInfo.Switch == 2) || ((AnnouncementInfo.Switch == 4) && AnnouncementInfo.RelatedPlayerState_1 && Cast<AUTPlayerController>(AnnouncementInfo.RelatedPlayerState_1->GetOwner()))) ? 99.f : MaxAnnouncementDelay;
}

FName UUTFlagRunGameMessage::GetTeamAnnouncement(int32 Switch, uint8 TeamNum, const UObject* OptionalObject, const class APlayerState* RelatedPlayerState_1, const class APlayerState* RelatedPlayerState_2) const
{
	switch (Switch)
	{
	case 0: return TEXT("FlagIsReturning"); break;
	case 1: return TEXT("FlagIsReturning"); break;
	case 2: return (TeamNum == 0) ? TEXT("RedTeamScores") : TEXT("BlueTeamScores"); break;
	case 3: return TEXT("FlagDropped"); break;
	case 4:
		if (RelatedPlayerState_1 && Cast<AUTPlayerController>(RelatedPlayerState_1->GetOwner()))
		{
			return TEXT("YouHaveTheFlag_02"); break;
		}
		return TEXT("FlagTaken"); break;
	}
	return NAME_None;
}

float UUTFlagRunGameMessage::GetAnnouncementSpacing_Implementation(int32 Switch, const UObject* OptionalObject) const
{
	return 0.f;
}

FText UUTFlagRunGameMessage::GetText(int32 Switch, bool bTargetsPlayerState1, APlayerState* RelatedPlayerState_1, APlayerState* RelatedPlayerState_2, UObject* OptionalObject) const
{
	if ((Switch < 2) || (Switch == 3) || ((Switch == 4) && (!RelatedPlayerState_1 || !Cast<AUTPlayerController>(RelatedPlayerState_1->GetOwner()))))
	{
		return FText::GetEmpty();
	}
	if (Switch == 30)
	{
		AUTFlagRunGameState* GS = RelatedPlayerState_1 ? RelatedPlayerState_1->GetWorld()->GetGameState<AUTFlagRunGameState>() : nullptr;
		AUTTeamInfo* Team = Cast<AUTPlayerState>(RelatedPlayerState_1) ? Cast<AUTPlayerState>(RelatedPlayerState_1)->Team : nullptr;
		return (GS && Team &&  GS->IsTeamOnDefense(Team->TeamIndex)) ? NoDefenderRallyMessage : NoPowerRallyMessage;
	}
	return Super::GetText(Switch, bTargetsPlayerState1, RelatedPlayerState_1, RelatedPlayerState_2, OptionalObject);
}
