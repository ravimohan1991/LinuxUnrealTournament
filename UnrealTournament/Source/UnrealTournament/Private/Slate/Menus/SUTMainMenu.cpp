// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "SUTMainMenu.h"
#include "../SUWindowsStyle.h"
#include "../Base/SUTDialogBase.h"
#include "../Dialogs/SUTSystemSettingsDialog.h"
#include "../Dialogs/SUTPlayerSettingsDialog.h"
#include "../Dialogs/SUTControlSettingsDialog.h"
#include "../Dialogs/SUTInputBoxDialog.h"
#include "../Dialogs/SUTMessageBoxDialog.h"
#include "../Dialogs/SUTGameSetupDialog.h"
#include "../Widgets/SUTScaleBox.h"
#include "../Dialogs/SUTDifficultyLevel.h"
#include "UTGameEngine.h"
#include "../Panels/SUTServerBrowserPanel.h"
#include "../Panels/SUTReplayBrowserPanel.h"
#include "../Panels/SUTStatsViewerPanel.h"
#include "../Panels/SUTCreditsPanel.h"
#include "../Panels/SUTChallengePanel.h"
#include "../Panels/SUTHomePanel.h"
#include "../Panels/SUTFragCenterPanel.h"
#include "UTEpicDefaultRulesets.h"
#include "UTReplicatedGameRuleset.h"
#include "UTAnalytics.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"
#include "../Panels/SUTUMGPanel.h"

#if !UE_SERVER

#include "UserWidget.h"
#include "AnalyticsEventAttribute.h"
#include "IAnalyticsProvider.h"

void SUTMainMenu::CreateDesktop()
{
	bNeedToShowGamePanel = false;
	SUTMenuBase::CreateDesktop();
}

SUTMainMenu::~SUTMainMenu()
{
	HomePanel.Reset();
}

TSharedRef<SWidget> SUTMainMenu::BuildBackground()
{
	return SNew(SOverlay)
	+SOverlay::Slot()
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.HAlign(HAlign_Fill)
			[
				SNew(SUTScaleBox)
				.bMaintainAspectRatio(false)
				[
					SNew(SImage)
					.Image(SUTStyle::Get().GetBrush("UT.HomePanel.Background"))
				]
			]
		]
	];
}

FReply SUTMainMenu::OnFragCenterClick()
{
	ShowFragCenter();
	return FReply::Handled();
}

void SUTMainMenu::DeactivatePanel(TSharedPtr<class SUTPanelBase> PanelToDeactivate)
{
	if (FragCenterPanel.IsValid()) FragCenterPanel.Reset();
	if (WebPanel.IsValid()) WebPanel.Reset();

	SUTMenuBase::DeactivatePanel(PanelToDeactivate);
}

void SUTMainMenu::ShowFragCenter()
{
	if (!FragCenterPanel.IsValid())
	{
		SAssignNew(FragCenterPanel, SUTFragCenterPanel, PlayerOwner)
			.ViewportSize(FVector2D(1920, 1020))
			.AllowScaling(true)
			.ShowControls(false);

		if (FragCenterPanel.IsValid())
		{
			FragCenterPanel->Browse(TEXT("http://www.unrealtournament.com/fragcenter"));
			ActivatePanel(FragCenterPanel);
		}
	}
}

void SUTMainMenu::SetInitialPanel()
{
	SAssignNew(HomePanel, SUTHomePanel, PlayerOwner);

	if (HomePanel.IsValid())
	{
		ActivatePanel(HomePanel);
	}
}

/****************************** [ Build Sub Menus ] *****************************************/

void SUTMainMenu::BuildLeftMenuBar()
{
	if (LeftMenuBar.IsValid())
	{
		LeftMenuBar->AddSlot()
		.Padding(50.0f, 0.0f, 0.0f, 0.0f)
		.AutoWidth()
		[
			AddPlayNow()
		];

		LeftMenuBar->AddSlot()
		.Padding(40.0f,0.0f,0.0f,0.0f)
		.AutoWidth()
		[
			BuildTutorialSubMenu()
		];
		
		LeftMenuBar->AddSlot()
		.Padding(40.0f,0.0f,0.0f,0.0f)
		.AutoWidth()
		[
			BuildWatchSubMenu()
		];
	}
}

