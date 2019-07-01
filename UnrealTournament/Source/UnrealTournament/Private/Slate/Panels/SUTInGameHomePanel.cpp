
// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "UTGameViewportClient.h"

#include "SlateBasics.h"
#include "../Widgets/SUTScaleBox.h"
#include "Slate/SlateGameResources.h"
#include "SUTInGameHomePanel.h"
#include "../Widgets/SUTBorder.h"
#include "../Widgets/SUTButton.h"
#include "UTConsole.h"
#include "SUTChatEditBox.h"
#include "UTVoiceChatFeature.h"


#if !UE_SERVER

const int32 ECONTEXT_COMMAND_ShowPlayerCard 	= 0;
const int32 ECONTEXT_COMMAND_MutePlayer 		= 255;
const int32 ECONTEXT_COMMAND_FriendRequest 		= 1;
const int32 ECONTEXT_COMMAND_KickVote			= 2;
const int32 ECONTEXT_COMMAND_AdminKick			= 3;
const int32 ECONTEXT_COMMAND_AdminBan			= 4;
const int32 ECONTEXT_COMMAND_ReportPlayer		= 5;

void SUTInGameHomePanel::ConstructPanel(FVector2D CurrentViewportSize)
{
	Tag = FName(TEXT("InGameHomePanel"));
	bCloseOnSubmit = false;


	AUTGameMode* Game = PlayerOwner->GetWorld()->GetAuthGameMode<AUTGameMode>();
	AUTGameState* GS = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
	AUTPlayerState* PS = PlayerOwner->PlayerController ? Cast<AUTPlayerState>(PlayerOwner->PlayerController->PlayerState) : NULL;
	bool bIsSpectator = PS && PS->bOnlySpectator;

	TSharedPtr<SVerticalBox> MatchBox;
	TSharedPtr<SHorizontalBox> MatchButtonBox;

	bShowingContextMenu = false;
	this->ChildSlot
	.VAlign(VAlign_Fill)
	.HAlign(HAlign_Fill)
	[
		SNew(SOverlay)
		+SOverlay::Slot()
		.VAlign(VAlign_Bottom)
		.HAlign(HAlign_Fill)
		[
			SAssignNew(MatchBox, SVerticalBox)
			+SVerticalBox::Slot().AutoHeight().Padding(0.0f,0.0f,0.0f,0.0f)
			[
				SNew(SBox).HeightOverride(68.0f)
				[
					SNew(SUTBorder)
					.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
					.HAlign(HAlign_Center).VAlign(VAlign_Center)
					[
						SAssignNew(MatchButtonBox, SHorizontalBox)
						.Visibility(this, &SUTInGameHomePanel::GetMatchButtonVis)
					]
				]
			]
		]
		+SOverlay::Slot()
		.VAlign(VAlign_Fill)
		.HAlign(HAlign_Fill)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0)
			.HAlign(HAlign_Fill)
			[
				SAssignNew(SubMenuOverlay, SOverlay)
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).HeightOverride(42).WidthOverride(450)
					[
						SAssignNew(ChatBar, SUTChatBar, PlayerOwner)
						.InitialChatDestination(ChatDestinations::Local)
					]
				] 
			]
		]

	];

	if (SubMenuOverlay.IsValid())
	{
		SubMenuOverlay->AddSlot(0)
		[
			// Allow children to place things over chat....
			SAssignNew(ChatArea,SVerticalBox)
		];
	}

	if (GS && (GS->GetMatchState() == MatchState::WaitingToStart))
	{
		if (GS->GetNetMode() == NM_Standalone)
		{
			MatchButtonBox->AddSlot().AutoWidth().Padding(5.0f,0.0f,0.0f,0.0f)
			[
				SNew(SOverlay)
				+SOverlay::Slot()
				[
					SAssignNew(MatchButton, SUTButton)
					.ButtonStyle(SUTStyle::Get(), "UT.Button.Soft.Gold")
					.OnClicked(this, &SUTInGameHomePanel::OnReadyChangeClick)
					.ContentPadding(FMargin(25.0, 0.0, 25.0, 5.0))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(NSLOCTEXT("SUTMenuBase", "MenuBar_StartMatch", "START MATCH"))
							.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Large.Bold")
							.ColorAndOpacity(this, &SUTInGameHomePanel::GetMatchLabelColor)
						]
					]
				]
			];
		}
		else if (PS && PS->bIsWarmingUp)
		{
			MatchButtonBox->AddSlot().AutoWidth().Padding(5.0f,0.0f,0.0f,0.0f)
			[
				SAssignNew(MatchButton, SUTButton)
				.ButtonStyle(SUTStyle::Get(), "UT.Button.Soft.Gold")
				.OnClicked(this, &SUTInGameHomePanel::OnReadyChangeClick)
				.ContentPadding(FMargin(25.0, 0.0, 25.0, 5.0))
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot().AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("SUTMenuBase", "MenuBar_LeaveWarmup", "LEAVE WARM UP"))
						.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Large.Bold")
						.ColorAndOpacity(this, &SUTInGameHomePanel::GetMatchLabelColor)
					]
				]
			];
		}
		else if (!bIsSpectator)
		{
			MatchButtonBox->AddSlot().AutoWidth().Padding(5.0f,0.0f,0.0f,0.0f)
			[
				SNew(SOverlay)
				+SOverlay::Slot()
				[
					SAssignNew(MatchButton, SUTButton)
					.ButtonStyle(SUTStyle::Get(), "UT.Button.Soft.Gold")
					.OnClicked(this, &SUTInGameHomePanel::OnReadyChangeClick)
					.ContentPadding(FMargin(25.0, 0.0, 25.0, 5.0))
					[
						SNew(SHorizontalBox)
						+ SHorizontalBox::Slot().AutoWidth()
						.VAlign(VAlign_Center)
						[
							SNew(STextBlock)
							.Text(NSLOCTEXT("SUTMenuBase", "MenuBar_ChangeReady", "JOIN WARM UP"))
							.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Large.Bold")
							.ColorAndOpacity(this, &SUTInGameHomePanel::GetMatchLabelColor)
						]
					]
				]
			];
		}

		if (GS->bTeamGame && !bIsSpectator && GS->bAllowTeamSwitches)
		{
			MatchButtonBox->AddSlot().AutoWidth().Padding(5.0f,0.0f,0.0f,0.0f)
			[
				SAssignNew(ChangeTeamButton, SUTButton)
				.ButtonStyle(SUTStyle::Get(), "UT.Button.Soft")
				.OnClicked(this, &SUTInGameHomePanel::OnTeamChangeClick)
				.Visibility(this, &SUTInGameHomePanel::GetChangeTeamVisibility)
				.ContentPadding(FMargin(25.0,0.0,25.0,5.0))
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot().AutoWidth()
					.VAlign(VAlign_Center)
					[
						SNew(STextBlock)
						.Text(NSLOCTEXT("SUTMenuBase","MenuBar_ChangeTeam","CHANGE TEAM"))
						.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Large.Bold")
						.ColorAndOpacity(this, &SUTInGameHomePanel::GetChangeTeamLabelColor)
					]
				]
			];
		}			


	}
}

