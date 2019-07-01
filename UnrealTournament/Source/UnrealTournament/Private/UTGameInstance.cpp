// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTGameInstance.h"
#include "UnrealNetwork.h"
#include "UTDemoNetDriver.h"
#include "UTGameEngine.h"
#include "UTGameViewportClient.h"
#include "UTMatchmaking.h"
#include "UTParty.h"
#include "UTPartyGameState.h"
#include "UTPlaylistManager.h"
#include "UnrealTournamentFullScreenMovie.h"
#include "OnlineSubsystemUtils.h"
#include "UTLevelSummary.h"
#include "UTWorldSettings.h"
#include "Classes/Engine/ActorChannel.h"
#include "UTEpicDefaultRulesets.h"

#include "UserActivityTracking.h"

#if !UE_SERVER
#include "SUTStyle.h"
#include "SlateBasics.h"
#include "SlateExtras.h"
#include "Widgets/SUTAspectPanel.h"
#endif

#define USE_SQLITE UE_SERVER

//#if USE_SQLITE
//#include "ThirdParty/sqlite/sqlite3.h"
//#endif

/* Delays for various timers during matchmaking */
#define DELETESESSION_DELAY 1.0f

UUTGameInstance::UUTGameInstance(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Matchmaking(nullptr)
	, Party(nullptr)
{
	bSkippedFirstCheck = false;
	bLevelIsLoading = false;
	bDisablePerformanceCounters = false;
#if USE_SQLITE
	Database = nullptr;
#endif
}

void UUTGameInstance::Init()
{
	Super::Init();
	InitPerfCounters();

	IPerfCounters* PerfCounters = IPerfCountersModule::Get().GetPerformanceCounters();
	if (PerfCounters)
	{
		// Attach a handler for exec commands passed in via the perf counter query port
		PerfCounters->OnPerfCounterExecCommand() = FPerfCounterExecCommandCallback::CreateUObject(this, &ThisClass::PerfExecCmd);
	}

	if (!IsDedicatedServerInstance())
	{
		// handles all matchmaking in game
		Matchmaking = NewObject<UUTMatchmaking>(this);
		check(Matchmaking);

		Party = NewObject<UUTParty>(this);
		check(Party);

		// Initialize both after construction (each needs the pointer of the other)
		Matchmaking->Init();

		if (Party)
		{
			Party->Init();
		}
		PlaylistManager = NewObject<UUTPlaylistManager>(this);
	}
	else
	{
		PlaylistManager = NewObject<UUTPlaylistManager>(this);
	}
	

	// Search the rules directory for any rules JSON files

	FString RulesDir = FString::Printf(TEXT("%s/Rulesets"), *FPaths::GeneratedConfigDir());
	TArray<FString> RuleJsonFiles;

	// Grab a list of files in there.
	IFileManager::Get().FindFiles(RuleJsonFiles, *RulesDir, TEXT("*.json"));

	for (int32 j=0; j < RuleJsonFiles.Num(); j++)
	{
		// Load the custom game rulesets from their json file if it exists
		FString CustomGameRulesetFilename = FString::Printf(TEXT("%s/Rulesets/%s"), *FPaths::GeneratedConfigDir(), *RuleJsonFiles[j]);

		FString RulesJson;
		FFileHelper::LoadFileToString(RulesJson, *CustomGameRulesetFilename);

		if (!RulesJson.IsEmpty())
		{
			FUTGameRulesetStorage JsonGameRules;
			if ( FJsonObjectConverter::JsonObjectStringToUStruct(RulesJson, &JsonGameRules, 0,0) )
			{
				GameRulesets.AddZeroed(JsonGameRules.Rules.Num());
				for (int32 i=0; i < JsonGameRules.Rules.Num(); i++)
				{
					GameRulesets[i] = JsonGameRules.Rules[i];
					GameRulesets[i].UniqueTag = GameRulesets[i].UniqueTag.Replace(TEXT(" " ), TEXT("_"), ESearchCase::IgnoreCase);
				}
			}
		}
	}

	// Now add any epic rulesets.  

	TArray<FString> EpicRules;
	UUTEpicDefaultRulesets::GetEpicRulesets(EpicRules);

	if (EpicRules.Num() > 0)
	{
		for (int32 i=0; i < EpicRules.Num(); i++)
		{
			FUTGameRuleset NewRuleset;
			NewRuleset.UniqueTag = EpicRules[i];
			int32 ExistingIndex = GameRulesets.Find(NewRuleset);
			if (ExistingIndex != INDEX_NONE)
			{
				InsureEpicDefaults(&GameRulesets[ExistingIndex]);
			}
			else
			{
				InsureEpicDefaults(&NewRuleset);
				GameRulesets.AddUnique(NewRuleset);
			}
		}
	}

	// If this is a dedicated server, acquire the title files from the MCP right now.  Local clients will acquire them
	// upon login.

	if (IsRunningDedicatedServer())
	{
		AcquireTitleFilesFromMCP();
	}

#if USE_SQLITE
	FString DatabasePath = FPaths::GameSavedDir() / "Mods.db";
	if (sqlite3_open_v2(TCHAR_TO_ANSI(*DatabasePath), &Database, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, nullptr) == SQLITE_OK)
	{
		UE_LOG(UT, Log, TEXT("SQL DB opened"));
	}
	else
	{
		UE_LOG(UT, Warning, TEXT("SQL DB failed to open"));
	}
#endif

	FCoreUObjectDelegates::PreLoadMap.AddUObject(this, &UUTGameInstance::BeginLevelLoading);
    FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UUTGameInstance::EndLevelLoading);
}

bool UUTGameInstance::ExecDatabaseCommand(const FString& DatabaseCommand, TArray<FDatabaseRow>& DatabaseRows)
{
#if USE_SQLITE
	int DBreturn = SQLITE_ERROR;

	if (Database)
	{
		// Might want to do small amount of input scrubbing to avoid injection
		// Removing anything after the first semicolon might be enough
		// Could go to a system where parameters get bound, but seems like a pain for mods to use
		sqlite3_stmt* DatabaseStatement = nullptr;
		DBreturn = sqlite3_prepare_v2(Database, TCHAR_TO_ANSI(*DatabaseCommand), -1, &DatabaseStatement, nullptr);
		if (DBreturn != SQLITE_OK)
		{
			return false;
		}

		DBreturn = sqlite3_step(DatabaseStatement);
		while (DBreturn == SQLITE_ROW)
		{
			FDatabaseRow NewRow;
			int DBColumnCount = sqlite3_column_count(DatabaseStatement);
			for (int i = 0; i < DBColumnCount; i++)
			{
				int DBColumnType = sqlite3_column_type(DatabaseStatement, i);
				if (DBColumnType == SQLITE_TEXT)
				{
					const unsigned char* DBText = sqlite3_column_text(DatabaseStatement, i);
					NewRow.Text.Add(ANSI_TO_TCHAR((char*)DBText));
				}
				else if (DBColumnType == SQLITE_INTEGER)
				{
					int DBInteger = sqlite3_column_int(DatabaseStatement, i);
					NewRow.Text.Add(FString::Printf(TEXT("%d"), DBInteger));
				}
				else if (DBColumnType == SQLITE_FLOAT)
				{
					float DBFloat = sqlite3_column_double(DatabaseStatement, i);
					NewRow.Text.Add(FString::Printf(TEXT("%f"), DBFloat));
				}
			}
			DatabaseRows.Add(NewRow);
			DBreturn = sqlite3_step(DatabaseStatement);
		}
	}

	return (DBreturn == SQLITE_DONE) || (DBreturn == SQLITE_OK);
#endif

	return false;
}