TSharedRef<SWidget> SUTMainMenu::BuildWatchSubMenu()
{
	TSharedPtr<SUTComboButton> DropDownButton = NULL;
		SNew(SBox)
	.HeightOverride(56)
	[
		SAssignNew(DropDownButton, SUTComboButton)
		.HasDownArrow(false)
		.Text(NSLOCTEXT("SUTMenuBase", "MenuBar_REPLAYS", "WATCH"))
		.TextStyle(SUTStyle::Get(), "UT.Font.MenuBarText")
		.ContentPadding(FMargin(35.0,0.0,35.0,0.0))
		.ContentHAlign(HAlign_Left)
	];

//	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_Watch_FragCenter", "Frag Center"), FOnClicked::CreateSP(this, &SUTMainMenu::OnFragCenterClick));
//	DropDownButton->AddSpacer();
	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_Replays_YourReplays", "Your Replays"), FOnClicked::CreateSP(this, &SUTMainMenu::OnYourReplaysClick));
	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_Replays_RecentReplays", "Recent Replays"), FOnClicked::CreateSP(this, &SUTMainMenu::OnRecentReplaysClick));
	DropDownButton->AddSpacer();
	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_Replays_LiveGames", "Live Games"), FOnClicked::CreateSP(this, &SUTMainMenu::OnLiveGameReplaysClick), true);

	return DropDownButton.ToSharedRef();
}

TSharedRef<SWidget> SUTMainMenu::BuildTutorialSubMenu()
{
	TSharedPtr<SUTComboButton> DropDownButton = NULL;
	SNew(SBox)
	.HeightOverride(56)
	[
		SAssignNew(DropDownButton, SUTComboButton)
		.HasDownArrow(false)
		.Text(NSLOCTEXT("SUTMenuBase", "MenuBar_TUTORIAL", "LEARN"))
		.TextStyle(SUTStyle::Get(), "UT.Font.MenuBarText")
		.ContentPadding(FMargin(35.0,0.0,35.0,0.0))
		.ContentHAlign(HAlign_Left)
	];

	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_Tutorial_LeanHoToPlay", "Basic Training"), FOnClicked::CreateSP(this, &SUTMainMenu::OnBootCampClick));
	DropDownButton->AddSpacer();
	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_Tutorial_Community", "Training Videos"), FOnClicked::CreateSP(this, &SUTMainMenu::OnCommunityClick), true);

	return DropDownButton.ToSharedRef();

}


TSharedRef<SWidget> SUTMainMenu::AddPlayNow()
{
	TSharedPtr<SUTComboButton> DropDownButton = NULL;

	SNew(SBox)
	.HeightOverride(56)
	[
		SAssignNew(DropDownButton, SUTComboButton)
		.HasDownArrow(false)
		.Text(NSLOCTEXT("SUTMenuBase", "MenuBar_QuickMatch", "PLAY"))
		.TextStyle(SUTStyle::Get(), "UT.Font.MenuBarText")
		.ContentPadding(FMargin(35.0,0.0,35.0,0.0))
		.ContentHAlign(HAlign_Left)
	];

	BuildQuickPlaySubMenu(DropDownButton);

	DropDownButton->AddSpacer();

	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_ChallengesGame", "Single Player Challenges"), FOnClicked::CreateSP(this, &SUTMainMenu::OnShowGamePanel));
	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_CreateGame", "Custom Single Player Match"), FOnClicked::CreateSP(this, &SUTMainMenu::OnShowCustomGamePanel));
	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_StartLANGame", "Start LAN Match"), FOnClicked::CreateSP(this, &SUTMainMenu::OnShowCustomGamePanel));

	DropDownButton->AddSpacer();
	DropDownButton->AddSubMenuItem(NSLOCTEXT("SUTMenuBase", "MenuBar_QuickMatch_FindGame", "Join a Hub                "), FOnClicked::CreateSP(this, &SUTMenuBase::OnShowServerBrowserPanel),true);
	
	return DropDownButton.ToSharedRef();
}

FReply SUTMainMenu::OnCloseClicked()
{

	for (int32 i=0; i<AvailableGameRulesets.Num();i++)
	{
		AvailableGameRulesets[i]->SlateBadge = NULL;
	}


	PlayerOwner->HideMenu();
	ConsoleCommand(TEXT("quit"));
	return FReply::Handled();
}



