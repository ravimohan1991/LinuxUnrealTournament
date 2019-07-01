// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTFlagRunGame.h"
#include "UTFlagRunGameState.h"
#include "UTCTFGameMode.h"
#include "UTPowerupSelectorUserWidget.h"
#include "Net/UnrealNetwork.h"
#include "UTCTFScoring.h"
#include "StatNames.h"
#include "UTCountDownMessage.h"
#include "UTAnnouncer.h"
#include "UTCTFMajorMessage.h"
#include "UTRallyPoint.h"
#include "UTWorldSettings.h"
#include "UTBlitzFlag.h"
#include "UTBlitzDeliveryPoint.h"
#include "UTBlitzFlagSpawner.h"
#include "UTATypes.h"

AUTFlagRunGameState::AUTFlagRunGameState(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsAtIntermission = false;
	HalftimeScoreDelay = 3.5f;
	GoalScoreText = NSLOCTEXT("UTScoreboard", "CTFGoalScoreFormat", "{0} Caps");

	GameScoreStats.Add(NAME_RegularKillPoints);
	GameScoreStats.Add(NAME_FCKills);
	GameScoreStats.Add(NAME_FCKillPoints);
	GameScoreStats.Add(NAME_FlagSupportKills);
	GameScoreStats.Add(NAME_FlagSupportKillPoints);
	GameScoreStats.Add(NAME_EnemyFCDamage);
	GameScoreStats.Add(NAME_FlagHeldDeny);
	GameScoreStats.Add(NAME_FlagHeldDenyTime);
	GameScoreStats.Add(NAME_FlagHeldTime);
	GameScoreStats.Add(NAME_FlagReturnPoints);
	GameScoreStats.Add(NAME_CarryAssist);
	GameScoreStats.Add(NAME_CarryAssistPoints);
	GameScoreStats.Add(NAME_FlagCapPoints);
	GameScoreStats.Add(NAME_DefendAssist);
	GameScoreStats.Add(NAME_DefendAssistPoints);
	GameScoreStats.Add(NAME_ReturnAssist);
	GameScoreStats.Add(NAME_ReturnAssistPoints);
	GameScoreStats.Add(NAME_TeamCapPoints);
	GameScoreStats.Add(NAME_FlagGrabs);

	TeamStats.Add(NAME_TeamFlagGrabs);
	TeamStats.Add(NAME_TeamFlagHeldTime);

	SecondaryAttackerStat = NAME_FlagHeldTime;

	HighlightMap.Add(HighlightNames::TopFlagCapturesRed, NSLOCTEXT("AUTGameMode", "TopFlagCapturesRed", "Most Flag Caps for Red with {0}."));
	HighlightMap.Add(HighlightNames::TopFlagCapturesBlue, NSLOCTEXT("AUTGameMode", "TopFlagCapturesBlue", "Most Flag Caps for Blue with {0}."));
	HighlightMap.Add(HighlightNames::TopAssistsRed, NSLOCTEXT("AUTGameMode", "TopAssistsRed", "Most Assists for Red with {0}."));
	HighlightMap.Add(HighlightNames::TopAssistsBlue, NSLOCTEXT("AUTGameMode", "TopAssistsBlue", "Most Assists for Blue with {0}."));
	HighlightMap.Add(HighlightNames::TopFlagReturnsRed, NSLOCTEXT("AUTGameMode", "TopFlagReturnsRed", "Most Flag Returns for Red with {0}."));
	HighlightMap.Add(HighlightNames::TopFlagReturnsBlue, NSLOCTEXT("AUTGameMode", "TopFlagReturnsBlue", "Most Flag Returns for Blue with {0}."));

	HighlightMap.Add(NAME_FCKills, NSLOCTEXT("AUTGameMode", "FCKills", "Killed Enemy Flag Carrier {0} times."));
	HighlightMap.Add(NAME_FlagGrabs, NSLOCTEXT("AUTGameMode", "FlagGrabs", "Grabbed Flag {0} times."));
	HighlightMap.Add(NAME_FlagSupportKills, NSLOCTEXT("AUTGameMode", "FlagSupportKills", "Killed Enemy chasing Flag Carrier ({0})."));
	HighlightMap.Add(HighlightNames::FlagCaptures, NSLOCTEXT("AUTGameMode", "FlagCaptures", "Captured Flag ({0})."));
	HighlightMap.Add(HighlightNames::Assists, NSLOCTEXT("AUTGameMode", "Assists", "Assisted Flag Capture ({0})."));
	HighlightMap.Add(HighlightNames::FlagReturns, NSLOCTEXT("AUTGameMode", "FlagReturns", "Returned Flag ({0})."));

	ShortHighlightMap.Add(HighlightNames::TopFlagCapturesRed, NSLOCTEXT("AUTGameMode", "ShortTopFlagCapturesRed", "Most Flag Caps for Red"));
	ShortHighlightMap.Add(HighlightNames::TopFlagCapturesBlue, NSLOCTEXT("AUTGameMode", "ShortTopFlagCapturesBlue", "Most Flag Caps for Blue"));
	ShortHighlightMap.Add(HighlightNames::TopAssistsRed, NSLOCTEXT("AUTGameMode", "ShortTopAssistsRed", "Most Assists for Red"));
	ShortHighlightMap.Add(HighlightNames::TopAssistsBlue, NSLOCTEXT("AUTGameMode", "ShortTopAssistsBlue", "Most Assists for Blue"));
	ShortHighlightMap.Add(HighlightNames::TopFlagReturnsRed, NSLOCTEXT("AUTGameMode", "ShortTopFlagReturnsRed", "Most Flag Returns for Red"));
	ShortHighlightMap.Add(HighlightNames::TopFlagReturnsBlue, NSLOCTEXT("AUTGameMode", "ShortTopFlagReturnsBlue", "Most Flag Returns for Blue"));

	ShortHighlightMap.Add(NAME_FCKills, NSLOCTEXT("AUTGameMode", "ShortFCKills", "{0} Enemy Flag Carrier Kills"));
	ShortHighlightMap.Add(NAME_FlagGrabs, NSLOCTEXT("AUTGameMode", "ShortFlagGrabs", "{0} Flag Grabs"));
	ShortHighlightMap.Add(NAME_FlagSupportKills, NSLOCTEXT("AUTGameMode", "ShortFlagSupportKills", "Killed Enemy chasing Flag Carrier"));
	ShortHighlightMap.Add(HighlightNames::FlagCaptures, NSLOCTEXT("AUTGameMode", "ShortFlagCaptures", "{0} Flag Captures"));
	ShortHighlightMap.Add(HighlightNames::Assists, NSLOCTEXT("AUTGameMode", "ShortAssists", "{0} Assists"));
	ShortHighlightMap.Add(HighlightNames::FlagReturns, NSLOCTEXT("AUTGameMode", "ShortFlagReturns", "{0} Flag Returns"));

	HighlightPriority.Add(HighlightNames::TopFlagCapturesRed, 4.5f);
	HighlightPriority.Add(HighlightNames::TopFlagCapturesBlue, 4.5f);
	HighlightPriority.Add(HighlightNames::TopAssistsRed, 3.5f);
	HighlightPriority.Add(HighlightNames::TopAssistsRed, 3.5f);
	HighlightPriority.Add(HighlightNames::TopFlagReturnsRed, 3.3f);
	HighlightPriority.Add(HighlightNames::TopFlagReturnsBlue, 3.3f);
	HighlightPriority.Add(NAME_FCKills, 3.5f);
	HighlightPriority.Add(NAME_FlagGrabs, 1.5f);
	HighlightPriority.Add(NAME_FlagSupportKills, 2.5f);
	HighlightPriority.Add(HighlightNames::FlagCaptures, 3.5f);
	HighlightPriority.Add(HighlightNames::Assists, 3.f);
	HighlightPriority.Add(HighlightNames::FlagReturns, 1.9f);

	RoundInProgressStatus = NSLOCTEXT("UTCharacter", "CTFRoundDisplay", "Round {RoundNum}");
	FullRoundInProgressStatus = NSLOCTEXT("UTCharacter", "CTFRoundDisplay", "Round {RoundNum} / {NumRounds}");
	IntermissionStatus = NSLOCTEXT("UTCTFGameState", "Intermission", "Intermission");

	bRedToCap = false;
	GoldBonusText = NSLOCTEXT("FlagRun", "GoldBonusText", "\u2605 \u2605 \u2605");
	SilverBonusText = NSLOCTEXT("FlagRun", "SilverBonusText", "\u2605 \u2605");
	GoldBonusTimedText = NSLOCTEXT("FlagRun", "GoldBonusTimeText", "\u2605 \u2605 \u2605 {BonusTime}");
	SilverBonusTimedText = NSLOCTEXT("FlagRun", "SilverBonusTimeText", "\u2605 \u2605 {BonusTime}");
	BronzeBonusTimedText = NSLOCTEXT("FlagRun", "BronzeBonusTimeText", "\u2605 {BonusTime}");
	BronzeBonusText = NSLOCTEXT("FlagRun", "BronzeBonusText", "\u2605");
	BonusLevel = 3;
	FlagRunMessageSwitch = 0;
	FlagRunMessageTeam = nullptr;
	bPlayStatusAnnouncements = true;
	bEnemyRallyPointIdentified = false;
	EarlyEndTime = 0;
	bTeamGame = true;
	GoldBonusThreshold = 120;
	SilverBonusThreshold = 60;
	AttackText = NSLOCTEXT("UUTHUDWidget_TeamGameClock", "AttackingRole", "Rd {RoundNum}: Attacking on");
	DefendText = NSLOCTEXT("UUTHUDWidget_TeamGameClock", "DefendingRole", "Rd {RoundNum}: Defending on");
	TiebreakValue = 0;
	RemainingPickupDelay = 0;
	RampStartTime = 1;

	HighlightMap.Add(HighlightNames::MostKillsTeam, NSLOCTEXT("AUTGameMode", "MostKillsTeam", "Most Kills for Team ({0})"));
	HighlightMap.Add(HighlightNames::BadMF, NSLOCTEXT("AUTGameMode", "MostKillsTeam", "Most Kills for Team ({0})"));
	HighlightMap.Add(HighlightNames::BadAss, NSLOCTEXT("AUTGameMode", "MostKillsTeam", "Most Kills for Team ({0})"));
	HighlightMap.Add(HighlightNames::LikeABoss, NSLOCTEXT("AUTGameMode", "MostKillsTeam", "Most Kills for Team ({0})"));
	HighlightMap.Add(HighlightNames::DeathIncarnate, NSLOCTEXT("AUTGameMode", "MostKills", "Most Kills ({0})"));
	HighlightMap.Add(HighlightNames::ComeAtMeBro, NSLOCTEXT("AUTGameMode", "MostKills", "Most Kills ({0})"));
	HighlightMap.Add(HighlightNames::ThisIsSparta, NSLOCTEXT("AUTGameMode", "MostKills", "Most Kills ({0})"));

	HighlightMap.Add(HighlightNames::NaturalBornKiller, NSLOCTEXT("AUTGameMode", "LotsOfKills", "{0} Kills"));
	HighlightMap.Add(HighlightNames::SpecialForces, NSLOCTEXT("AUTGameMode", "LotsOfKills", "{0} Kills"));
	HighlightMap.Add(HighlightNames::HiredGun, NSLOCTEXT("AUTGameMode", "LotsOfKills", "{0} Kills"));
	HighlightMap.Add(HighlightNames::HappyToBeHere, NSLOCTEXT("AUTGameMode", "LotsOfKills", "{0} Kills"));
	HighlightMap.Add(HighlightNames::BobLife, NSLOCTEXT("AUTGameMode", "LotsOfKills", "{0} Kills"));
	HighlightMap.Add(HighlightNames::GameOver, NSLOCTEXT("AUTGameMode", "LotsOfKills", "{0} Kills"));
	HighlightMap.Add(HighlightNames::CoolBeans, NSLOCTEXT("AUTGameMode", "LotsOfKills", "{0} Kills"));
	HighlightMap.Add(HighlightNames::NotSureIfSerious, NSLOCTEXT("AUTGameMode", "LotsOfKills", "{0} Kills"));
	HighlightMap.Add(HighlightNames::AllOutOfBubbleGum, NSLOCTEXT("AUTGameMode", "LotsOfKills", "{0} Kills"));
	HighlightMap.Add(HighlightNames::MoreThanAHandful, NSLOCTEXT("AUTGameMode", "LotsOfKills", "{0} Kills"));
	HighlightMap.Add(HighlightNames::ToughGuy, NSLOCTEXT("AUTGameMode", "LotsOfKills", "{0} Kills"));
	HighlightMap.Add(HighlightNames::LargerThanLife, NSLOCTEXT("AUTGameMode", "LotsOfKills", "{0} Kills"));
	HighlightMap.Add(HighlightNames::AssKicker, NSLOCTEXT("AUTGameMode", "LotsOfKills", "{0} Kills"));
	HighlightMap.Add(HighlightNames::Destroyer, NSLOCTEXT("AUTGameMode", "LotsOfKills", "{0} Kills"));
	HighlightMap.Add(HighlightNames::LockedAndLoaded, NSLOCTEXT("AUTGameMode", "LotsOfKills", "{0} Kills"));

	HighlightMap.Add(HighlightNames::RedeemerRejection, NSLOCTEXT("AUTGameMode", "RedeemerRejection", "Rejected Redeemer"));
	HighlightMap.Add(HighlightNames::FlagDenials, NSLOCTEXT("AUTGameMode", "FlagDenials", "{0} Denials"));
	HighlightMap.Add(HighlightNames::WeaponKills, NSLOCTEXT("AUTGameMode", "WeaponKills", "{0} kills with {1}"));
	HighlightMap.Add(HighlightNames::MostKillingBlowsAward, NSLOCTEXT("AUTGameMode", "MostKillingBlowsAward", "Most killing blows ({0})"));
	HighlightMap.Add(HighlightNames::CoupDeGrace, NSLOCTEXT("AUTGameMode", "MostKillingBlowsAward", "Most killing blows ({0})"));
	HighlightMap.Add(HighlightNames::HardToKill, NSLOCTEXT("AUTGameMode", "HardToKill", "Only died {0} times"));
	HighlightMap.Add(HighlightNames::Rallies, NSLOCTEXT("AUTGameMode", "Rallies", "{0} Rallies"));
	HighlightMap.Add(HighlightNames::RallyPointPowered, NSLOCTEXT("AUTGameMode", "RallyPointPowered", "{0} RallyPoints Powered"));
	HighlightMap.Add(HighlightNames::HatTrick, NSLOCTEXT("AUTGameMode", "HatTrick", "3 Flag Caps"));
	HighlightMap.Add(HighlightNames::LikeTheWind, NSLOCTEXT("AUTGameMode", "LikeTheWind", "\u2605 \u2605 \u2605 Cap"));
	HighlightMap.Add(HighlightNames::DeliveryBoy, NSLOCTEXT("AUTGameMode", "DeliveryBoy", "Capped the Flag"));

	ShortHighlightMap.Add(HighlightNames::BadMF, NSLOCTEXT("AUTGameMode", "BadMF", "Bad to the Bone"));
	ShortHighlightMap.Add(HighlightNames::BadAss, NSLOCTEXT("AUTGameMode", "BadAss", "Superior Genetics"));
	ShortHighlightMap.Add(HighlightNames::LikeABoss, NSLOCTEXT("AUTGameMode", "LikeABoss", "Like a Boss"));
	ShortHighlightMap.Add(HighlightNames::DeathIncarnate, NSLOCTEXT("AUTGameMode", "DeathIncarnate", "Death Incarnate"));
	ShortHighlightMap.Add(HighlightNames::NaturalBornKiller, NSLOCTEXT("AUTGameMode", "NaturalBornKiller", "Natural Born Killer"));
	ShortHighlightMap.Add(HighlightNames::SpecialForces, NSLOCTEXT("AUTGameMode", "SpecialForces", "Honey Badger"));
	ShortHighlightMap.Add(HighlightNames::HiredGun, NSLOCTEXT("AUTGameMode", "HiredGun", "Hired Gun"));
	ShortHighlightMap.Add(HighlightNames::HappyToBeHere, NSLOCTEXT("AUTGameMode", "HappyToBeHere", "Just Happy To Be Here"));
	ShortHighlightMap.Add(HighlightNames::MostKillsTeam, NSLOCTEXT("AUTGameMode", "ShortMostKills", "Top Gun"));
	ShortHighlightMap.Add(HighlightNames::BobLife, NSLOCTEXT("AUTGameMode", "BobLife", "Living the Bob Life"));
	ShortHighlightMap.Add(HighlightNames::GameOver, NSLOCTEXT("AUTGameMode", "GameOver", "Game Over, Man"));
	ShortHighlightMap.Add(HighlightNames::CoolBeans, NSLOCTEXT("AUTGameMode", "CoolBeans", "Cool Beans Yo"));
	ShortHighlightMap.Add(HighlightNames::NotSureIfSerious, NSLOCTEXT("AUTGameMode", "NotSureIfSerious", "Not Sure If Serious"));
	ShortHighlightMap.Add(HighlightNames::ComeAtMeBro, NSLOCTEXT("AUTGameMode", "ComeAtMeBro", "Come at Me Bro"));
	ShortHighlightMap.Add(HighlightNames::ThisIsSparta, NSLOCTEXT("AUTGameMode", "ThisIsSparta", "This is Sparta!"));
	ShortHighlightMap.Add(HighlightNames::AllOutOfBubbleGum, NSLOCTEXT("AUTGameMode", "AllOutOfBubbleGum", "All Out of Bubblegum"));
	ShortHighlightMap.Add(HighlightNames::MoreThanAHandful, NSLOCTEXT("AUTGameMode", "MoreThanAHandful", "More Than A Handful"));
	ShortHighlightMap.Add(HighlightNames::ToughGuy, NSLOCTEXT("AUTGameMode", "ToughGuy", "Tough Guy"));
	ShortHighlightMap.Add(HighlightNames::LargerThanLife, NSLOCTEXT("AUTGameMode", "LargerThanLife", "Larger Than Life"));
	ShortHighlightMap.Add(HighlightNames::AssKicker, NSLOCTEXT("AUTGameMode", "AssKicker", "Ass Kicker"));
	ShortHighlightMap.Add(HighlightNames::Destroyer, NSLOCTEXT("AUTGameMode", "Destroyer", "Destroyer"));
	ShortHighlightMap.Add(HighlightNames::LockedAndLoaded, NSLOCTEXT("AUTGameMode", "LockedAndLoaded", "Locked And Loaded"));

	ShortHighlightMap.Add(HighlightNames::RedeemerRejection, NSLOCTEXT("AUTGameMode", "ShortRejection", "Redeem this"));
	ShortHighlightMap.Add(HighlightNames::FlagDenials, NSLOCTEXT("AUTGameMode", "ShortDenials", "You shall not pass"));
	ShortHighlightMap.Add(HighlightNames::WeaponKills, NSLOCTEXT("AUTGameMode", "ShortWeaponKills", "Weapon Master"));
	ShortHighlightMap.Add(HighlightNames::MostKillingBlowsAward, NSLOCTEXT("AUTGameMode", "ShortMostKillingBlowsAward", "Punisher"));
	ShortHighlightMap.Add(HighlightNames::CoupDeGrace, NSLOCTEXT("AUTGameMode", "CoupDeGrace", "Coup de Grace"));
	ShortHighlightMap.Add(HighlightNames::HardToKill, NSLOCTEXT("AUTGameMode", "ShortHardToKill", "Hard to Kill"));
	ShortHighlightMap.Add(HighlightNames::Rallies, NSLOCTEXT("AUTGameMode", "ShortRallies", "Beam me Up"));
	ShortHighlightMap.Add(HighlightNames::RallyPointPowered, NSLOCTEXT("AUTGameMode", "ShortRallyPointPowered", "Power Source"));
	ShortHighlightMap.Add(HighlightNames::HatTrick, NSLOCTEXT("AUTGameMode", "ShortHatTrick", "Hat Trick"));
	ShortHighlightMap.Add(HighlightNames::LikeTheWind, NSLOCTEXT("AUTGameMode", "ShortLikeTheWind", "Run Like the Wind"));
	ShortHighlightMap.Add(HighlightNames::DeliveryBoy, NSLOCTEXT("AUTGameMode", "ShortDeliveryBoy", "Delivery Boy"));
}