bool UUTGameInstance::PerfExecCmd(const FString& ExecCmd, FOutputDevice& Ar)
{
	FWorldContext* CurrentWorldContext = GetWorldContext();
	if (CurrentWorldContext)
	{
		UWorld* World = CurrentWorldContext->World();
		if (World)
		{
			if (GEngine->Exec(World, *ExecCmd, Ar))
			{
				return true;
			}
			Ar.Log(FString::Printf(TEXT("ExecCmd %s not found"), *ExecCmd));
			return false;
		}
	}

	Ar.Log(FString::Printf(TEXT("WorldContext for ExecCmd %s not found"), *ExecCmd));
	return false;
}

void UUTGameInstance::StartGameInstance()
{
	if (IsRunningCommandlet())
	{
		return;
	}

	Super::StartGameInstance();

	UWorld* World = GetWorld();
	if (World)
	{
		FTimerHandle UnusedHandle;
		World->GetTimerManager().SetTimer(UnusedHandle, FTimerDelegate::CreateUObject(this, &UUTGameInstance::DeferredStartGameInstance), 0.1f, false);
	}
	else
	{
		UE_LOG(UT, Warning, TEXT("Failed to schedule DeferredStartGameInstance because the world was null during StartGameInstance"));
	}
}

void UUTGameInstance::DeferredStartGameInstance()
{
#if !UE_SERVER
	// Autodetect detail settings, if needed
	UUTGameUserSettings* Settings = CastChecked<UUTGameUserSettings>(GEngine->GetGameUserSettings());
	UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(GetFirstGamePlayer());
	if (LocalPlayer)
	{
		Settings->BenchmarkDetailSettingsIfNeeded(LocalPlayer);

		if (!Settings->bBenchmarkInProgress)
		{
			UUTGameEngine* Engine = Cast<UUTGameEngine>(GEngine);
			if (Engine)
			{
				if (Engine->PaksThatFailedToLoad.Num() > 0)
				{
					LocalPlayer->ShowMessage(NSLOCTEXT("UUTGameInstance", "FailedToLoadPaksTitle", "Could Not Load Custom Content"),
						                     NSLOCTEXT("UUTGameInstance", "FailedToLoadPaks", "One or more custom content packages were not loaded because they are not compatible with this version of Unreal Tournament.\n\nWe apologize for the inconvenience. We recommend downloading updated content packages from the content creator."), 
											 UTDIALOG_BUTTON_OK, FDialogResultDelegate(), FVector2D(0.6f, 0.4f));
					Engine->PaksThatFailedToLoad.Empty();
				}
			}
		}
	}
#endif
}

// @TODO FIXMESTEVE - we want open to be relative like it used to be
bool UUTGameInstance::HandleOpenCommand(const TCHAR* Cmd, FOutputDevice& Ar, UWorld* InWorld)
{
	// the netcode adds implicit "?game=" to network URLs that we don't want when going from online game to local game
	if (WorldContext != NULL && !WorldContext->LastURL.IsLocalInternal())
	{
		WorldContext->LastURL.RemoveOption(TEXT("game="));
	}

	// Invalidate the LastSession member in UTLocalPlayer since we connected via IP.  NOTE: clients will attempt to rejoin the session
	// of servers connected to the internet and at that point LastSession will become valid again.
	UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(GetFirstGamePlayer());
	if (LocalPlayer != nullptr)
	{
		LocalPlayer->InvalidateLastSession();
		LocalPlayer->LastConnectToIP = LocalPlayer->StripOptionsFromAddress(Cmd);
	}


	return GEngine->HandleTravelCommand(Cmd, Ar, InWorld);
}

void UUTGameInstance::HandleDemoPlaybackFailure( EDemoPlayFailure::Type FailureType, const FString& ErrorString )
{
	UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(GetFirstGamePlayer());
	if (LocalPlayer != nullptr)
	{
		LocalPlayer->ReturnToMainMenu();
	}

	// skip this if the "failure" is waiting for redirects
	if (FailureType != EDemoPlayFailure::Generic || LastTriedDemo.IsEmpty() || bRetriedDemoAfterRedirects)
	{
		if (LocalPlayer != nullptr)
		{
#if !UE_SERVER
			LocalPlayer->ShowMessage(
				NSLOCTEXT("UUTGameInstance", "ReplayErrorTitle", "Replay Error"),
				NSLOCTEXT("UUTGameInstance", "ReplayErrorMessage", "There was an error with the replay. Returning to the main menu."), UTDIALOG_BUTTON_OK, nullptr
				);
#endif
		}
	}
}

bool UUTGameInstance::IsAutoDownloadingContent()
{
#if !UE_SERVER
	UUTGameViewportClient* Viewport = Cast<UUTGameViewportClient>(GetGameViewportClient());
	return Viewport && Viewport->IsDownloadInProgress();
#else
	return false;
#endif
}


bool UUTGameInstance::RedirectDownload(const FString& PakName, const FString& URL, const FString& Checksum)
{
#if !UE_SERVER
	UUTGameViewportClient* Viewport = Cast<UUTGameViewportClient>(GetGameViewportClient());
	if (Viewport != NULL && !Viewport->CheckIfRedirectExists(FPackageRedirectReference(PakName, TEXT(""), TEXT(""), Checksum)))
	{
		if (OnDownloadCompleteDelegateHandle.IsValid())
		{
			Viewport->RemoveContentDownloadCompleteDelegate(OnDownloadCompleteDelegateHandle);
		}

		OnDownloadCompleteDelegateHandle = Viewport->RegisterContentDownloadCompleteDelegate(FContentDownloadComplete::FDelegate::CreateUObject(this, &ThisClass::OnDownloadComplete));
		Viewport->DownloadRedirect(URL, PakName, Checksum);
		return true;
	}
#endif
	return false;
}

void UUTGameInstance::OnDownloadComplete(class UUTGameViewportClient* ViewportClient, ERedirectStatus::Type RedirectStatus, const FString& PackageName)
{
	if (OnDownloadCompleteDelegateHandle.IsValid())
	{
		ViewportClient->RemoveContentDownloadCompleteDelegate(OnDownloadCompleteDelegateHandle);
	}

	if (RedirectStatus == ERedirectStatus::Completed)
	{
		if (!LastTriedDemo.IsEmpty())
		{
			LastTriedDemo.RemoveFromEnd(TEXT("?Remote"));
			GEngine->Exec(GetWorld(), *FString::Printf(TEXT("DEMOPLAY %s"), *LastTriedDemo));
			LastTriedDemo.Empty();
		}
	}
}

