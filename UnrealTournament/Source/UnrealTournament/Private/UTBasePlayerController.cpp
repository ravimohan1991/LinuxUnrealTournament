// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTChatMessage.h"
#include "Engine/Console.h"
#include "UTOnlineGameSearchBase.h"
#include "UTOnlineGameSettingsBase.h"
#include "UTWeap_RocketLauncher.h"
#include "UTGameEngine.h"
#include "UnrealNetwork.h"
#include "UTGameViewportClient.h"
#include "UTRconAdminInfo.h"
#include "UTLocalPlayer.h"
#include "UTProfileSettings.h"
#include "UTGameInstance.h"
#include "UTParty.h"
#include "Net/OnlineEngineInterface.h"
#include "UnrealTournamentFullScreenMovie.h"
#include "UTHeartBeatManager.h"
#include "BlueprintContextLibrary.h"
#include "PartyContext.h"
#include "SUTQuickChatWindow.h"
#include "UTVoiceChatFeature.h"

AUTBasePlayerController::AUTBasePlayerController(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	ChatOverflowTime = 0.0f;
	bOverflowed = false;
	SpamText = NSLOCTEXT("AUTBasePlayerController", "SpamMessage", "You must wait a few seconds before sending another message.");
	bShouldPerformFullTickWhenPaused=true;
}

void AUTBasePlayerController::Destroyed()
{
	ClientCloseAllUI(true); // need to leave dialogs open for connection failures that might already be up

	GetWorldTimerManager().ClearAllTimersForObject(this);
	if (MyHUD)
	{
		GetWorldTimerManager().ClearAllTimersForObject(MyHUD);
	}
	if (PlayerCameraManager != NULL)
	{
		GetWorldTimerManager().ClearAllTimersForObject(PlayerCameraManager);
	}
	if (PlayerInput != NULL)
	{
		GetWorldTimerManager().ClearAllTimersForObject(PlayerInput);
	}
	Super::Destroyed();
}

void AUTBasePlayerController::InitInputSystem()
{
	Super::InitInputSystem();

	// read profile on every level change so we can detect updates
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);

#if WITH_PROFILE
	if (LP != NULL && LP->IsLoggedIn() && LP->GetMcpProfileManager() && LP->GetMcpProfileManager()->GetMcpProfileAs<UUtMcpProfile>(EUtMcpProfile::Profile))
	{
		FClientUrlContext QueryContext = FClientUrlContext::Default; // IMPORTANT to make a copy!
		LP->GetMcpProfileManager()->GetMcpProfileAs<UUtMcpProfile>(EUtMcpProfile::Profile)->ForceQueryProfile(QueryContext);
	}
#endif

	// Let the viewport client know we have connected to a server.
	if (GetWorld()->GetNetMode() == ENetMode::NM_Client)
	{
		if (LP && LP->ViewportClient)
		{
			UUTGameViewportClient* VC = Cast<UUTGameViewportClient>(LP->ViewportClient);
			if (VC)
			{
				VC->ClientConnectedToServer();
			}
		}
	}
}

void AUTBasePlayerController::SetupInputComponent()
{

	UUTProfileSettings* ProfileSettings = GetProfileSettings();
	if (ProfileSettings != nullptr) ProfileSettings->ApplyInputSettings(Cast<UUTLocalPlayer>(Player));

	Super::SetupInputComponent();
	InputComponent->BindAction("ShowMenu", IE_Released, this, &AUTBasePlayerController::execShowMenu);
}

void AUTBasePlayerController::SetName(const FString& S)
{
}

void AUTBasePlayerController::ClanName(const FString& S)
{
}

void AUTBasePlayerController::execShowMenu()
{
	ShowMenu(TEXT(""));
}

void AUTBasePlayerController::ShowMenu(const FString& Parameters)
{
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
	if (LP != NULL)
	{
		LP->ShowMenu(Parameters);
	}
}

void AUTBasePlayerController::HideMenu()
{
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
	if (LP != NULL)
	{
		LP->HideMenu();
	}
}

void AUTBasePlayerController::ServerChangeClanName_Implementation(const FString& S)
{
	if (UTPlayerState)
	{
		// Unicode 160 is an empty space, not sure what other characters are broken in our font
		FString ClampedName = (S.Len() > 8) ? S.Left(8) : S;
		FString InvalidNameChars = FString(INVALID_NAME_CHARACTERS);
		for (int32 i = ClampedName.Len() - 1; i >= 0; i--)
		{
			if (InvalidNameChars.GetCharArray().Contains(ClampedName.GetCharArray()[i]))
			{
				ClampedName.GetCharArray().RemoveAt(i);
			}
		}
		UTPlayerState->ClanName = ClampedName;
		AUTGameMode* Game = GetWorld()->GetAuthGameMode<AUTGameMode>();
		if (Game && Game->bUseProtoTeams && !UTPlayerState->ClanName.IsEmpty())
		{
			UTPlayerState->SetPlayerCard(UTPlayerState->ClanName);
		}
	}
}

bool AUTBasePlayerController::ServerChangeClanName_Validate(const FString& S)
{
	return true;
}

#if !UE_SERVER
void AUTBasePlayerController::ShowMessage(FText MessageTitle, FText MessageText, uint16 Buttons, const FDialogResultDelegate& Callback)
{
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
	if (LP != NULL)
	{
		LP->ShowMessage(MessageTitle, MessageText, Buttons, Callback);
	}	
}
#endif

// LEAVE ME for quick debug commands when we need them.
void AUTBasePlayerController::DebugTest(FString TestCommand)
{
}

void AUTBasePlayerController::ServerDebugTest_Implementation(const FString& TestCommand)
{
}

bool AUTBasePlayerController::ServerDebugTest_Validate(const FString& TestCommand) {return true;}

void AUTBasePlayerController::InitPlayerState()
{
	Super::InitPlayerState();
	UTPlayerState = Cast<AUTPlayerState>(PlayerState);
	if (UTPlayerState)
	{
		UTPlayerState->ChatDestination = ChatDestinations::Local;
	}
}


void AUTBasePlayerController::Talk()
{
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
	if (LP != nullptr && LP->ViewportClient->ViewportConsole != nullptr)
	{
		LP->ShowQuickChat(ChatDestinations::Local);
	}
}

void AUTBasePlayerController::TeamTalk()
{
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
	if (LP != nullptr && LP->ViewportClient->ViewportConsole != nullptr)
	{
		LP->ShowQuickChat(ChatDestinations::Team);
	}
}