void AUTFlagRunGameState::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld() && GetWorld()->GetAuthGameMode<AUTFlagRunGame>())
	{
		bAllowBoosts = GetWorld()->GetAuthGameMode<AUTFlagRunGame>()->bAllowBoosts;
	}

	UpdateSelectablePowerups();
	AddModeSpecificOverlays();
}

void AUTFlagRunGameState::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUTFlagRunGameState, bIsAtIntermission);
	DOREPLIFETIME(AUTFlagRunGameState, DeliveryPoint);
	DOREPLIFETIME(AUTFlagRunGameState, FlagSpawner);
	DOREPLIFETIME(AUTFlagRunGameState, ScoringPlays);
	DOREPLIFETIME(AUTFlagRunGameState, CTFRound);
	DOREPLIFETIME(AUTFlagRunGameState, NumRounds);
	DOREPLIFETIME(AUTFlagRunGameState, IntermissionTime);
	DOREPLIFETIME(AUTFlagRunGameState, bRedToCap);
	DOREPLIFETIME(AUTFlagRunGameState, BonusLevel);
	DOREPLIFETIME(AUTFlagRunGameState, CurrentRallyPoint);
	DOREPLIFETIME(AUTFlagRunGameState, bEnemyRallyPointIdentified);
	DOREPLIFETIME(AUTFlagRunGameState, RemainingPickupDelay);
	DOREPLIFETIME(AUTFlagRunGameState, RampStartTime);
	DOREPLIFETIME(AUTFlagRunGameState, FlagRunMessageSwitch);
	DOREPLIFETIME(AUTFlagRunGameState, FlagRunMessageTeam);
	DOREPLIFETIME(AUTFlagRunGameState, bAttackersCanRally);
	DOREPLIFETIME(AUTFlagRunGameState, EarlyEndTime);
	DOREPLIFETIME(AUTFlagRunGameState, TiebreakValue);
	DOREPLIFETIME(AUTFlagRunGameState, bAllowBoosts);
	DOREPLIFETIME(AUTFlagRunGameState, OffenseSelectablePowerups);
	DOREPLIFETIME(AUTFlagRunGameState, DefenseSelectablePowerups);
}

