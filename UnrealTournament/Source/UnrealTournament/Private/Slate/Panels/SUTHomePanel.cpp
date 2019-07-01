// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "SlateBasics.h"
#include "../SUTUtils.h"
#include "../SUTStyle.h"
#include "../Widgets/SUTScaleBox.h"
#include "../Widgets/SUTBorder.h"
#include "Slate/SlateGameResources.h"
#include "SUTHomePanel.h"
#include "../Menus/SUTMainMenu.h"
#include "../Widgets/SUTButton.h"
#include "NetworkVersion.h"
#include "UTGameInstance.h"
#include "BlueprintContextLibrary.h"
#include "MatchmakingContext.h"
#include "SUTServerBrowserPanel.h"
#include "UTWorldSettings.h"
#include "UTMenuGameMode.h"


#if !UE_SERVER

void SUTHomePanel::ConstructPanel(FVector2D ViewportSize)
{
	this->ChildSlot
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SAssignNew(AnimWidget, SUTBorder)
			.OnAnimEnd(this, &SUTHomePanel::AnimEnd)
			[
				BuildHomePanel()
			]
		]
	];

}

void SUTHomePanel::OnShowPanel(TSharedPtr<SUTMenuBase> inParentWindow)
{
	SUTPanelBase::OnShowPanel(inParentWindow);

	AnnouncmentTimer = 3.0;

	PlayerOwner->GetWorld()->GetTimerManager().SetTimer(LanTimerHandle, FTimerDelegate::CreateSP(this, &SUTHomePanel::CheckForLanServers), 8.0f, true);
	PlayerOwner->GetWorld()->GetTimerManager().SetTimer(LanPingHandle, FTimerDelegate::CreateSP(this, &SUTHomePanel::PingLanServers), 3.5f, true);
	if (AnimWidget.IsValid())
	{
		AnimWidget->Animate(FVector2D(100.0f, 0.0f), FVector2D(0.0f, 0.0f), 0.0f, 1.0f, 0.3f);
	}

	UUTGameUserSettings* UserSettings = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());
	UserSettings->SetSoundClassVolume(EUTSoundClass::Music, UserSettings->GetSoundClassVolume(EUTSoundClass::Music));
	CheckForLanServers();
}



void SUTHomePanel::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	if (AnnouncmentTimer > 0)
	{
		AnnouncmentTimer -= InDeltaTime;
		if (AnnouncmentTimer <= 0.0)
		{
			BuildAnnouncement();
		}
	}

	if (AnnouncmentFadeTimer > 0)
	{
		AnnouncmentFadeTimer -= InDeltaTime;
	}

	if (NewChallengeBox.IsValid())
	{
		if (NewChallengeImage.IsValid())
		{
			float Scale = 1.0f + (0.1 * FMath::Sin(PlayerOwner->GetWorld()->GetTimeSeconds() * 30.0f));
			NewChallengeImage->SetRenderTransform(FSlateRenderTransform(Scale));
		}
	}
}

void SUTHomePanel::CheckForLanServers()
{
	TSharedPtr<SUTServerBrowserPanel> Browser = PlayerOwner->GetServerBrowser(false);
	if (Browser.IsValid() && Browser->GetBrowserState() == EBrowserState::RefreshInProgress)
	{
		return;
	}

	// Don't do the lan search if we are in a party and we are not the party leader
	if (!PlayerOwner->LastRankedMatchSessionId.IsEmpty() || (PlayerOwner->IsInAnActiveParty()) || PlayerOwner->MatchmakingDialog.IsValid() )
	{
		return;
	}

	// Search for LAN Servers

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	
	IOnlineSessionPtr OnlineSessionInterface;
	if (OnlineSubsystem) OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();

	if (OnlineSessionInterface.IsValid())
	{
		OnlineSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindLanSessionCompleteDelegate);
	}

	LanSearchSettings = MakeShareable(new FUTOnlineGameSearchBase(false));
	LanSearchSettings->MaxSearchResults = 10000;
	LanSearchSettings->bIsLanQuery = true;
	LanSearchSettings->TimeoutInSeconds = 2.0;
	LanSearchSettings->QuerySettings.Set(SETTING_GAMEINSTANCE, 1, EOnlineComparisonOp::NotEquals);												// Must not be a Hub server instance
	LanSearchSettings->QuerySettings.Set(SETTING_RANKED, 1, EOnlineComparisonOp::NotEquals);

	TSharedRef<FUTOnlineGameSearchBase> LanSearchSettingsRef = LanSearchSettings.ToSharedRef();
	FOnFindSessionsCompleteDelegate Delegate;
	Delegate.BindSP(this, &SUTHomePanel::OnFindLANSessionsComplete);
	if (OnlineSessionInterface.IsValid())
	{
		OnFindLanSessionCompleteDelegate = OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(Delegate);
		OnlineSessionInterface->FindSessions(0, LanSearchSettingsRef);
	}
}