bool AUTBasePlayerController::AllowTextMessage(FString& Msg, bool bIsTeamMessage)
{
	const float TIME_PER_MSG = 1.0f;
	const float MAX_OVERFLOW = 4.0f;

	if (GetNetMode() == NM_Standalone || (GetNetMode() == NM_ListenServer && Role == ROLE_Authority))
	{
		return true;
	}

	if (Role == ROLE_Authority)
	{
		// Give the game mode a chance to adjust/deny the message.

		AUTBaseGameMode* BaseGameMode = GetWorld()->GetAuthGameMode<AUTBaseGameMode>();
		if ( BaseGameMode && !BaseGameMode->AllowTextMessage(Msg, bIsTeamMessage, this) )
		{
			return false;
		}
	
	}

	ChatOverflowTime = FMath::Max(ChatOverflowTime, GetWorld()->RealTimeSeconds);

	//When overflowed, wait till the time is back to 0
	if (bOverflowed && ChatOverflowTime > GetWorld()->RealTimeSeconds)
	{
		return false;
	}
	bOverflowed = false;

	//Accumulate time for each message, double for a duplicate message
	ChatOverflowTime += (LastChatMessage == Msg) ? TIME_PER_MSG * 2 : TIME_PER_MSG;
	LastChatMessage = Msg;

	if (ChatOverflowTime - GetWorld()->RealTimeSeconds <= MAX_OVERFLOW)
	{
		return true;
	}

	bOverflowed = true;
	return false;
}

void AUTBasePlayerController::Say(FString Message)
{
	// clamp message length; aside from troll prevention this is needed for networking reasons
	Message = Message.Left(MAX_CHAT_TEXT_SIZE);
	if (AllowTextMessage(Message, false))
	{
		ServerSay(Message, false);
	}
}

void AUTBasePlayerController::TeamSay(FString Message)
{
	// clamp message length; aside from troll prevention this is needed for networking reasons
	Message = Message.Left(MAX_CHAT_TEXT_SIZE);
	if (AllowTextMessage(Message, true))
	{
		ServerSay(Message, true);
	}
}

bool AUTBasePlayerController::ServerSay_Validate(const FString& Message, bool bTeamMessage) { return true; }

void AUTBasePlayerController::ServerSay_Implementation(const FString& inMessage, bool bTeamMessage)
{
	FString Message = inMessage;
	if (AllowTextMessage(Message, bTeamMessage) && PlayerState != nullptr)
	{
		// Look to see if this message is a direct message to a given player.

		FString TrimmedMessage = Message;
        TrimmedMessage = TrimmedMessage.TrimStart();//TrimmedMessage = TrimmedMessage.Trim();

		if (TrimmedMessage.Left(1) == TEXT("@"))
		{
			// Remove the @
			TrimmedMessage = TrimmedMessage.Right(TrimmedMessage.Len()-1);
			DirectSay(TrimmedMessage);
			return;
		}

		bool bSpectatorMsg = PlayerState->bOnlySpectator;

		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			AUTBasePlayerController* UTPC = Cast<AUTBasePlayerController>(*Iterator);
			if (UTPC != nullptr)
			{
				if (!bTeamMessage || UTPC->GetTeamNum() == GetTeamNum())
				{
					TSharedPtr<const FUniqueNetId> Id = PlayerState->UniqueId.GetUniqueNetId();
					bool bIsMuted =  Id.IsValid() && UTPC->MuteList.VoiceMuteList.IndexOfByPredicate(FUniqueNetIdMatcher(*Id)) != INDEX_NONE;

					// Dont send spectator chat to players
					if (UTPC->PlayerState != nullptr && (!bSpectatorMsg || UTPC->PlayerState->bOnlySpectator) && !bIsMuted)
					{
						UTPC->ClientSay(UTPlayerState, TrimmedMessage, (bTeamMessage ? ChatDestinations::Team : ChatDestinations::Local));
					}
				}
			}
		}
	}
}

void AUTBasePlayerController::DirectSay(const FString& Message)
{
	// Figure out who we are talking too...

	AUTGameState* UTGameState = GetWorld()->GetGameState<AUTGameState>();
	if (UTGameState)
	{
		FString TargetPlayerName;
		FString FinalMessage = Message;

		bool bSent = false;

		// Look for a local player controller to send to...
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			AUTBasePlayerController* UTPC = Cast<AUTBasePlayerController>(*Iterator);
			if (UTPC != nullptr && UTPC->PlayerState != nullptr)
			{
				AUTPlayerState* TargetPlayerState = Cast<AUTPlayerState>(UTPC->PlayerState);
				if (TargetPlayerState != nullptr)
				{
                    TargetPlayerName = TargetPlayerState->GetPlayerName();

					if (Message.Left(TargetPlayerName.Len()).Equals(TargetPlayerName, ESearchCase::IgnoreCase))
					{
                        FinalMessage = FinalMessage.Right(FinalMessage.Len() - TargetPlayerName.Len()).TrimStart();
						bSent = true;
		
						TSharedPtr<const FUniqueNetId> Id = UTPC->PlayerState->UniqueId.GetUniqueNetId();
						bool bIsMuted = Id.IsValid() && IsPlayerMuted(Id.ToSharedRef().Get());

						if (!bIsMuted)
						{
							UTPC->ClientSay(UTPlayerState, FinalMessage, ChatDestinations::Whisper);
						}
						FinalMessage = FString::Printf(TEXT("to %s \"%s\""), *TargetPlayerName, *FinalMessage);
						break;
					}
				}
			}
		}

		// If we haven't sent the message. Look to see if we need to forward this message elsewhere
		if (!bSent)
		{
			bSent = ForwardDirectSay(UTPlayerState, FinalMessage);
		}

		if (bSent)
		{
			// Make sure I see that I sent it..
			ClientSay(UTPlayerState, FinalMessage, ChatDestinations::Whisper);
		}
	}
}

bool AUTBasePlayerController::ForwardDirectSay(AUTPlayerState* SenderPlayerState, FString& FinalMessage)
{
	return false;
}

bool AUTBasePlayerController::AllowSay(AUTPlayerState* Speaker, const FString& Message, FName Destination)
{
	UUTProfileSettings* ProfileSettings = GetProfileSettings();
	if (ProfileSettings != nullptr && Speaker != nullptr)
	{
		if (ProfileSettings->ComFilter != EComFilter::AllComs)
		{
			if (ProfileSettings->ComFilter == EComFilter::NoComs)
			{
				return false;
			}
			else if (ProfileSettings->ComFilter == EComFilter::TeamComs)
			{
				if (Speaker->GetTeamNum() != GetTeamNum())			
				{
					return false;
				}
			}
			else if (ProfileSettings->ComFilter == EComFilter::FriendComs)
			{
				if (!Speaker->bIsFriend)
				{
					return false;
				}
			}
		}
	}

	return !IsPlayerGameMuted(Speaker);

}

void AUTBasePlayerController::ClientSay_Implementation(AUTPlayerState* Speaker, const FString& Message, FName Destination)
{

	if ( AllowSay(Speaker, Message, Destination) )
	{
		FClientReceiveData ClientData;
		ClientData.LocalPC = this;
		ClientData.MessageIndex = (Destination == ChatDestinations::Team) ? 1 : 0;
		ClientData.RelatedPlayerState_1 = Speaker;
		ClientData.MessageString = Message;

		UUTChatMessage::StaticClass()->GetDefaultObject<UUTChatMessage>()->ClientReceiveChat(ClientData, Destination);
	}
}

uint8 AUTBasePlayerController::GetTeamNum() const
{
	return (UTPlayerState != NULL && UTPlayerState->Team != NULL) ? UTPlayerState->Team->TeamIndex : 255;
}