void SUTInGameHomePanel::OnShowPanel(TSharedPtr<SUTMenuBase> inParentWindow)
{
	SUTPanelBase::OnShowPanel(inParentWindow);
	AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	if (PC && PC->MyUTHUD)
	{
		PC->MyUTHUD->bForceScores = true;
		UUTScoreboard* SB = PC->MyUTHUD->GetScoreboard();
		if (SB)
		{
			SB->BecomeInteractive();
		}
		UUTHUDWidget_ReplayTimeSlider* ReplayTimeSlider = PC->MyUTHUD->GetReplayTimeSlider();
		if (ReplayTimeSlider)
		{
			ReplayTimeSlider->BecomeInteractive();
		}
	}
}

void SUTInGameHomePanel::OnHidePanel()
{
	SUTPanelBase::OnHidePanel();
	AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	if (PC && PC->MyUTHUD)
	{
		PC->MyUTHUD->bForceScores = false;
		UUTScoreboard* SB = PC->MyUTHUD->GetScoreboard();
		if (SB)
		{
			SB->BecomeNonInteractive();
		}
		UUTHUDWidget_ReplayTimeSlider* ReplayTimeSlider = PC->MyUTHUD->GetReplayTimeSlider();
		if (ReplayTimeSlider)
		{
			ReplayTimeSlider->BecomeNonInteractive();
		}
	}
}

