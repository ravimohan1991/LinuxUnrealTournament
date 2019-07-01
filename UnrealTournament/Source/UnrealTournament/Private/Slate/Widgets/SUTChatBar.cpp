// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "SUTChatBar.h"
#include "SUTChatEditBox.h"
#include "UTGameViewportClient.h"

#if !UE_SERVER

void SUTChatBar::Construct(const FArguments& InArgs, TWeakObjectPtr<UUTLocalPlayer> inPlayerOwner)
{
	PlayerOwner = inPlayerOwner;
	ChatDestination = InArgs._InitialChatDestination; // ChatDestinations::Local;

	TSharedPtr<SUTChatEditBox> ChatWidget = PlayerOwner->GetChatWidget();
	ExternalOnTextCommitted = InArgs._OnTextCommitted;

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
		[
			SAssignNew(ChatArea, SHorizontalBox)
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SBox)
				.HeightOverride(42)
				.WidthOverride(60)
				[
					BuildChatDestinationsButton()
				]
			]
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.AutoWidth()
			[
				SNew(STextBlock)
				.Text(this, &SUTChatBar::GetChatDestinationText)
				.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small.Bold")
			]
			
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.FillWidth(1.0f)
			[
				SAssignNew(ChatSlot, SHorizontalBox)
				+SHorizontalBox::Slot()
				.AutoWidth()
				.Padding(0.0f,5.0f,0.0f,0.0f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT(">")))
					.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Small.Bold")
				]
				+SHorizontalBox::Slot()
				[
					ChatWidget.ToSharedRef()
				]
			]
		]
	];

	ChatWidget->SetCommittedDelegate(FOnTextCommitted::CreateSP(this,&SUTChatBar::ChatTextCommited));
	ChatWidget->JumpToEnd();
}

FReply SUTChatBar::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return FReply::Handled();
}

FReply SUTChatBar::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return FReply::Handled();
}


TSharedRef<SWidget> SUTChatBar::BuildChatDestinationsButton()
{
	SAssignNew(ChatDestinationsButton, SComboButton)
		.HasDownArrow(false)
		.ButtonStyle(SUTStyle::Get(), "UT.SimpleButton.Dark")
		.ButtonContent()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot().AutoHeight()
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot().AutoWidth()
				[
					SNew(SBox).WidthOverride(36).HeightOverride(36)
					[
						SNew(SImage)
						.Image(SUTStyle::Get().GetBrush("UT.Icon.Chat36"))
					]
				]
			]
		];

	SAssignNew(ChatMenu, SVerticalBox);
	if (ChatMenu.IsValid())
	{
		BuildChatDestinationMenu();
	}

	ChatDestinationsButton->SetMenuContent(ChatMenu.ToSharedRef());
	return ChatDestinationsButton.ToSharedRef();

}

void SUTChatBar::BuildChatDestinationMenu()
{
	if (ChatMenu.IsValid())
	{
		ChatMenu->ClearChildren();
		ChatMenu->AddSlot()
		.AutoHeight()
		[
			SNew(SButton)
			.ButtonStyle(SUTStyle::Get(), "UT.ContextMenu.Item")
			.ContentPadding(FMargin(10.0f, 5.0f))
			.Text(NSLOCTEXT("Chat", "ChatDestination_Game", "Game"))
			.TextStyle(SUTStyle::Get(), "UT.Font.ContextMenuItem")
			.OnClicked(this, &SUTChatBar::ChangeChatDestination, ChatDestinationsButton, ChatDestinations::Local)
		];
	
		AUTGameState* GS = PlayerOwner->GetWorld()->GetGameState<AUTGameState>();
		if (GS)
		{
			if (GS->bTeamGame)
			{
				ChatMenu->AddSlot()
				.AutoHeight()
				[
					SNew(SButton)
					.ButtonStyle(SUTStyle::Get(), "UT.ContextMenu.Item")
					.ContentPadding(FMargin(10.0f, 5.0f))
					.Text(NSLOCTEXT("Chat", "ChatDestination_Team", "Team"))
					.TextStyle(SUTStyle::Get(), "UT.Font.ContextMenuItem")
					.OnClicked(this, &SUTChatBar::ChangeChatDestination, ChatDestinationsButton, ChatDestinations::Team)
				];
			}

			if (GS->bIsInstanceServer)
			{
				ChatMenu->AddSlot()
				.AutoHeight()
				[
					SNew(SButton)
					.ButtonStyle(SUTStyle::Get(), "UT.ContextMenu.Item")
					.ContentPadding(FMargin(10.0f, 5.0f))
					.Text(NSLOCTEXT("Chat", "ChatDestination_Lobby", "Lobby"))
					.TextStyle(SUTStyle::Get(), "UT.Font.ContextMenuItem")
					.OnClicked(this, &SUTChatBar::ChangeChatDestination, ChatDestinationsButton, ChatDestinations::Lobby)
				];
			}

			if (PlayerOwner->IsLoggedIn())
			{
				ChatMenu->AddSlot()
				.AutoHeight()
				[
					SNew(SButton)
					.ButtonStyle(SUTStyle::Get(), "UT.ContextMenu.Item")
					.ContentPadding(FMargin(10.0f, 5.0f))
					.Text(NSLOCTEXT("Chat", "ChatDestination_Friends", "Friends"))
					.TextStyle(SUTStyle::Get(), "UT.Font.ContextMenuItem")
					.OnClicked(this, &SUTChatBar::ChangeChatDestination, ChatDestinationsButton, ChatDestinations::Friends)
				];
			}
		}
	}
}