void AUTBasePlayerController::ClientRankedGameAbandoned_Implementation()
{
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
	if (LP)
	{
		LP->LastRankedMatchSessionId.Empty();
		LP->LastRankedMatchPlayerId.Empty();
		LP->LastRankedMatchTimeString.Empty();
		LP->SaveConfig();

#if !UE_SERVER
		LP->ShowGameAbandonedDialog();
#endif
	}

	ConsoleCommand("Disconnect");
}

void AUTBasePlayerController::ClientMatchmakingGameComplete_Implementation()
{
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
	if (LP)
	{
		LP->LastRankedMatchSessionId.Empty();
		LP->LastRankedMatchPlayerId.Empty();
		LP->LastRankedMatchTimeString.Empty();
		LP->SaveConfig();
	}
}

void AUTBasePlayerController::ClientReturnToLobby_Implementation(bool bKicked, bool bIdle)
{
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
	if (LP)
	{
		LP->LastRankedMatchSessionId.Empty();
		LP->LastRankedMatchPlayerId.Empty();
		LP->LastRankedMatchTimeString.Empty();
		LP->SaveConfig();
	}

	if (bKicked)
	{
		ClientWasKicked(bIdle ? NSLOCTEXT("General", "IdleKick", "You were kicked for being idle.") : NSLOCTEXT("General", "HostKick", "You were kicked by the host or admin.") );
	}

	AUTGameState* GameState = GetWorld()->GetGameState<AUTGameState>();
	if (LP && !LP->ReturnDestinationGuidString.IsEmpty())
	{
		ConnectToServerViaGUID(LP->ReturnDestinationGuidString, false);
	}
	else
	{
		ConsoleCommand("Disconnect");
	}
}

void AUTBasePlayerController::CancelConnectViaGUID()
{
		GUIDJoinWantsToSpectate = false;
		GUIDJoin_CurrentGUID = TEXT("");
		GUIDJoinAttemptCount = 0;
		GUIDSessionSearchSettings.Reset();
}

void AUTBasePlayerController::ConnectToServerViaGUID(FString ServerGUID, int32 DesiredTeam, bool bSpectate, bool bVerifyServerFirst)
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem && !GUIDSessionSearchSettings.IsValid()) 
	{

		UE_LOG(UT,Verbose,TEXT("Attempting to Connect to Server Via GUID: %s"), *ServerGUID);

		IOnlineSessionPtr OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();

		GUIDJoinWantsToSpectate = bSpectate;
		GUIDJoin_CurrentGUID = ServerGUID;
		GUIDJoinAttemptCount = 0;
		GUIDSessionSearchSettings.Reset();
		GUIDJoinDesiredTeam = DesiredTeam;
		bGUIDJoinVerifyFirst = bVerifyServerFirst;
		
		// Check to make sure we are not downloading content.  If we are.. stall until it's completed.

		UUTGameViewportClient* ViewportClient = Cast<UUTGameViewportClient>(Cast<ULocalPlayer>(Player)->ViewportClient);
		if (ViewportClient == nullptr || !ViewportClient->IsDownloadInProgress())
		{
			StartGUIDJoin();
		}
	}
}

void AUTBasePlayerController::StartGUIDJoin()
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem && !GUIDSessionSearchSettings.IsValid()) 
	{
		IOnlineSessionPtr OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();

		OnCancelGUIDFindSessionCompleteDelegate.BindUObject(this, &AUTBasePlayerController::OnCancelGUIDFindSessionComplete);
		OnCancelGUIDFindSessionCompleteDelegateHandle = OnlineSessionInterface->AddOnCancelFindSessionsCompleteDelegate_Handle(OnCancelGUIDFindSessionCompleteDelegate);
		OnlineSessionInterface->CancelFindSessions();
	}
	else
	{
		UE_LOG(UT,Warning,TEXT("Not in a proper session.  Things will be broken."));
	}
}

void AUTBasePlayerController::OnCancelGUIDFindSessionComplete(bool bWasSuccessful)
{

	UE_LOG(UT,Log,TEXT("OnCancelGUIDFindSessionComplete %i"), bWasSuccessful);

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem && !GUIDSessionSearchSettings.IsValid()) 
	{
		IOnlineSessionPtr OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();
		OnlineSessionInterface->ClearOnCancelFindSessionsCompleteDelegate_Handle(OnCancelGUIDFindSessionCompleteDelegateHandle);
		OnlineSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindGUIDSessionCompleteDelegateHandle);

		AttemptGUIDJoin();
	}
	else
	{
		UE_LOG(UT,Warning,TEXT("Did not have valid search settings after a cancel.  Things will be broken."));
	}

}


void AUTBasePlayerController::AttemptGUIDJoin()
{

	UE_LOG(UT,Log,TEXT("Attempting a join #%i"), GUIDJoinAttemptCount);

	if (GUIDJoinAttemptCount > 5)
	{
		UE_LOG(UT,Log,TEXT("AttemptedGUIDJoin Timeout at 5 attempts"));
		return;
	}

	GUIDJoinAttemptCount++;

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem && !GUIDSessionSearchSettings.IsValid()) 
	{
		IOnlineSessionPtr OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();
		
		GUIDSessionSearchSettings = MakeShareable(new FUTOnlineGameSearchBase(false));
		GUIDSessionSearchSettings->MaxSearchResults = 1;
		FString GameVer = FString::Printf(TEXT("%i"), FNetworkVersion::GetLocalNetworkVersion());
		GUIDSessionSearchSettings->QuerySettings.Set(SETTING_SERVERVERSION, GameVer, EOnlineComparisonOp::Equals);		// Must equal the game version
		GUIDSessionSearchSettings->QuerySettings.Set(SETTING_SERVERINSTANCEGUID, GUIDJoin_CurrentGUID, EOnlineComparisonOp::Equals);	// The GUID to find

		TSharedRef<FUTOnlineGameSearchBase> SearchSettingsRef = GUIDSessionSearchSettings.ToSharedRef();

		OnFindGUIDSessionCompleteDelegate.BindUObject(this, &AUTBasePlayerController::OnFindSessionsComplete);
		OnFindGUIDSessionCompleteDelegateHandle = OnlineSessionInterface->AddOnFindSessionsCompleteDelegate_Handle(OnFindGUIDSessionCompleteDelegate);

		OnlineSessionInterface->FindSessions(0, SearchSettingsRef);
	}
	else
	{
		UE_LOG(UT,Warning,TEXT("No Session to Join.  Things will be broken."));
	}

}