void SUTHomePanel::RebuildLanBox()
{
	LanBox->ClearChildren();		
	for (int32 ServerIndex = 0; ServerIndex < LanMatches.Num(); ServerIndex++)
	{
		TSharedPtr<FServerData> NewServer = LanMatches[ServerIndex];

		if (NewServer.IsValid() && NewServer->Version == FString::FromInt(FNetworkVersion::GetLocalNetworkVersion()))
		{
			FText ServerName = NewServer->GetBrowserName();
			FText ServerInfo = FText::Format(NSLOCTEXT("SUTHomePanel","LanServerFormat","Game: {0}  Map: {1}   # Players: {2}   # Friends: {3}"),
												NewServer->GetBrowserGameMode(),
												NewServer->GetBrowserMapName(),
												NewServer->GetBrowserNumPlayers(),
												NewServer->GetBrowserNumFriends());
			LanBox->AddSlot().AutoHeight()
			[

				SNew(SVerticalBox)
				+SVerticalBox::Slot().AutoHeight().Padding(0.0f,10.0f,0.0f,0.0f)
				[
					SNew(SBorder)
					.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.SuperDark"))
					.ColorAndOpacity(this, &SUTHomePanel::GetFadeColor)
					.BorderBackgroundColor(this, &SUTHomePanel::GetFadeBKColor)
					.HAlign(HAlign_Right)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("SUTHomePanel","LanServerTitle","...Found a LAN Server"))
						.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Tiny.Bold")
						.ColorAndOpacity(FLinearColor::Yellow)
					]
				]
				+SVerticalBox::Slot().AutoHeight()
				[
					SNew(SBorder)
					.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
					.ColorAndOpacity(this, &SUTHomePanel::GetFadeColor)
					.BorderBackgroundColor(this, &SUTHomePanel::GetFadeBKColor)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot().AutoWidth()
						[
							SNew(SBox).WidthOverride(940)
							[
								SNew(SHorizontalBox)
								+SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
								[
									SNew(SVerticalBox)
									+SVerticalBox::Slot().AutoHeight()
									[
										SNew(SUTImage)
										.WidthOverride(90)
										.HeightOverride(64)
										.Image(SUTStyle::Get().GetBrush("UT.Icon.Lan.Big"))
									]
								]
								+SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
								[
									SNew(SBox).WidthOverride(115).HeightOverride(86)
									[
										SNew(SUTButton)
										.ButtonStyle(SUTStyle::Get(),"UT.ClearButton")
										[
											SNew(SVerticalBox)
											+SVerticalBox::Slot()
											.Padding(0.0,4.0,0.0,0.0)
											.AutoHeight()
											[
												SNew(SUTButton)
												.ButtonStyle(SUTStyle::Get(),"UT.SimpleButton.Medium")
												.Text(NSLOCTEXT("SUTMatchPanel","JoinText","JOIN"))
												.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
												.CaptionHAlign(HAlign_Center)
												.OnClicked(this, &SUTHomePanel::OnJoinLanClicked, NewServer)
												//.IsEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(InItem.Get(), &FTrackedMatch::CanJoin, PlayerOwner)))
											]
											+SVerticalBox::Slot()
											.Padding(0.0,10.0,0.0,0.0)
											.AutoHeight()
											[
												SNew(SUTButton)
												.ButtonStyle(SUTStyle::Get(),"UT.SimpleButton.Medium")
												.Text(NSLOCTEXT("SUTMatchPanel","SpectateText","SPECTATE"))
												.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
												.CaptionHAlign(HAlign_Center)
												.OnClicked(this, &SUTHomePanel::OnSpectateLanClicked, NewServer)
												//.IsEnabled(TAttribute<bool>::Create(TAttribute<bool>::FGetter::CreateSP(InItem.Get(), &FTrackedMatch::CanSpectate, PlayerOwner)))
											]
										]
									]
								]
								+SHorizontalBox::Slot().FillWidth(1.0)
								[
									SNew(SVerticalBox)
									+SVerticalBox::Slot().AutoHeight()
									[
										SNew(STextBlock)
										.Text(ServerName)
										.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Large.Bold")
										.ColorAndOpacity(FLinearColor(1.0f, 0.412f, 0.027f, 1.0f))
									]
									+SVerticalBox::Slot().AutoHeight()
									[									
										SNew(STextBlock)
										.Text(ServerInfo)
										.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Tiny")
									]
								]

							]
						]
					]
				]
			];
		}
	}
}