void UUTGameInstance::HandleGameNetControlMessage(class UNetConnection* Connection, uint8 MessageByte, const FString& MessageStr)
{
	switch (MessageByte)
	{
		case UNMT_Redirect:
		{
			UE_LOG(UT, Verbose, TEXT("Received redirect request: %s"), *MessageStr);
			TArray<FString> Pieces;
			MessageStr.ParseIntoArray(Pieces, TEXT("\n"), false);
			if (Pieces.Num() == 3)
			{
				// 0: pak name, 1: URL, 2: checksum
				RedirectDownload(Pieces[0], Pieces[1], Pieces[2]);
			}
			break;
		}
		default:
			UE_LOG(UT, Warning, TEXT("Unexpected net control message of type %i"), int32(MessageByte));
			break;
	}
}


void UUTGameInstance::StartRecordingReplay(const FString& Name, const FString& FriendlyName, const TArray<FString>& AdditionalOptions)
{
	if (FParse::Param(FCommandLine::Get(), TEXT("NOREPLAYS")))
	{
		UE_LOG(UT, Warning, TEXT("UGameInstance::StartRecordingReplay: Rejected due to -noreplays option"));
		return;
	}

	UWorld* CurrentWorld = GetWorld();

	if (CurrentWorld == nullptr)
	{
		UE_LOG(UT, Warning, TEXT("UGameInstance::StartRecordingReplay: GetWorld() is null"));
		return;
	}

	if (CurrentWorld->WorldType == EWorldType::PIE)
	{
		UE_LOG(UT, Warning, TEXT("UGameInstance::StartRecordingReplay: Function called while running a PIE instance, this is disabled."));
		return;
	}

	if (CurrentWorld->DemoNetDriver && CurrentWorld->DemoNetDriver->IsPlaying())
	{
		UE_LOG(UT, Warning, TEXT("UGameInstance::StartRecordingReplay: A replay is already playing, cannot begin recording another one."));
		return;
	}

	FURL DemoURL;
	FString DemoName = Name;

	DemoName.ReplaceInline(TEXT("%m"), *CurrentWorld->GetMapName());

	// replace the current URL's map with a demo extension
	DemoURL.Map = DemoName;
	DemoURL.AddOption(*FString::Printf(TEXT("DemoFriendlyName=%s"), *FriendlyName));
	DemoURL.AddOption(*FString::Printf(TEXT("Remote")));

	for (const FString& Option : AdditionalOptions)
	{
		DemoURL.AddOption(*Option);
	}

	CurrentWorld->DestroyDemoNetDriver();

	const FName NAME_DemoNetDriver(TEXT("DemoNetDriver"));

	if (!GEngine->CreateNamedNetDriver(CurrentWorld, NAME_DemoNetDriver, NAME_DemoNetDriver))
	{
		UE_LOG(UT, Warning, TEXT("RecordReplay: failed to create demo net driver!"));
		return;
	}

	CurrentWorld->DemoNetDriver = Cast< UDemoNetDriver >(GEngine->FindNamedNetDriver(CurrentWorld, NAME_DemoNetDriver));

	check(CurrentWorld->DemoNetDriver != NULL);

	CurrentWorld->DemoNetDriver->SetWorld(CurrentWorld);

	// Set the new demo driver as the current collection's driver
	FLevelCollection* CurrentLevelCollection = CurrentWorld->FindCollectionByType(ELevelCollectionType::DynamicSourceLevels);
	if (CurrentLevelCollection)
	{
		CurrentLevelCollection->SetDemoNetDriver(CurrentWorld->DemoNetDriver);
	}

	/*
	if (DemoURL.Map == TEXT("_DeathCam"))
	{
		UUTDemoNetDriver* UTDemoNetDriver = Cast<UUTDemoNetDriver>(CurrentWorld->DemoNetDriver);
		if (UTDemoNetDriver)
		{
			UTDemoNetDriver->bIsLocalReplay = true;
		}
	}
	*/
	FString Error;

	if (!CurrentWorld->DemoNetDriver->InitListen(CurrentWorld, DemoURL, false, Error))
	{
		UE_LOG(UT, Warning, TEXT("Demo recording failed: %s"), *Error);
		CurrentWorld->DemoNetDriver = NULL;
	}
	else
	{
		//UE_LOG(UT, VeryVerbose, TEXT("Num Network Actors: %i"), CurrentWorld->DemoNetDriver->GetNetworkObjectList().GetObjects().Num());
		
		// propagate level actors destroyed prior to replay being initialized as this isn't normally tracked in all cases
		// NOTE: in the server case we could copy from the game net driver, but there's a lot of interdependencies between the list and the net GUID cache
		//		and anyway the code below has to be reliable for non-server cases so there's no clear value in having split codepaths
		if (CurrentWorld->DemoNetDriver->ClientConnections.Num() > 0 && CurrentWorld->DemoNetDriver->ClientConnections[0] != nullptr)
		{
			AUTWorldSettings* WS = Cast<AUTWorldSettings>(CurrentWorld->GetWorldSettings());
			if (WS != nullptr)
			{
				for (const AUTWorldSettings::FDestroyedActorInfo& Obj : WS->GetDestroyedLevelActors())
				{
					if (Obj.Level.IsValid())
					{
						FActorDestructionInfo Info;

						FNetGuidCacheObject CacheItem;
						CacheItem.PathName = Obj.ActorName;
						CacheItem.OuterGUID = CurrentWorld->DemoNetDriver->GuidCache->GetOrAssignNetGUID(Obj.Level.Get());
						// warning: copied from AssignNewNetGUID_Server()
						Info.NetGUID = FNetworkGUID((++CurrentWorld->DemoNetDriver->GuidCache->UniqueNetIDs[1] << 1) | 1);
						CurrentWorld->DemoNetDriver->GuidCache->RegisterNetGUID_Internal(Info.NetGUID, CacheItem);
						Info.PathName = Obj.ActorName.ToString();
						Info.ObjOuter = Obj.Level.Get();
                        UActorChannel* Channel = (UActorChannel*)CurrentWorld->DemoNetDriver->ClientConnections[0]->CreateChannelByName(NAME_Actor, EChannelCreateFlags::OpenedLocally);//CreateChannel(CHTYPE_Actor, 1);
						if (Channel != nullptr)
						{
							Channel->SetChannelActorForDestroy(&Info);
						}
						CurrentWorld->DemoNetDriver->DeletedNetStartupActors.Add(Obj.FullName);
					}
				}
			}
		}
	}
}
bool UUTGameInstance::PlayReplay(const FString& Name, UWorld* WorldOverride, const TArray<FString>& AdditionalOptions)
{
	UWorld* CurrentWorld = WorldOverride != nullptr ? WorldOverride : GetWorld();

	if (CurrentWorld == nullptr)
	{
		UE_LOG(UT, Warning, TEXT("UGameInstance::PlayReplay: GetWorld() is null"));
        return false;
	}

	if (CurrentWorld->WorldType == EWorldType::PIE)
	{
		UE_LOG(UT, Warning, TEXT("UUTGameInstance::PlayReplay: Function called while running a PIE instance, this is disabled."));
        return false;
	}

	CurrentWorld->DestroyDemoNetDriver();

	FURL DemoURL;
	UE_LOG(UT, Log, TEXT("PlayReplay: Attempting to play demo %s"), *Name);

	DemoURL.Map = Name;
	DemoURL.AddOption(*FString::Printf(TEXT("Remote")));

	for (const FString& Option : AdditionalOptions)
	{
		DemoURL.AddOption(*Option);
	}

	const FName NAME_DemoNetDriver(TEXT("DemoNetDriver"));

	if (!GEngine->CreateNamedNetDriver(CurrentWorld, NAME_DemoNetDriver, NAME_DemoNetDriver))
	{
		UE_LOG(UT, Warning, TEXT("PlayReplay: failed to create demo net driver!"));
        return false;
	}

	CurrentWorld->DemoNetDriver = Cast< UDemoNetDriver >(GEngine->FindNamedNetDriver(CurrentWorld, NAME_DemoNetDriver));

	check(CurrentWorld->DemoNetDriver != NULL);

	CurrentWorld->DemoNetDriver->SetWorld(CurrentWorld);

	if (DemoURL.Map == TEXT("_DeathCam"))
	{
		UUTDemoNetDriver* UTDemoNetDriver = Cast<UUTDemoNetDriver>(CurrentWorld->DemoNetDriver);
		if (UTDemoNetDriver)
		{
			UTDemoNetDriver->bIsLocalReplay = true;
		}
	}
	FString Error;

	if (!CurrentWorld->DemoNetDriver->InitConnect(CurrentWorld, DemoURL, Error))
	{
		UE_LOG(UT, Warning, TEXT("Demo playback failed: %s"), *Error);
		CurrentWorld->DestroyDemoNetDriver();
        return false;
	}
	else
	{
		FCoreUObjectDelegates::PostDemoPlay.Broadcast();
        return false;
	}

    return true;
}