// @Returns true if the mouse position is inside the viewport
bool SUTInGameHomePanel::GetGameMousePosition(FVector2D& MousePosition)
{
	// We need to get the mouse input but the mouse event only has the mouse in screen space.  We need it in viewport space and there
	// isn't a good way to get there.  So we punt and just get it from the game viewport.

	UUTGameViewportClient* GVC = Cast<UUTGameViewportClient>(PlayerOwner->ViewportClient);
	if (GVC)
	{
		return GVC->GetMousePosition(MousePosition);
	}
	return false;
}

void SUTInGameHomePanel::ShowContextMenu(UUTScoreboard* Scoreboard, FVector2D ContextMenuLocation, FVector2D ViewportBounds)
{
	if (bShowingContextMenu)
	{
		HideContextMenu();
	}
	AUTPlayerState* OwnerPlayerState = nullptr;
	if (PlayerOwner->PlayerController)
	{
		OwnerPlayerState = Cast<AUTPlayerState>(PlayerOwner->PlayerController->PlayerState);
	}

	if (Scoreboard == nullptr) return;
	
	TWeakObjectPtr<AUTPlayerState> SelectedPlayer = Scoreboard->GetSelectedPlayer();
	
	if (!SelectedPlayer.IsValid()) return;

	TSharedPtr<SVerticalBox> MenuBox;

	bShowingContextMenu = true;
	SubMenuOverlay->AddSlot(201)
	.Padding(FMargin(ContextMenuLocation.X, ContextMenuLocation.Y, 0, 0))
	[
		SNew(SHorizontalBox)
		+SHorizontalBox::Slot().AutoWidth()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot().AutoHeight()
			[
				SNew(SUTBorder)
				.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.SuperDark"))
				[
					SAssignNew(MenuBox, SVerticalBox)
				]
			]
		]
	];

	if (MenuBox.IsValid())
	{

		if (Scoreboard != nullptr)
		{
			TArray<FScoreboardContextMenuItem> ContextItems;
			Scoreboard->GetContextMenuItems(ContextItems);
			if (ContextItems.Num() > 0)
			{
				for (int32 i=0; i < ContextItems.Num(); i++)
				{
					// Add the show player card
					MenuBox->AddSlot()
					.AutoHeight()
					[
						SNew(SUTButton)
						.OnClicked(this, &SUTInGameHomePanel::ContextCommand, ContextItems[i].Id, SelectedPlayer)
						.ButtonStyle(SUTStyle::Get(),"UT.ContextMenu.Item")
						.Text(ContextItems[i].MenuText)
						.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Small")
					];
				}
			
				return;
			}
		}

		if (!SelectedPlayer->bIsABot)
		{
			// Add the show player card
			MenuBox->AddSlot()
			.AutoHeight()
			[
				SNew(SUTButton)
				.OnClicked(this, &SUTInGameHomePanel::ContextCommand, ECONTEXT_COMMAND_ShowPlayerCard, SelectedPlayer)
				.ButtonStyle(SUTStyle::Get(),"UT.ContextMenu.Item")
				.Text(NSLOCTEXT("SUTInGameHomePanel","ShowPlayerCard","Show Player Card"))
				.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Small")
			];
		}
		else
		{
			// Add the show player card
			MenuBox->AddSlot()
			.AutoHeight()
			[
				SNew(SUTButton)
				.OnClicked(this, &SUTInGameHomePanel::ContextCommand, 9000, SelectedPlayer)
				.ButtonStyle(SUTStyle::Get(),"UT.ContextMenu.Item")
				.Text(NSLOCTEXT("SUTInGameHomePanel","ShowPlayerCardBot","AI Player Cards Offline"))
				.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Small")
			];
		
		}

		// If we are in a netgame, show online options.
		if ( PlayerOwner->GetWorld()->GetNetMode() == ENetMode::NM_Client)
		{
			if (PlayerOwner->PlayerController == nullptr || SelectedPlayer.Get() != PlayerOwner->PlayerController->PlayerState)
			{
				if (!SelectedPlayer->bReported && !SelectedPlayer->bIsABot)
				{
					// Report a player
					MenuBox->AddSlot()
					.AutoHeight()
					[
						SNew(SUTButton)
						.OnClicked(this, &SUTInGameHomePanel::ContextCommand, ECONTEXT_COMMAND_ReportPlayer, SelectedPlayer)
						.ButtonStyle(SUTStyle::Get(),"UT.ContextMenu.Item")
						.Text(NSLOCTEXT("SUTInGameHomePanel","ReportAbuse","Report Abuse"))
						.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Small")
					];
				}

				AUTGameState* UTGameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();

				if (!SelectedPlayer->bIsABot && SelectedPlayer.Get() != PlayerOwner->PlayerController->PlayerState)
				{
					// Mute Player
					MenuBox->AddSlot()
						.AutoHeight()
						[
							SNew(SUTButton)
							.OnClicked(this, &SUTInGameHomePanel::ContextCommand, ECONTEXT_COMMAND_MutePlayer, SelectedPlayer)
							.ButtonStyle(SUTStyle::Get(), "UT.ContextMenu.Item")
							.Text(this, &SUTInGameHomePanel::GetMuteLabelText)
							.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small")
						];
				}

				MenuBox->AddSlot()
				.AutoHeight()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot().Padding(FMargin(10.0,0.0,10.0,0.0))
					[
						SNew(SBox).HeightOverride(3)
						[
							SNew(SImage)
							.Image(SUTStyle::Get().GetBrush("UT.HeaderBackground.SuperDark"))
						]
					]
				];

				if (!SelectedPlayer->bIsABot && !PlayerOwner->IsAFriend(SelectedPlayer->UniqueId))
				{
					MenuBox->AddSlot()
					.AutoHeight()
					[
						SNew(SUTButton)
						.OnClicked(this, &SUTInGameHomePanel::ContextCommand, ECONTEXT_COMMAND_FriendRequest, SelectedPlayer)
						.ButtonStyle(SUTStyle::Get(),"UT.ContextMenu.Item")
						.Text(NSLOCTEXT("SUTInGameHomePanel","SendFriendRequest","Send Friend Request"))
						.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Small")
					];
				}

				if (OwnerPlayerState && 
						!UTGameState->bDisableVoteKick && 
						!OwnerPlayerState->bOnlySpectator && 
						!SelectedPlayer->bIsABot &&
						(!UTGameState->bOnlyTeamCanVoteKick || UTGameState->OnSameTeam(OwnerPlayerState, SelectedPlayer.Get()))
					)
				{
					MenuBox->AddSlot()
					.AutoHeight()
					[
						SNew(SUTButton)
						.OnClicked(this, &SUTInGameHomePanel::ContextCommand, ECONTEXT_COMMAND_KickVote, SelectedPlayer)
						.ButtonStyle(SUTStyle::Get(),"UT.ContextMenu.Item")
						.Text(NSLOCTEXT("SUTInGameHomePanel","VoteToKick","Vote to Kick"))
						.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Small")
					];
				}
			}

			if (SelectedPlayer != OwnerPlayerState)
			{			
				AUTGameState* UTGameState= PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
				bool bHasAdminPower = (OwnerPlayerState && OwnerPlayerState->bIsRconAdmin);
				bHasAdminPower = bHasAdminPower || (UTGameState && UTGameState->GetMatchState() == MatchState::WaitingToStart && UTGameState->HostIdString == OwnerPlayerState->UniqueId.ToString());

				if (bHasAdminPower)
				{
					MenuBox->AddSlot()
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot().Padding(FMargin(10.0,0.0,10.0,0.0))
						[
							SNew(SBox).HeightOverride(3)
							[
								SNew(SImage)
								.Image(SUTStyle::Get().GetBrush("UT.HeaderBackground.SuperDark"))
							]
						]
					];

					MenuBox->AddSlot()
					.AutoHeight()
					[
						SNew(SUTButton)
						.OnClicked(this, &SUTInGameHomePanel::ContextCommand, ECONTEXT_COMMAND_AdminKick, SelectedPlayer)
						.ButtonStyle(SUTStyle::Get(),"UT.ContextMenu.Item")
						.Text(NSLOCTEXT("SUTInGameHomePanel","AdminKick","Admin Kick"))
						.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Small")
					];

					if (OwnerPlayerState && OwnerPlayerState->bIsRconAdmin)
					{
						MenuBox->AddSlot()
						.AutoHeight()
						[
							SNew(SUTButton)
							.OnClicked(this, &SUTInGameHomePanel::ContextCommand, ECONTEXT_COMMAND_AdminBan, SelectedPlayer)
							.ButtonStyle(SUTStyle::Get(),"UT.ContextMenu.Item")
							.Text(NSLOCTEXT("SUTInGameHomePanel","AdminBan","Admin Ban"))
							.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Small")
						];
					}
				}
			}
		}
	}
}