void SUTHomePanel::OnFindLANSessionsComplete(bool bWasSuccessful)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	
	IOnlineSessionPtr OnlineSessionInterface;
	if (OnlineSubsystem) OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();

	if (OnlineSessionInterface.IsValid())
	{
		OnlineSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindLanSessionCompleteDelegate);
	}

	if (bWasSuccessful)
	{
		bool bNeedsRefresh = false;
		for (int32 ServerIndex = 0; ServerIndex < LanSearchSettings->SearchResults.Num(); ServerIndex++)
		{
			TSharedPtr<FServerData> NewServer = SUTServerBrowserPanel::CreateNewServerData(LanSearchSettings->SearchResults[ServerIndex]);
			bool bFound = false;
			for (int32 i = 0; i < LanMatches.Num(); i++)
			{
				if (NewServer->GetId() == LanMatches[i]->GetId())
				{
					bFound = true;
					break;
				}
			}

			if (!bFound)
			{
				LanMatches.Add(NewServer);
				bNeedsRefresh = true;
			}
		}

		if (bNeedsRefresh)
		{
			RebuildLanBox();
		}
	}
}

void SUTHomePanel::PingLanServers()
{
	for (int32 ServerIndex =0; ServerIndex < LanMatches.Num(); ServerIndex++)
	{
		UE_LOG(UT,Log,TEXT("PING"));
		// Build the beacon
		TSharedPtr<FServerData> ServerToPing = LanMatches[ServerIndex];
		AUTServerBeaconClient* Beacon = PlayerOwner->GetWorld()->SpawnActor<AUTServerBeaconClient>(AUTServerBeaconClient::StaticClass());
		if (Beacon && !ServerToPing->BeaconIP.IsEmpty())
		{
			Beacon->SetBeaconConnectionTimeout(1.5f);
			Beacon->OnServerRequestResults = FServerRequestResultsDelegate::CreateSP(this, &SUTHomePanel::OnPingResult );
			Beacon->OnServerRequestFailure = FServerRequestFailureDelegate::CreateSP(this, &SUTHomePanel::OnPingFailure);
			FURL BeaconURL(nullptr, *ServerToPing->BeaconIP, TRAVEL_Absolute);
			Beacon->InitClient(BeaconURL);
			PingTrackers.Add(FServerPingTracker(ServerToPing, Beacon));
		}

	}
}

void SUTHomePanel::OnPingResult(AUTServerBeaconClient* Sender, FServerBeaconInfo ServerInfo)
{
	for (int32 i=0; i < PingTrackers.Num(); i++)
	{
		UE_LOG(UT,Log,TEXT("PONG"));

		if (PingTrackers[i].Beacon == Sender)
		{
			PingTrackers.RemoveAt(i);
			break;
		}
	}

	// TODO: Add code to update stats..
}

void SUTHomePanel::OnPingFailure(AUTServerBeaconClient* Sender)
{
	bool bNeedsRefresh = false;
	for (int32 i=0; i < PingTrackers.Num(); i++)
	{
		UE_LOG(UT,Log,TEXT("BAM"));

		if (PingTrackers[i].Beacon == Sender)
		{
			// Remove the server
			for (int32 j = 0; j < LanMatches.Num(); j++)
			{
				if (PingTrackers[i].Server->GetId() == LanMatches[j]->GetId())
				{
					LanMatches.RemoveAt(j);
					bNeedsRefresh = true;
					break;
				}
			}
			PingTrackers.RemoveAt(i);
			break;
		}
	}

	if (bNeedsRefresh)
	{
		RebuildLanBox();
	}
}


FLinearColor SUTHomePanel::GetFadeColor() const
{
	FLinearColor Color = FLinearColor::White;
	Color.A = FMath::Clamp<float>(1.0 - (AnnouncmentFadeTimer / 0.8f),0.0f, 1.0f);
	return Color;
}

FSlateColor SUTHomePanel::GetFadeBKColor() const
{
	FLinearColor Color = FLinearColor::White;
	Color.A = FMath::Clamp<float>(1.0 - (AnnouncmentFadeTimer / 0.8f),0.0f, 1.0f);
	return Color;
}