void UUTGameInstance::Shutdown()
{
	if (Party)
	{
		Party->OnShutdown();
	}
	
#if USE_SQLITE
	if (Database)
	{
		sqlite3_close(Database);
	}
#endif

	Super::Shutdown();
}

/*static*/ UUTGameInstance* UUTGameInstance::Get(UObject* ContextObject)
{
	UUTGameInstance* GameInstance = NULL;
    UWorld* OwningWorld = GEngine->GetWorldFromContextObjectChecked(ContextObject);//, false);

	if (OwningWorld)
	{
		GameInstance = Cast<UUTGameInstance>(OwningWorld->GetGameInstance());
	}
	return GameInstance;
}

UUTMatchmaking* UUTGameInstance::GetMatchmaking() const
{
	return Matchmaking;
}

UUTParty* UUTGameInstance::GetParties() const
{
	return Party;
}

UUTPlaylistManager* UUTGameInstance::GetPlaylistManager() const
{
	return PlaylistManager;
}

bool UUTGameInstance::IsInSession(const FUniqueNetId& SessionId) const
{
	IOnlineSessionPtr SessionInt = Online::GetSessionInterface(/*World*/);
	if (SessionInt.IsValid())
	{
		FNamedOnlineSession* Session = SessionInt->GetNamedSession(GameSessionName);
		if (Session && Session->SessionInfo.IsValid() && Session->SessionInfo->GetSessionId() == SessionId)
		{
			return true;
		}
	}
	return false;
}

void UUTGameInstance::SafeSessionDelete(FName SessionName, FOnDestroySessionCompleteDelegate DestroySessionComplete)
{
	UWorld* World = GetWorld();
	check(World);

	IOnlineSessionPtr SessionInt = Online::GetSessionInterface(/*World*/);
	if (SessionInt.IsValid())
	{
		EOnlineSessionState::Type SessionState = SessionInt->GetSessionState(SessionName);
		if (SessionState != EOnlineSessionState::NoSession)
		{
			if (SessionState != EOnlineSessionState::Destroying)
			{
				if (SessionState != EOnlineSessionState::Creating &&
					SessionState != EOnlineSessionState::Ending)
				{
					SafeSessionDeleteTimerHandle.Invalidate();

					FOnDestroySessionCompleteDelegate CompletionDelegate;
					CompletionDelegate = FOnDestroySessionCompleteDelegate::CreateUObject(this, &UUTGameInstance::OnDeleteSessionComplete, DestroySessionComplete);

					DeleteSessionDelegateHandle = SessionInt->AddOnDestroySessionCompleteDelegate_Handle(CompletionDelegate);
					SessionInt->DestroySession(SessionName);
				}
				else
				{
					if (!SafeSessionDeleteTimerHandle.IsValid())
					{
						// Retry shortly
						FTimerDelegate RetryDelegate;
						RetryDelegate.BindUObject(this, &UUTGameInstance::SafeSessionDelete, SessionName, DestroySessionComplete);
						World->GetTimerManager().SetTimer(SafeSessionDeleteTimerHandle, RetryDelegate, DELETESESSION_DELAY, false);
					}
					else
					{
						// Timer already in flight
						TArray<FOnDestroySessionCompleteDelegate>& DestroyDelegates = PendingDeletionDelegates.FindOrAdd(SessionName);
						DestroyDelegates.Add(DestroySessionComplete);
					}
				}
			}
			else
			{
				// Destroy already in flight
				TArray<FOnDestroySessionCompleteDelegate>& DestroyDelegates = PendingDeletionDelegates.FindOrAdd(SessionName);
				DestroyDelegates.Add(DestroySessionComplete);
			}
		}
		else
		{
			UE_LOG(LogOnlineGame, Verbose, TEXT("SafeSessionDelete called on session %s in state %s, skipping."), *SessionName.ToString(), EOnlineSessionState::ToString(SessionState));
			DestroySessionComplete.ExecuteIfBound(SessionName, true);
		}

		return;
	}

	// Non shipping builds can return success, otherwise fail
	bool bWasSuccessful = !UE_BUILD_SHIPPING;
	DestroySessionComplete.ExecuteIfBound(SessionName, bWasSuccessful);
}

void UUTGameInstance::OnDeleteSessionComplete(FName SessionName, bool bWasSuccessful, FOnDestroySessionCompleteDelegate DestroySessionComplete)
{
	UE_LOG(LogOnlineGame, Verbose, TEXT("OnDeleteSessionComplete %s bSuccess: %d"), *SessionName.ToString(), bWasSuccessful);
	SafeSessionDeleteTimerHandle.Invalidate();

	IOnlineSessionPtr SessionInt = Online::GetSessionInterface(GetWorld());
	if (SessionInt.IsValid())
	{
		SessionInt->ClearOnDestroySessionCompleteDelegate_Handle(DeleteSessionDelegateHandle);
	}

	DestroySessionComplete.ExecuteIfBound(SessionName, bWasSuccessful);

	TArray<FOnDestroySessionCompleteDelegate> DelegatesCopy;
	if (PendingDeletionDelegates.RemoveAndCopyValue(SessionName, DelegatesCopy))
	{
		// All other delegates captured while destroy was in flight
		for (const FOnDestroySessionCompleteDelegate& ExtraDelegate : DelegatesCopy)
		{
			ExtraDelegate.ExecuteIfBound(SessionName, bWasSuccessful);
		}
	}
}