FReply SUTChatBar::ChangeChatDestination(TSharedPtr<SComboButton> Button, FName NewDestination)
{
	if (Button.IsValid()) Button->SetIsOpen(false);
	ChatDestination = NewDestination;
	return FReply::Handled();
}


FText SUTChatBar::GetChatDestinationText() const
{
	if (ChatDestination == ChatDestinations::Team)		return NSLOCTEXT("Chat", "TeamTag","Team");
	if (ChatDestination == ChatDestinations::Local)		return NSLOCTEXT("Chat", "LocalTag","Game");
	if (ChatDestination == ChatDestinations::Friends)	return NSLOCTEXT("Chat", "FriendsTag","Friends");
	if (ChatDestination == ChatDestinations::Lobby)		return NSLOCTEXT("Chat", "LobbyTag","Hub");
	if (ChatDestination == ChatDestinations::Match)		return NSLOCTEXT("Chat", "MatchTag","Match");
	
	return NSLOCTEXT("Chat", "GlobalTag","Global Chat");
}

void SUTChatBar::ChatConsoleCommand(const FString& Destination, const FString& FinalText)
{
	UUTGameViewportClient* UTGVC = Cast<UUTGameViewportClient>(PlayerOwner->ViewportClient);
	if (UTGVC && UTGVC->HasActiveWorldOverride())
	{
		UWorld* RealWorld = UTGVC->GetWorldNoActiveWorldOverride();
		for (FConstPlayerControllerIterator Iterator = RealWorld->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			if (Iterator->Get()->Player == PlayerOwner)
			{
				Iterator->Get()->ProcessConsoleExec(*FString::Printf(TEXT("%s %s"), *Destination, *FinalText), *GLog, Iterator->Get()->GetPawnOrSpectator());
			}
		}
	}
	else
	{
		PlayerOwner->ConsoleCommand(FString::Printf(TEXT("%s %s"), *Destination, *FinalText));
	}
}

void SUTChatBar::ChatTextCommited(const FText& NewText, ETextCommit::Type CommitType)
{
	if (CommitType == ETextCommit::OnEnter)
	{
		FString FinalText = NewText.ToString();
		// Figure out the type of chat...
		if (FinalText != TEXT(""))
		{
			if (FinalText.Left(1) == TEXT("\\") || FinalText.Left(1) == TEXT("/"))
			{
				FinalText = FinalText.Right(FinalText.Len() - 1);
				PlayerOwner->ConsoleCommand(FinalText);
				PlayerOwner->GetChatWidget()->SetText(FText::GetEmpty());
				return;
			}

			// World can go null during load
			if (PlayerOwner->GetWorld() != nullptr)
			{
				if (ChatDestination == ChatDestinations::Global)	ChatConsoleCommand(TEXT("GlobalChat"), FinalText);
				if (ChatDestination == ChatDestinations::Friends)	ChatConsoleCommand(TEXT("FriendSay"), FinalText);
				if (ChatDestination == ChatDestinations::Lobby)		ChatConsoleCommand(TEXT("LobbySay"), FinalText);
				if (ChatDestination == ChatDestinations::Local)		ChatConsoleCommand(TEXT("Say"), FinalText);
				if (ChatDestination == ChatDestinations::Match)		ChatConsoleCommand(TEXT("MatchChat"), FinalText);
				if (ChatDestination == ChatDestinations::Team)		ChatConsoleCommand(TEXT("TeamSay"), FinalText);
				PlayerOwner->GetChatWidget()->SetText(FText::GetEmpty());
			}
		}
		else
		{
			// Add code to change chat mode here.
		}
	}

	ExternalOnTextCommitted.ExecuteIfBound(NewText, CommitType);
}



#endif