void AUTFlagRunGameState::ManageMusicVolume(float DeltaTime)
{
	AUTWorldSettings* Settings = Cast<AUTWorldSettings>(GetWorldSettings());
	if (Settings && Settings->MusicComp)
	{
		UUTGameUserSettings* UserSettings = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());
		float DesiredVolume = IsMatchInProgress() && !IsMatchIntermission() && (RemainingPickupDelay < RampStartTime) ? UserSettings->GetSoundClassVolume(EUTSoundClass::GameMusic) : 1.f;
		if (bLocalMenusAreActive) DesiredVolume = 1.0f;
		MusicVolume = MusicVolume * (1.f - 0.5f*DeltaTime) + 0.5f*DeltaTime*DesiredVolume;
		Settings->MusicComp->SetVolumeMultiplier(MusicVolume);
	}
}

void AUTFlagRunGameState::PlayTimeWarningSound()
{
	USoundBase* SoundToPlay = UUTCountDownMessage::StaticClass()->GetDefaultObject<UUTCountDownMessage>()->TimeEndingSound;
	if (SoundToPlay != NULL)
	{
		for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
		{
			AUTPlayerController* PC = Cast<AUTPlayerController>(It->PlayerController);
			if (PC && PC->IsLocalPlayerController())
			{
				PC->UTClientPlaySound(SoundToPlay);
			}
		}
	}
}