void AUTBasePlayerController::OnFindSessionsComplete(bool bWasSuccessful)
{
	// Clear the delegate
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem)
	{
		IOnlineSessionPtr OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();
		if (OnlineSessionInterface.IsValid())
		{
			OnlineSessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(OnFindGUIDSessionCompleteDelegateHandle);
		}
	}

	UE_LOG(UT,Log,TEXT("OnFindSesssionComplete %i"), bWasSuccessful);
	if (bWasSuccessful)
	{
		if (GUIDSessionSearchSettings.IsValid()) 
		{
			if (GUIDSessionSearchSettings->SearchResults.Num() > 0)
			{
				FOnlineSessionSearchResult Result = GUIDSessionSearchSettings->SearchResults[0];
				UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
				if (LP)
				{
					// Clear the Quickmatch wait timer.
					LP->QuickMatchLimitTime = 0.0;

					if (bGUIDJoinVerifyFirst)
					{
						// Build the beacon
						PingBeacon = GetWorld()->SpawnActor<AUTServerBeaconClient>(AUTServerBeaconClient::StaticClass());
						if (PingBeacon)
						{
							PingBeacon->OnServerRequestResults = FServerRequestResultsDelegate::CreateUObject(this, &AUTBasePlayerController::OnPingBeaconResult);
							PingBeacon->OnServerRequestFailure = FServerRequestFailureDelegate::CreateUObject(this, &AUTBasePlayerController::OnPingBeaconFailure);
							PingBeacon->bQuickPing = true;
						
							IOnlineSessionPtr OnlineSessionInterface;
							if (OnlineSubsystem) OnlineSessionInterface = OnlineSubsystem->GetSessionInterface();

							if (OnlineSessionInterface.IsValid())
							{
								FString BeaconIP;
								OnlineSessionInterface->GetResolvedConnectString(Result, FName(TEXT("BeaconPort")), BeaconIP);
								FURL BeaconURL(nullptr, *BeaconIP, TRAVEL_Absolute);
								PingBeacon->InitClient(BeaconURL);
								return;
							}
						}
					}

					LP->JoinSession(Result, GUIDJoinWantsToSpectate, GUIDJoinDesiredTeam);
				}

				GUIDJoinWantsToSpectate = false;
				GUIDSessionSearchSettings.Reset();
				return;
			}
		}
	}
	
	GUIDSessionSearchSettings.Reset();
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimer(TempHandle, this, &AUTBasePlayerController::AttemptGUIDJoin, 10.0f, false);
}

void AUTBasePlayerController::OnPingBeaconResult(AUTServerBeaconClient* Sender, FServerBeaconInfo ServerInfo)
{
	PingBeacon->Destroy();
	PingBeacon = nullptr;

	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
	if (LP)
	{
		LP->JoinSession(GUIDSessionSearchSettings->SearchResults[0], GUIDJoinWantsToSpectate, GUIDJoinDesiredTeam);
	}
}
void AUTBasePlayerController::OnPingBeaconFailure(AUTServerBeaconClient* Sender)
{
	if (PingBeacon != nullptr)
	{
		PingBeacon->Destroy();
		PingBeacon = nullptr;
	}

	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
	if (LP)
	{
		LP->ReturnToMainMenu();
	}
}


void AUTBasePlayerController::ClientReturnedToMenus()
{
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
	if (LP)
	{
		LP->LeaveSession();	
		LP->UpdatePresence(TEXT("In Menus"), false, false, false, false);

		if (LP->IsPartyLeader())
		{
			UUTGameInstance* GameInstance = Cast<UUTGameInstance>(LP->GetGameInstance());
			if (GameInstance)
			{
				UUTParty* Party = GameInstance->GetParties();
				if (Party && Party->GetPersistentParty())
				{
					Party->RestorePersistentPartyState();
				}
			}
		}
	}
}

void AUTBasePlayerController::ClientSetPresence_Implementation(const FString& NewPresenceString, bool bAllowInvites, bool bAllowJoinInProgress, bool bAllowJoinViaPresence, bool bAllowJoinViaPresenceFriendsOnly)
{
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
	if (LP)
	{
		LP->UpdatePresence(NewPresenceString, bAllowInvites, bAllowJoinInProgress, bAllowJoinViaPresence, bAllowJoinViaPresenceFriendsOnly);
	}
}

void AUTBasePlayerController::ClientGenericInitialization_Implementation()
{
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
	if (LP)
	{
		ServerReceiveStars(LP->GetTotalChallengeStars());
	}
}

// FIXMESTEVE shouldn't receive this from client
bool AUTBasePlayerController::ServerReceiveStars_Validate(int32 TotalStars) { return true; }
void AUTBasePlayerController::ServerReceiveStars_Implementation(int32 TotalStars)
{
	if (UTPlayerState)
	{
		UTPlayerState->TotalChallengeStars = TotalStars;
	}
}

void AUTBasePlayerController::ClientRequireContentItem_Implementation(const FString& PakFile, const FString& MD5)
{
	bool bContentMatched = false;

	UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
	if (UTEngine)
	{
		if (UTEngine->LocalContentChecksums.Contains(PakFile) && UTEngine->LocalContentChecksums[PakFile] == MD5)
		{
			UE_LOG(UT, Log, TEXT("ClientRequireContentItem %s is my content"), *PakFile);
			bContentMatched = true;
		}

		if (UTEngine->MountedDownloadedContentChecksums.Contains(PakFile))
		{
			if (UTEngine->MountedDownloadedContentChecksums[PakFile] == MD5)
			{
				UE_LOG(UT, Log, TEXT("ClientRequireContentItem %s was already downloaded"), *PakFile);
				bContentMatched = true;
			}
			else
			{
				UE_LOG(UT, Log, TEXT("ClientRequireContentItem %s was already downloaded, but an old version"), *PakFile);
			}
		}

		if (UTEngine->DownloadedContentChecksums.Contains(PakFile))
		{
			UE_LOG(UT, Log, TEXT("ClientRequireContentItem %s was already downloaded, but it is not mounted yet"), *PakFile);
		}

		if (!bContentMatched)
		{
			UTEngine->FilesToDownload.Add(PakFile, MD5);
		}
	}
}

void AUTBasePlayerController::ClientRequireContentItemListBegin_Implementation(const FString& CloudId)
{
	UUTGameEngine* UTEngine = Cast<UUTGameEngine>(GEngine);
	if (UTEngine)
	{
		UTEngine->ContentDownloadCloudId = CloudId;
		UTEngine->FilesToDownload.Empty();
	}
}

void AUTBasePlayerController::ClientRequireContentItemListComplete_Implementation()
{

}


void AUTBasePlayerController::RconAuth(FString Password)
{
	ServerRconAuth(Password);
}

bool AUTBasePlayerController::ServerRconAuth_Validate(const FString& Password)
{
	return true;
}

void AUTBasePlayerController::ServerRconAuth_Implementation(const FString& Password)
{
	AUTBaseGameMode* GameMode = GetWorld()->GetAuthGameMode<AUTBaseGameMode>();
	if (GameMode)
	{
		GameMode->RconAuth(this, Password);
	}
}

void AUTBasePlayerController::RconNormal()
{
	ServerRconNormal();
}

bool AUTBasePlayerController::ServerRconNormal_Validate() {return true;}
void AUTBasePlayerController::ServerRconNormal_Implementation()
{
	AUTBaseGameMode* GameMode = GetWorld()->GetAuthGameMode<AUTBaseGameMode>();
	if (GameMode)
	{
		GameMode->RconNormal(this);
	}
}

UUTLocalPlayer* AUTBasePlayerController::GetUTLocalPlayer()
{
	return Cast<UUTLocalPlayer>(Player);
}