void SUTInGameHomePanel::HideContextMenu()
{
	if (bShowingContextMenu)
	{
		SubMenuOverlay->RemoveSlot(201);
		bShowingContextMenu = false;
	}
}

FReply SUTInGameHomePanel::ContextCommand(int32 CommandId, TWeakObjectPtr<AUTPlayerState> TargetPlayerState)
{
	HideContextMenu();
	if (TargetPlayerState.IsValid())
	{
		AUTPlayerState* MyPlayerState =  Cast<AUTPlayerState>(PlayerOwner->PlayerController->PlayerState);
		AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);

		if (MyPlayerState && PC)
		{
			UUTScoreboard* SB = PC->MyUTHUD->GetScoreboard();
			if (SB && SB->HandleContextCommand(CommandId, TargetPlayerState.Get()))
			{
				return FReply::Handled();
			}

			switch (CommandId)
			{
				case ECONTEXT_COMMAND_ShowPlayerCard:	PlayerOwner->ShowPlayerInfo(TargetPlayerState->UniqueId.ToString(), TargetPlayerState->PlayerName); break;
				case ECONTEXT_COMMAND_FriendRequest:	PlayerOwner->RequestFriendship(TargetPlayerState->UniqueId.GetUniqueNetId()); break;
				case ECONTEXT_COMMAND_KickVote: 
						if (TargetPlayerState != MyPlayerState)
						{
							PC->ServerRegisterBanVote(TargetPlayerState.Get());
						}
						break;
				case ECONTEXT_COMMAND_AdminKick:	PC->RconKick(TargetPlayerState->UniqueId.ToString(), false,TEXT("Kicked by Host/Admin")); break;
				case ECONTEXT_COMMAND_AdminBan:		PC->RconKick(TargetPlayerState->UniqueId.ToString(), true,TEXT("Banned by Host/Admin")); break;
				case ECONTEXT_COMMAND_MutePlayer:
				{
					PlayerOwner->GameMutePlayer(TargetPlayerState->UniqueId.ToString());
					HideContextMenu();
					break;
				}
				case ECONTEXT_COMMAND_ReportPlayer:	PlayerOwner->ReportAbuse(TargetPlayerState); break;
			}
		}
	}
	return FReply::Handled();
}