FReply SUTMainMenu::OnShowGamePanel()
{
	ShowGamePanel();
	return FReply::Handled();
}

FReply SUTMainMenu::OnShowCustomGamePanel()
{
	ShowCustomGamePanel();
	return FReply::Handled();
}


void SUTMainMenu::ShowGamePanel()
{
	bool bIsInParty = false;
	UPartyContext* PartyContext = Cast<UPartyContext>(UBlueprintContextLibrary::GetContext(PlayerOwner->GetWorld(), UPartyContext::StaticClass()));
	if (PartyContext)
	{
		bIsInParty = PartyContext->GetPartySize() > 1;
	}

	if (bIsInParty)
	{
		PlayerOwner->ShowToast(NSLOCTEXT("SUTMenuBase", "ShowGamePanelNotLeader", "You may not do challenges while in a party"));
		return;
	}

	if ( !ChallengePanel.IsValid() )
	{
		SAssignNew(ChallengePanel, SUTChallengePanel, PlayerOwner);
	}

	ActivatePanel(ChallengePanel);
}

void SUTMainMenu::ShowCustomGamePanel()
{
	bool bIsInParty = false;
	UPartyContext* PartyContext = Cast<UPartyContext>(UBlueprintContextLibrary::GetContext(PlayerOwner->GetWorld(), UPartyContext::StaticClass()));
	if (PartyContext)
	{
		bIsInParty = PartyContext->GetPartySize() > 1;
	}

	if (bIsInParty)
	{
		PlayerOwner->ShowToast(NSLOCTEXT("SUTMenuBase", "ShowCustomGamePanelNotLeader", "You may not do custom matches while in a party"));
		return;
	}

	if (TickCountDown <= 0)
	{
		PlayerOwner->ShowContentLoadingMessage();
		bNeedToShowGamePanel = true;
		TickCountDown = 3;
	}
}

void SUTMainMenu::OpenDelayedMenu()
{
	SUTMenuBase::OpenDelayedMenu();
	if (bNeedToShowGamePanel)
	{
		bNeedToShowGamePanel = false;
		AvailableGameRulesets.Empty();

		// Grab all of the available map assets.
		TArray<FAssetData> MapAssets;
		GetAllAssetData(UWorld::StaticClass(), MapAssets, false);

		UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(PlayerOwner->GetGameInstance());
		if (UTGameInstance)
		{
			for (int32 i=0; i < UTGameInstance->GameRulesets.Num(); i++)
			{
				UE_LOG(UT,Verbose,TEXT("Loading Rule %s"), *UTGameInstance->GameRulesets[i].UniqueTag)
				if (!UTGameInstance->GameRulesets[i].bHideFromUI)
				{
					bool bExistsAlready = false;
					for (int32 j=0; j < AvailableGameRulesets.Num(); j++)
					{
						if ( AvailableGameRulesets[j]->Data.UniqueTag.Equals(UTGameInstance->GameRulesets[i].UniqueTag, ESearchCase::IgnoreCase) || AvailableGameRulesets[j]->Data.Title.ToLower() == UTGameInstance->GameRulesets[i].Title.ToLower() )
						{
							bExistsAlready = true;
							break;
						}
					}

					if ( !bExistsAlready )
					{
						FActorSpawnParameters Params;
						Params.Owner = PlayerOwner->GetWorld()->GetGameState();
						AUTReplicatedGameRuleset* NewReplicatedRuleset = PlayerOwner->GetWorld()->SpawnActor<AUTReplicatedGameRuleset>(Params);
						if (NewReplicatedRuleset)
						{
							// Build out the map info
							NewReplicatedRuleset->SetRules(UTGameInstance->GameRulesets[i], MapAssets);

							// If this ruleset doesn't have any maps, then don't use it
							if (NewReplicatedRuleset->MapList.Num() > 0)
							{
								AvailableGameRulesets.Add(NewReplicatedRuleset);
							}
							else
							{
								UE_LOG(UT,Warning,TEXT("Detected a ruleset [%s] that has no maps"), *UTGameInstance->GameRulesets[i].UniqueTag);
								NewReplicatedRuleset->Destroy();
							}
						}
					}
					else
					{
						UE_LOG(UT,Verbose,TEXT("Rule already exists."));
					}
				}
			}
		}	
		for (int32 i=0; i < AvailableGameRulesets.Num(); i++)
		{
			AvailableGameRulesets[i]->BuildSlateBadge();
		}

		if (AvailableGameRulesets.Num() > 0)
		{
			SAssignNew(CreateGameDialog, SUTGameSetupDialog)
			.PlayerOwner(PlayerOwner)
			.GameRuleSets(AvailableGameRulesets)
			.DialogSize(FVector2D(1920,1080))
#if PLATFORM_WINDOWS
			.ButtonMask(UTDIALOG_BUTTON_PLAY | UTDIALOG_BUTTON_LAN | UTDIALOG_BUTTON_CANCEL);
#else
			.ButtonMask(UTDIALOG_BUTTON_PLAY | UTDIALOG_BUTTON_CANCEL);
#endif

			if ( CreateGameDialog.IsValid() )
			{
				PlayerOwner->OpenDialog(CreateGameDialog.ToSharedRef(), 100);
			}
	
		}
	}
	PlayerOwner->HideContentLoadingMessage();
}

