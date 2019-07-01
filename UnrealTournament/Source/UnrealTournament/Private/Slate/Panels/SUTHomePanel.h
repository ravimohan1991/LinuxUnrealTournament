// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "Slate/SlateGameResources.h"
#include "../Base/SUTPanelBase.h"
#include "../SUWindowsStyle.h"
#include "../Widgets/SUTImage.h"
#include "SUTPartyWidget.h"
#include "SUTPartyInviteWidget.h"
#include "SUTServerBrowserPanel.h"

#if !UE_SERVER

class SUTMainMenu;
class SUTBorder;

class UNREALTOURNAMENT_API SUTHomePanel : public SUTPanelBase
{
	virtual void ConstructPanel(FVector2D ViewportSize);
	virtual TSharedRef<SWidget> BuildHomePanel();

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime );

	virtual bool ShouldShowBackButton()
	{
		return false;
	}
	virtual void PanelClosed();


protected:

	virtual void OnShowPanel(TSharedPtr<SUTMenuBase> inParentWindow);
	virtual void OnHidePanel();

	FReply MainButtonClick(FName ButtonTag);
	FReply OfflineAction_Click();

	virtual FLinearColor GetFadeColor() const;
	virtual FSlateColor GetFadeBKColor() const;

	TSharedPtr<SVerticalBox> AnnouncementBox;
	TSharedPtr<SUTPartyWidget> PartyBox;
	TSharedPtr<SUTPartyInviteWidget> PartyInviteBox;

	TSharedPtr<SVerticalBox> NewChallengeBox;
	TSharedPtr<SUTImage> NewChallengeImage;

	virtual void BuildAnnouncement();

	float AnnouncmentTimer;
	float AnnouncmentFadeTimer;

	EVisibility ShowNewChallengeImage() const;

	TSharedPtr<SUTBorder> AnimWidget;

	virtual void AnimEnd();
	virtual void BuildQuickplayButton(FName BackgroundTexture, FText Caption, int32 PlayListId, float Padding=0.0f);
	
	virtual TSharedRef<SBox> BuildHomePanelButton(FName ButtonTag, FName BackgroundTexture, const FText& Caption);

	FReply QuickPlayClick(int32 PlaylistId);

	TSharedRef<SWidget> BuildRankedPlaylist();
	FReply OnStartRankedPlaylist(int32 PlaylistId);

	TArray<TSharedPtr<FServerData>> LanMatches;
	TSharedPtr<SVerticalBox> LanBox;
	FTimerHandle LanTimerHandle;
	FTimerHandle LanPingHandle;
	TArray<FServerPingTracker> PingTrackers;

	void CheckForLanServers();
	void RebuildLanBox();

	void PingLanServers();
	virtual void OnPingResult(AUTServerBeaconClient* Sender, FServerBeaconInfo ServerInfo);
	virtual void OnPingFailure(AUTServerBeaconClient* Sender);


	virtual FReply OnJoinLanClicked(TSharedPtr<FServerData> Server);
	virtual FReply OnSpectateLanClicked(TSharedPtr<FServerData> Server);
	FDelegateHandle OnFindLanSessionCompleteDelegate;
	TSharedPtr<class FUTOnlineGameSearchBase> LanSearchSettings;
	void OnFindLANSessionsComplete(bool bWasSuccessful);

	TArray<TSharedPtr<SUTWebBrowserPanel>> AnnouncementBrowserList;
	TSharedPtr<SHorizontalBox> RankedBox;
	EVisibility RankedBoxVisibile() const;
	EVisibility RankedButtonVis(int32 PlaylistId) const;

	TSharedPtr<SHorizontalBox> QuickPlayBox;
	TSharedPtr<SVerticalBox> NoQuickPlayBox;

	EVisibility QuickplayVis(int32 Index) const;
	FText GetQuickPlayUnavailableText() const;
	void BuildQuickPlayPanel();
};

#endif