// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"

#include "UTAnalytics.h"
#include "UTGameInstance.h"
#include "UTGameState.h"
#include "UTHeartBeatManager.h"
#include "UTParty.h"
#include "UTPartyGameState.h"
#include "UTPlayerController.h"
#include "UTMenuGameMode.h"
#include "UTLobbyGameState.h"
#include "UTDemoRecSpectator.h"
#include "UTMatchmaking.h"
#include "UTGameEngine.h"

#include "UserActivityTracking.h"

UUTHeartbeatManager::UUTHeartbeatManager(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UUTHeartbeatManager::StartManager(AUTBasePlayerController* UTPlayerController)
{
	if (!UTPlayerController)
	{
		return;
	}

	UTPC = UTPlayerController;

	//Do per minute events and wipe the minute stat periods
	UTPC->GetWorldTimerManager().SetTimer(DoMinuteEventsTimerHandle, this, &UUTHeartbeatManager::DoMinuteEvents, 60.0f, true);

	//Do per five second events and wipe the minute stat periods
	UTPC->GetWorldTimerManager().SetTimer(DoFiveSecondEventsTimerHandle, this, &UUTHeartbeatManager::DoFiveSecondEvents, 5.0f, true);
}

void UUTHeartbeatManager::StopManager()
{
	if (UTPC)
	{
		UTPC->GetWorldTimerManager().ClearAllTimersForObject(this);
	}
}

void UUTHeartbeatManager::DoMinuteEvents()
{
	if (UTPC == nullptr)
	{
		return;
	}

	if (UTPC->Role == ROLE_Authority)
	{
		DoMinuteEventsServer();
	}
	if (UTPC->IsLocalPlayerController())
	{
		DoMinuteEventsLocal();
	}
}

void UUTHeartbeatManager::DoFiveSecondEvents()
{

}

void UUTHeartbeatManager::DoMinuteEventsServer()
{

}

void UUTHeartbeatManager::DoMinuteEventsLocal()
{
	SendPlayerContextLocationPerMinute();
}

void UUTHeartbeatManager::SendPlayerContextLocationPerMinute()
{
	AUTDemoRecSpectator* UTDemoRecSpec = Cast<AUTDemoRecSpectator>(UTPC);

	if (UTPC != NULL && UTPC->GetWorld() && (UTDemoRecSpec == nullptr || !UTDemoRecSpec->IsKillcamSpectator()))
	{
		AUTPlayerState* UTPS = Cast<AUTPlayerState>(UTPC->PlayerState);
		if (UTPS)
		{
			UUTGameInstance* GameInstance = Cast<UUTGameInstance>(UTPC->GetGameInstance());
			if (GameInstance && GameInstance->GetParties())
			{
				int32 PlaylistID = INDEX_NONE;

				// Check to see if we are in a social party
				int32 NumPartyMembers = 0;
				if (UUTPartyGameState* SocialParty = Cast<UUTPartyGameState>(GameInstance->GetParties()->GetPersistentParty()))
				{
					TArray<UPartyMemberState*> PartyMembers;
					SocialParty->GetAllPartyMembers(PartyMembers);

					NumPartyMembers = PartyMembers.Num();
				}
							
				FString PlayerConxtextLocation;
				AUTGameState* UTGS = Cast<AUTGameState>(UTPC->GetWorld()->GetGameState());
				if (UTGS)
				{
					PlayerConxtextLocation = TEXT("Match");

					if (UTGS->bIsQuickMatch)
					{
						PlayerConxtextLocation.Append(TEXT(" - Quickmatch"));
						
						if (GameInstance->GetMatchmaking())
						{
							PlaylistID = GameInstance->GetMatchmaking()->GetPlaylistID();
						}
					}

					if (UTGS->bRankedSession)
					{
						PlayerConxtextLocation.Append(TEXT(" - Ranked"));

						if (GameInstance->GetMatchmaking())
						{
							PlaylistID = GameInstance->GetMatchmaking()->GetPlaylistID();
						}
					}
					
					if (UTGS->HubGuid.IsValid())
					{
						PlayerConxtextLocation.Append(TEXT(" - HUB"));
					}

					AUTGameMode* UTGM = Cast<AUTGameMode>(UTPC->GetWorld()->GetAuthGameMode());
					if (UTGM && UTGM->bOfflineChallenge)
					{
						PlayerConxtextLocation.Append(TEXT(" - Offline Challenge"));
					}
					else if (UTPC->GetWorld()->GetNetMode() == NM_Standalone)
					{
						PlayerConxtextLocation.Append(TEXT(" - Offline"));
					}

					AUTMenuGameMode* MenuGM = UTPC->GetWorld()->GetAuthGameMode<AUTMenuGameMode>();
					if (MenuGM)
					{
						PlayerConxtextLocation = TEXT("Menu");
						if (GameInstance->GetMatchmaking())
						{
							PlaylistID = GameInstance->GetMatchmaking()->GetPlaylistID();
							if (PlaylistID != INDEX_NONE)
							{
								PlayerConxtextLocation.Append(TEXT("_InQueue"));
							}
						}
					}

					AUTLobbyGameState* LobbyGS = Cast<AUTLobbyGameState>(UTGS);
					if (LobbyGS)
					{
						PlayerConxtextLocation = TEXT("Lobby");
					}

					if ( UTPC->GetWorld()->DemoNetDriver && UTPC->IsLocalController() && UTDemoRecSpec && !UTDemoRecSpec->IsKillcamSpectator())
					{
						PlayerConxtextLocation = TEXT("Replay");
					}
				}

				FUTAnalytics::FireEvent_PlayerContextLocationPerMinute(UTPC, PlayerConxtextLocation, NumPartyMembers, PlaylistID);
			
				//Setting context for User Activity Tracking
				UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
				if (UTEngine && ((UTEngine->LocalContentChecksums.Num() + UTEngine->MountedDownloadedContentChecksums.Num()) > 0))
				{
					PlayerConxtextLocation.Append(TEXT("_CustomContent"));
				}

				if (PlayerConxtextLocation.Contains(TEXT("Match")))
				{
					PlayerConxtextLocation.Append(FString::Printf(TEXT("_%s"), *UTPC->GetWorld()->GetMapName()));
				}

				FUserActivityTracking::SetActivity(FUserActivity(PlayerConxtextLocation));
			}
		}
	}
}
