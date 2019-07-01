// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTCharacterVoice.h"
#include "UTAnnouncer.h"
#include "UTCTFGameMessage.h"

UUTCharacterVoice::UUTCharacterVoice(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	MessageArea = FName(TEXT("ConsoleMessage"));
	bIsStatusAnnouncement = false;
	bOptionalSpoken = true;
	FontSizeIndex = -1;
	Lifetime = 6.0f;
	bPlayDuringIntermission = false;

	// < 1000 is reserved for taunts
	SameTeamBaseIndex = 1000;
	FriendlyReactionBaseIndex = 2000;
	EnemyReactionBaseIndex = 2500;

	// 10000+ is reserved for status messages
	StatusBaseIndex = 10000;
	StatusOffsets.Add(StatusMessage::NeedBackup, 0);
	StatusOffsets.Add(StatusMessage::EnemyFCHere, 100);
	StatusOffsets.Add(StatusMessage::AreaSecure, 200);
	StatusOffsets.Add(StatusMessage::IGotFlag, 300);
	StatusOffsets.Add(StatusMessage::DefendFlag, 400);
	StatusOffsets.Add(StatusMessage::DefendFC, 500);
	StatusOffsets.Add(StatusMessage::GetFlagBack, 600);
	StatusOffsets.Add(StatusMessage::ImOnDefense, 700);
	StatusOffsets.Add(StatusMessage::ImOnOffense, 900);
	StatusOffsets.Add(StatusMessage::SpreadOut, 1000);

	StatusOffsets.Add(StatusMessage::ImGoingIn, KEY_CALLOUTS + 100);
	StatusOffsets.Add(StatusMessage::BaseUnderAttack, KEY_CALLOUTS + 200);
	StatusOffsets.Add(StatusMessage::Incoming, KEY_CALLOUTS + 250);

	StatusOffsets.Add(StatusMessage::EnemyRally, KEY_CALLOUTS + 5000);
	StatusOffsets.Add(StatusMessage::FindFC, KEY_CALLOUTS + 5001);
	StatusOffsets.Add(StatusMessage::LastLife, KEY_CALLOUTS + 5002);
	StatusOffsets.Add(StatusMessage::EnemyLowLives, KEY_CALLOUTS + 5003);
	StatusOffsets.Add(StatusMessage::EnemyThreePlayers, KEY_CALLOUTS + 5004);
	StatusOffsets.Add(StatusMessage::NeedRally, KEY_CALLOUTS + 5006);
	StatusOffsets.Add(StatusMessage::NeedHealth, KEY_CALLOUTS + 5007);
	StatusOffsets.Add(StatusMessage::BehindYou, KEY_CALLOUTS + 5008);
	StatusOffsets.Add(StatusMessage::RedeemerSpotted, KEY_CALLOUTS + 5009);
	StatusOffsets.Add(StatusMessage::GetTheFlag, KEY_CALLOUTS + 5010);
	StatusOffsets.Add(StatusMessage::RallyNow, KEY_CALLOUTS + 5011);
	StatusOffsets.Add(StatusMessage::DoorRally, KEY_CALLOUTS + 5012);
	StatusOffsets.Add(StatusMessage::SniperSpotted, KEY_CALLOUTS + 5013);
	StatusOffsets.Add(StatusMessage::RallyHot, KEY_CALLOUTS + 5014);

	//FIRSTPICKUPSPEECH = KEY_CALLOUTS + 5099;
	StatusOffsets.Add(PickupSpeechType::RedeemerPickup, KEY_CALLOUTS + 5100);
	StatusOffsets.Add(PickupSpeechType::UDamagePickup, KEY_CALLOUTS + 5200);
	StatusOffsets.Add(PickupSpeechType::ShieldbeltPickup, KEY_CALLOUTS + 5300);
	// LASTPICKUPSPEECH = KEY_CALLOUTS + 9999;

	StatusOffsets.Add(StatusMessage::RedeemerKills, KEY_CALLOUTS + 10000);	

	TauntText = NSLOCTEXT("UTCharacterVoice", "Taunt", ": {TauntMessage}");
	StatusTextFormat = NSLOCTEXT("UTCharacterVoice", "StatusFormat", " at {LastKnownLocation}: {TauntMessage}");
}