void AUTBasePlayerController::RconDBExec(FString Command)
{
	ServerRconDBExec(Command);
}

bool AUTBasePlayerController::ServerRconDBExec_Validate(const FString& Command)
{
	return true;
}

void AUTBasePlayerController::ServerRconDBExec_Implementation(const FString& Command)
{
	if (UTPlayerState == nullptr || !UTPlayerState->bIsRconAdmin)
	{
		ClientSay(UTPlayerState, TEXT("Rcon not authenticated"), ChatDestinations::System);
		return;
	}
	
	UUTGameInstance* GI = Cast<UUTGameInstance>(GetGameInstance());
	if (GI)
	{
		TArray<FDatabaseRow> DBRows;
		bool DBReturn = GI->ExecDatabaseCommand(Command, DBRows);

		if (DBRows.Num() > 0)
		{
			// Failsafe to not blow out on clientsay
			const int MAXROWS = 10;
			for (int i = 0; i < DBRows.Num() && i < MAXROWS; i++)
			{
				FString Message;
				for (int j = 0; j < DBRows[i].Text.Num(); j++)
				{
					Message += DBRows[i].Text[j] + TEXT(" ");
				}

				ClientSay(UTPlayerState, Message, ChatDestinations::System);
			}
		}
		else
		{
			if (DBReturn)
			{
				ClientSay(UTPlayerState, TEXT("DB command accepted"), ChatDestinations::System);
			}
			else
			{
				ClientSay(UTPlayerState, TEXT("DB command rejected"), ChatDestinations::System);
			}
		}
	}
}

void AUTBasePlayerController::RconExec(FString Command)
{
	ServerRconExec(Command);
}

bool AUTBasePlayerController::ServerRconExec_Validate(const FString& Command)
{
	return true;
}

void AUTBasePlayerController::ServerRconExec_Implementation(const FString& Command)
{
	if (UTPlayerState == nullptr || !UTPlayerState->bIsRconAdmin)
	{
		ClientSay(UTPlayerState, TEXT("Rcon not authenticated"), ChatDestinations::System);
		return;
	}

	if (Command.ToLower() == TEXT("adminmenu"))
	{
		FActorSpawnParameters Params;
		Params.Owner = this;
		AUTRconAdminInfo* AdminInfo = GetWorld()->SpawnActor<AUTRconAdminInfo>(AUTRconAdminInfo::StaticClass(), Params);
		return;
	}

	ConsoleCommand(Command);
}

void AUTBasePlayerController::RconKick(FString NameOrUIDStr, bool bBan, FString Reason)
{
	ServerRconKick(NameOrUIDStr, bBan, Reason);
}

bool AUTBasePlayerController::ServerRconKick_Validate(const FString& NameOrUIDStr, bool bBan, const FString& Reason) { return true; }
void AUTBasePlayerController::ServerRconKick_Implementation(const FString& NameOrUIDStr, bool bBan, const FString& Reason)
{
	AUTBaseGameMode* GM = GetWorld()->GetAuthGameMode<AUTBaseGameMode>();
	if (GM)
	{
		// Quick out if we haven't been authenticated.
		if (UTPlayerState == nullptr || !UTPlayerState->bIsRconAdmin)
		{
			if (GM->GetMatchState() != MatchState::WaitingToStart || GM->GetHostId().IsEmpty() || GM->GetHostId() != UTPlayerState->UniqueId.ToString())
			{
				ClientSay(UTPlayerState, TEXT("Rcon not authenticated"), ChatDestinations::System);
				return;
			}
		}

		GM->RconKick(NameOrUIDStr, bBan, Reason);
	}
}

void AUTBasePlayerController::RconUnban(const FString& UIDStr)
{
	ServerRconUnban(UIDStr);
}

bool AUTBasePlayerController::ServerRconUnban_Validate(const FString& UIDStr) { return true; }
void AUTBasePlayerController::ServerRconUnban_Implementation(const FString& UIDStr)
{
	// Quick out if we haven't been authenticated.
	if (UTPlayerState == nullptr || !UTPlayerState->bIsRconAdmin)
	{
		ClientSay(UTPlayerState, TEXT("Rcon not authenticated"), ChatDestinations::System);
		return;
	}

	AUTBaseGameMode* GM = GetWorld()->GetAuthGameMode<AUTBaseGameMode>();
	if (GM)
	{
		GM->RconUnban(UIDStr);
	}

}



void AUTBasePlayerController::RconMessage(const FString& DestinationId, const FString &Message)
{
	ServerRconMessage(DestinationId, Message);
}

bool AUTBasePlayerController::ServerRconMessage_Validate(const FString& DestinationId, const FString &Message) { return true; }
void AUTBasePlayerController::ServerRconMessage_Implementation(const FString& DestinationId, const FString &Message)
{
	if (UTPlayerState && UTPlayerState->bIsRconAdmin)
	{
		AUTBaseGameMode* GameMode = GetWorld()->GetAuthGameMode<AUTBaseGameMode>();
		if (GameMode)
		{
			GameMode->SendRconMessage(DestinationId, Message);
		}
	}
}


void AUTBasePlayerController::HandleNetworkFailureMessage(enum ENetworkFailure::Type FailureType, const FString& ErrorString)
{
}

void AUTBasePlayerController::ClientCloseAllUI_Implementation(bool bExceptDialogs)
{
	UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(Player);
	if (LocalPlayer)
	{
		LocalPlayer->CloseAllUI(bExceptDialogs);
	}
}

#if !UE_SERVER
void AUTBasePlayerController::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateInputMode();

	if (bRequestShowMenu)
	{
		bRequestShowMenu = false;
		ShowMenu(TEXT(""));
	}
}

void AUTBasePlayerController::UpdateInputMode(bool bForce)
{
	EInputMode::Type NewInputMode = EInputMode::EIM_None;
	UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(Player);
	if (LocalPlayer)
	{
		UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(LocalPlayer->GetGameInstance());
		if (UTGameInstance && UTGameInstance->bLevelIsLoading)
		{
			return;
		}

		bool bSetWidgetFocus = true;

		//Menus default to UI
		if (LocalPlayer->AreMenusOpen())
		{
			NewInputMode = EInputMode::EIM_UIOnly;

			if (LocalPlayer->IsQuickChatOpen())
			{
				bSetWidgetFocus = false;
			}
		}
		else
		{
			//Give blueprints a chance to set the input
			AUTHUD* UTHUD = Cast<AUTHUD>(MyHUD);
			if (UTHUD != nullptr)
			{
				NewInputMode = UTHUD->GetInputMode();
			}
			
			//Default to game only if no other input mode is wanted
			if (NewInputMode == EInputMode::EIM_None)
			{
				NewInputMode = EInputMode::EIM_GameOnly;
			}
		}

		//Apply the new input if it needs to be changed
		if (bForce || (NewInputMode != InputMode && NewInputMode != EInputMode::EIM_None) )
		{
			TSharedPtr<SWidget> WidgetToFocus = LocalPlayer->GetBestWidgetToFocus(); 
			InputMode = NewInputMode;
			switch (NewInputMode)
			{
			case EInputMode::EIM_GameOnly:
				Super::SetInputMode(FInputModeGameOnly());
				break;
			case EInputMode::EIM_GameAndUI:
				Super::SetInputMode(FInputModeGameAndUI().SetLockMouseToViewportBehavior(EMouseLockMode::LockOnCapture).SetWidgetToFocus(WidgetToFocus));
				break;
			case EInputMode::EIM_UIOnly:
				if (bSetWidgetFocus)
				{
					Super::SetInputMode(FInputModeUIOnly().SetLockMouseToViewportBehavior(EMouseLockMode::LockOnCapture).SetWidgetToFocus(WidgetToFocus));
				}
				else
				{
					Super::SetInputMode(FInputModeUIOnly().SetLockMouseToViewportBehavior(EMouseLockMode::LockOnCapture));
				}
				break;
			}
		}

		bShowMouseCursor = (InputMode == EInputMode::EIM_GameAndUI || InputMode == EInputMode::EIM_UIOnly);
	}
}
#endif

