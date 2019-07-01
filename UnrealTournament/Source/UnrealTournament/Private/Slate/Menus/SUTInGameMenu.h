// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "Slate/SlateGameResources.h"
#include "../Base/SUTMenuBase.h"
#include "../Widgets/SUTComboButton.h"

#if !UE_SERVER

class SUTMessageBoxDialog;

class UNREALTOURNAMENT_API SUTInGameMenu : public SUTMenuBase
{
public:
	virtual FReply OnReturnToLobby();
	virtual FReply OnReturnToMainMenu();

	virtual void OnMenuOpened(const FString& Parameters);
	virtual FReply OnKeyUp( const FGeometry& MyGeometry, const FKeyEvent& InKeyboardEvent ) override;
protected:

	virtual void BuildLeftMenuBar();
	virtual void BuildExitMenu(TSharedPtr <SUTComboButton> ExitButton);

	virtual FReply OnCloseMenu();
	virtual void OnMenuClosed();

	virtual FReply OnMapVoteClick();
	virtual void SetInitialPanel();
	
	virtual FReply OpenHUDSettings();
	virtual FText GetMapVoteTitle() const;
	virtual void WriteQuitMidGameAnalytics();

	virtual void ShowExitDestinationMenu();
	virtual void QuitConfirmation();
	void OnDestinationResult(int32 PickedIndex);
	void ShowHomePanel();
	void BackResult(TSharedPtr<SCompoundWidget> Dialog, uint16 ButtonPressed);

	EVisibility GetMapVoteVisibility() const;
	EVisibility GetChangeTeamVisibility() const;

	TSharedPtr<SUTMessageBoxDialog> MessageDialog;

	virtual FReply OnTeamChangeClick();
	FSlateColor GetChangeTeamLabelColor() const;
	TSharedPtr<SUTButton> ChangeTeamButton;
public:
	virtual bool SkipWorldRender();


};
#endif