FName UUTCharacterVoice::GetFallbackLines(FName InName) const
{
	// try to construct lines
	FString LocationString = TEXT("GV_") + InName.ToString();
	FName FallbackLines = FName(*LocationString);
	for (int32 i = 0; i < LocationLines.Num(); i++)
	{
		if (FallbackLines == LocationLines[i].VoiceLinesSet)
		{
			return FallbackLines;
		}
	}
	return NAME_None;
}

bool UUTCharacterVoice::IsOptionalSpoken(int32 MessageIndex) const
{
	return bOptionalSpoken && (MessageIndex < StatusBaseIndex+KEY_CALLOUTS);
}

int32 UUTCharacterVoice::GetDestinationIndex(int32 MessageIndex) const
{
	return (MessageIndex < 1000) ? 4 : 6;
}

FText UUTCharacterVoice::GetText(int32 Switch, bool bTargetsPlayerState1, class APlayerState* RelatedPlayerState_1, class APlayerState* RelatedPlayerState_2, class UObject* OptionalObject) const
{
	// hack for comms menu
	if (Switch < 0)
	{
		Switch = -Switch;
	}
	else
	{
		// @TOOD FIXMESTEVE option to turn these on
		return FText::GetEmpty();
	}

	FFormatNamedArguments Args;
	if (!RelatedPlayerState_1)
	{
		UE_LOG(UT, Warning, TEXT("Character voice w/ no playerstate index %d"), Switch);
		return FText::GetEmpty();
	}

	if (Switch == DROP_FLAG_SWITCH_INDEX)
	{
		return NSLOCTEXT("CharacterVoice", "DropFlag", "Drop the flag!");
	}

	UUTGameUserSettings* GS = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());
	if (GS != NULL && GS->GetBotSpeech() < ((Switch >= StatusBaseIndex) ? BSO_StatusTextOnly : BSO_All))
	{ 
		return FText::GetEmpty();
	}
	FCharacterSpeech PickedSpeech = GetCharacterSpeech(Switch);
    Args.Add("PlayerName", FText::AsCultureInvariant(RelatedPlayerState_1->GetPlayerName()));
	Args.Add("TauntMessage", PickedSpeech.SpeechText);

	bool bStatusMessage = (Switch >= StatusBaseIndex) && OptionalObject && Cast<AUTGameVolume>(OptionalObject);
	if (bStatusMessage)
	{
		Args.Add("LastKnownLocation", Cast<AUTGameVolume>(OptionalObject)->VolumeName);
	}
	return bStatusMessage ? FText::Format(StatusTextFormat, Args) : FText::Format(TauntText, Args);
}

FName UUTCharacterVoice::GetAnnouncementName_Implementation(int32 Switch, const UObject* OptionalObject, const class APlayerState* RelatedPlayerState_1, const class APlayerState* RelatedPlayerState_2) const
{
	return NAME_Custom;
}

USoundBase* UUTCharacterVoice::GetAnnouncementSound_Implementation(int32 Switch, const UObject* OptionalObject) const
{
	const AUTGameVolume* LocationVolume = Cast<AUTGameVolume>(OptionalObject);
	if (LocationVolume != nullptr)
	{
		for (int32 i = 0; i < LocationLines.Num(); i++)
		{
			if (LocationVolume->VoiceLinesSet == LocationLines[i].VoiceLinesSet)
			{
				// @TODO FIXMESTEVE optimized sorted array somewhere, or keep map of indices
				if (Switch < 2)
				{
					// @TODO FIXMESTEVE
					Switch = (LocationVolume->bIsDefenderBase || LocationVolume->bIsWarningZone || LocationVolume->bPlayIncomingWarning) ? 1 : 0;
					if ((Switch == 1) && (LocationLines[0].FlagHereUrgent.Num() == 0))
					{
						Switch = 0;
					}
				}
				switch (Switch)
				{
				case 0: return (LocationLines[i].FlagHereNormal.Num() > 0) ? LocationLines[i].FlagHereNormal[FMath::RandRange(0, LocationLines[i].FlagHereNormal.Num() - 1)].SpeechSound : nullptr;
				case 1: return (LocationLines[i].FlagHereUrgent.Num() > 0) ? LocationLines[i].FlagHereUrgent[FMath::RandRange(0, LocationLines[i].FlagHereUrgent.Num() - 1)].SpeechSound : nullptr;
				case 2: return (LocationLines[i].SecureSpeech.Num() > 0) ? LocationLines[i].SecureSpeech[FMath::RandRange(0, LocationLines[i].SecureSpeech.Num() - 1)].SpeechSound : nullptr;
				case 3: return (LocationLines[i].UndefendedSpeech.Num() > 0) ? LocationLines[i].UndefendedSpeech[FMath::RandRange(0, LocationLines[i].UndefendedSpeech.Num() - 1)].SpeechSound : nullptr;
				}
			}
		}
		UE_LOG(UT, Warning, TEXT("Lines not found for %s"), *LocationVolume->VoiceLinesSet.ToString());
		return nullptr;
	}
	FCharacterSpeech PickedSpeech = GetCharacterSpeech(Switch);
	return PickedSpeech.SpeechSound;
}