bool UUTGameInstance::ClientTravelToSession(int32 ControllerId, FName InSessionName)
{
	IOnlineSessionPtr SessionInt = Online::GetSessionInterface(/*World*/);
	if (SessionInt.IsValid())
	{
		FString URL;
		if (SessionInt->GetResolvedConnectString(InSessionName, URL))
		{
			APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), ControllerId);
			if (PC)
			{
				bool bRanked = false;
				UUTParty* Parties = GetParties();
				if (Parties)
				{
					UUTPartyGameState* PartyGameState = Parties->GetUTPersistentParty();
					if (PartyGameState)
					{
						bRanked = PartyGameState->IsMatchRanked();
						URL += TEXT("?PartySize=") + FString::FromInt(PartyGameState->GetPartySize());
						URL += TEXT("?PartyLeader=") + PartyGameState->GetPartyLeader()->ToString();
					}
					//Parties->NotifyPreClientTravel();
				}

				// Save the last ranked match in case we crash
				FNamedOnlineSession* Session = SessionInt->GetNamedSession(InSessionName);
				if (Session && Session->SessionInfo.IsValid())
				{
					UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(PC->Player);
					if (LP)
					{
						LP->LastRankedMatchSessionId = Session->SessionInfo->GetSessionId().ToString();
						LP->LastRankedMatchPlayerId = Online::GetIdentityInterface()->GetUniquePlayerId(LP->GetControllerId())->ToString();
						LP->LastRankedMatchTimeString = FDateTime::Now().ToString();
						LP->SaveConfig();
					}
				}

				PC->ClientTravel(URL, TRAVEL_Absolute);
				return true;
			}
		}
		else
		{
			UE_LOG(LogOnlineGame, Warning, TEXT("Failed to resolve session connect string for %s"), *InSessionName.ToString());
		}
	}
	else
	{
		UE_LOG(LogOnlineGame, Warning, TEXT("ClientTravelToSession: No online subsystem"));
	}

	return false;
}

void UUTGameInstance::BeginLevelLoading(const FString& LevelName)
{
	if (bIgnoreLevelLoad)
	{
		bIgnoreLevelLoad = false;
		return;
	}

	bLevelIsLoading	 = true;

	//Log user activity so Crash Reporter knows if we crash during level loading
	FUserActivityTracking::SetActivity(FUserActivity(FString::Printf(TEXT("BeginLevelLoading:_%s"), *LevelName)));

#if !UE_SERVER
	if (!IsDedicatedServerInstance())
	{
		bool bIsEpicMap = false;
		bool bIsMeshedMap = false;
		bool bHasRights = false;

		bShowCommunityBadge = false;
		AUTBaseGameMode* DefaultGameModeObject = AUTBaseGameMode::StaticClass()->GetDefaultObject<AUTBaseGameMode>();
		if (DefaultGameModeObject)
		{
			DefaultGameModeObject->CheckMapStatus(LevelName, bIsEpicMap, bIsMeshedMap, bHasRights);
			bShowCommunityBadge = !bIsEpicMap;
		}

		LoadingMapFriendlyName = TEXT("");

		TArray<FAssetData> MapAssets;
		GetAllAssetData(UWorld::StaticClass(), MapAssets);

		for (const FAssetData& Asset : MapAssets)
		{
			if (Asset.PackageName.ToString() == LevelName)
			{
                const FString* Title = &Asset.TagsAndValues.FindTag(NAME_MapInfo_Title).GetValue();
				if (Title != nullptr)
				{
					LoadingMapFriendlyName = *Title;
				}
				break;
			}
		}

		if (LoadingMapFriendlyName.IsEmpty())
		{
			int32 Pos = INDEX_NONE;
			LevelName.FindLastChar('/', Pos);
			LoadingMapFriendlyName = (Pos == INDEX_NONE) ? LevelName : LevelName.Right(LevelName.Len() - Pos - 1);
		}

		// Find the map title 

		if (LevelLoadText.IsEmpty())
		{
			LevelLoadText = FText::Format(NSLOCTEXT("UTGameInstance", "GenericMapLoading", "Loading {0}"), FText::FromString(LoadingMapFriendlyName));
		}

		AUTBaseGameMode* GM = GetWorld()->GetAuthGameMode<AUTBaseGameMode>();
		if (GM)
		{
			GM->OnLoadingMovieBegin();
		}

		// Grab just the map name, minus the path

		FString CleanLevelName = FPaths::GetCleanFilename(LevelName);
		FString MovieList = TEXT("");

		// This is mostly for the benefit of local replay recording
		// The preload delegate really needs the loading world context piped through now
		bool bTransitioningToSameMap = false;
		if (GetWorldContext() && GetWorldContext()->World())
		{
			bTransitioningToSameMap = GetWorldContext()->World()->GetName() == CleanLevelName;
		}

		MovieVignettes.Empty();
		VignetteIndex = 0;

		if (LevelLoadText.ToString().Contains(TEXT("UT-Entry")))
		{
			LevelLoadText = NSLOCTEXT("UTGameInstance", "MainMenu", "Returning to Main Menu");
		}

		IUnrealTournamentFullScreenMovieModule* const FullScreenMovieModule = FModuleManager::LoadModulePtr<IUnrealTournamentFullScreenMovieModule>("UnrealTournamentFullScreenMovie");
		if (FullScreenMovieModule != nullptr)
		{
			FullScreenMovieModule->OnClipFinished().Clear();
			FullScreenMovieModule->OnClipFinished().AddUObject(this, &UUTGameInstance::OnMovieClipFinished);
		}

		if (FParse::Param(FCommandLine::Get(), TEXT("nomovie")) || CleanLevelName.Equals(TEXT("ut-entry"), ESearchCase::IgnoreCase))
		{
			MovieVignettes.Add(FMapVignetteInfo(TEXT("load_generic_nosound"), FText::GetEmpty()));
			PlayLoadingMovies(true);
			return;
		}

		if (CleanLevelName.ToLower() == TEXT("tut-movementtraining"))
		{
			UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(GetFirstGamePlayer());
			if (LocalPlayer)
			{
				if (!LocalPlayer->IsTutorialMaskCompleted(TUTORIAL_Movement))
				{
					MovieVignettes.Add(FMapVignetteInfo(TEXT("intro_full"), FText::GetEmpty()));
					MovieVignettes.Add(FMapVignetteInfo(TEXT("load_generic_nosound"), FText::GetEmpty()));
					PlayLoadingMovies(false);
					return;
				}
			}
		}

		if (!LoadingMovieToPlay.IsEmpty())
		{
			MovieVignettes.Add(FMapVignetteInfo(LoadingMovieToPlay, FText::GetEmpty()));
			MovieVignettes.Add(FMapVignetteInfo(TEXT("load_generic_nosound"), FText::GetEmpty()));
			LoadingMovieToPlay = TEXT("");
			PlayLoadingMovies(!bSuppressLoadingText);
			return;
		}

		// Now Try to find the map asset for this map.

		TArray<FMapVignetteInfo> MapVignettes;
		for (const FAssetData& Asset : MapAssets)
		{
			if (Asset.PackageName.ToString() == LevelName)
			{
                const FString* VignetteArrayAsString = &Asset.TagsAndValues.FindTag(NAME_Vignettes).GetValue();
				if (VignetteArrayAsString != nullptr)
				{
					FindField<UProperty>(UUTLevelSummary::StaticClass(), NAME_Vignettes)->ImportText(**VignetteArrayAsString, &MapVignettes, 0, nullptr);
				}
			}
		}

		while (MapVignettes.Num() > 0)
		{
			int32 Next = int32(float(MapVignettes.Num()) * FMath::FRand());
			if (!MapVignettes[Next].MovieFilename.IsEmpty())
			{
				MovieVignettes.Add(MapVignettes[Next]);
			}
			MapVignettes.RemoveAt(Next);
		}

		if (MovieVignettes.Num() == 0)
		{
			MovieVignettes.Add(FMapVignetteInfo(TEXT("load_generic_nosound"), FText::GetEmpty()));
		}

		VignetteIndex = 0;
		PlayLoadingMovies(true);
	}
#endif
}