FReply SUTMainMenu::OnPlayQuickMatch(int32 PlaylistId)
{
	QuickPlay(PlaylistId);
	return FReply::Handled();
}


void SUTMainMenu::QuickPlay(int32 PlaylistId)
{
	if (!PlayerOwner->IsPartyLeader())
	{
		PlayerOwner->ShowToast(NSLOCTEXT("SUTMenuBase", "QuickPlayNotLeader", "Only the party leader may start Quick Play"));
		return;
	}

	if (!PlayerOwner->IsLoggedIn())
	{
		PlayerOwner->GetAuth();
		return;
	}

	PlayerOwner->StartQuickMatch(PlaylistId);
}


FReply SUTMainMenu::OnBootCampClick()
{
	OpenTutorialMenu();
	return FReply::Handled();
}

void SUTMainMenu::OpenTutorialMenu()
{
	bool bIsInParty = false;
	UPartyContext* PartyContext = Cast<UPartyContext>(UBlueprintContextLibrary::GetContext(PlayerOwner->GetWorld(), UPartyContext::StaticClass()));
	if (PartyContext)
	{
		bIsInParty = PartyContext->GetPartySize() > 1;
	}

	if (bIsInParty)
	{
		PlayerOwner->ShowToast(NSLOCTEXT("SUTMenuBase", "TutorialNotLeader", "You may not enter tutorials while in a party"));
		return;
	}

	if (!TutorialPanel.IsValid())
	{
		SAssignNew(TutorialPanel,SUTUMGPanel,PlayerOwner).UMGClass(TEXT("/Game/RestrictedAssets/Tutorials/Blueprints/TutMainMenuWidget.TutMainMenuWidget_C"));
	}

	if (TutorialPanel.IsValid() && ActivePanel != TutorialPanel)
	{
		ActivatePanel(TutorialPanel);
	}
}


FReply SUTMainMenu::OnYourReplaysClick()
{
	if (!PlayerOwner->IsLoggedIn())
	{
		PlayerOwner->LoginOnline( TEXT( "" ), TEXT( "" ) );
		return FReply::Handled();
	}

	TSharedPtr<class SUTReplayBrowserPanel> ReplayBrowser = PlayerOwner->GetReplayBrowser();
	if (ReplayBrowser.IsValid())
	{
		ReplayBrowser->bLiveOnly = false;
		ReplayBrowser->bShowReplaysFromAllUsers = false;
		ReplayBrowser->MetaString = TEXT("");

		if (ReplayBrowser == ActivePanel)
		{
			ReplayBrowser->BuildReplayList(PlayerOwner->GetPreferredUniqueNetId()->ToString());
		}
		else
		{
			ActivatePanel(ReplayBrowser);
		}
	}

	return FReply::Handled();
}

FReply SUTMainMenu::OnRecentReplaysClick()
{
	RecentReplays();
	return FReply::Handled();
}

