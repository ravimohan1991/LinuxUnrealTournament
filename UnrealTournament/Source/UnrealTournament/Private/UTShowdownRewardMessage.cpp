// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTAnnouncer.h"
#include "UTShowdownRewardMessage.h"

UUTShowdownRewardMessage::UUTShowdownRewardMessage(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bIsStatusAnnouncement = false;
	bIsPartiallyUnique = true;
	MessageArea = FName(TEXT("Announcements"));
	MessageSlot = FName(TEXT("MajorRewardMessage"));

	FinishItMsg = NSLOCTEXT("ShowdownRewardMessage", "FinishItMsg", "FINISH IT!");
	LastManMsg = NSLOCTEXT("ShowdownRewardMessage", "LastManMsg", " is Last Man Standing");
	OverChargeMsg = NSLOCTEXT("ShowdownRewardMessage", "OverChargeMsg", "OVERCHARGE AVAILABLE!");
	TerminationMsg = NSLOCTEXT("ShowdownRewardMessage", "TerminationMsg", " TERMINATED!");
	AnnihilationMsg = NSLOCTEXT("ShowdownRewardMessage", "AnnihilationMsg", "ANNIHILATION!");
	FinishIt = FName(TEXT("RW_FinishIt"));
	LastMan = FName(TEXT("RW_LMS"));
	OverCharge = FName(TEXT("RW_Overload"));
	Annihilation = FName(TEXT("RW_Annihilation"));
	Termination = FName(TEXT("RW_Termination"));
	AnnouncementDelay = 0.f;

	static ConstructorHelpers::FObjectFinder<USoundBase> TerminationSoundFinder(TEXT("SoundWave'/Game/RestrictedAssets/Audio/Stingers/Terminated.Terminated'"));
	TerminationSound = TerminationSoundFinder.Object;
}

float UUTShowdownRewardMessage::GetAnnouncementPriority(const FAnnouncementInfo AnnouncementInfo) const
{
	return ((AnnouncementInfo.Switch == 4) || (AnnouncementInfo.Switch == 3)) ? 1.f : 0.9f;
}

void UUTShowdownRewardMessage::ClientReceive(const FClientReceiveData& ClientData) const
{
	Super::ClientReceive(ClientData);
	if (TerminationSound && (ClientData.MessageIndex == 3))
	{
		AUTPlayerController* PC = Cast<AUTPlayerController>(ClientData.LocalPC);
		if (PC != NULL)
		{
			if (TerminationSound != NULL)
			{
				PC->UTClientPlaySound(TerminationSound);
			}
		}
	}
}

float UUTShowdownRewardMessage::GetAnnouncementDelay(int32 Switch)
{
	return (Switch == 3) ? 0.5f : 0.f;
}

void UUTShowdownRewardMessage::GetEmphasisText(FText& PrefixText, FText& EmphasisText, FText& PostfixText, FLinearColor& EmphasisColor, int32 Switch, class APlayerState* RelatedPlayerState_1, class APlayerState* RelatedPlayerState_2, class UObject* OptionalObject) const
{
	if ((Switch == 1) || (Switch == 3))
	{
		PrefixText = FText::GetEmpty();
		PostfixText = (Switch == 1) ? LastManMsg : TerminationMsg;
		EmphasisText = RelatedPlayerState_1 ? FText::FromString(RelatedPlayerState_1->PlayerName) : FText::GetEmpty();
		AUTPlayerState* PS = Cast<AUTPlayerState>(RelatedPlayerState_1);
		EmphasisColor = (PS && PS->Team) ? PS->Team->TeamColor : REDHUDCOLOR;
		return;
	}

	Super::GetEmphasisText(PrefixText, EmphasisText, PostfixText, EmphasisColor, Switch, RelatedPlayerState_1, RelatedPlayerState_2, OptionalObject);
}

FText UUTShowdownRewardMessage::GetText(int32 Switch, bool bTargetsPlayerState1, class APlayerState* RelatedPlayerState_1, class APlayerState* RelatedPlayerState_2, class UObject* OptionalObject) const 
{
	switch (Switch)
	{
	case 0:
		return FinishItMsg;
	case 1:
		return BuildEmphasisText(Switch, RelatedPlayerState_1, RelatedPlayerState_2, OptionalObject); break;  //LastManMsg;
	case 2:
		return OverChargeMsg;
	case 3:
		return BuildEmphasisText(Switch, RelatedPlayerState_1, RelatedPlayerState_2, OptionalObject); break; // TerminationMsg;
	case 4:
		return AnnihilationMsg;
	default:
		return FText();
	}
}

bool UUTShowdownRewardMessage::ShouldPlayAnnouncement(const FClientReceiveData& ClientData) const
{
	return true;
}

FName UUTShowdownRewardMessage::GetAnnouncementName_Implementation(int32 Switch, const UObject* OptionalObject, const class APlayerState* RelatedPlayerState_1, const class APlayerState* RelatedPlayerState_2) const
{
	switch (Switch)
	{
	case 0:
		return FinishIt;
	case 1:
		return LastMan;
	case 2:
		return OverCharge;
	case 3:
		return Termination;
	case 4:
		return Annihilation;
	default:
		return NAME_None;
	}
}

void UUTShowdownRewardMessage::PrecacheAnnouncements_Implementation(UUTAnnouncer* Announcer) const
{
	Announcer->PrecacheAnnouncement(FinishIt);
	Announcer->PrecacheAnnouncement(LastMan);
	Announcer->PrecacheAnnouncement(OverCharge);
	Announcer->PrecacheAnnouncement(Termination);
	Announcer->PrecacheAnnouncement(Annihilation);
}
