// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalMessage.h"
#include "UTDoorMessage.h"
#include "GameFramework/LocalMessage.h"


UUTDoorMessage::UUTDoorMessage(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsUnique = true;
	bIsConsoleMessage = false;
	bIsStatusAnnouncement = false;
	bPlayDuringIntermission = false;
	Lifetime = 2.f;
	AnnouncementDelay = 1.5f;
	MessageArea = FName(TEXT("Announcements"));
	MessageSlot = FName(TEXT("MajorRewardMessage"));
	DoorOpenText = NSLOCTEXT("RedeemerMessage", "DoorOpen", "Door is Open");
	DoorClosedText = NSLOCTEXT("RedeemerMessage", "DoorClosed", "Door is Closing");
}

FText UUTDoorMessage::GetText(int32 Switch, bool bTargetsPlayerState1, class APlayerState* RelatedPlayerState_1, class APlayerState* RelatedPlayerState_2, class UObject* OptionalObject) const
{
	if (Switch == 1)
	{
		return DoorOpenText;
	}
	if (Switch == 0)
	{
		return DoorClosedText;
	}
	return FText::GetEmpty();
}


float UUTDoorMessage::GetAnnouncementDelay(int32 Switch)
{
	return (Switch == 1) ? 1.f : AnnouncementDelay;
}

FName UUTDoorMessage::GetAnnouncementName_Implementation(int32 Switch, const UObject* OptionalObject, const class APlayerState* RelatedPlayerState_1, const class APlayerState* RelatedPlayerState_2) const
{
	switch (Switch)
	{
	case 0: return TEXT("RZE_DoorClosing"); break;
	case 1: return TEXT("RZE_DoorOpening"); break;
	}
	return NAME_None;
}

void UUTDoorMessage::PrecacheAnnouncements_Implementation(class UUTAnnouncer* Announcer) const
{
	for (int32 i = 0; i < 2; i++)
	{
		FName SoundName = GetAnnouncementName(i, NULL, NULL, NULL);
		if (SoundName != NAME_None)
		{
			Announcer->PrecacheAnnouncement(SoundName);
		}
	}
}

float UUTDoorMessage::GetAnnouncementPriority(const FAnnouncementInfo AnnouncementInfo) const
{
	return 1.f;
}

bool UUTDoorMessage::ShouldPlayAnnouncement(const FClientReceiveData& ClientData) const
{
	return true;
}
