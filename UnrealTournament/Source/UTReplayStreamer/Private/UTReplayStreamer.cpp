// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UTReplayStreamer.h"
#include "Runtime/NetworkReplayStreaming/NullNetworkReplayStreaming/Public/NullNetworkReplayStreaming.h"
#include "EngineDefines.h"
#include "EngineSettings.h"
#include "EngineStats.h"
#include "EngineLogs.h"
#include "EngineGlobals.h"
#include "Engine/EngineBaseTypes.h"

DEFINE_LOG_CATEGORY_STATIC( LogUTReplay, Log, All );

FUTReplayStreamer::FUTReplayStreamer()
{
	FString McpConfigOverride;
	FParse::Value( FCommandLine::Get(), TEXT( "EPICAPP=" ), McpConfigOverride );

	const bool bCmdProductionEnvironment	= McpConfigOverride.Equals( TEXT( "Dev" ), ESearchCase::IgnoreCase ) || McpConfigOverride.Equals(TEXT("PublicTest"), ESearchCase::IgnoreCase);
	const bool bCmdGamedevEnvironment		= McpConfigOverride.Equals( TEXT( "DevLatest" ), ESearchCase::IgnoreCase );
	const bool bCmdLocalhostEnvironment		= McpConfigOverride.Equals( TEXT( "localhost" ), ESearchCase::IgnoreCase );

	const TCHAR* ProdURL	= TEXT( "https://utreplay-public-service-prod10.ol.epicgames.com/replay/v2/" );
	const TCHAR* GamedevURL	= TEXT( "https://utreplay-public-service-gamedev.ol.epicgames.net/replay/v2/" );
	const TCHAR* LocalURL	= TEXT( "http://localhost:8080/replay/" );

	//
	// In case nothing explicitly specified:
	//	If we're ARE a shipping build, we need to opt out of prod
	//	If we're NOT a shipping build, we have to opt in
	//
#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	ServerURL = GamedevURL;
#else
	ServerURL = ProdURL;
#endif

	// Check if the mode was explicitly specified, and override appropriately
	if ( bCmdProductionEnvironment )
	{
		ServerURL = ProdURL;
	}
	else if ( bCmdLocalhostEnvironment )
	{
		ServerURL = LocalURL;
	}
	else if ( bCmdGamedevEnvironment )
	{
		ServerURL = GamedevURL;
	}

	check( !ServerURL.IsEmpty() )
}

void FUTReplayStreamer::ProcessRequestInternal( TSharedPtr< class IHttpRequest > Request )
{
	IOnlineSubsystem* OnlineSubsystem = IOnlineSubsystem::Get();
	IOnlineIdentityPtr OnlineIdentityInterface;
	if (OnlineSubsystem) OnlineIdentityInterface = OnlineSubsystem->GetIdentityInterface();

	if ( OnlineIdentityInterface.IsValid() )
	{
		FString AuthToken = OnlineIdentityInterface->GetAuthToken( 0 );
		Request->SetHeader( TEXT( "Authorization" ), FString( TEXT( "bearer " ) ) + AuthToken );
	}
	else
	{
		UE_LOG( LogUTReplay, Warning, TEXT( "FUTReplayStreamer::ProcessRequestInternal: No identity interface." ) );
	}

	Request->ProcessRequest();
}

IMPLEMENT_MODULE( FUTReplayStreamingFactory, UTReplayStreamer )

TSharedPtr< INetworkReplayStreamer > FUTReplayStreamingFactory::CreateReplayStreamer()
{
	// use FUTReplayStreamer for remote streams
	TSharedPtr< FHttpNetworkReplayStreamer > Streamer(new FUTReplayStreamer);
	HttpStreamers.Add(Streamer);
	return Streamer;
}
