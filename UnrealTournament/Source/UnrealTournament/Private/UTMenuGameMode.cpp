// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTMenuGameMode.h"
#include "GameFramework/GameMode.h"
#include "UTGameMode.h"
#include "UTDMGameMode.h"
#include "UTWorldSettings.h"
#include "UTProfileSettings.h"
#include "UTAnalytics.h"

AUTMenuGameMode::AUTMenuGameMode(const class FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	GameStateClass = AUTGameState::StaticClass();
	PlayerStateClass = AUTPlayerState::StaticClass();
	PlayerControllerClass = AUTPlayerController::StaticClass();

	static ConstructorHelpers::FObjectFinder<USoundBase> DefaultMusicRef[] = 
		{ 
				TEXT("/Game/RestrictedAssets/Audio/Music/Music_UTMuenu_New01.Music_UTMuenu_New01"), 
				TEXT("/Game/RestrictedAssets/Audio/Music/Music_FragCenterIntro.Music_FragCenterIntro") 
		};
	

	for (int32 i = 0; i < ARRAY_COUNT(DefaultMusicRef); i++)
	{
		DefaultMusics.Add(DefaultMusicRef[i].Object);
	}
}

void AUTMenuGameMode::RestartGame()
{
	return;
}
void AUTMenuGameMode::BeginGame()
{	
	return;
}

void AUTMenuGameMode::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	IConsoleVariable* PhotoCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Photography.Allow"));
	if (PhotoCVar)
	{
		PhotoCVar->Set(0, ECVF_SetByCode);
	}

	ClearPause();

	AUTWorldSettings* WorldSettings;
	WorldSettings = Cast<AUTWorldSettings>(GetWorld()->GetWorldSettings());
	if (WorldSettings)
	{
		FString MenuMusicPath = MenuMusicAssetName;
		// We use a second var to make sure we never write this value back out
		
		if ( MenuMusicAssetName != TEXT("") )
		{
			MenuMusic = LoadObject<USoundBase>(NULL, *MenuMusicPath, NULL, LOAD_NoWarn | LOAD_Quiet);
			if (MenuMusic)
			{
				MenuMusicAC = UGameplayStatics::SpawnSoundAtLocation( this, MenuMusic, FVector(0,0,0), FRotator::ZeroRotator, 1.0, 1.0 );
			}
		}
	}
	return;
}

void AUTMenuGameMode::OnLoadingMovieEnd()
{
	Super::OnLoadingMovieEnd();

	AUTBasePlayerController* PC = Cast<AUTBasePlayerController>(GetWorld()->GetFirstPlayerController());
	if (PC != NULL)
	{
		PC->ClientReturnedToMenus();
		UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(PC->Player);
		if (LP != NULL)
		{
			if (LP->LoginPhase == ELoginPhase::Offline || LP->LoginPhase == ELoginPhase::LoggedIn)
			{
				ShowMenu(PC);
			}
			else
			{
				LP->GetAuth();
			}
		}
	}
}

void AUTMenuGameMode::ShowMenu(AUTBasePlayerController* PC)
{
	if (PC != NULL)
	{
		PC->ClientReturnedToMenus();
		FURL& LastURL = GEngine->GetWorldContextFromWorld(GetWorld())->LastURL;
		bool bReturnedFromChallenge = LastURL.HasOption(TEXT("showchallenge"));
		bool bReturnedFromHub = LastURL.HasOption(TEXT("returnfromhub"));

		PC->ShowMenu((bReturnedFromChallenge ? TEXT("showchallenge") : ( bReturnedFromHub ? TEXT("showbrowser") : TEXT(""))));
		UUTProfileSettings* ProfileSettings = PC->GetProfileSettings();

		LastURL.RemoveOption(TEXT("showchallenge"));
		LastURL.RemoveOption(TEXT("returnfromhub"));


#if !UE_SERVER
		UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(PC->Player);
		// start with tutorial menu if requested
		if (LastURL.HasOption(TEXT("tutorialmenu")))
		{
			// NOTE: If we are in a party, never return to the tutorial menu
			if (LP != NULL && !LP->IsInAnActiveParty())
			{
				if (FUTAnalytics::IsAvailable())
				{
					FUTAnalytics::FireEvent_UTTutorialStarted(Cast<AUTPlayerController>(PC), FString("Onboarding"));
				}

				LP->OpenTutorialMenu();
			}
			// make sure this doesn't get kept around
			LastURL.RemoveOption(TEXT("tutorialmenu"));
		}



		LP->CheckForNewUpdate();
#endif
	}

}



void AUTMenuGameMode::RestartPlayer(AController* aPlayer)
{
	return;
}

TSubclassOf<AGameMode> AUTMenuGameMode::SetGameMode(const FString& MapName, const FString& Options, const FString& Portal)
{
	// note that map prefixes are handled by the engine code so we don't need to do that here
	// TODO: mod handling?
	return (MapName == TEXT("UT-Entry")) ? GetClass() : AUTDMGameMode::StaticClass();
}

void AUTMenuGameMode::Logout( AController* Exiting )
{
	Super::Logout(Exiting);

	AUTBasePlayerController* PC = Cast<AUTBasePlayerController>(Exiting);
	if (PC != NULL)
	{
		PC->HideMenu();
	}
}