void SUTHomePanel::BuildAnnouncement()
{
	TSharedPtr<SVerticalBox> SlotBox;
	int32 Day = 0;
	int32 Month = 0;
	int32 Year = 0;

	FDateTime Now = FDateTime::UtcNow();
	AnnouncmentFadeTimer = 0.8;

	// DEBUG Announcement.. 
	// PlayerOwner->MCPAnnouncements.Announcements.Add(FMCPAnnouncement(Now, Now , TEXT("Told you not to uncomment!"), TEXT("https://www.epicgames.com/unrealtournament/flag-run"), 400.0f, true));

	if (PlayerOwner->MCPAnnouncements.Announcements.Num() > 0)
	{
		for (int32 i=0; i < PlayerOwner->MCPAnnouncements.Announcements.Num(); i++)
		{

			FDateTime Start = PlayerOwner->MCPAnnouncements.Announcements[i].StartDate;
			FDateTime End = PlayerOwner->MCPAnnouncements.Announcements[i].EndDate;

			if (PlayerOwner->MCPAnnouncements.Announcements[i].bHasAudio)
			{
				// Temporarily change audio level
				UUTAudioSettings* AudioSettings = UUTAudioSettings::StaticClass()->GetDefaultObject<UUTAudioSettings>();
				if (AudioSettings)
				{
					AudioSettings->SetSoundClassVolume(EUTSoundClass::Music,0.0f);
				}
			}

			if ( Now >= Start && Now <= End)
			{
				AnnouncementBox->AddSlot().AutoHeight().Padding(0.0f,10.0f,0.0f,0.0f)
				[
					SNew(SBorder)
					.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.SuperDark"))
					.ColorAndOpacity(this, &SUTHomePanel::GetFadeColor)
					.BorderBackgroundColor(this, &SUTHomePanel::GetFadeBKColor)
					.HAlign(HAlign_Right)
					[
						SNew(STextBlock)
						.Text(FText::FromString(PlayerOwner->MCPAnnouncements.Announcements[i].Title))
						.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Tiny.Bold")
						.ColorAndOpacity(FLinearColor::Yellow)
					]
				];

				TSharedPtr<SUTWebBrowserPanel> Browser;

				AnnouncementBox->AddSlot().AutoHeight().Padding(0.0f,0.0f,0.0f,0.0f)
				[
					SNew(SBorder)
					.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
					.ColorAndOpacity(this, &SUTHomePanel::GetFadeColor)
					.BorderBackgroundColor(this, &SUTHomePanel::GetFadeBKColor)
					[
						SAssignNew(SlotBox,SVerticalBox)
						+SVerticalBox::Slot()
						.Padding(5.0,5.0,5.0,5.0)
						.AutoHeight()
						[
							SNew(SBox).HeightOverride(PlayerOwner->MCPAnnouncements.Announcements[i].MinHeight)
							[
								SAssignNew(Browser,SUTWebBrowserPanel, PlayerOwner)
								.InitialURL(PlayerOwner->MCPAnnouncements.Announcements[i].AnnouncementURL)
								.ShowControls(false)
							]	
						]
					]
				];

				AnnouncementBrowserList.Add(Browser);

			}
		}
	}
}