FReply SUTInGameHomePanel::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	if (PC)
	{
		TArray<FKey> Keys;
		PC->ResolveKeybindToFKey(TEXT("PushToTalk"), Keys);
		for (int i = 0; i < Keys.Num(); i++)
		{
			if (MouseEvent.IsMouseButtonDown(Keys[i]))
			{
				PC->StartVOIPTalking();
			}
		}
	}

	return FReply::Unhandled();
}

FReply SUTInGameHomePanel::OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	if (PC && PC->MyUTHUD)
	{
		TArray<FKey> Keys;
		PC->ResolveKeybindToFKey(TEXT("PushToTalk"), Keys);
		for (int i = 0; i < Keys.Num(); i++)
		{
			if (MouseEvent.IsMouseButtonDown(Keys[i]))
			{
				PC->StopVOIPTalking();;
			}
		}

		PC->MyUTHUD->bForceScores = true;
		FVector2D MousePosition;
		if (GetGameMousePosition(MousePosition))
		{
			UUTScoreboard* SB = PC->MyUTHUD->GetScoreboard();
			if (SB)
			{
				//if ( MouseEvent.GetEffectingButton() == EKeys::RightMouseButton )
				{
					if (SB->AttemptSelection(MousePosition))
					{
						// We are over a item.. pop up the context menu
						FVector2D LocalPosition = MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() );
						ShowContextMenu(SB, LocalPosition, MyGeometry.GetLocalSize());
					}
					else
					{
						HideContextMenu();
					}
				}
/*
				else
				{
					HideContextMenu();
					if (SB->AttemptSelection(MousePosition))
					{
						SB->SelectionClick();
						return FReply::Handled();
					}
					else
					{
						AUTGameState* UTGameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
						if (UTGameState == nullptr || UTGameState->GetMatchState() != MatchState::WaitingToStart)
						{
							PlayerOwner->HideMenu();
						}
					}
				}
*/
			}

			UUTHUDWidget_ReplayTimeSlider* ReplayTimeSlider = PC->MyUTHUD->GetReplayTimeSlider();
			if (ReplayTimeSlider)
			{
				if (ReplayTimeSlider->SelectionClick(MousePosition))
				{
					return FReply::Handled();
				}
			}
		}
	}
	return FReply::Unhandled();
}