void SUTMainMenu::RecentReplays()
{
	if (!PlayerOwner->IsLoggedIn())
	{
		PlayerOwner->LoginOnline( TEXT( "" ), TEXT( "" ) );
		return;
	}

	TSharedPtr<class SUTReplayBrowserPanel> ReplayBrowser = PlayerOwner->GetReplayBrowser();
	if (ReplayBrowser.IsValid())
	{
		ReplayBrowser->bLiveOnly = false;
		ReplayBrowser->bShowReplaysFromAllUsers = true;
		ReplayBrowser->MetaString = TEXT("");

		if (ReplayBrowser == ActivePanel)
		{
			ReplayBrowser->BuildReplayList(TEXT(""));
		}
		else
		{
			ActivatePanel(ReplayBrowser);
		}
	}
}

FReply SUTMainMenu::OnLiveGameReplaysClick()
{
	ShowLiveGameReplays();

	return FReply::Handled();
}

void SUTMainMenu::ShowLiveGameReplays()
{
	if (!PlayerOwner->IsLoggedIn())
	{
		PlayerOwner->LoginOnline( TEXT( "" ), TEXT( "" ) );
		return;
	}

	TSharedPtr<class SUTReplayBrowserPanel> ReplayBrowser = PlayerOwner->GetReplayBrowser();
	if (ReplayBrowser.IsValid())
	{
		ReplayBrowser->bLiveOnly = true;
		ReplayBrowser->bShowReplaysFromAllUsers = true;
		ReplayBrowser->MetaString = TEXT("");

		if (ReplayBrowser == ActivePanel)
		{
			ReplayBrowser->BuildReplayList(TEXT(""));
		}
		else
		{
			ActivatePanel(ReplayBrowser);
		}
	}
}

FReply SUTMainMenu::OnCommunityClick()
{
	ShowCommunity();
	return FReply::Handled();
}

void SUTMainMenu::ShowCommunity()
{
	if ( !WebPanel.IsValid() )
	{
		TSharedPtr<SUTWebBrowserPanel> NewWebPanel;

		// Create the Web panel
		SAssignNew(NewWebPanel, SUTWebBrowserPanel, PlayerOwner);
		if (NewWebPanel.IsValid())
		{
			if (ActivePanel.IsValid() && ActivePanel != NewWebPanel)
			{
				ActivatePanel(NewWebPanel);
			}
			NewWebPanel->Browse(CommunityVideoURL);
			WebPanel = NewWebPanel;
		}
	}
}

bool SUTMainMenu::ShouldShowBrowserIcon()
{
	return (PlayerOwner.IsValid() && PlayerOwner->bShowBrowserIconOnMainMenu);
}

FReply SUTMainMenu::OnShowServerBrowserPanel()
{
	return SUTMenuBase::OnShowServerBrowserPanel();

}

void SUTMainMenu::OnMenuOpened(const FString& Parameters)
{
	SUTMenuBase::OnMenuOpened(Parameters);
	if (Parameters.Equals(TEXT("showchallenge"), ESearchCase::IgnoreCase))
	{
		ShowGamePanel();
	}
	if (Parameters.Equals(TEXT("showbrowser"), ESearchCase::IgnoreCase))
	{
		OnShowServerBrowser();
	}

}

void SUTMainMenu::BuildQuickPlaySubMenu(TSharedPtr<SUTComboButton> Button)
{
	UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(PlayerOwner->GetGameInstance());
	if (UTGameInstance && UTGameInstance->GetPlaylistManager())
	{
		TArray<FPlaylistItem*> QuickPlayPlaylist;
		UTGameInstance->GetPlaylistManager()->GetPlaylist(false, QuickPlayPlaylist);
		for (int32 i=0; i < QuickPlayPlaylist.Num(); i++)
		{
			FPlaylistItem* Item = QuickPlayPlaylist[i];
			if (!Item->bHideInUI)
			{
				FUTGameRuleset* Ruleset = UTGameInstance->GetPlaylistManager()->GetRuleset(Item->PlaylistId);
				Button->AddSubMenuItem( FText::Format( NSLOCTEXT("SUTMenuBase", "QuickPlay_Format", "QuickPlay - {0}"), FText::FromString(Ruleset->Title)), FOnClicked::CreateSP(this, &SUTMainMenu::OnPlayQuickMatch,	Item->PlaylistId));
			}
		}
	}				
}

#endif