bool AUTBasePlayerController::ServerSetAvatar_Validate(FName NewAvatar) { return true; }

void AUTBasePlayerController::ServerSetAvatar_Implementation(FName NewAvatar)
{
	if (UTPlayerState)
	{
		UTPlayerState->Avatar = NewAvatar;
	}

}

void AUTBasePlayerController::SendStatsIDToServer()
{
	UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(Player);
	if (UTLocalPlayer)
	{
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub != nullptr)
		{
			IOnlineIdentityPtr OnlineIdentityInterface = OnlineSub->GetIdentityInterface();
			if (OnlineIdentityInterface.IsValid())
			{
				if (OnlineIdentityInterface->GetLoginStatus(UTLocalPlayer->GetControllerId()))
				{
					TSharedPtr<const FUniqueNetId> UserId = OnlineIdentityInterface->GetUniquePlayerId(UTLocalPlayer->GetControllerId());
					if (UserId.IsValid())
					{
						ServerReceiveStatsID(UserId->ToString());
					}
					/*
					#if WITH_PROFILE
					if (GetNetMode() != NM_DedicatedServer)
					{
					InitializeMcpProfile();
					}
					#endif
					*/
				}
				else
				{
					//OnLoginStatusChangedDelegate = OnlineIdentityInterface->AddOnLoginStatusChangedDelegate_Handle(LP->GetControllerId(), FOnLoginStatusChangedDelegate::CreateUObject(this, &AUTPlayerController::OnLoginStatusChanged));
				}
			}
		}
	}
}

void AUTBasePlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();
	
	UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(Player);
	if (UTLocalPlayer)
	{
		ServerChangeClanName(UTLocalPlayer->GetClanName());
		ServerSetAvatar(UTLocalPlayer->GetAvatar());
		UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(UTLocalPlayer->GetGameInstance());
		if (UTGameInstance)
		{
			UTGameInstance->bLevelIsLoading = false;
		}
	}

	SendStatsIDToServer();
	SendCosmeticsToServer();

	if (GetNetMode() == NM_Client || GetNetMode() == NM_Standalone)
	{
		InitializeHeartbeatManager();
	}
}

void AUTBasePlayerController::SendCosmeticsToServer()
{
	UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(Player);
	if (UTLocalPlayer)
	{
		FString CosmeticsUsage = FString::Printf(TEXT("?Hat=%s?LeaderHat=%s?Eyewear=%s?GroupTaunt=%s?Taunt=%s?Taunt2=%s?Intro=%s?HatVar=%d?EyewearVar=%d?"), 
			*UTLocalPlayer->GetHatPath(), 
			*UTLocalPlayer->GetLeaderHatPath(),
			*UTLocalPlayer->GetEyewearPath(), 
			*UTLocalPlayer->GetGroupTauntPath(), 
			*UTLocalPlayer->GetTauntPath(), 
			*UTLocalPlayer->GetTaunt2Path(), 
			*UTLocalPlayer->GetIntroPath(),
			UTLocalPlayer->GetHatVariant(), 
			UTLocalPlayer->GetEyewearVariant());

		ServerReceiveCosmetics(CosmeticsUsage);
	}
}

bool AUTBasePlayerController::ServerReceiveCosmetics_Validate(const FString& CosmeticString)
{
	return true;
}

void AUTBasePlayerController::ServerReceiveCosmetics_Implementation(const FString& CosmeticString)
{
	FString InOpt;
	AUTPlayerState* PS = Cast<AUTPlayerState>(PlayerState);
	if (PS)
	{
		InOpt = UGameplayStatics::ParseOption(CosmeticString, TEXT("Hat"));
		if (InOpt.Len() > 0)
		{
			PS->ServerReceiveHatClass(InOpt);
		}
		InOpt = UGameplayStatics::ParseOption(CosmeticString, TEXT("LeaderHat"));
		if (InOpt.Len() > 0)
		{
			PS->ServerReceiveLeaderHatClass(InOpt);
		}
		InOpt = UGameplayStatics::ParseOption(CosmeticString, TEXT("Eyewear"));
		if (InOpt.Len() > 0)
		{
			PS->ServerReceiveEyewearClass(InOpt);
		}
		InOpt = UGameplayStatics::ParseOption(CosmeticString, TEXT("GroupTaunt"));
		if (InOpt.Len() > 0)
		{
			PS->ServerReceiveGroupTauntClass(InOpt);
		}
		InOpt = UGameplayStatics::ParseOption(CosmeticString, TEXT("Intro"));
		if (InOpt.Len() > 0)
		{
			PS->ServerReceiveIntroClass(InOpt);
		}
		InOpt = UGameplayStatics::ParseOption(CosmeticString, TEXT("Taunt"));
		if (InOpt.Len() > 0)
		{
			PS->ServerReceiveTauntClass(InOpt);
		}
		InOpt = UGameplayStatics::ParseOption(CosmeticString, TEXT("Taunt2"));
		if (InOpt.Len() > 0)
		{
			PS->ServerReceiveTaunt2Class(InOpt);
		}
		int32 HatVar = UGameplayStatics::GetIntOption(CosmeticString, TEXT("HatVar"), 0);
		PS->ServerReceiveHatVariant(HatVar);
		int32 EyewearVar = UGameplayStatics::GetIntOption(CosmeticString, TEXT("EyewearVar"), 0);
		PS->ServerReceiveEyewearVariant(EyewearVar);
	}
}

bool AUTBasePlayerController::ServerReceiveStatsID_Validate(const FString& NewStatsID)
{
	return true;
}