#if !UE_SERVER
void UUTGameInstance::OnMoviePlaybackFinished()
{

	bSuppressLoadingText = false;

	UUTLocalPlayer* LocalPlayer = Cast<UUTLocalPlayer>(GetFirstGamePlayer());
	if (LocalPlayer)
	{
		if (LocalPlayer->bCloseUICalledDuringMoviePlayback)
		{
			LocalPlayer->CloseAllUI(LocalPlayer->bDelayedCloseUIExcludesDialogs);
		}
		else if (LocalPlayer->bHideMenuCalledDuringMoviePlayback)
		{
			LocalPlayer->HideMenu();
		}
	}

	if (GetWorld())
	{
		AUTBaseGameMode* GM = GetWorld()->GetAuthGameMode<AUTBaseGameMode>();
		if (GM)
		{
			GM->OnLoadingMovieEnd();
		}
	}
}
#endif

void UUTGameInstance::EndLevelLoading(UWorld* sos)
{
	bLevelIsLoading	 = false;
	LevelLoadText = FText::GetEmpty();

	//Log user activity so Crash Reporter knows if we crash after leaving level loading
	FUserActivityTracking::SetActivity(FUserActivity(TEXT("EndLevelLoading")));

#if !UE_SERVER
	if (!IsDedicatedServerInstance())
	{
		if (!GIsRequestingExit)
		{
			if (GetMoviePlayer()->IsMovieCurrentlyPlaying())
			{
				GetMoviePlayer()->OnMoviePlaybackFinished().Clear();
				GetMoviePlayer()->OnMoviePlaybackFinished().AddUObject(this, &UUTGameInstance::OnMoviePlaybackFinished);

				IUnrealTournamentFullScreenMovieModule* const FullScreenMovieModule = FModuleManager::LoadModulePtr<IUnrealTournamentFullScreenMovieModule>("UnrealTournamentFullScreenMovie");
				if (FullScreenMovieModule != nullptr)
				{
					FullScreenMovieModule->WaitForMovieToFinished();
				}
			}
		}
	}
#endif
}

#if !UE_SERVER

FText UUTGameInstance::GetLevelLoadText() const
{
	if (bLevelIsLoading)
	{
		return LevelLoadText;
	}
	else if (GetMoviePlayer()->WillAutoCompleteWhenLoadFinishes())
	{
		return FText::GetEmpty();
	}

	return NSLOCTEXT("UTGameInstance","PressFireToSkip","Press Any Key to Skip");
}

FText UUTGameInstance::GetVignetteText() const
{
	if (MovieVignettes.Num() > 0 && MovieVignettes.IsValidIndex(VignetteIndex))
	{
		if (!MovieVignettes[VignetteIndex].Description.IsEmpty())	
		{
			return MovieVignettes[VignetteIndex].Description;
		}
	}

	return NSLOCTEXT("Loading","LoadingWarning","This is an early version of the game, with a lot of placeholder content. Unreal Tournament is\na collaboration between gamers,  developers and Epic Games. There is still a lot to do!\nHead over to <UT.Font.NormalText.Tween.Bold.SkyBlue>UnrealTournament.com</> to see how you can make a difference.");
}

						
EVisibility UUTGameInstance::GetLevelLoadThrobberVisibility() const
{
	return bLevelIsLoading ? EVisibility::Visible : EVisibility::Hidden;
}

EVisibility UUTGameInstance::GetLevelLoadTextVisibility() const
{
	return (bLevelIsLoading && bSuppressLoadingText) ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility UUTGameInstance::GetEpicLogoVisibility() const
{
	return bSuppressLoadingText ? EVisibility::Collapsed : EVisibility::Visible;
}

EVisibility UUTGameInstance::GetVignetteVisibility() const
{
	return bSuppressLoadingText ? EVisibility::Collapsed : EVisibility::Visible;
}

void UUTGameInstance::CreateLoadingMovieOverlay()
{
	if (!LoadingMovieOverlay.IsValid())
	{
		TSharedPtr<SOverlay> InnerOverlay;
		SAssignNew(LoadingMovieOverlay, SOverlay)
		+SOverlay::Slot()
		[
			SNew(SUTAspectPanel)     
			.bCenterPanel(true)
			.Visibility(EVisibility::SelfHitTestInvisible)
			[
				SAssignNew(InnerOverlay, SOverlay)			
			]
		];

		InnerOverlay->AddSlot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SNew(SSafeZone)
			.VAlign(VAlign_Top)
			.HAlign(HAlign_Center)
			.Padding(10.0f)
			.IsTitleSafe(true)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock)
					.TextStyle(SUTStyle::Get(),"UT.Font.NormalText.Medium.Bold")
					.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateUObject(this, &UUTGameInstance::GetLevelLoadText)))
					.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateUObject(this, &UUTGameInstance::GetLevelLoadTextVisibility)))
				]
				+SVerticalBox::Slot().HAlign(HAlign_Center).AutoHeight()
				[
					SNew(SThrobber)
					.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateUObject(this, &UUTGameInstance::GetLevelLoadThrobberVisibility)))
				]
			]
		];

		InnerOverlay->AddSlot()
		[
			SNew(SSafeZone)
			.VAlign(VAlign_Top)
			.HAlign(HAlign_Center)
			.Padding(FMargin(0.0f,100.0f,0.0f,0.0f))
			.IsTitleSafe(true)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(188.0f).HeightOverride(124.0f)
						[
							SNew(SImage)
							.Image(SUTStyle::Get().GetBrush("UT.Logo.Community"))
							.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateUObject(this, &UUTGameInstance::GetCommunityVisibility)))
						]
					]
				]
			]
		];

		InnerOverlay->AddSlot()
		[
			SNew(SSafeZone)
			.VAlign(VAlign_Bottom)
			.HAlign(HAlign_Left)
			.Padding(10.0f)
			.IsTitleSafe(true)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot().AutoWidth()
					[
						SNew(SBox).WidthOverride(810.0f).HeightOverride(290.0f)
						[
							SNew(SImage)
							.Image(SUTStyle::Get().GetBrush("UT.Logo.Loading"))
							.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateUObject(this, &UUTGameInstance::GetEpicLogoVisibility)))
						]
					]
				]
			]
		];

		InnerOverlay->AddSlot()
		[
			SNew(SSafeZone)
			.VAlign(VAlign_Bottom)
			.HAlign(HAlign_Right)
			.Padding(FMargin(10.0f,0.0f,25.0f,35.0f))
			.IsTitleSafe(true)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot().AutoHeight()
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot().HAlign(HAlign_Right)
					[
						SNew(SRichTextBlock)
						.Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateUObject(this, &UUTGameInstance::GetVignetteText)))
						.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Tween.Bold")
						.Justification(ETextJustify::Right)
						.DecoratorStyleSet(&SUTStyle::Get())
						.AutoWrapText(false)
						.Visibility(TAttribute<EVisibility>::Create(TAttribute<EVisibility>::FGetter::CreateUObject(this, &UUTGameInstance::GetVignetteVisibility)))
					]
				]
			]
		];
	}
}

