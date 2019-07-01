// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "SUTMatchmakingDialog.h"
#include "UTGameInstance.h"
#include "UTParty.h"
#include "UTMatchmaking.h"
#include "UTPartyGameState.h"
#include "../SUTStyle.h"

#if !UE_SERVER
#include "SlateBasics.h"
#include "SlateExtras.h"

void SUTMatchmakingDialog::Construct(const FArguments& InArgs)
{
	LastMatchmakingPlayersNeeded = 0;
	RetryTime = 120.0f;
	RetryCountdown = RetryTime;
	
	SUTDialogBase::Construct(SUTDialogBase::FArguments()
		.PlayerOwner(InArgs._PlayerOwner)
		.DialogTitle(InArgs._DialogTitle)
		.DialogSize(InArgs._DialogSize)
		.bDialogSizeIsRelative(InArgs._bDialogSizeIsRelative)
		.DialogPosition(InArgs._DialogPosition)
		.DialogAnchorPoint(InArgs._DialogAnchorPoint)
		.ContentPadding(InArgs._ContentPadding)
		.ButtonMask(InArgs._ButtonMask)
		.OnDialogResult(InArgs._OnDialogResult)
		);

	TimeDialogOpened = PlayerOwner->GetWorld()->RealTimeSeconds;

	if (DialogContent.IsValid())
	{		
		DialogContent->AddSlot()
		[
			SNew(SVerticalBox)

			// Status

			+ SVerticalBox::Slot()
			.Padding(0.0f, 50.0f, 0.0f, 10.0f)
			.AutoHeight()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SUTMatchmakingDialog::GetMatchmakingText)
				.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Medium.Orange")
			]

			// Region

			+ SVerticalBox::Slot()
			.Padding(0.0f, 5.0f, 0.0f, 0.0f)
			.AutoHeight()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SUTMatchmakingDialog::GetRegionText)
				.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Small.Gray")
			]

			// Throbber
			+SVerticalBox::Slot()
			.Padding(0.0f, 5.0f, 0.0f, 0.0f)
			.AutoHeight()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(SThrobber)
			]


			// Elapsed Time
			+ SVerticalBox::Slot()
			.Padding(0.0f, 45.0f, 0.0f, 5.0f)
			.AutoHeight()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SUTMatchmakingDialog::GetMatchmakingTimeElapsedText)
				.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Small.Gray")
			]

			// Estimated Time
			+ SVerticalBox::Slot()
			.Padding(0.0f, 10.0f, 0.0f, 5.0f)
			.AutoHeight()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			[
				SNew(STextBlock)
				.Text(this, &SUTMatchmakingDialog::GetMatchmakingEstimatedTimeText)
				.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Small.Gray")
			]
		];
	}
}

FText SUTMatchmakingDialog::GetRegionText() const
{
	UUTGameInstance* GameInstance = Cast<UUTGameInstance>(GetPlayerOwner()->GetGameInstance());
	if (GameInstance)
	{
		UUTParty* Party = GameInstance->GetParties();
		if (Party)
		{
			UUTPartyGameState* PartyState = Party->GetUTPersistentParty();
			if (PartyState)
			{
				FString MatchMakingRegion = PartyState->GetMatchmakingRegion();

				if (MatchMakingRegion == TEXT("NA"))
				{
					MatchMakingRegion = TEXT("North America");
				}
				else if (MatchMakingRegion == TEXT("EU"))
				{
					MatchMakingRegion = TEXT("Europe");
				}

				return FText::Format(NSLOCTEXT("Generic", "Region", "Region: {0}"), FText::FromString(MatchMakingRegion));
			}
		}
	}

	return NSLOCTEXT("Generic", "LeaderMatchmaking", "Leader is Matchmaking");
}

FText SUTMatchmakingDialog::GetMatchmakingText() const
{
	FText SearchType = FText::GetEmpty();

	UUTGameInstance* GameInstance = Cast<UUTGameInstance>(GetPlayerOwner()->GetGameInstance());
	if (GameInstance)
	{
		UUTMatchmaking* Matchmaking = GameInstance->GetMatchmaking();

		int32 PlaylistID = INDEX_NONE;

		UUTParty* Party = GameInstance->GetParties();
		if (Party)
		{
			UUTPartyGameState* PartyState = Party->GetUTPersistentParty();
			if (PartyState)
			{
				PlaylistID = PartyState->GetPlaylistID();
				switch (PartyState->GetPartyProgression())
				{
				case EUTPartyState::PostMatchmaking:
					return NSLOCTEXT("Generic", "JoiningServer", "Match found!  Joining...");
				}
			}
		}

		SearchType = PlayerOwner->PlayListIDToText(PlaylistID);
	}

	return PlayerOwner->IsPartyLeader()  
				? FText::Format(NSLOCTEXT("Generic", "SearchingForServer", "Searching for {0}Match..."), SearchType) 
				: FText::Format(NSLOCTEXT("Generic", "SearchingForServer", "Your leader is searching for {0}Match..."), SearchType);

}