/** Store an id for stats tracking.  Right now we are using the machine ID for this PC until we have have a proper ID available.  */
void AUTBasePlayerController::ServerReceiveStatsID_Implementation(const FString& NewStatsID)
{
	if (UTPlayerState != NULL && !GetWorld()->IsPlayInEditor()) // && GetWorld()->GetNetMode() != NM_Standalone)
	{
		if (NewStatsID != UTPlayerState->StatsID)
		{
            UE_LOG(LogOnlineParty, Display, TEXT("%s sent stats id %s"), *UTPlayerState->GetPlayerName(), *NewStatsID);

			UTPlayerState->StatsID = NewStatsID;
			UTPlayerState->ReadStatsFromCloud();
			UTPlayerState->ReadMMRFromBackend();

			AUTGameState* GameState = GetWorld()->GetGameState<AUTGameState>();
			if (GameState)
			{
				TSharedRef<const FUniqueNetId> UserId = MakeShareable(new FUniqueNetIdString(NewStatsID));
				if (UserId->IsValid())
				{
					GameState->AddUserInfoQuery(UserId);
				}
			}
		}
		else
		{
			UE_LOG(UT, Warning, TEXT("ServerReceiveStatsID called twice on the same PlayerController"));
		}
	}
}

void AUTBasePlayerController::ShowAdminDialog(AUTRconAdminInfo* AdminInfo)
{
	UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(Player);
	if (UTLocalPlayer)
	{
		UTLocalPlayer->ShowAdminDialog(AdminInfo);
	}
}

void AUTBasePlayerController::ShowAdminMessage(const FString& Message)
{
	UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(Player);
	if (UTLocalPlayer)
	{
		UTLocalPlayer->ShowAdminMessage(Message);
	}
}

void AUTBasePlayerController::UTLogOut()
{
	UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(Player);
	if (UTLocalPlayer)
	{
		UTLocalPlayer->Logout();
	}
}

void AUTBasePlayerController::FriendSay(FString Message)
{
	TArray<FUniqueNetIdRepl> DesiredIds;
	AUTGameState* GameState = GetWorld()->GetGameState<AUTGameState>();
	UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(Player);
	if (GameState && LocalPlayer)
	{
		for (int32 i = 0; i <  GameState->PlayerArray.Num(); i++)
		{
			if (LocalPlayer->IsAFriend(GameState->PlayerArray[i]->UniqueId))
			{
				DesiredIds.Add(GameState->PlayerArray[i]->UniqueId);
			}
		}
	}

	if (DesiredIds.Num() > 0)
	{
		ServerFriendSay(Message, DesiredIds);	
	}
}

bool AUTBasePlayerController::ServerFriendSay_Validate(const FString& Message, const TArray<FUniqueNetIdRepl>& FriendIds) { return true; }
void AUTBasePlayerController::ServerFriendSay_Implementation(const FString& Message, const TArray<FUniqueNetIdRepl>& FriendIds)
{
	AUTGameState* GameState = GetWorld()->GetGameState<AUTGameState>();
	if (GameState)
	{
		for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
		{
			AUTBasePlayerController* UTPC = Cast<AUTBasePlayerController>(*Iterator);
			if (UTPC != nullptr && UTPC->PlayerState != nullptr)
			{
				if (FriendIds.Find(UTPC->PlayerState->UniqueId) != INDEX_NONE)
				{
					UTPC->ClientSay(UTPlayerState, Message, ChatDestinations::Friends);
				}
			}
		}
	}

	// Tell yourself
	ClientSay(UTPlayerState, Message, ChatDestinations::Friends);
}

void AUTBasePlayerController::LobbySay(FString Message)
{
	// clamp message length; aside from troll prevention this is needed for networking reasons
	Message = Message.Left(MAX_CHAT_TEXT_SIZE);
	if (AllowTextMessage(Message, false))
	{
		ServerLobbySay(Message);
	}
}

bool AUTBasePlayerController::ServerLobbySay_Validate(const FString& Message) { return true; }
void AUTBasePlayerController::ServerLobbySay_Implementation(const FString& inMessage)
{
	AUTGameMode * GameMode = GetWorld()->GetAuthGameMode<AUTGameMode>();
	FString Message = inMessage;
	if (GameMode && GameMode->IsGameInstanceServer()  && AllowTextMessage(Message, false) && PlayerState != nullptr)
	{
		GameMode->SendLobbyMessage(Message, Cast<AUTPlayerState>(PlayerState));
	}
}

UUTProfileSettings* AUTBasePlayerController::GetProfileSettings()
{
	if (Cast<UUTLocalPlayer>(Player) != nullptr) return Cast<UUTLocalPlayer>(Player)->GetProfileSettings();
	return nullptr;
}

void AUTBasePlayerController::ClientPlayMovie_Implementation(const FString& MovieName)
{
#if !UE_SERVER
	UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(GetGameInstance());
	if (UTGameInstance)
	{
		UTGameInstance->PlayMovie(MovieName, SNullWidget::NullWidget, false, false, EMoviePlaybackType::MT_Normal, false);
	}
#endif
}


void AUTBasePlayerController::ClientStopMovie_Implementation()
{
#if !UE_SERVER
	UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(GetGameInstance());
	if (UTGameInstance)
	{
		UTGameInstance->StopMovie();
	}
#endif
}

void AUTBasePlayerController::ClientEnableNetworkVoice_Implementation(bool bEnable)
{
	UUTProfileSettings* ProfileSettings = GetProfileSettings();
	ToggleSpeaking(ProfileSettings ? !ProfileSettings->bPushToTalk : bEnable);
}

void AUTBasePlayerController::StartVOIPTalking()
{
	UUTProfileSettings* ProfileSettings = GetProfileSettings();
	if (ProfileSettings && ProfileSettings->bPushToTalk)
	{
		ToggleSpeaking(true);
		
		static const FName VoiceChatFeatureName("VoiceChat");
		if (IModularFeatures::Get().IsModularFeatureAvailable(VoiceChatFeatureName))
		{
			UTVoiceChatFeature* VoiceChat = &IModularFeatures::Get().GetModularFeature<UTVoiceChatFeature>(VoiceChatFeatureName);
			VoiceChat->SetAudioInputDeviceMuted(false);
		}
	}
}

void AUTBasePlayerController::StopVOIPTalking()
{
	UUTProfileSettings* ProfileSettings = GetProfileSettings();
	if (ProfileSettings && ProfileSettings->bPushToTalk)
	{
		ToggleSpeaking(false);

		static const FName VoiceChatFeatureName("VoiceChat");
		if (IModularFeatures::Get().IsModularFeatureAvailable(VoiceChatFeatureName))
		{
			UTVoiceChatFeature* VoiceChat = &IModularFeatures::Get().GetModularFeature<UTVoiceChatFeature>(VoiceChatFeatureName);
			VoiceChat->SetAudioInputDeviceMuted(true);
		}
	}
}

bool AUTBasePlayerController::AreMenusOpen()
{
#if !UE_SERVER
	UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(Player);
	return (LocalPlayer && LocalPlayer->AreMenusOpen());
#else
	return false;
#endif
}

void AUTBasePlayerController::ExportKeyBinds()
{
	UUTProfileSettings* ProfileSettings = GetProfileSettings();
	if (ProfileSettings && ProfileSettings->bPushToTalk)
	{
		ProfileSettings->ExportKeyBinds();
	}
}
void AUTBasePlayerController::ImportKeyBinds()
{
	UUTProfileSettings* ProfileSettings = GetProfileSettings();
	if (ProfileSettings && ProfileSettings->bPushToTalk)
	{
		ProfileSettings->ImportKeyBinds();
	}
}