TSharedRef<SWidget> SUTHomePanel::BuildHomePanel()
{

	FString BuildVersion = FString::FromInt(FNetworkVersion::GetLocalNetworkVersion());

	TSharedPtr<SOverlay> Final;
	SAssignNew(Final, SOverlay)

		// Announcement box
		+SOverlay::Slot()
		.Padding(920.0,0.0,0.0,32.0)
		.VAlign(VAlign_Bottom)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight()
			.VAlign(VAlign_Bottom)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox).WidthOverride(940)
					[
						SAssignNew(AnnouncementBox, SVerticalBox)
					]
				]
			]
			+SVerticalBox::Slot().AutoHeight()
			.AutoHeight()
			.VAlign(VAlign_Bottom)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox).WidthOverride(940).MaxDesiredHeight(400)
					[
						SNew(SScrollBox)
						.Orientation(EOrientation::Orient_Vertical)
						+SScrollBox::Slot()
						[
							SAssignNew(LanBox, SVerticalBox)
						]
					]
				]
			]
		]

		+SOverlay::Slot()
		.Padding(64.0,50.0,6.0,32.0)
		[
			SNew(SVerticalBox)

			// Main Button

			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(800).HeightOverride(143)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot().AutoWidth()
							[
								SNew(SUTImage)
								.Image(SUTStyle::Get().GetBrush("UT.Logo.Loading")).WidthOverride(400).HeightOverride(143)	
							]
							+SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Bottom).Padding(0.0f,0.0f,0.0f,10.0f)
							[
								SNew(SBox).WidthOverride(400).HAlign(HAlign_Right)
								[
									SNew(STextBlock)
									.Text(FText::Format(NSLOCTEXT("Common","VersionFormat","Build {0}"), FText::FromString(BuildVersion)))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small.Bold")
									.ColorAndOpacity(FLinearColor(1.0f, 0.412f, 0.027f, 1.0f))
								]
							]
						]
					]
				]
			]

			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox).WidthOverride(800).HeightOverride(270)
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot()
						[
							BuildHomePanelButton(EMenuCommand::MC_Tutorial, TEXT("UT.HomePanel.BasicTraining"), NSLOCTEXT("SUTHomePanel","BasicTraining","Basic Training"))
						]
						+ SHorizontalBox::Slot().Padding(25.0f,0.0f,25.0f,0.0f)
						[
							SNew(SOverlay)
							+SOverlay::Slot()
							[
								BuildHomePanelButton(EMenuCommand::MC_Challenges, TEXT("UT.HomePanel.Challenges"), NSLOCTEXT("SUTHomePanel","Challenges","Challenges"))
							]
							+SOverlay::Slot()
							[
								SAssignNew(NewChallengeBox, SVerticalBox)
								.Visibility(this, &SUTHomePanel::ShowNewChallengeImage)
								+SVerticalBox::Slot()
								.AutoHeight()
								[
									SNew(SHorizontalBox)
									+SHorizontalBox::Slot().HAlign(HAlign_Center)
									[
										SAssignNew(NewChallengeImage, SUTImage)
										.Image(SUTStyle::Get().GetBrush("UT.HomePanel.ChallengesNewIcon"))
										.WidthOverride(72.0f).HeightOverride(72.0f)
										.RenderTransformPivot(FVector2D(0.5f, 0.5f))
									]
								]
							]
						]
						+ SHorizontalBox::Slot()
						[
							BuildHomePanelButton(EMenuCommand::MC_InstantAction, TEXT("UT.HomePanel.vsBots"), NSLOCTEXT("SUTHomePanel","InstantAction","vs. Bots"))
						]
					]
				]
			]

			// PLAY

			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(0.0,30.0)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox).WidthOverride(800).HeightOverride(270)
					[
						SNew(SOverlay)
						+SOverlay::Slot()
						[
							SAssignNew(QuickPlayBox, SHorizontalBox)
							.Visibility(this, &SUTHomePanel::QuickplayVis,0)
						]
						+SOverlay::Slot()
						[
							SAssignNew(NoQuickPlayBox, SVerticalBox)
							.Visibility(this, &SUTHomePanel::QuickplayVis,1)
							+SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text(FText::FromString(TEXT("QUICK PLAY")))
								.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium.Bold")
							]
							+SVerticalBox::Slot().AutoHeight()
							[
								SNew(STextBlock)
								.Text(this, &SUTHomePanel::GetQuickPlayUnavailableText)
								.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Tween")
							]
						]
					]
				]
			]

			+ SVerticalBox::Slot()
			.Padding(0.0,0.0)
			.AutoHeight()
			[
				BuildRankedPlaylist()
			]
		]
		+SOverlay::Slot()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot().FillHeight(1.0f).HAlign(HAlign_Right)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Top)
				[
					SAssignNew(PartyBox, SUTPartyWidget, PlayerOwner->PlayerController)
				]
			]
		]
		+SOverlay::Slot()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot().FillHeight(1.0f).HAlign(HAlign_Right).Padding(0.0f,142.0f,0.0f,0.0f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot().FillWidth(1.0f).VAlign(VAlign_Top)
				[
					SAssignNew(PartyInviteBox, SUTPartyInviteWidget, PlayerOwner->PlayerController)
				]
			]
		];

	BuildQuickPlayPanel();

	return Final.ToSharedRef();
}

FText SUTHomePanel::GetQuickPlayUnavailableText() const
{
	if (PlayerOwner->IsLoggedIn())
	{
		return NSLOCTEXT("SUTHomePanel","QPNotAvailableLoggedIn","The quick play service is currently unavailable.");
	}
	else
	{
		return NSLOCTEXT("SUTHomePanel","QPNotAvailableNotLoggedIn","Not logged in!  Quick play is disabled.");
	}

}

EVisibility SUTHomePanel::QuickplayVis(int32 Index) const
{
	EVisibility Result = EVisibility::Collapsed;
	UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(PlayerOwner->GetGameInstance());
	if (UTGameInstance && UTGameInstance->GetPlaylistManager())
	{
		int32 NoQuickPlays = UTGameInstance->GetPlaylistManager()->HowManyQuickPlay();
		if (!PlayerOwner->IsLoggedIn()) NoQuickPlays = 0;
		if (Index == 0)
		{
			Result = NoQuickPlays > 0 ? EVisibility::Visible : EVisibility::Collapsed;
		}	
		else
		{
			Result = NoQuickPlays == 0 ? EVisibility::Visible : EVisibility::Collapsed;
		}
	}

	return Result;
}