FReply SUTInGameHomePanel::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	if (PC && PC->MyUTHUD)
	{
		PC->MyUTHUD->bForceScores = true;
		FVector2D MousePosition;
		if (GetGameMousePosition(MousePosition))
		{
			UUTScoreboard* SB = PC->MyUTHUD->GetScoreboard();
			if (SB)
			{
				SB->TrackMouseMovement(MousePosition);
			}
			UUTHUDWidget_ReplayTimeSlider* ReplayTimeSlider = PC->MyUTHUD->GetReplayTimeSlider();
			if (ReplayTimeSlider)
			{
				ReplayTimeSlider->BecomeInteractive();
				ReplayTimeSlider->TrackMouseMovement(MousePosition);
			}
		}
	}

	return FReply::Unhandled();
}

FReply SUTInGameHomePanel::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyboardEvent)
{
	AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	if (PC && PC->MyUTHUD)
	{
		PC->MyUTHUD->bForceScores = true;
		UUTScoreboard* SB = PC->MyUTHUD->GetScoreboard();
		if (SB)
		{
			if (InKeyboardEvent.GetKey() == EKeys::Up || InKeyboardEvent.GetKey() == EKeys::Gamepad_DPad_Up)
			{
				SB->SelectionUp();
				return FReply::Handled();
			}
			else if (InKeyboardEvent.GetKey() == EKeys::Down || InKeyboardEvent.GetKey() == EKeys::Gamepad_DPad_Down)
			{
				SB->SelectionDown();
				return FReply::Handled();
			}
			else if (InKeyboardEvent.GetKey() == EKeys::Left || InKeyboardEvent.GetKey() == EKeys::Gamepad_DPad_Left)
			{
				SB->SelectionLeft();
				return FReply::Handled();
			}
			else if (InKeyboardEvent.GetKey() == EKeys::Right || InKeyboardEvent.GetKey() == EKeys::Gamepad_DPad_Right)
			{
				SB->SelectionRight();
				return FReply::Handled();
			}
			else if (InKeyboardEvent.GetKey() == EKeys::Enter)
			{
				SB->SelectionClick();
				return FReply::Handled();
			}
			else
			{
				TArray<FKey> Keys;
				PC->ResolveKeybindToFKey(TEXT("PushToTalk"), Keys);
				for (int i = 0; i < Keys.Num(); i++)
				{
					if (Keys[i] == InKeyboardEvent.GetKey())
					{
						PC->StartVOIPTalking();
						return FReply::Handled();
					}
				}
			}
		}
	}

	return FReply::Unhandled();
}