void UUTGameInstance::PlayLoadingMovies(bool bStopWhenLoadingIsComnpleted) 
{
	CreateLoadingMovieOverlay();
	if (LoadingMovieOverlay.IsValid())
	{						 
		FString MovieName = TEXT("");
		for (auto Vignette : MovieVignettes)
		{
			MovieName += MovieName.IsEmpty() ? Vignette.MovieFilename : TEXT(";") + Vignette.MovieFilename;
		}

		PlayMovie(MovieName, LoadingMovieOverlay, true, bStopWhenLoadingIsComnpleted, EMoviePlaybackType::MT_LoadingLoop, true);

		bMovieWillAutocompleteWhenLoadFinishes = false;
		if (GetMoviePlayer())
		{
			bMovieWillAutocompleteWhenLoadFinishes = GetMoviePlayer()->WillAutoCompleteWhenLoadFinishes();
		}
	}
}


// Plays a full screen movie
void UUTGameInstance::PlayMovie(const FString& MoviePlayList, TSharedPtr<SWidget> SlateOverlayWidget, bool bSkippable, bool bAutoComplete, TEnumAsByte<EMoviePlaybackType> PlaybackType, bool bForce)
{
	IUnrealTournamentFullScreenMovieModule* const FullScreenMovieModule = FModuleManager::LoadModulePtr<IUnrealTournamentFullScreenMovieModule>("UnrealTournamentFullScreenMovie");
	if (FullScreenMovieModule != nullptr)
	{
		FullScreenMovieModule->PlayMovie(MoviePlayList, SlateOverlayWidget.IsValid() ? SlateOverlayWidget : SNullWidget::NullWidget, bSkippable, bAutoComplete, PlaybackType, bForce);
	}
}

// Stops a movie from playing
void UUTGameInstance::StopMovie()
{
	IUnrealTournamentFullScreenMovieModule* const FullScreenMovieModule = FModuleManager::LoadModulePtr<IUnrealTournamentFullScreenMovieModule>("UnrealTournamentFullScreenMovie");
	if (FullScreenMovieModule != nullptr)
	{
		FullScreenMovieModule->StopMovie();
		FullScreenMovieModule->WaitForMovieToFinished();
	}
}

// returns true if a movie is currently being played
bool UUTGameInstance::IsMoviePlaying()
{
	IUnrealTournamentFullScreenMovieModule* const FullScreenMovieModule = FModuleManager::LoadModulePtr<IUnrealTournamentFullScreenMovieModule>("UnrealTournamentFullScreenMovie");
	if (FullScreenMovieModule != nullptr)
	{
		return FullScreenMovieModule->IsMoviePlaying();
	}

	return false;
}


void UUTGameInstance::OnMovieClipFinished(const FString& ClipName)
{
	VignetteIndex = (VignetteIndex + 1) % MovieVignettes.Num(); 
}

EVisibility UUTGameInstance::GetCommunityVisibility() const
{
	return bShowCommunityBadge ? EVisibility::Visible : EVisibility::Collapsed;
}
#endif

int32 UUTGameInstance::GetBotSkillForTeamElo(int32 TeamElo)
{
	if (TeamElo > 1500)
	{
		return 3 + FMath::Clamp((float(TeamElo) - 1500.f)/60.f , 0.f, 4.f);
	}
	else if (TeamElo > 1400)
	{
		return 2;
	}

	return 1;
}

void UUTGameInstance::AcquireTitleFilesFromMCP()
{
	if (RequestedTitleFiles.Num() > 0) return; // Already in progress

	bReceivedTitleFiles = false;

	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	if (OnlineSubsystem) 
	{
		IOnlineTitleFilePtr OnlineTitleFileInterface = OnlineSubsystem->GetTitleFileInterface();
		if (OnlineTitleFileInterface.IsValid())
		{
			OnReadTitleFileCompleteDelegate = OnlineTitleFileInterface->AddOnReadFileCompleteDelegate_Handle(FOnReadFileCompleteDelegate::CreateUObject(this, &ThisClass::OnReadTitleFileComplete));
			OnEnumerateTitleFilesCompleteDelegate = OnlineTitleFileInterface->AddOnEnumerateFilesCompleteDelegate_Handle(FOnEnumerateFilesCompleteDelegate::CreateUObject(this, &ThisClass::OnEnumerateTitleFilesComplete));

			RequestedTitleFiles.Add(UUTLocalPlayer::GetMCPStorageFilename());
			RequestedTitleFiles.Add(UUTLocalPlayer::GetOnlineSettingsFilename());
			RequestedTitleFiles.Add(UUTLocalPlayer::GetMCPAnnouncementFilename());
			RequestedTitleFiles.Add(UUTLocalPlayer::GetMCPGameRulesUpdateFilename());
			RequestedTitleFiles.Add(UUTLocalPlayer::GetMCPPlaylistFilename());

			OnlineTitleFileInterface->EnumerateFiles();
			return;
		}
	}
}

void UUTGameInstance::OnEnumerateTitleFilesComplete(bool bWasSuccessful, const FString& lol)
{
	if (bWasSuccessful && RequestedTitleFiles.Num() > 0)
	{
		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		if (OnlineSubsystem) 
		{
			IOnlineTitleFilePtr OnlineTitleFileInterface = OnlineSubsystem->GetTitleFileInterface();
			if (OnlineTitleFileInterface.IsValid())
			{
				OnlineTitleFileInterface->ReadFile(RequestedTitleFiles[0]);
			}
		}
	}
}