void SUTHomePanel::BuildQuickplayButton(FName BackgroundTexture, FText Caption, int32 PlaylistId, float Padding)
{
	QuickPlayBox->AddSlot()
		.Padding(Padding,0.0,Padding,0.0)
		.AutoWidth()
		[
			SNew(SBox).WidthOverride(250)
			[
				SNew(SUTButton)
				.ButtonStyle(SUTStyle::Get(), "UT.HomePanel.Button")
				.bSpringButton(true)
				.OnClicked(this, &SUTHomePanel::QuickPlayClick, PlaylistId)
				.ContentPadding(FMargin(2.0f, 2.0f, 2.0f, 2.0f))
				[
					SNew(SOverlay)
					+SOverlay::Slot()
					[
						SNew(SUTImage)
						.Image(SUTStyle::Get().GetBrush(BackgroundTexture))
						.WidthOverride(250)
						.HeightOverride(270)
					]
					+SOverlay::Slot()
					.VAlign(VAlign_Bottom)
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Fill)
						[
							SNew(SBorder)
							.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Shaded"))
							.Padding(FMargin(0.0f,0.0f,0.0f,0.0f))
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Center)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("QUICK PLAY")))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small.Bold")
								]
								+SVerticalBox::Slot()
								.AutoHeight()
								.HAlign(HAlign_Center)
								[
									SNew(STextBlock)
									.Text(Caption)
									.ColorAndOpacity(FLinearColor(1.0f, 0.412f, 0.027f, 1.0f))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
								]
							]
						]
					]
				]
			]
		];
}

TSharedRef<SBox> SUTHomePanel::BuildHomePanelButton(FName ButtonTag, FName BackgroundTexture, const FText& Caption)
{
	TSharedPtr<SBox> Box;
	SAssignNew(Box, SBox).WidthOverride(250)
	[
		SNew(SUTButton)
		.ButtonStyle(SUTStyle::Get(), "UT.HomePanel.Button")
		.bSpringButton(true)
		.OnClicked(this, &SUTHomePanel::MainButtonClick, ButtonTag)
		.ToolTipText( TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateUObject(PlayerOwner.Get(), &UUTLocalPlayer::GetMenuCommandTooltipText, ButtonTag) ) )
		.ContentPadding(FMargin(2.0f, 2.0f, 2.0f, 2.0f))
		[
			SNew(SOverlay)
			+SOverlay::Slot()
			[
				SNew(SUTImage)
				.Image(SUTStyle::Get().GetBrush(BackgroundTexture))
				.WidthOverride(250)
				.HeightOverride(270)
			]
			+SOverlay::Slot()
			.VAlign(VAlign_Bottom)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Fill)
				[
					SNew(SBorder)
					.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Shaded"))
					.Padding(FMargin(0.0f,0.0f,0.0f,0.0f))
					[
						SNew(SVerticalBox)
						+SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(FText::FromString(TEXT("SINGLE PLAYER")))
							.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small.Bold")
						]

						+SVerticalBox::Slot()
						.AutoHeight()
						.HAlign(HAlign_Center)
						[
							SNew(STextBlock)
							.Text(Caption)
							.ColorAndOpacity(FLinearColor(1.0f, 0.412f, 0.027f, 1.0f))
							.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
						]
					]
				]
			]
		]
	];

	return Box.ToSharedRef();
}



FReply SUTHomePanel::QuickPlayClick(int32 PlaylistId)
{
	TSharedPtr<SUTMainMenu> MainMenu = StaticCastSharedPtr<SUTMainMenu>(GetParentWindow());
	if (MainMenu.IsValid())
	{
		MainMenu->QuickPlay(PlaylistId);
	}

	return FReply::Handled();
}

FReply SUTHomePanel::MainButtonClick(FName ButtonTag)
{
	TSharedPtr<SUTMainMenu> MainMenu = StaticCastSharedPtr<SUTMainMenu>(GetParentWindow());
	if (MainMenu.IsValid())
	{
		if (ButtonTag == EMenuCommand::MC_Tutorial) MainMenu->OpenTutorialMenu();
		else if (ButtonTag == EMenuCommand::MC_Challenges) MainMenu->ShowGamePanel();
		else if (ButtonTag == EMenuCommand::MC_InstantAction) MainMenu->ShowCustomGamePanel();
	}

	return FReply::Handled();
}

EVisibility SUTHomePanel::ShowNewChallengeImage() const
{
	if (PlayerOwner.IsValid() && PlayerOwner->MCPPulledData.bValid)
	{
		if (PlayerOwner->ChallengeRevisionNumber< PlayerOwner->MCPPulledData.ChallengeRevisionNumber)
		{
			return EVisibility::HitTestInvisible;
		}
	}

	return EVisibility::Collapsed;
}