void AUTBasePlayerController::MarkTutorialAsCompleted(int32 TutorialMask)
{
	UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(Player);
	if (LocalPlayer)
	{
		LocalPlayer->SetTutorialFinished(TutorialMask);
	}
}

void AUTBasePlayerController::SaveProfileSettings()
{
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
	if (LP)
	{
		LP->SaveProfileSettings();
	}
}

void AUTBasePlayerController::ApplyProfileSettings()
{
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
	if (LP)
	{
		LP->ApplyProfileSettings();
	}
}

bool AUTBasePlayerController::SkipTutorialCheck()
{
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
	if (LP)
	{
		return LP->SkipTutorialCheck();
	}

	return false;
}

void AUTBasePlayerController::SetLoadingMovieToPlay(const FString& MoviePath, bool bSuppressLoadingText)
{
#if !UE_SERVER
	UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(GetGameInstance());
	if (!MoviePath.IsEmpty() && UTGameInstance)
	{
		UTGameInstance->LoadingMovieToPlay = MoviePath;
		UTGameInstance->bSuppressLoadingText =  bSuppressLoadingText;
	}
#endif
	
}

void AUTBasePlayerController::UTDumpOnlineSessionState()
{
	UOnlineEngineInterface::Get()->DumpSessionState(GetWorld());
}

void AUTBasePlayerController::UTDumpPartyState()
{
	UOnlineEngineInterface::Get()->DumpPartyState(GetWorld());
}

void AUTBasePlayerController::LaunchTutorial(FName TutorialName)
{
	UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(Player);
	if (LocalPlayer)
	{
		LocalPlayer->LaunchTutorial(TutorialName);
	}
}

void AUTBasePlayerController::NextTutorial()
{
	UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(Player);
	if (LocalPlayer)
	{
		LocalPlayer->NextTutorial();
	}
}

void AUTBasePlayerController::PrevTutorial()
{
	UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(Player);
	if (LocalPlayer)
	{
		LocalPlayer->PrevTutorial();
	}
}


void AUTBasePlayerController::RepeatTutorial()
{
	UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(Player);
	if (LocalPlayer)
	{
		LocalPlayer->RestartTutorial();
	}
}

FText AUTBasePlayerController::GetTutorialSectionText(TEnumAsByte<ETutorialSections::Type> Section) const
{
	UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(Player);
	if (LocalPlayer)
	{
		return LocalPlayer->GetTutorialSectionText(Section);
	}

	return FText::GetEmpty();
}

void AUTBasePlayerController::InitializeHeartbeatManager()
{
	if (!HeartbeatManager)
	{
		HeartbeatManager = NewObject<UUTHeartbeatManager>(this);
		HeartbeatManager->StartManager(this);
	}
}

void AUTBasePlayerController::GuaranteedKick( const FText& KickReason, bool bKickToHubIfPossible)
{
	if (!AuthKickHandle.IsValid())
	{

		if (bKickToHubIfPossible)
		{
			ClientReturnToLobby(true,false);
		}
		else
		{
			ClientWasKicked(KickReason);
		}

		GetWorldTimerManager().SetTimer(AuthKickHandle, this, &AUTBasePlayerController::TimedKick, 1.0f, false);
	}
}

void AUTBasePlayerController::TimedKick()
{
	Destroy();
}

void AUTBasePlayerController::ClientWasKicked_Implementation(const FText& KickReason)
{
	ULocalPlayer* UTLocalPlayer = Cast<ULocalPlayer>(Player);
	if (UTLocalPlayer != nullptr)
	{
		UUTGameViewportClient* ViewportClient = Cast<UUTGameViewportClient>(UTLocalPlayer->ViewportClient);
		if (ViewportClient != nullptr)
		{
			ViewportClient->KickReason = KickReason;
		}
	}

	UPartyContext* PartyContext = Cast<UPartyContext>(UBlueprintContextLibrary::GetContext(GetWorld(), UPartyContext::StaticClass()));
	if (PartyContext)
	{
		if (PartyContext->GetPartySize() > 1)
		{
			PartyContext->LeaveParty();
		}
	}
}


FText AUTBasePlayerController::GetNextTutorialName()
{
	UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(Player);
	return (UTLocalPlayer != nullptr) ? UTLocalPlayer->GetNextTutorialName() : FText::GetEmpty();
}

FText AUTBasePlayerController::GetPrevTutorialName()
{
	UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(Player);
	return (UTLocalPlayer != nullptr) ? UTLocalPlayer->GetPrevTutorialName() : FText::GetEmpty();
}

void AUTBasePlayerController::ExportGameRulesets(FString Filename)
{
	UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(GetWorld()->GetGameInstance());
	if (UTGameInstance )
	{
		FUTGameRulesetStorage Storage;
		for (int32 i =0 ; i < 16; i++)
		{
			FString Tag;
			switch (i)
			{
				case 0 : Tag = EEpicDefaultRuleTags::FlagRun; break;
				case 1 : Tag = EEpicDefaultRuleTags::FlagRunVSAI; break;
				case 2 : Tag = EEpicDefaultRuleTags::Deathmatch; break;
				case 3 : Tag = EEpicDefaultRuleTags::Siege; break;
				case 4 : Tag = EEpicDefaultRuleTags::CTF; break;
				case 5 : Tag = EEpicDefaultRuleTags::TDM; break;
				case 6 : Tag = EEpicDefaultRuleTags::BIGCTF; break;
				case 7 : Tag = EEpicDefaultRuleTags::COMPCTF; break;
				case 8 : Tag = EEpicDefaultRuleTags::SHOWDOWN; break;
				case 9 : Tag = EEpicDefaultRuleTags::TEAMSHOWDOWN; break;
				case 11: Tag = EEpicDefaultRuleTags::DUEL; break;
				case 12: Tag = EEpicDefaultRuleTags::iDM; break;
				case 13: Tag = EEpicDefaultRuleTags::iTDM; break;
				case 14: Tag = EEpicDefaultRuleTags::iCTF; break;
				case 15: Tag = EEpicDefaultRuleTags::iCTFT; break;
			}

			FUTGameRuleset Ruleset;
			Ruleset.UniqueTag = Tag;
			UTGameInstance->InsureEpicDefaults(&Ruleset);
			Storage.Rules.Add(Ruleset);
		}

		if (Filename.Equals(TEXT("CustomGameRules.json")))
		{
			Filename = TEXT("new_") + Filename;
		}

		if (Storage.Rules.Num() > 0)
		{
			FString JsonString;
			FJsonObjectConverter::UStructToJsonObjectString(Storage, JsonString);
			FString SaveFilename = FString::Printf(TEXT("%s/%s"), *FPaths::GeneratedConfigDir(), *Filename);
			FFileHelper::SaveStringToFile(JsonString,*SaveFilename);
		}
	}
}

bool AUTBasePlayerController::IsPlayerGameMuted(AUTPlayerState* PlayerToCheck)
{
	UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(Player);
	return (LP && LP->IsPlayerGameMuted(PlayerToCheck));
}