FReply SUTInGameHomePanel::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyboardEvent)
{
	AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	if (PC)
	{
		TArray<FKey> Keys;
		PC->ResolveKeybindToFKey(TEXT("PushToTalk"), Keys);
		for (int i = 0; i < Keys.Num(); i++)
		{
			if (Keys[i] == InKeyboardEvent.GetKey())
			{
				PC->StopVOIPTalking();
				return FReply::Handled();
			}
		}
	}

	return FReply::Unhandled();
}

TSharedPtr<SWidget> SUTInGameHomePanel::GetInitialFocus()
{
	if (PlayerOwner->HasChatText())
	{
		return PlayerOwner->GetChatWidget();
	}

	return ChatArea;
}

FText SUTInGameHomePanel::GetMuteLabelText() const
{
	AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	if (!PC) return FText::GetEmpty();
	UUTScoreboard* Scoreboard = PC->MyUTHUD->GetScoreboard();
	if (Scoreboard == nullptr) return FText::GetEmpty();
	
	TWeakObjectPtr<AUTPlayerState> SelectedPlayer = Scoreboard->GetSelectedPlayer();
	
	if (!SelectedPlayer.IsValid()) return FText::GetEmpty();

	bool bIsMuted = PC->IsPlayerGameMuted(SelectedPlayer.Get());
	
	static const FName VoiceChatFeatureName("VoiceChat");
	if (IModularFeatures::Get().IsModularFeatureAvailable(VoiceChatFeatureName))
	{
		UTVoiceChatFeature* VoiceChat = &IModularFeatures::Get().GetModularFeature<UTVoiceChatFeature>(VoiceChatFeatureName);
		bIsMuted = bIsMuted | VoiceChat->IsPlayerMuted(SelectedPlayer->PlayerName);
	}

	return bIsMuted ? NSLOCTEXT("SUTInGameHomePanel","Unmute","Unmute Player") : NSLOCTEXT("SUTInGameHomePanel","Mute","Mute Player");
}


FReply SUTInGameHomePanel::OnTeamChangeClick()
{
	AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	if (PC)
	{
		PC->ServerSwitchTeam();
	}
	return FReply::Handled();
}

FReply SUTInGameHomePanel::OnReadyChangeClick()
{
	AUTPlayerController* PC = Cast<AUTPlayerController>(PlayerOwner->PlayerController);
	if (PC)
	{
//		PC->PlayMenuSelectSound();
		PC->ServerToggleWarmup();
		PlayerOwner->HideMenu();
	}
	return FReply::Handled();
}

FReply SUTInGameHomePanel::OnSpectateClick()
{
	ConsoleCommand(TEXT("ChangeTeam 255"));
	return FReply::Handled();
}
		
		
EVisibility SUTInGameHomePanel::GetChangeTeamVisibility() const
{
	AUTGameState* GameState = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
	if (GameState && GameState->GetMatchState() != MatchState::WaitingPostMatch && GameState->GetMatchState() != MatchState::PlayerIntro && GameState->GetMatchState() != MatchState::MapVoteHappening)
	{
		return EVisibility::Visible;
	}
	else
	{
		return EVisibility::Collapsed;
	}
}
		
FSlateColor SUTInGameHomePanel::GetChangeTeamLabelColor() const
{
	return FSlateColor(FLinearColor::White);
}

FSlateColor SUTInGameHomePanel::GetMatchLabelColor() const
{
	return FSlateColor(FLinearColor::Yellow);
}
		
EVisibility SUTInGameHomePanel::GetMatchButtonVis() const
{
	AUTPlayerState* PS = PlayerOwner->PlayerController ? Cast<AUTPlayerState>(PlayerOwner->PlayerController->PlayerState) : NULL;
	bool bIsSpectator = PS && PS->bOnlySpectator;
	if (PS && !PS->bOnlySpectator)
	{
		return EVisibility::Visible;
	}

	return EVisibility::Collapsed;
}

void SUTInGameHomePanel::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	if (MatchButton.IsValid())
	{
		float Scale = 1.0f + (0.05 * FMath::Sin(PlayerOwner->GetWorld()->GetRealTimeSeconds() * 6.0f));
		MatchButton->SetRenderTransform(FSlateRenderTransform(Scale));
	}

}


#endif