void AUTFlagRunGameState::UpdateTimeMessage()
{
	if (!bIsAtIntermission && (GetNetMode() != NM_DedicatedServer) && IsMatchInProgress())
	{
		// bonus time countdowns
		// add first stinger, fix delay for playing bonus level change
		if (RemainingTime <= GoldBonusThreshold + 9)
		{
			if (RemainingTime > GoldBonusThreshold)
			{
				if ((RemainingTime <= GoldBonusThreshold + 5) || (RemainingTime == GoldBonusThreshold + 9))
				{
					for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
					{
						AUTPlayerController* PC = Cast<AUTPlayerController>(It->PlayerController);
						if (PC != NULL)
						{
							PC->ClientReceiveLocalizedMessage(UUTCountDownMessage::StaticClass(), 4000 + RemainingTime - GoldBonusThreshold);
						}
					}
					if (RemainingTime == GoldBonusThreshold + 1)
					{
						FTimerHandle TempHandle;
						GetWorldTimerManager().SetTimer(TempHandle, this, &AUTFlagRunGameState::PlayTimeWarningSound, 0.9f, false);
					}
				}
			}
			else if ((RemainingTime > SilverBonusThreshold) && ((RemainingTime <= SilverBonusThreshold + 5) || (RemainingTime == SilverBonusThreshold + 9)))
			{
				for (FLocalPlayerIterator It(GEngine, GetWorld()); It; ++It)
				{
					AUTPlayerController* PC = Cast<AUTPlayerController>(It->PlayerController);
					if (PC != NULL)
					{
						PC->ClientReceiveLocalizedMessage(UUTCountDownMessage::StaticClass(), 3000 + RemainingTime - SilverBonusThreshold);
					}
				}
				if (RemainingTime == SilverBonusThreshold + 1)
				{
					FTimerHandle TempHandle;
					GetWorldTimerManager().SetTimer(TempHandle, this, &AUTFlagRunGameState::PlayTimeWarningSound, 0.9f, false);
				}
			}
		}
	}
}

FLinearColor AUTFlagRunGameState::GetGameStatusColor()
{
	if (CTFRound > 0)
	{
		switch(BonusLevel)
		{
			case 1: return BRONZECOLOR; break;
			case 2: return SILVERCOLOR; break;
			case 3: return GOLDCOLOR; break;
		}
	}
	return FLinearColor::White;
}

FText AUTFlagRunGameState::GetRoundStatusText(bool bForScoreboard)
{
	if (bForScoreboard)
	{
		FFormatNamedArguments Args;
		Args.Add("RoundNum", FText::AsNumber(CTFRound));
		Args.Add("NumRounds", FText::AsNumber(NumRounds));
		return (NumRounds > 0) ? FText::Format(FullRoundInProgressStatus, Args) : FText::Format(RoundInProgressStatus, Args);
	}
	else
	{
		FText StatusText = BronzeBonusTimedText;
		int32 RemainingBonus = FMath::Clamp(RemainingTime, 0, 59);
		if (BonusLevel == 3)
		{
			RemainingBonus = FMath::Clamp(RemainingTime - GoldBonusThreshold, 0, 60);
			StatusText = GoldBonusTimedText;
			FFormatNamedArguments Args;
			Args.Add("BonusTime", FText::AsNumber(RemainingBonus));
			return FText::Format(GoldBonusTimedText, Args);
		}
		else if (BonusLevel == 2)
		{
			RemainingBonus = FMath::Clamp(RemainingTime - SilverBonusThreshold, 0, 59);
			StatusText = SilverBonusTimedText;
		}
		FFormatNamedArguments Args;
		Args.Add("BonusTime", FText::AsNumber(RemainingBonus));
		return FText::Format(StatusText, Args);
	}
}