void SUTHomePanel::OnHidePanel()
{
	PlayerOwner->GetWorld()->GetTimerManager().ClearTimer(LanTimerHandle);

	AnnouncementBox->ClearChildren();
	for (int32 i=0; i < AnnouncementBrowserList.Num(); i++)
	{
		if (AnnouncementBrowserList[i].IsValid())
		{
			AnnouncementBrowserList[i]->Browse(TEXT(""));
		}
	}
	AnnouncementBrowserList.Empty();

	UUTGameUserSettings* UserSettings = Cast<UUTGameUserSettings>(GEngine->GetGameUserSettings());
	UserSettings->SetSoundClassVolume(EUTSoundClass::Music, UserSettings->GetSoundClassVolume(EUTSoundClass::Music));

	bClosing = true;
	if (AnimWidget.IsValid())
	{
		AnimWidget->Animate(FVector2D(0.0f, 0.0f), FVector2D(-100.0f, 0.0f), 1.0f, 0.0f, 0.3f);
	}
	else
	{
		SUTPanelBase::OnHidePanel();
	}
}

void SUTHomePanel::PanelClosed()
{
	for (int32 i=0; i < AnnouncementBrowserList.Num(); i++)
	{
		if (AnnouncementBrowserList[i].IsValid())
		{
			AnnouncementBrowserList[i]->Browse(TEXT("about:blank"));
		}
	}
	AnnouncementBrowserList.Empty();

	bClosing = true;
	AnimEnd();
}



void SUTHomePanel::AnimEnd()
{
	if (bClosing)
	{
		bClosing = false;
		TSharedPtr<SWidget> Panel = this->AsShared();
		ParentWindow->PanelHidden(Panel);
		ParentWindow.Reset();
	}
}

TSharedRef<SWidget> SUTHomePanel::BuildRankedPlaylist()
{
	UUTGameInstance* UTGameInstance = CastChecked<UUTGameInstance>(PlayerOwner->GetGameInstance());
	if (UTGameInstance && UTGameInstance->GetPlaylistManager() && PlayerOwner->IsLoggedIn())
	{
		TSharedPtr<SHorizontalBox> FinalBox;
		SAssignNew(FinalBox, SHorizontalBox);

		FinalBox->AddSlot()
		.AutoWidth()
		.Padding(FMargin(2.0f, 0.0f))
		[
			SNew(SUTImage)
			.Image(SUTStyle::Get().GetBrush("UT.HomePanel.NewFragCenter.Transparent"))
			.WidthOverride(196).HeightOverride(196)
		];

		FinalBox->AddSlot().AutoWidth().VAlign(VAlign_Bottom)
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot().AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString(TEXT("League Matches")))
				.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium.Bold")

			]
			+SVerticalBox::Slot().AutoHeight().Padding(0.0f,5.0f,0.0f,0.0f)
			[
				SAssignNew(RankedBox, SHorizontalBox)
				.Visibility(this, &SUTHomePanel::RankedBoxVisibile)
			]
		];
			
		TArray<FPlaylistItem*> RankedPlaylist;
		UTGameInstance->GetPlaylistManager()->GetPlaylist(true, RankedPlaylist);

		int32 ButtonCount = 0;
		for (int32 i = 0; i < RankedPlaylist.Num(); i++)
		{
			int32 PlaylistId = RankedPlaylist[i]->PlaylistId;
			FUTGameRuleset* Ruleset = UTGameInstance->GetPlaylistManager()->GetRuleset(PlaylistId);
			if (Ruleset == nullptr || !PlayerOwner->IsRankedMatchmakingEnabled(PlaylistId)) continue;

			int32 MaxTeamCount = 0;
			int32 MaxTeamSize = 0;
			int32 MaxPartySize = 0;

			UTGameInstance->GetPlaylistManager()->GetMaxTeamInfoForPlaylist(PlaylistId, MaxTeamCount, MaxTeamSize,MaxPartySize);

			FString PlaylistPlayerCount = FString::Printf(TEXT("%dv%d"), MaxTeamSize, MaxTeamSize);
			FName SlateBadgeName = FName(*RankedPlaylist[i]->SlateBadgeName);
			if (SlateBadgeName == NAME_None) SlateBadgeName = FName(TEXT("UT.HomePanel.DMBadge"));

			ButtonCount++;
			RankedBox->AddSlot()
			.AutoWidth()

			.Padding(FMargin(2.0f, 0.0f))
			[
				SNew(SUTButton)
				.ButtonStyle(SUTStyle::Get(), "UT.HomePanel.Button")
				.bSpringButton(true)
				.OnClicked(FOnClicked::CreateSP(this, &SUTHomePanel::OnStartRankedPlaylist, PlaylistId))
				.ToolTip(SUTUtils::CreateTooltip(NSLOCTEXT("SUTHomePanel","Ranked","Play a ranked match and earn XP.")))
				.Visibility(this, &SUTHomePanel::RankedButtonVis, PlaylistId)
				[

					SNew(SOverlay)
					+ SOverlay::Slot()
					[
						SNew(SBox)
						.WidthOverride(128)
						.HeightOverride(128)
						[
							SNew(SImage)
							.Image(SUTStyle::Get().GetBrush(SlateBadgeName))
						]
					]
					+ SOverlay::Slot()
					[
						SNew(SBox)
						.WidthOverride(128)
						.HeightOverride(128)
						[
							SNew(SVerticalBox)
							+SVerticalBox::Slot()
							.HAlign(HAlign_Center)
							.VAlign(VAlign_Top)
							.FillHeight(1.0f)
							[
								SNew(STextBlock)
								.Text(FText::FromString(Ruleset->Title))
								.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small.Bold")
							]
							+SVerticalBox::Slot()
							.HAlign(HAlign_Fill)
							.VAlign(VAlign_Center)
							.AutoHeight()
							[
								SNew(SBorder)
								.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Shaded"))
								[
									SNew(SVerticalBox)
									+SVerticalBox::Slot()
									.AutoHeight()
									.HAlign(HAlign_Center)
									.VAlign(VAlign_Center)
									.Padding(0.0f, 0.0f, 0.0f, 2.0f)
									[
										SNew(STextBlock)
										.Text(FText::FromString(PlaylistPlayerCount))
										.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium.Bold")
									]
								]
							]
						]
					]
				]
			];					
		}

		if (ButtonCount > 0)
		{
			return FinalBox.ToSharedRef();
		}

		FinalBox->ClearChildren();
	}
	return SNullWidget::NullWidget;
}