FText SUTMatchmakingDialog::GetMatchmakingTimeElapsedText() const
{
	UUTGameInstance* GameInstance = Cast<UUTGameInstance>(GetPlayerOwner()->GetGameInstance());
	if (GameInstance)
	{
		UUTParty* Party = GameInstance->GetParties();
		if (Party)
		{
			UUTPartyGameState* PartyState = Party->GetUTPersistentParty();
			if (PartyState)
			{
				//if (PlayerOwner->IsPartyLeader())
				{
					int32 ElapsedTime = PlayerOwner->GetWorld()->RealTimeSeconds - TimeDialogOpened;
					FTimespan TimeSpan(0, 0, ElapsedTime);
					return FText::Format(NSLOCTEXT("Generic", "ElapsedMatchMakingTime", "Elapsed: {0}"), FText::AsTimespan(TimeSpan));
				}
			}
		}
	}
	
	return FText::GetEmpty();	
}

FText SUTMatchmakingDialog::GetMatchmakingEstimatedTimeText() const
{
	UUTGameInstance* GameInstance = Cast<UUTGameInstance>(GetPlayerOwner()->GetGameInstance());
	if (GameInstance)
	{
		UUTParty* Party = GameInstance->GetParties();
		if (Party)
		{
			UUTPartyGameState* PartyState = Party->GetUTPersistentParty();
			if (PartyState)
			{
				//if (PlayerOwner->IsPartyLeader())
				{
					UUTMatchmaking* Matchmaking = GameInstance->GetMatchmaking();
					if (Matchmaking)
					{
						int32 EstimatedWaitTime = Matchmaking->GetEstimatedMatchmakingTime();
						if (EstimatedWaitTime > 0 && EstimatedWaitTime < 60*10)
						{
							FTimespan TimeSpan(0, 0, EstimatedWaitTime);
							return FText::Format(NSLOCTEXT("Generic", "EstimateMatchMakingTime", "Estimated wait {0}"), FText::AsTimespan(TimeSpan));
						}
					}
				}
			}
		}
	}

	return FText::GetEmpty();
}

FReply SUTMatchmakingDialog::OnButtonClick(uint16 ButtonID)
{
	OnDialogResult.ExecuteIfBound(SharedThis(this), UTDIALOG_BUTTON_CANCEL);
	PlayerOwner->CloseDialog(SharedThis(this));

	if (PlayerOwner->IsMenuGame())
	{
		PlayerOwner->CancelQuickmatch();
	}
	else
	{
		PlayerOwner->ReturnToMainMenu();
	}
	
	return FReply::Handled();
}

void SUTMatchmakingDialog::Tick(const FGeometry & AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SUTDialogBase::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	// Failsafe in case we join a server
	if (PlayerOwner.IsValid() && PlayerOwner->GetWorld()->GetNetMode() == NM_Client)
	{
		PlayerOwner->HideMatchmakingDialog();
	}

	if (PlayerOwner.IsValid() && PlayerOwner->IsMenuGame() && !PlayerOwner->IsPartyLeader())
	{
		UUTGameInstance* GameInstance = Cast<UUTGameInstance>(GetPlayerOwner()->GetGameInstance());
		if (GameInstance)
		{
			UUTParty* Party = GameInstance->GetParties();
			if (Party)
			{
				UUTPartyGameState* PartyState = Party->GetUTPersistentParty();
				if (PartyState && PartyState->GetPartyProgression() == EUTPartyState::Menus)
				{
					PlayerOwner->HideMatchmakingDialog();
				}
			}
		}
	}

	if (PlayerOwner.IsValid() && PlayerOwner->IsMenuGame() && PlayerOwner->IsPartyLeader())
	{
		UUTGameInstance* GameInstance = Cast<UUTGameInstance>(GetPlayerOwner()->GetGameInstance());
		if (GameInstance)
		{
			UUTParty* Party = GameInstance->GetParties();
			if (Party)
			{
				UUTPartyGameState* PartyState = Party->GetUTPersistentParty();
				if (PartyState && PartyState->GetPartyProgression() == EUTPartyState::PostMatchmaking)
				{
					const int32 MatchmakingPlayersNeeded = PartyState->GetMatchmakingPlayersNeeded();
					if (MatchmakingPlayersNeeded > 0)
					{
						if (MatchmakingPlayersNeeded == LastMatchmakingPlayersNeeded)
						{
							RetryCountdown -= InDeltaTime;
							if (RetryCountdown < 0)
							{
								UUTMatchmaking* Matchmaking = GameInstance->GetMatchmaking();
								if (Matchmaking)
								{
									// Disconnect from current server
									// Start matchmaking with a higher elo gate for starting own server and skip joining current server
									Matchmaking->RetryFindGatheringSession();
									LastMatchmakingPlayersNeeded = 0;
									RetryCountdown = RetryTime;
								}
							}
						}
						else
						{
							LastMatchmakingPlayersNeeded = MatchmakingPlayersNeeded;
							RetryCountdown = RetryTime;
						}
					}
				}
			}
		}
	}
}


#endif