void AUTFlagRunGameState::UpdateSelectablePowerups()
{
	if (!bAllowBoosts)
	{
		OffenseSelectablePowerups.Empty();
		DefenseSelectablePowerups.Empty();
		return;
	}
	const int32 RedTeamIndex = 0;
	const int32 BlueTeamIndex = 1;
	const bool bIsRedTeamOffense = IsTeamOnDefenseNextRound(RedTeamIndex);

	TSubclassOf<UUTPowerupSelectorUserWidget> OffensePowerupSelectorWidget;
	TSubclassOf<UUTPowerupSelectorUserWidget> DefensePowerupSelectorWidget;

	if (bIsRedTeamOffense)
	{
		OffensePowerupSelectorWidget = LoadClass<UUTPowerupSelectorUserWidget>(NULL, *GetPowerupSelectWidgetPath(RedTeamIndex), NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
		DefensePowerupSelectorWidget = LoadClass<UUTPowerupSelectorUserWidget>(NULL, *GetPowerupSelectWidgetPath(BlueTeamIndex), NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
	}
	else
	{
		OffensePowerupSelectorWidget = LoadClass<UUTPowerupSelectorUserWidget>(NULL, *GetPowerupSelectWidgetPath(BlueTeamIndex), NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
		DefensePowerupSelectorWidget = LoadClass<UUTPowerupSelectorUserWidget>(NULL, *GetPowerupSelectWidgetPath(RedTeamIndex), NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
	}

	if (OffensePowerupSelectorWidget)
	{
		for (TSubclassOf<class AUTInventory> BoostItem : OffensePowerupSelectorWidget.GetDefaultObject()->SelectablePowerups)
		{
			OffenseSelectablePowerups.Add(BoostItem);
		}
	}

	if (DefensePowerupSelectorWidget)
	{
		for (TSubclassOf<class AUTInventory> BoostItem : DefensePowerupSelectorWidget.GetDefaultObject()->SelectablePowerups)
		{
			DefenseSelectablePowerups.Add(BoostItem);
		}
	}
}

void AUTFlagRunGameState::SetSelectablePowerups(const TArray<TSubclassOf<AUTInventory>>& OffenseList, const TArray<TSubclassOf<AUTInventory>>& DefenseList)
{
	OffenseSelectablePowerups = OffenseList;
	DefenseSelectablePowerups = DefenseList;
	for (int32 i = 0; i < PlayerArray.Num(); i++)
	{
		AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerArray[i]);
		if (PS != nullptr && PS->BoostClass != nullptr && !OffenseList.Contains(PS->BoostClass) && !DefenseList.Contains(PS->BoostClass))
		{
			PS->ServerSetBoostItem(0);
		}
	}
}

FString AUTFlagRunGameState::GetPowerupSelectWidgetPath(int32 TeamNumber)
{
	if (!bAllowBoosts)
	{
		if (IsTeamOnDefenseNextRound(TeamNumber))
		{
			return TEXT("/Game/RestrictedAssets/Blueprints/BP_PowerupSelector_EmptyDefense.BP_PowerupSelector_EmptyDefense_C");
		}
		else
		{
			return TEXT("/Game/RestrictedAssets/Blueprints/BP_PowerupSelector_EmptyOffense.BP_PowerupSelector_EmptyOffense_C");
		}
	}
	else
	{
		if (IsTeamOnDefenseNextRound(TeamNumber))
		{
			return TEXT("/Game/RestrictedAssets/Blueprints/BP_PowerupSelector_Defense.BP_PowerupSelector_Defense_C");
		}
		else
		{
			return TEXT("/Game/RestrictedAssets/Blueprints/BP_PowerupSelector_Offense.BP_PowerupSelector_Offense_C");
		}
	}
}

void AUTFlagRunGameState::AddModeSpecificOverlays()
{
	for (TSubclassOf<class AUTInventory> BoostClass : OffenseSelectablePowerups)
	{
		BoostClass.GetDefaultObject()->AddOverlayMaterials(this);
	}

	for (TSubclassOf<class AUTInventory> BoostClass : DefenseSelectablePowerups)
	{
		BoostClass.GetDefaultObject()->AddOverlayMaterials(this);
	}
}

TSubclassOf<class AUTInventory> AUTFlagRunGameState::GetSelectableBoostByIndex(AUTPlayerState* PlayerState, int Index) const
{
	if (PlayerState != nullptr && (IsMatchInProgress() ? IsTeamOnDefense(PlayerState->GetTeamNum()) : IsTeamOnDefenseNextRound(PlayerState->GetTeamNum())))
	{
		if ((DefenseSelectablePowerups.Num() > 0) && (Index < DefenseSelectablePowerups.Num()))
		{
			return DefenseSelectablePowerups[Index];
		}
	}
	else
	{
		if ((OffenseSelectablePowerups.Num() > 0) && (Index < OffenseSelectablePowerups.Num()))
		{
			return OffenseSelectablePowerups[Index];
		}
	}

	return nullptr;
}

bool AUTFlagRunGameState::IsSelectedBoostValid(AUTPlayerState* PlayerState) const
{
	if (PlayerState == nullptr || PlayerState->BoostClass == nullptr)
	{
		return false;
	}

	return IsTeamOnDefenseNextRound(PlayerState->GetTeamNum()) ? DefenseSelectablePowerups.Contains(PlayerState->BoostClass) : OffenseSelectablePowerups.Contains(PlayerState->BoostClass);
}

void AUTFlagRunGameState::PrecacheAllPowerupAnnouncements(class UUTAnnouncer* Announcer) const
{
	for (TSubclassOf<class AUTInventory> PowerupClass : DefenseSelectablePowerups)
	{
		CachePowerupAnnouncement(Announcer, PowerupClass);
	}

	for (TSubclassOf<class AUTInventory> PowerupClass : OffenseSelectablePowerups)
	{
		CachePowerupAnnouncement(Announcer, PowerupClass);
	}
}

void AUTFlagRunGameState::CachePowerupAnnouncement(class UUTAnnouncer* Announcer, const TSubclassOf<AUTInventory> PowerupClass) const
{
	AUTInventory* Powerup = PowerupClass->GetDefaultObject<AUTInventory>();
	if (Powerup)
	{
		Announcer->PrecacheAnnouncement(Powerup->AnnouncementName);
	}
}

AUTBlitzFlag* AUTFlagRunGameState::GetOffenseFlag()
{
	return FlagSpawner ? Cast<AUTBlitzFlag>(FlagSpawner->MyFlag) : nullptr;
}

void AUTFlagRunGameState::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (Role == ROLE_Authority)
	{
		if (FlagSpawner)
		{
			AUTBlitzFlag* Flag = Cast<AUTBlitzFlag>(FlagSpawner->GetCarriedObject());
			bAttackersCanRally = (CurrentRallyPoint != nullptr) && (CurrentRallyPoint->RallyPointState == RallyPointStates::Powered);
			AUTGameVolume* GV = Flag && Flag->HoldingPawn && Flag->HoldingPawn->UTCharacterMovement ? Cast<AUTGameVolume>(Flag->HoldingPawn->UTCharacterMovement->GetPhysicsVolume()) : nullptr;
			bool bInFlagRoom = GV && (GV->bIsDefenderBase || GV->bIsTeamSafeVolume);
			bHaveEstablishedFlagRunner = (!bInFlagRoom && Flag && Flag->Holder && Flag->HoldingPawn && (GetWorld()->GetTimeSeconds() - Flag->PickedUpTime > 2.f));
		}
	}
}

bool AUTFlagRunGameState::IsTeamOnOffense(int32 TeamNumber) const
{
	const bool bIsOnRedTeam = (TeamNumber == 0);
	return (bRedToCap == bIsOnRedTeam);
}

bool AUTFlagRunGameState::IsTeamOnDefense(int32 TeamNumber) const
{
	return !IsTeamOnOffense(TeamNumber);
}

bool AUTFlagRunGameState::IsTeamOnDefenseNextRound(int32 TeamNumber) const
{
	//We alternate teams, so if we are on offense now, next round we will be on defense
	return IsTeamOnOffense(TeamNumber);
}

float AUTFlagRunGameState::GetClockTime()
{
	RemainingTime -= EarlyEndTime;
	float ClockTime = 0.f;
	if (IsMatchIntermission())
	{
		ClockTime = IntermissionTime;
	}
	else
	{
		ClockTime = ((TimeLimit > 0.f) || !HasMatchStarted()) ? GetRemainingTime() : ElapsedTime;
	}
	RemainingTime += EarlyEndTime;
	return ClockTime;
}

void AUTFlagRunGameState::CheckTimerMessage()
{
	RemainingTime -= EarlyEndTime;
	Super::CheckTimerMessage();
	RemainingTime += EarlyEndTime;
}

int32 AUTFlagRunGameState::NumHighlightsNeeded()
{
	return HasMatchEnded() ? 4 : 1;
}

// new plan - rank order kills, give pending award.. Early out if good enough, override for lower
void AUTFlagRunGameState::UpdateRoundHighlights()
{
	ClearHighlights();

	bHaveRallyHighlight = false;
	bHaveRallyPoweredHighlight = false;
	HappyCount = 0;
	HiredGunCount = 0;

	//Collect all the weapons
	TArray<AUTWeapon *> StatsWeapons;
	if (StatsWeapons.Num() == 0)
	{
		for (FActorIterator It(GetWorld()); It; ++It)
		{
			AUTPickupWeapon* Pickup = Cast<AUTPickupWeapon>(*It);
			if (Pickup && Pickup->GetInventoryType())
			{
				StatsWeapons.AddUnique(Pickup->GetInventoryType()->GetDefaultObject<AUTWeapon>());
			}
		}
	}

	AUTPlayerState* MostKills = NULL;
	AUTPlayerState* MostKillsRed = NULL;
	AUTPlayerState* MostKillsBlue = NULL;
	AUTPlayerState* MostHeadShotsPS = NULL;
	AUTPlayerState* MostAirRoxPS = NULL;
	AUTPlayerState* MostKillingBlowsRed = NULL;
	AUTPlayerState* MostKillingBlowsBlue = NULL;

	for (TActorIterator<AUTPlayerState> It(GetWorld()); It; ++It)
	{
		AUTPlayerState* PS = *It;
		if (PS && !PS->bOnlySpectator)
		{
			int32 TeamIndex = PS->Team ? PS->Team->TeamIndex : 0;
			int32 TotalKills = PS->RoundKills + PS->RoundKillAssists;
			if (TotalKills > (MostKills ? MostKills->RoundKills + MostKills->RoundKillAssists : 0))
			{
				MostKills = PS;
			}
			AUTPlayerState* TopTeamKiller = (TeamIndex == 0) ? MostKillsRed : MostKillsBlue;
			if (TotalKills > (TopTeamKiller ? TopTeamKiller->RoundKills + TopTeamKiller->RoundKillAssists : 0))
			{
				if (TeamIndex == 0)
				{
					MostKillsRed = PS;
				}
				else
				{
					MostKillsBlue = PS;
				}
			}
			AUTPlayerState* TopKillingBlows = (TeamIndex == 0) ? MostKillingBlowsRed : MostKillingBlowsBlue;
			if (PS->RoundKills > (TopKillingBlows ? TopKillingBlows->RoundKills : 0))
			{
				if (TeamIndex == 0)
				{
					MostKillingBlowsRed = PS;
				}
				else
				{
					MostKillingBlowsBlue = PS;
				}
			}

			//Figure out what weapon killed the most
			PS->FavoriteWeapon = nullptr;
			int32 BestKills = 0;
			for (AUTWeapon* Weapon : StatsWeapons)
			{
				int32 Kills = Weapon->GetWeaponKillStatsForRound(PS);
				if (Kills > BestKills)
				{
					BestKills = Kills;
					PS->FavoriteWeapon = Weapon->GetClass();
				}
			}

			if (PS->GetRoundStatsValue(NAME_SniperHeadshotKills) > (MostHeadShotsPS ? MostHeadShotsPS->GetRoundStatsValue(NAME_SniperHeadshotKills) : 1.f))
			{
				MostHeadShotsPS = PS;
			}
			if (PS->GetRoundStatsValue(NAME_AirRox) > (MostAirRoxPS ? MostAirRoxPS->GetRoundStatsValue(NAME_AirRox) : 1.f))
			{
				MostAirRoxPS = PS;
			}
		}
	}

	for (TActorIterator<AUTPlayerState> It(GetWorld()); It; ++It)
	{
		AUTPlayerState* PS = *It;
		if (PS && !PS->bOnlySpectator)
		{
			int32 TotalKills = PS->RoundKills + PS->RoundKillAssists;
			if (MostKills && (TotalKills >= 20) && (TotalKills >= MostKills->RoundKills + MostKills->RoundKillAssists))
			{
				PS->AddMatchHighlight(HighlightNames::DeathIncarnate, TotalKills);
			}
			else if (PS->Team)
			{
				if ((PS->FlagCaptures == 3) && ((PS->Team->TeamIndex == 0) == bRedToCap))
				{
					PS->AddMatchHighlight(HighlightNames::HatTrick, 3);
				}
				else
				{
					AUTPlayerState* TopTeamKiller = (PS->Team->TeamIndex == 0) ? MostKillsRed : MostKillsBlue;
					if (TopTeamKiller && (TotalKills >= TopTeamKiller->RoundKills + TopTeamKiller->RoundKillAssists))
					{
						if (TotalKills >= 15)
						{
							PS->AddMatchHighlight((PS == TopTeamKiller) ? HighlightNames::BadMF : HighlightNames::BadAss, TotalKills);
						}
						else if (TotalKills >= 10)
						{
							PS->AddMatchHighlight((PS == TopTeamKiller) ? HighlightNames::LikeABoss : HighlightNames::ThisIsSparta, TotalKills);
						}
						else
						{
							PS->AddMatchHighlight((PS == TopTeamKiller) ? HighlightNames::MostKillsTeam : HighlightNames::ComeAtMeBro, TotalKills);
						}
					}
					else
					{
						AUTPlayerState* TopKillingBlows = (PS->Team->TeamIndex == 0) ? MostKillingBlowsRed : MostKillingBlowsBlue;
						if (TopKillingBlows && (PS->RoundKills >= FMath::Max(2, TopKillingBlows->RoundKills)))
						{
							PS->AddMatchHighlight((PS == TopKillingBlows) ? HighlightNames::MostKillingBlowsAward : HighlightNames::CoupDeGrace, PS->RoundKills);
						}
					}
				}
			}
			if (PS->MatchHighlights[0] == NAME_None)
			{
				if (PS->GetRoundStatsValue(NAME_FlagDenials) > 1)
				{
					PS->AddMatchHighlight(HighlightNames::FlagDenials, PS->GetRoundStatsValue(NAME_FlagDenials));
				}
				else if (PS->GetRoundStatsValue(NAME_RedeemerRejected) > 0)
				{
					PS->AddMatchHighlight(HighlightNames::RedeemerRejection, PS->GetRoundStatsValue(NAME_RedeemerRejected));
				}
			}
		}
	}

	for (TActorIterator<AUTPlayerState> It(GetWorld()); It; ++It)
	{
		AUTPlayerState* PS = *It;
		if (PS && !PS->bOnlySpectator && PS->MatchHighlights[0] == NAME_None)
		{
			if (MostHeadShotsPS && (PS->GetRoundStatsValue(NAME_SniperHeadshotKills) == MostHeadShotsPS->GetRoundStatsValue(NAME_SniperHeadshotKills)))
			{
				PS->AddMatchHighlight(HighlightNames::MostHeadShots, MostHeadShotsPS->GetRoundStatsValue(NAME_SniperHeadshotKills));
			}
			else if (MostAirRoxPS && (PS->GetRoundStatsValue(NAME_AirRox) == MostAirRoxPS->GetRoundStatsValue(NAME_AirRox)))
			{
				PS->AddMatchHighlight(HighlightNames::MostAirRockets, MostAirRoxPS->GetRoundStatsValue(NAME_AirRox));
			}
		}
	}

	for (TActorIterator<AUTPlayerState> It(GetWorld()); It; ++It)
	{
		AUTPlayerState* PS = *It;
		if (PS && !PS->bOnlySpectator && PS->MatchHighlights[0] == NAME_None)
		{
			// only add low priority highlights if not enough high priority highlights
			AddMinorRoundHighlights(PS);
		}
	}
}

void AUTFlagRunGameState::AddMinorRoundHighlights(AUTPlayerState* PS)
{
	if (PS->MatchHighlights[0] != NAME_None)
	{
		return;
	}

	// sprees and multikills
	FName SpreeStatsNames[5] = { NAME_SpreeKillLevel4, NAME_SpreeKillLevel3, NAME_SpreeKillLevel2, NAME_SpreeKillLevel1, NAME_SpreeKillLevel0 };
	for (int32 i = 0; i < 5; i++)
	{
		if (PS->GetRoundStatsValue(SpreeStatsNames[i]) > 0)
		{
			PS->AddMatchHighlight(SpreeStatsNames[i], PS->GetRoundStatsValue(SpreeStatsNames[i]));
			return;
		}
	}

	if (PS->RoundKills + PS->RoundKillAssists >= 15)
	{
		FName KillerNames[2] = { HighlightNames::NaturalBornKiller, HighlightNames::LargerThanLife };
		PS->MatchHighlights[0] = KillerNames[NaturalKillerCount % 2];
		NaturalKillerCount++;
		PS->MatchHighlightData[0] = PS->RoundKills + PS->RoundKillAssists;
		return;
	}

	FName MultiKillsNames[4] = { NAME_MultiKillLevel3, NAME_MultiKillLevel2, NAME_MultiKillLevel1, NAME_MultiKillLevel0 };
	for (int32 i = 0; i < 2; i++)
	{
		if (PS->GetRoundStatsValue(MultiKillsNames[i]) > 0)
		{
			PS->AddMatchHighlight(MultiKillsNames[i], PS->GetRoundStatsValue(MultiKillsNames[i]));
			return;
		}
	}
	if (PS->RoundKills + PS->RoundKillAssists >= 13)
	{
		PS->MatchHighlights[0] = HighlightNames::AssKicker;
		PS->MatchHighlightData[0] = PS->RoundKills + PS->RoundKillAssists;
		return;
	}
	if (PS->RoundKills + PS->RoundKillAssists >= 10)
	{
		FName DestroyerNames[2] = { HighlightNames::SpecialForces, HighlightNames::Destroyer };
		PS->MatchHighlights[0] = DestroyerNames[DestroyerCount % 2];
		DestroyerCount++;
		PS->MatchHighlightData[0] = PS->RoundKills + PS->RoundKillAssists;
		return;
	}

	// announced kills
	FName AnnouncedKills[5] = { NAME_AmazingCombos, NAME_AirRox, NAME_AirSnot, NAME_SniperHeadshotKills, NAME_FlakShreds };
	for (int32 i = 0; i < 5; i++)
	{
		if (PS->GetRoundStatsValue(AnnouncedKills[i]) > 1)
		{
			PS->AddMatchHighlight(AnnouncedKills[i], PS->GetRoundStatsValue(AnnouncedKills[i]));
			return;
		}
	}

	// Most kills with favorite weapon, if needed
	bool bHasMultipleKillWeapon = false;
	if (PS->FavoriteWeapon)
	{
		AUTWeapon* DefaultWeapon = PS->FavoriteWeapon->GetDefaultObject<AUTWeapon>();
		int32 WeaponKills = DefaultWeapon->GetWeaponKillStatsForRound(PS);
		if (WeaponKills > 1)
		{
			bHasMultipleKillWeapon = true;
			bool bIsBestOverall = true;
			for (int32 i = 0; i < PlayerArray.Num(); i++)
			{
				AUTPlayerState* OtherPS = Cast<AUTPlayerState>(PlayerArray[i]);
				if (OtherPS && (PS != OtherPS) && (DefaultWeapon->GetWeaponKillStatsForRound(OtherPS) > WeaponKills))
				{
					bIsBestOverall = false;
					break;
				}
			}
			if (bIsBestOverall)
			{
				PS->AddMatchHighlight(HighlightNames::MostWeaponKills, WeaponKills);
				return;
			}
		}
	}

	int32 NumRallies = PS->GetRoundStatsValue(NAME_Rallies);
	int32 NumRalliesPowered = PS->GetRoundStatsValue(NAME_RalliesPowered);

	if (bHasMultipleKillWeapon)
	{
		AUTWeapon* DefaultWeapon = PS->FavoriteWeapon->GetDefaultObject<AUTWeapon>();
		int32 WeaponKills = DefaultWeapon->GetWeaponKillStatsForRound(PS);
		PS->AddMatchHighlight(HighlightNames::WeaponKills, WeaponKills);
	}
	else if (PS->GetRoundStatsValue(NAME_FCKills) > 1)
	{
		PS->AddMatchHighlight(NAME_FCKills, PS->GetRoundStatsValue(NAME_FCKills));
	}
	else if (PS->RoundKills >= FMath::Max(3, 2 * PS->RoundDeaths))
	{
		PS->AddMatchHighlight(HighlightNames::HardToKill, PS->RoundDeaths);
	}
	else if (!bHaveRallyPoweredHighlight && (NumRalliesPowered > 1))
	{
		PS->AddMatchHighlight(HighlightNames::RallyPointPowered, NumRalliesPowered);
		bHaveRallyPoweredHighlight = true;
	}
	else if (!bHaveRallyHighlight && (NumRallies > 3))
	{
		PS->AddMatchHighlight(HighlightNames::Rallies, NumRallies);
		bHaveRallyHighlight = true;
	}
	else if (PS->Team && (PS->CarriedObject != nullptr) && (PS->Team->RoundBonus > GoldBonusThreshold))
	{
		PS->AddMatchHighlight(HighlightNames::LikeTheWind, 0);
	}
	else if (PS->RoundKills + PS->RoundKillAssists > 0)
	{
		PS->MatchHighlightData[0] = PS->RoundKills + PS->RoundKillAssists;

		if (PS->MatchHighlightData[0] > 7)
		{
			FName BubbleNames[2] = { HighlightNames::AllOutOfBubbleGum, HighlightNames::ToughGuy };
			PS->MatchHighlights[0] = BubbleNames[BubbleGumCount % 2];
			BubbleGumCount++;
		}
		else if (PS->MatchHighlightData[0] == 6)
		{
			PS->MatchHighlights[0] = HighlightNames::MoreThanAHandful;
		}
		else if (PS->CarriedObject != nullptr)
		{
			PS->AddMatchHighlight(HighlightNames::DeliveryBoy, 0);
		}
		else if (PS->MatchHighlightData[0] > 4)
		{
			FName HappyNames[2] = { HighlightNames::BobLife, HighlightNames::GameOver };
			PS->MatchHighlights[0] = HappyNames[BobLifeCount % 2];
			BobLifeCount++;
		}
		else if (PS->RoundKills > 1)
		{
			PS->MatchHighlights[0] = HighlightNames::KillingBlowsAward;
			PS->MatchHighlightData[0] = PS->RoundKills;
		}
		else if (PS->MatchHighlightData[0] > 2)
		{
			FName HappyNames[3] = { HighlightNames::HiredGun, HighlightNames::CoolBeans, HighlightNames::LockedAndLoaded };
			PS->MatchHighlights[0] = HappyNames[HiredGunCount % 3];
			HiredGunCount++;
		}
		else if (PS->RoundDamageDone > 150)
		{
			PS->MatchHighlights[0] = HighlightNames::DamageAward;
			PS->MatchHighlightData[0] = PS->RoundDamageDone;
		}
		else
		{
			FName HappyNames[2] = { HighlightNames::HappyToBeHere, HighlightNames::NotSureIfSerious };
			PS->MatchHighlights[0] = HappyNames[HappyCount % 2];
			HappyCount++;
		}
	}
	else if (PS->CarriedObject != nullptr)
	{
		PS->AddMatchHighlight(HighlightNames::DeliveryBoy, 0);
	}
	else
	{
		PS->MatchHighlights[0] = HighlightNames::ParticipationAward;
	}
}

FText AUTFlagRunGameState::OverrideRoleText(AUTPlayerState* PS)
{
	if (PS && (CTFRound > 0))
	{
		// Change role text to include round and role (attacking/defending)
		FFormatNamedArguments Args;
		Args.Add("RoundNum", FText::AsNumber(CTFRound));
		FText RoleText = IsTeamOnOffense(PS->Team->TeamIndex) ? AttackText : DefendText;
		return FText::Format(RoleText, Args);
	}
	return FText::GetEmpty();
}

/** Returns true if P1 should be sorted before P2.  */
bool AUTFlagRunGameState::InOrder(AUTPlayerState* P1, AUTPlayerState* P2)
{
	// spectators are sorted last
	if (P1->bOnlySpectator)
	{
		return P2->bOnlySpectator;
	}
	else if (P2->bOnlySpectator)
	{
		return true;
	}

	// sort by Score
	if (P1->Score < P2->Score)
	{
		return false;
	}
	if (P1->Score == P2->Score)
	{
		// if score tied, use roundkills then remaininglives to sort
		if (P1->RoundKills < P2->RoundKills)
		{
			return false;
		}
		if (P1->RoundKills > P2->RoundKills)
		{
			return true;
		}

		if (P1->RemainingLives < P2->RemainingLives)
		{
			return false;
		}
		if (P1->RemainingLives > P2->RemainingLives)
		{
			return true;
		}
		// keep local player highest on list
		if (Cast<APlayerController>(P2->GetOwner()) != NULL)
		{
			ULocalPlayer* LP2 = Cast<ULocalPlayer>(Cast<APlayerController>(P2->GetOwner())->Player);
			if (LP2 != NULL)
			{
				// make sure ordering is consistent for splitscreen players
				ULocalPlayer* LP1 = Cast<ULocalPlayer>(Cast<APlayerController>(P2->GetOwner())->Player);
				return (LP1 != NULL);
			}
		}
	}
	return true;
}

FText AUTFlagRunGameState::GetGameStatusText(bool bForScoreboard)
{
	if (HasMatchEnded())
	{
		return GameOverStatus;
	}
	else if (GetMatchState() == MatchState::MapVoteHappening)
	{
		return MapVoteStatus;
	}
	else if ((CTFRound > 0) && IsMatchInProgress())
	{
		return GetRoundStatusText(bForScoreboard);
	}
	else if (IsMatchIntermission())
	{
		return IntermissionStatus;
	}

	return AUTGameState::GetGameStatusText(bForScoreboard);
}

void AUTFlagRunGameState::CacheGameObjective(AUTGameObjective* BaseToCache)
{
	if (Cast<AUTBlitzDeliveryPoint>(BaseToCache))
	{
		DeliveryPoint = Cast<AUTBlitzDeliveryPoint>(BaseToCache);
	}
	else if (Cast<AUTBlitzFlagSpawner>(BaseToCache))
	{
		FlagSpawner = Cast<AUTBlitzFlagSpawner>(BaseToCache);
	}
}

AUTPlayerState* AUTFlagRunGameState::GetFlagHolder()
{
	return FlagSpawner ? FlagSpawner->GetCarriedObjectHolder() : nullptr;
}

AUTGameObjective* AUTFlagRunGameState::GetFlagBase(uint8 TeamNum)
{
	if (DeliveryPoint && (DeliveryPoint->TeamNum == TeamNum))
	{
		return DeliveryPoint;
	}
	if (FlagSpawner && (FlagSpawner->TeamNum == TeamNum))
	{
		return FlagSpawner;
	}
	return nullptr;
}

void AUTFlagRunGameState::ResetFlags()
{
	if (DeliveryPoint)
	{
		DeliveryPoint->RecallFlag();
	}
}

bool AUTFlagRunGameState::IsMatchInProgress() const
{
	const FName CurrentMatchState = GetMatchState();
	return (CurrentMatchState == MatchState::InProgress || CurrentMatchState == MatchState::MatchIsInOvertime || CurrentMatchState == MatchState::MatchIntermission || CurrentMatchState == MatchState::MatchExitingIntermission);
}

bool AUTFlagRunGameState::IsMatchIntermission() const
{
	const FName CurrentMatchState = GetMatchState();
	return (CurrentMatchState == MatchState::MatchIntermission) || (CurrentMatchState == MatchState::MatchIntermission || CurrentMatchState == MatchState::MatchExitingIntermission);
}

FName AUTFlagRunGameState::OverrideCameraStyle(APlayerController* PCOwner, FName CurrentCameraStyle)
{
	if (IsLineUpActive())
	{
		return FName(TEXT("LineUpCam"));
	}
	else
	{
		return (IsMatchIntermission() || HasMatchEnded()) ? FName(TEXT("FreeCam")) : Super::OverrideCameraStyle(PCOwner, CurrentCameraStyle);
	}
}

AUTLineUpZone* AUTFlagRunGameState::GetAppropriateSpawnList(LineUpTypes ZoneType)
{
	AUTLineUpZone* FoundPotentialMatch = nullptr;
	AUTGameObjective* ScoringBase = GetLeadTeamFlagBase();

	if (GetWorld())
	{
		for (TActorIterator<AUTLineUpZone> It(GetWorld()); It; ++It)
		{
			if (It->ZoneType == ZoneType)
			{
				//Found perfect match, lets return it!
				if (It->GameObjectiveReference == ScoringBase)
				{
					return *It;
				}

				//imperfect match, but it might be all we have
				else if (It->GameObjectiveReference == nullptr)
				{
					FoundPotentialMatch = *It;
				}
			}
		}
	}

	return (FoundPotentialMatch == nullptr) ? Super::GetAppropriateSpawnList(ZoneType) : FoundPotentialMatch;
}

void AUTFlagRunGameState::DefaultTimer()
{
	Super::DefaultTimer();
	if (bIsAtIntermission)
	{
		IntermissionTime--;
	}
}

float AUTFlagRunGameState::GetIntermissionTime()
{
	return IntermissionTime;
}

void AUTFlagRunGameState::SpawnDefaultLineUpZones()
{
	if (GetAppropriateSpawnList(LineUpTypes::Intro) == nullptr)
	{
		if (DeliveryPoint)
		{
			SpawnLineUpZoneOnFlagBase(DeliveryPoint, LineUpTypes::Intro);
		}
	}

	if (GetAppropriateSpawnList(LineUpTypes::Intermission) == nullptr)
	{
		if (DeliveryPoint)
		{
			SpawnLineUpZoneOnFlagBase(DeliveryPoint, LineUpTypes::Intermission);
		}
	}

	if (GetAppropriateSpawnList(LineUpTypes::PostMatch) == nullptr)
	{
		if (DeliveryPoint)
		{
			SpawnLineUpZoneOnFlagBase(DeliveryPoint, LineUpTypes::PostMatch);
		}
	}

	Super::SpawnDefaultLineUpZones();
}

void AUTFlagRunGameState::SpawnLineUpZoneOnFlagBase(AUTGameObjective* BaseToSpawnOn, LineUpTypes TypeToSpawn)
{
	static const FName NAME_FreeCam = FName(TEXT("FreeCam"));

	if (BaseToSpawnOn)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.Owner = BaseToSpawnOn;

		AUTLineUpZone* NewZone = GetWorld()->SpawnActor<AUTLineUpZone>(AUTLineUpZone::StaticClass(), SpawnParams);
		NewZone->ZoneType = TypeToSpawn;
		NewZone->bIsTeamSpawnList = true;
		NewZone->bSnapToFloor = false;

		NewZone->AttachToActor(BaseToSpawnOn, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
		NewZone->SetActorRelativeLocation(FVector(0.0f, 0.0f, NewZone->SnapFloorOffset));

		if (TypeToSpawn == LineUpTypes::Intro)
		{
			NewZone->DefaultCreateForIntro();
		}
		else if (TypeToSpawn == LineUpTypes::Intermission)
		{
			NewZone->DefaultCreateForIntermission();
		}
		else if (TypeToSpawn == LineUpTypes::PostMatch)
		{
			NewZone->DefaultCreateForEndMatch();
		}

		//See if the new zone's camera is stuck inside of a wall
		if (GetWorld())
		{
			FHitResult CameraCollision;
			FCollisionQueryParams Params(NAME_FreeCam, false, this);
			Params.AddIgnoredActor(NewZone);
			Params.AddIgnoredActor(BaseToSpawnOn);

			GetWorld()->SweepSingleByChannel(CameraCollision, NewZone->GetActorLocation(), NewZone->Camera->GetComponentLocation(), FQuat::Identity, COLLISION_TRACE_WEAPON, FCollisionShape::MakeBox(FVector(12.f)), Params);

			if (CameraCollision.bBlockingHit)
			{
				NewZone->Camera->SetWorldLocation(CameraCollision.ImpactPoint);
			}
		}

		NewZone->GameObjectiveReference = BaseToSpawnOn;

		SpawnedLineUps.Add(NewZone);
	}
}

AUTGameObjective* AUTFlagRunGameState::GetLeadTeamFlagBase()
{
	return DeliveryPoint;
}

void AUTFlagRunGameState::AddScoringPlay(const FCTFScoringPlay& NewScoringPlay)
{
	if (Role == ROLE_Authority)
	{
		ScoringPlays.AddUnique(NewScoringPlay);
	}
}

float AUTFlagRunGameState::ScoreCameraView(AUTPlayerState* InPS, AUTCharacter *Character)
{
	// bonus score to player near but not holding enemy flag
	if (InPS && Character && !InPS->CarriedObject && InPS->Team && (InPS->Team->GetTeamNum() < 2))
	{
		uint8 EnemyTeamNum = 1 - InPS->Team->GetTeamNum();
		AUTFlag* EnemyFlag = FlagSpawner ? FlagSpawner->MyFlag : nullptr;
		if (EnemyFlag && ((EnemyFlag->GetActorLocation() - Character->GetActorLocation()).Size() < DeliveryPoint->LastSecondSaveDistance))
		{
			float MaxScoreDist = DeliveryPoint->LastSecondSaveDistance;
			return FMath::Clamp(10.f * (MaxScoreDist - (EnemyFlag->GetActorLocation() - Character->GetActorLocation()).Size()) / MaxScoreDist, 0.f, 10.f);
		}
	}
	return 0.f;
}

uint8 AUTFlagRunGameState::NearestTeamSide(AActor* InActor)
{
	if (DeliveryPoint && FlagSpawner)
	{
		// if there is only one of this pickup, return 255
		bool bFoundAnother = false;
		AUTPickupInventory* InPickupInventory = Cast<AUTPickupInventory>(InActor);
		if (InPickupInventory)
		{
			for (FActorIterator It(GetWorld()); It; ++It)
			{
				AUTPickupInventory* PickupInventory = Cast<AUTPickupInventory>(*It);
				if (PickupInventory && (PickupInventory != InPickupInventory) && (PickupInventory->GetInventoryType() == InPickupInventory->GetInventoryType()))
				{
					bFoundAnother = true;
					break;
				}
			}
		}
		if (bFoundAnother)
		{
			float DistDiff = (InActor->GetActorLocation() - DeliveryPoint->GetActorLocation()).Size() - (InActor->GetActorLocation() - FlagSpawner->GetActorLocation()).Size();
			return (DistDiff < 0) ? DeliveryPoint->TeamNum : FlagSpawner->TeamNum;
		}
	}
	return 255;
}

void AUTFlagRunGameState::UpdateHighlights_Implementation()
{
	AUTPlayerState* TopFlagCaps[2] = { NULL, NULL };
	AUTPlayerState* TopAssists[2] = { NULL, NULL };
	AUTPlayerState* TopFlagReturns[2] = { NULL, NULL };

	for (TActorIterator<AUTPlayerState> It(GetWorld()); It; ++It)
	{
		AUTPlayerState* PS = *It;
		if (PS && !PS->bOnlySpectator)
		{
			int32 TeamIndex = PS->Team ? PS->Team->TeamIndex : 0;
			if (PS->FlagCaptures > (TopFlagCaps[TeamIndex] ? TopFlagCaps[TeamIndex]->FlagCaptures : 0))
			{
				TopFlagCaps[TeamIndex] = PS;
			}
			PS->Score = PS->Kills + PS->KillAssists;
		}
	}

	for (TActorIterator<AUTPlayerState> It(GetWorld()); It; ++It)
	{
		AUTPlayerState* PS = *It;
		if (PS && !PS->bOnlySpectator)
		{
			int32 TeamIndex = PS->Team ? PS->Team->TeamIndex : 0;
			if ((TopFlagCaps[TeamIndex] != NULL) && (PS->FlagCaptures == TopFlagCaps[TeamIndex]->FlagCaptures))
			{
				PS->AddMatchHighlight((TeamIndex == 0) ? HighlightNames::TopFlagCapturesRed : HighlightNames::TopFlagCapturesBlue, PS->FlagCaptures);
			}
			else if (PS->FlagCaptures > 0)
			{
				PS->AddMatchHighlight(HighlightNames::FlagCaptures, PS->FlagCaptures);
			}
		}
	}

	Super::UpdateHighlights_Implementation();
}

void AUTFlagRunGameState::AddMinorHighlights_Implementation(AUTPlayerState* PS)
{
	// skip if already filled with major highlights
	if (PS->MatchHighlights[3] != NAME_None)
	{
		return;
	}

	if (PS->GetStatsValue(NAME_FCKills) > 0)
	{
		PS->AddMatchHighlight(NAME_FCKills, PS->GetStatsValue(NAME_FCKills));
		if (PS->MatchHighlights[3] != NAME_None)
		{
			return;
		}
	}
	Super::AddMinorHighlights_Implementation(PS);
	if (PS->MatchHighlights[3] != NAME_None)
	{
		return;
	}

	if (PS->GetStatsValue(NAME_FlagGrabs) > 0)
	{
		PS->AddMatchHighlight(NAME_FlagGrabs, PS->GetStatsValue(NAME_FlagGrabs));
		if (PS->MatchHighlights[3] != NAME_None)
		{
			return;
		}
	}
	if (PS->GetStatsValue(NAME_FlagSupportKills) > 0)
	{
		PS->AddMatchHighlight(NAME_FlagSupportKills, PS->GetStatsValue(NAME_FlagSupportKills));
		if (PS->MatchHighlights[3] != NAME_None)
		{
			return;
		}
	}
}