void UUTGameInstance::OnReadTitleFileComplete(bool bWasSuccessful, const FString& Filename)
{
	if (Filename == UUTLocalPlayer::GetMCPGameRulesUpdateFilename())
	{
		if (bWasSuccessful)
		{
			IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
			if (OnlineSubsystem) 
			{
				IOnlineTitleFilePtr OnlineTitleFileInterface = OnlineSubsystem->GetTitleFileInterface();
				if (OnlineTitleFileInterface.IsValid())
				{
					FString JsonString = TEXT("");
					TArray<uint8> FileContents;
					OnlineTitleFileInterface->GetFileContents(UUTLocalPlayer::GetMCPGameRulesUpdateFilename(), FileContents);
					FileContents.Add(0);
					JsonString = ANSI_TO_TCHAR((char*)FileContents.GetData());
					ProcessMCPRulesetUpdate(JsonString);
				}
			}
		}
	}
	else if (Filename == UUTLocalPlayer::GetMCPPlaylistFilename())
	{
		if (bWasSuccessful)
		{
			IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
			if (OnlineSubsystem) 
			{
				IOnlineTitleFilePtr OnlineTitleFileInterface = OnlineSubsystem->GetTitleFileInterface();
				if (OnlineTitleFileInterface.IsValid())
				{
					FString JsonString = TEXT("");
					TArray<uint8> FileContents;
					OnlineTitleFileInterface->GetFileContents(UUTLocalPlayer::GetMCPPlaylistFilename(), FileContents);
					FileContents.Add(0);
					JsonString = ANSI_TO_TCHAR((char*)FileContents.GetData());
				
					FPlaylistItemStorage PlaylistItemStorage;
					if ( FJsonObjectConverter::JsonObjectStringToUStruct(JsonString, &PlaylistItemStorage, 0,0) )
					{
						if (PlaylistManager)
						{
							PlaylistManager->UpdatePlaylistFromMCP(PlaylistItemStorage);
						}
					}
				}
			}
		}
	}
	else if (bWasSuccessful)
	{
		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		if (OnlineSubsystem) 
		{
			IOnlineTitleFilePtr OnlineTitleFileInterface = OnlineSubsystem->GetTitleFileInterface();
			if (OnlineTitleFileInterface.IsValid())
			{
				TArray<uint8> FileContents;
				OnlineTitleFileInterface->GetFileContents(Filename, FileContents);
				FileContents.Add(0);		
	
				UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(GetFirstGamePlayer());
				if (UTLocalPlayer)
				{
					UTLocalPlayer->ProcessTitleFile(Filename, FileContents);
				}
			}
		}
	}

	int32 Index = RequestedTitleFiles.Find(Filename);
	if (Index != INDEX_NONE) RequestedTitleFiles.RemoveAt(Index);

	if (RequestedTitleFiles.Num() > 0)
	{
        OnEnumerateTitleFilesComplete(true);
	}
	else
	{

		bReceivedTitleFiles = true;

		UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(GetFirstGamePlayer());
		if (UTLocalPlayer && UTLocalPlayer->LoginPhase != ELoginPhase::LoggedIn)
		{
			UTLocalPlayer->FinalizeLogin();
		}

		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		if (OnlineSubsystem) 
		{
			IOnlineTitleFilePtr OnlineTitleFileInterface = OnlineSubsystem->GetTitleFileInterface();
			if (OnlineTitleFileInterface.IsValid())
			{
				OnlineTitleFileInterface->ClearOnReadFileCompleteDelegate_Handle(OnReadTitleFileCompleteDelegate);
				OnlineTitleFileInterface->ClearOnEnumerateFilesCompleteDelegate_Handle(OnEnumerateTitleFilesCompleteDelegate);
			}
		}
	}
}

void UUTGameInstance::InsureEpicDefaults(FUTGameRuleset* NewRuleset)
{
	// First, attempt to apply the hard coded rules.
	UUTEpicDefaultRulesets::InsureEpicDefaults(NewRuleset);
}

FUTGameRuleset* UUTGameInstance::GetRuleset(const FString& RulesetTag)
{
	for (int32 i=0; i < GameRulesets.Num(); i++)
	{
		if (GameRulesets[i].UniqueTag == RulesetTag)
		{
			return &GameRulesets[i];
		}
	}

	return nullptr;
}

void UUTGameInstance::ProcessMCPRulesetUpdate(FString MCPRulesetJson)
{
	if (MCPRulesetJson.IsEmpty()) return;

	FUTGameRulesetStorage JsonGameRules;
	if ( FJsonObjectConverter::JsonObjectStringToUStruct(MCPRulesetJson, &JsonGameRules, 0,0) )
	{
		for (int32 i = 0; i < JsonGameRules.Rules.Num(); i++)
		{
			// Look to see if this rule already exists in the database
			
			bool bUpdated = false;
			for (int32 j=0; j < GameRulesets.Num(); j++)
			{
				if ( GameRulesets[j] == JsonGameRules.Rules[i] )
				{
					GameRulesets[j].UniqueTag = JsonGameRules.Rules[i].UniqueTag;

					// Remove spaces in the UniqueTag
					GameRulesets[j].UniqueTag = GameRulesets[j].UniqueTag.Replace(TEXT(" " ), TEXT("_"), ESearchCase::IgnoreCase);

					GameRulesets[j].Categories = JsonGameRules.Rules[i].Categories;
					GameRulesets[j].Title = JsonGameRules.Rules[i].Title;
					GameRulesets[j].Tooltip = JsonGameRules.Rules[i].Tooltip;
					GameRulesets[j].Description = JsonGameRules.Rules[i].Description;
					GameRulesets[j].MapPrefixes = JsonGameRules.Rules[i].MapPrefixes;
					GameRulesets[j].EpicMaps = JsonGameRules.Rules[i].EpicMaps;
					GameRulesets[j].DefaultMap = JsonGameRules.Rules[i].DefaultMap;
					GameRulesets[j].CustomMapList = JsonGameRules.Rules[i].CustomMapList;
					GameRulesets[j].MaxPlayers = JsonGameRules.Rules[i].MaxPlayers;
					GameRulesets[j].DisplayTexture = JsonGameRules.Rules[i].DisplayTexture;
					GameRulesets[j].GameMode = JsonGameRules.Rules[i].GameMode;
					GameRulesets[j].GameOptions = JsonGameRules.Rules[i].GameOptions;
					GameRulesets[j].RequiredPackages = JsonGameRules.Rules[i].RequiredPackages;
					GameRulesets[j].bTeamGame = JsonGameRules.Rules[i].bTeamGame;
					GameRulesets[j].bCompetitiveMatch = JsonGameRules.Rules[i].bCompetitiveMatch;
					GameRulesets[j].OptionFlags = JsonGameRules.Rules[i].OptionFlags;

					if (JsonGameRules.Rules[i].EpicForceUIVisibility < 0)
					{
						GameRulesets[j].bHideFromUI = true;
					}
					else if (JsonGameRules.Rules[i].EpicForceUIVisibility > 0)
					{
						GameRulesets[j].bHideFromUI = false;
					}

					bUpdated = true;
					break;
				}			
			}
		
			if (!bUpdated)
			{
				// This is a new rule pushed from Epic.. add it.
				GameRulesets.AddUnique(JsonGameRules.Rules[i]);
			}
		}
	}
}

void UUTGameInstance::CheckForNewUpdate()
{

	if (bSkippedFirstCheck)
	{
		IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
		if (OnlineSubsystem) 
		{
			IOnlineTitleFilePtr OnlineTitleFileInterface = OnlineSubsystem->GetTitleFileInterface();
			if (OnlineTitleFileInterface.IsValid())
			{
				OnReadTitleFileCompleteDelegate = OnlineTitleFileInterface->AddOnReadFileCompleteDelegate_Handle(FOnReadFileCompleteDelegate::CreateUObject(this, &ThisClass::OnReadTitleFileComplete));
				OnEnumerateTitleFilesCompleteDelegate = OnlineTitleFileInterface->AddOnEnumerateFilesCompleteDelegate_Handle(FOnEnumerateFilesCompleteDelegate::CreateUObject(this, &ThisClass::OnEnumerateTitleFilesComplete));

				RequestedTitleFiles.Add(UUTLocalPlayer::GetMCPStorageFilename());
				OnlineTitleFileInterface->EnumerateFiles();
			}
		}
	}

	bSkippedFirstCheck = true;

}