FReply SUTHomePanel::OnStartRankedPlaylist(int32 PlaylistId)
{
	if (PlayerOwner.IsValid())
	{
		if (!PlayerOwner->IsRankedMatchmakingEnabled(PlaylistId))
		{
			PlayerOwner->ShowToast(NSLOCTEXT("SUTPartyWidget", "RankedPlayDisabled", "This playlist is not currently active"));
			return FReply::Handled();
		}

		UMatchmakingContext* MatchmakingContext = Cast<UMatchmakingContext>(UBlueprintContextLibrary::GetContext(PlayerOwner->GetWorld(), UMatchmakingContext::StaticClass()));
		if (MatchmakingContext)
		{
			MatchmakingContext->StartMatchmaking(PlaylistId);
		}
	}

	return FReply::Handled();
}


FReply SUTHomePanel::OnJoinLanClicked(TSharedPtr<FServerData> Server)
{
	TSharedPtr<SUTServerBrowserPanel> Browser = PlayerOwner->GetServerBrowser(true);
	if (Browser.IsValid() && Server.IsValid())
	{
		Browser->ConnectTo(*Server, false);	
	}

	return FReply::Handled();
}

FReply SUTHomePanel::OnSpectateLanClicked(TSharedPtr<FServerData> Server)
{
	TSharedPtr<SUTServerBrowserPanel> Browser = PlayerOwner->GetServerBrowser(true);
	if (Browser.IsValid() && Server.IsValid())
	{
		Browser->ConnectTo(*Server, true);	
	}

	return FReply::Handled();

}

EVisibility SUTHomePanel::RankedBoxVisibile() const
{
	UUTGameInstance* UTGameInstance = CastChecked<UUTGameInstance>(PlayerOwner->GetGameInstance());
	if (UTGameInstance && UTGameInstance->GetPlaylistManager() && PlayerOwner->IsLoggedIn())
	{
		int32 NumPlaylists = UTGameInstance->GetPlaylistManager()->GetNumPlaylists();
		if (NumPlaylists > 0)
		{
			return EVisibility::Visible;
		}
	}

	return EVisibility::Collapsed;
}

EVisibility SUTHomePanel::RankedButtonVis(int32 PlaylistId) const
{
	return ( PlayerOwner->IsRankedMatchmakingEnabled(PlaylistId) ) ? EVisibility::Visible : EVisibility::Collapsed;
}

void SUTHomePanel::BuildQuickPlayPanel()
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
				float Padding = (i & 0x01) == 0x01 ? 25.0f : 0.0f;
				BuildQuickplayButton(FName(*Item->SlateBadgeName), FText::FromString(Ruleset->Title), Item->PlaylistId, Padding);
			}
		}
	}				
}


#endif