FCharacterSpeech UUTCharacterVoice::GetCharacterSpeech(int32 Switch) const
{
	FCharacterSpeech EmptySpeech;
	EmptySpeech.SpeechSound = nullptr;
	EmptySpeech.SpeechText = FText::GetEmpty();

	if (TauntMessages.Num() > Switch)
	{
		return TauntMessages[Switch];
	}
	else if (SameTeamMessages.Num() > Switch - SameTeamBaseIndex)
	{
		return SameTeamMessages[Switch - SameTeamBaseIndex];
	}
	else if (FriendlyReactions.Num() > Switch - FriendlyReactionBaseIndex)
	{
		return FriendlyReactions[Switch - FriendlyReactionBaseIndex];
	}
	else if (EnemyReactions.Num() > Switch - EnemyReactionBaseIndex)
	{
		return EnemyReactions[Switch - EnemyReactionBaseIndex];
	}
	else if (Switch == ACKNOWLEDGE_SWITCH_INDEX )
	{
		return AcknowledgeMessages[FMath::RandRange(0, AcknowledgeMessages.Num() - 1)];
	}
	else if (Switch == NEGATIVE_SWITCH_INDEX )
	{
		return NegativeMessages[FMath::RandRange(0, NegativeMessages.Num() - 1)];
	}
	else if (Switch == GOT_YOUR_BACK_SWITCH_INDEX)
	{
		return GotYourBackMessages[FMath::RandRange(0, GotYourBackMessages.Num() - 1)];
	}
	else if (Switch == UNDER_HEAVY_ATTACK_SWITCH_INDEX)
	{
		return UnderHeavyAttackMessages[FMath::RandRange(0, UnderHeavyAttackMessages.Num() - 1)];
	}
	else if (Switch == ATTACK_THEIR_BASE_SWITCH_INDEX)
	{
		return AttackTheirBaseMessages[FMath::RandRange(0, AttackTheirBaseMessages.Num() - 1)];
	}
	else if (Switch >= StatusBaseIndex)
	{
		if (Switch < StatusBaseIndex + KEY_CALLOUTS)
		{
			if (Switch == GetStatusIndex(StatusMessage::NeedBackup))
			{
				if (NeedBackupMessages.Num() == 0)
				{
					return EmptySpeech;
				}
				return NeedBackupMessages[FMath::RandRange(0, NeedBackupMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::EnemyFCHere))
			{
				if (EnemyFCHereMessages.Num() == 0)
				{
					return EmptySpeech;
				}
				return EnemyFCHereMessages[FMath::RandRange(0, EnemyFCHereMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::AreaSecure))
			{
				if (AreaSecureMessages.Num() == 0)
				{
					return EmptySpeech;
				}
				return AreaSecureMessages[FMath::RandRange(0, AreaSecureMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::IGotFlag))
			{
				if (IGotFlagMessages.Num() == 0)
				{
					return EmptySpeech;
				}
				return IGotFlagMessages[FMath::RandRange(0, IGotFlagMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::DefendFlag))
			{
				if (DefendFlagMessages.Num() == 0)
				{
					return EmptySpeech;
				}
				return DefendFlagMessages[FMath::RandRange(0, DefendFlagMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::DefendFC))
			{
				if (DefendFCMessages.Num() == 0)
				{
					return EmptySpeech;
				}
				return DefendFCMessages[FMath::RandRange(0, DefendFCMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::GetFlagBack))
			{
				if (GetFlagBackMessages.Num() == 0)
				{
					return EmptySpeech;
				}
				return GetFlagBackMessages[FMath::RandRange(0, GetFlagBackMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::ImOnDefense))
			{
				if (ImOnDefenseMessages.Num() == 0)
				{
					return EmptySpeech;
				}
				return ImOnDefenseMessages[FMath::RandRange(0, ImOnDefenseMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::ImOnOffense))
			{
				if (ImOnOffenseMessages.Num() == 0)
				{
					return EmptySpeech;
				}
				return ImOnOffenseMessages[FMath::RandRange(0, ImOnOffenseMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::SpreadOut))
			{
				if (SpreadOutMessages.Num() == 0)
				{
					return EmptySpeech;
				}
				return SpreadOutMessages[FMath::RandRange(0, SpreadOutMessages.Num() - 1)];
			}
		}
		else
		{
			if (Switch == GetStatusIndex(StatusMessage::ImGoingIn))
			{
				if (ImGoingInMessages.Num() == 0)
				{
					return EmptySpeech;
				}
				return ImGoingInMessages[FMath::RandRange(0, ImGoingInMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::BaseUnderAttack))
			{
				if (BaseUnderAttackMessages.Num() == 0)
				{
					return EmptySpeech;
				}
				return BaseUnderAttackMessages[FMath::RandRange(0, BaseUnderAttackMessages.Num() - 1)];
			}
			else if (Switch /100 == GetStatusIndex(StatusMessage::Incoming) / 100)
			{
				if (IncomingMessages.Num() == 0)
				{
					return EmptySpeech;
				}
				// incoming messages use switch to specify offset in array of possibilities ordered (center, high, low, left, right)
				int32 ResolvedSwitch = Switch - GetStatusIndex(StatusMessage::Incoming);
				if (ResolvedSwitch >= IncomingMessages.Num())
				{
					return IncomingMessages[0];
				}
				return IncomingMessages[ResolvedSwitch];
			}
			else if (Switch / 100 == GetStatusIndex(PickupSpeechType::UDamagePickup) / 100)
			{
				switch (Switch - GetStatusIndex(PickupSpeechType::UDamagePickup))
				{
				case 0: return UDamageAvailableLine;
				case 1: return UDamagePickupLine;
				case 2: return UDamageDroppedLine;
				}
				return EmptySpeech;
			}
			else if (Switch / 100 == GetStatusIndex(PickupSpeechType::ShieldbeltPickup) / 100)
			{
				return (Switch - GetStatusIndex(PickupSpeechType::ShieldbeltPickup) == 0) ? ShieldbeltAvailableLine : ShieldbeltPickupLine;
			}
			else if (Switch / 100 == GetStatusIndex(PickupSpeechType::RedeemerPickup) / 100)
			{
				int32 Index = Switch - GetStatusIndex(PickupSpeechType::RedeemerPickup);
				if (Index == 2)
				{
					if (DroppedRedeemerMessages.Num() == 0)
					{
						return EmptySpeech;
					}
					return DroppedRedeemerMessages[FMath::RandRange(0, DroppedRedeemerMessages.Num() - 1)];
				}
				return (Index == 0) ? RedeemerAvailableLine : RedeemerPickupLine;
			}
			else if (Switch == GetStatusIndex(StatusMessage::EnemyRally))
			{
				return (EnemyRallyMessages.Num() == 0) ? EmptySpeech : EnemyRallyMessages[FMath::RandRange(0, EnemyRallyMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::RallyNow))
			{
				return (RallyNowMessages.Num() == 0) ? EmptySpeech : RallyNowMessages[FMath::RandRange(0, RallyNowMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::DoorRally))
			{
				return (DoorRallyMessages.Num() == 0) ? EmptySpeech : DoorRallyMessages[FMath::RandRange(0, DoorRallyMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::SniperSpotted))
			{
				return (SniperSpottedMessages.Num() == 0) ? EmptySpeech : SniperSpottedMessages[FMath::RandRange(0, SniperSpottedMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::RallyHot))
			{
				return (RallyHotMessages.Num() == 0) ? EmptySpeech : RallyHotMessages[FMath::RandRange(0, RallyHotMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::FindFC))
			{
				return (FindFCMessages.Num() == 0) ? EmptySpeech : FindFCMessages[FMath::RandRange(0, FindFCMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::LastLife))
			{
				return (LastLifeMessages.Num() == 0) ? EmptySpeech : LastLifeMessages[FMath::RandRange(0, LastLifeMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::EnemyLowLives))
			{
				return (EnemyLowLivesMessages.Num() == 0) ? EmptySpeech : EnemyLowLivesMessages[FMath::RandRange(0, EnemyLowLivesMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::EnemyThreePlayers))
			{
				return (EnemyThreePlayersMessages.Num() == 0) ? EmptySpeech : EnemyThreePlayersMessages[FMath::RandRange(0, EnemyThreePlayersMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::NeedRally))
			{
				return (NeedRallyMessages.Num() == 0) ? EmptySpeech : NeedRallyMessages[FMath::RandRange(0, NeedRallyMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::NeedHealth))
			{
				return (NeedHealthMessages.Num() == 0) ? EmptySpeech : NeedHealthMessages[FMath::RandRange(0, NeedHealthMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::BehindYou))
			{
				return (BehindYouMessages.Num() == 0) ? EmptySpeech : BehindYouMessages[FMath::RandRange(0, BehindYouMessages.Num() - 1)];
			}
			else if (Switch == GetStatusIndex(StatusMessage::GetTheFlag))
			{
				return (GetTheFlagMessages.Num() == 0) ? EmptySpeech : GetTheFlagMessages[FMath::RandRange(0, GetTheFlagMessages.Num() - 1)];
			}
			else if (Switch/100 == GetStatusIndex(StatusMessage::RedeemerKills)/100)
			{
				int32 Index = FMath::Min(4, Switch - GetStatusIndex(StatusMessage::RedeemerKills));
				return (RedeemerKillMessages.Num() > Index) ? RedeemerKillMessages[Index] : EmptySpeech;
			}
			else if (Switch == GetStatusIndex(StatusMessage::RedeemerSpotted))
			{
				return (RedeemerSpottedMessages.Num() == 0) ? EmptySpeech : RedeemerSpottedMessages[FMath::RandRange(0, RedeemerSpottedMessages.Num() - 1)];
			}
		}
	}
	return EmptySpeech;
}

void UUTCharacterVoice::PrecacheAnnouncements_Implementation(UUTAnnouncer* Announcer) const
{
}

bool UUTCharacterVoice::ShouldPlayAnnouncement(const FClientReceiveData& ClientData) const
{
	if (ClientData.RelatedPlayerState_1 && ClientData.RelatedPlayerState_1->bIsABot && (TauntMessages.Num() > ClientData.MessageIndex))
	{
		UUTGameUserSettings* GS = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());
		if (GS != NULL && GS->GetBotSpeech() < BSO_All)
		{
			return false;
		}
		return !Cast<AUTPlayerController>(ClientData.LocalPC) || ((AUTPlayerController*)(ClientData.LocalPC))->bHearsTaunts;
	}
	else
	{
		return true;
	}
}

bool UUTCharacterVoice::IsFlagLocationUpdate(int32 Switch, const class UObject* OptionalObject) const
{
	if (Switch == GetStatusIndex(StatusMessage::DoorRally))
	{
		return true;
	}
	return Cast<AUTGameVolume>(OptionalObject) && (Switch < 2);
}

bool UUTCharacterVoice::IsPickupUpdate(int32 Switch) const
{
	return (Switch > StatusBaseIndex + FIRSTPICKUPSPEECH) && (Switch < StatusBaseIndex + LASTPICKUPSPEECH);
}

bool UUTCharacterVoice::InterruptAnnouncement(const FAnnouncementInfo NewAnnouncementInfo, const FAnnouncementInfo OtherAnnouncementInfo) const
{
	if (NewAnnouncementInfo.Switch == GetStatusIndex(StatusMessage::Incoming))
	{
		return (NewAnnouncementInfo.MessageClass == OtherAnnouncementInfo.MessageClass)
			|| OtherAnnouncementInfo.MessageClass->GetDefaultObject<UUTLocalMessage>()->IsOptionalSpoken(OtherAnnouncementInfo.Switch)
			|| (OtherAnnouncementInfo.MessageClass->GetDefaultObject<UUTLocalMessage>()->GetAnnouncementPriority(OtherAnnouncementInfo) < 1.f);
	}
	if (NewAnnouncementInfo.MessageClass == OtherAnnouncementInfo.MessageClass)
	{
		bool bNewIsFlagLocUpdate = IsFlagLocationUpdate(NewAnnouncementInfo.Switch, NewAnnouncementInfo.OptionalObject);
		bool bOldIsFlagLocUpdate = IsFlagLocationUpdate(OtherAnnouncementInfo.Switch, OtherAnnouncementInfo.OptionalObject);
		if (bNewIsFlagLocUpdate)
		{
			return bOldIsFlagLocUpdate || IsPickupUpdate(OtherAnnouncementInfo.Switch) || (OtherAnnouncementInfo.Switch < StatusBaseIndex + KEY_CALLOUTS);
		}
		if (NewAnnouncementInfo.Switch >= StatusBaseIndex + KEY_CALLOUTS)
		{
			if ((OtherAnnouncementInfo.Switch < StatusBaseIndex + KEY_CALLOUTS) && !bOldIsFlagLocUpdate)
			{
				return true;
			}
			if ((NewAnnouncementInfo.Switch == GetStatusIndex(StatusMessage::RallyNow)) && (OtherAnnouncementInfo.Switch == GetStatusIndex(StatusMessage::NeedRally)))
			{
				return true;
			}
		}
	}
	return false;
}

// check using right names
bool UUTCharacterVoice::CancelByAnnouncement_Implementation(int32 Switch, const UObject* OptionalObject, TSubclassOf<UUTLocalMessage> OtherMessageClass, int32 OtherSwitch, const UObject* OtherOptionalObject) const
{
	if (GetClass() == OtherMessageClass)
	{
		if ((Switch < StatusBaseIndex + KEY_CALLOUTS) && !IsFlagLocationUpdate(Switch, OptionalObject))
		{
			return (OtherSwitch >= StatusBaseIndex + KEY_CALLOUTS) || IsFlagLocationUpdate(OtherSwitch, OtherOptionalObject);
		}
		if ((OtherSwitch == GetStatusIndex(StatusMessage::RallyNow)) && (Switch == GetStatusIndex(StatusMessage::NeedRally)))
		{
			return true;
		}
		if ( IsFlagLocationUpdate(OtherSwitch, OtherOptionalObject) && IsFlagLocationUpdate(Switch, OptionalObject) )
		{
			return true;
		}
		return false;
	}
	else
	{
		if ((OtherMessageClass == UUTCTFGameMessage::StaticClass()) && (OtherSwitch == 4) && (Switch == GetStatusIndex(StatusMessage::GetTheFlag)))
		{
			return true;
		}
		return (Switch < StatusBaseIndex + KEY_CALLOUTS);
	}
}

float UUTCharacterVoice::GetAnnouncementPriority(const FAnnouncementInfo AnnouncementInfo) const
{
	if (AnnouncementInfo.Switch == GetStatusIndex(StatusMessage::Incoming))
	{
		return 1.1f;
	}
	if (IsFlagLocationUpdate(AnnouncementInfo.Switch, AnnouncementInfo.OptionalObject))
	{
		return 0.7f;
	}
	return (AnnouncementInfo.Switch >= StatusBaseIndex + KEY_CALLOUTS) ? 0.5f : 0.1f;
}

int32 UUTCharacterVoice::GetStatusIndex(FName NewStatus) const
{
	return StatusBaseIndex + StatusOffsets.FindRef(NewStatus);
}

