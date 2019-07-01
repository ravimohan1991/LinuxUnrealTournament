// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTPlaylistManager.h"
#include "UTGameEngine.h"



FUTGameRuleset* UUTPlaylistManager::GetRuleset(int32 PlaylistId)
{
	// Find the play list
	for (const FPlaylistItem& PlaylistEntry : Playlist)
	{
		if (PlaylistEntry.PlaylistId == PlaylistId)
		{
			UUTGameInstance* UTGameInstance = Cast<UUTGameInstance>(GetOuter());
			if (UTGameInstance )
			{
				return UTGameInstance ->GetRuleset(PlaylistEntry.RulesetTag);
			}
		}
	}

	return nullptr;
}

bool UUTPlaylistManager::IsValidPlaylist(int32 PlaylistId)
{
	// Find the play list
	for (const FPlaylistItem& PlaylistEntry : Playlist)
	{
		if (PlaylistEntry.PlaylistId == PlaylistId)
		{
			return true;
		}
	}

	return false;
}


bool UUTPlaylistManager::GetMaxTeamInfoForPlaylist(int32 PlaylistId, int32& MaxTeamCount, int32& MaxTeamSize, int32& MaxPartySize)
{
	FUTGameRuleset* Ruleset = GetRuleset(PlaylistId);
	if (Ruleset)
	{
		MaxTeamCount = Ruleset != nullptr ? Ruleset->MaxTeamCount : -1;
		MaxTeamSize = Ruleset != nullptr ? Ruleset->MaxTeamSize : -1;
		MaxPartySize = Ruleset != nullptr ? Ruleset->MaxPartySize : -1;
		return true;
	}

	return false;
}

bool UUTPlaylistManager::IsPlaylistRanked(int32 PlaylistId)
{
	for (const FPlaylistItem& PlaylistEntry : Playlist)
	{
		if (PlaylistEntry.PlaylistId == PlaylistId)
		{
			return PlaylistEntry.bRanked;
		}
	}

	return false;
}


bool UUTPlaylistManager::ShouldPlaylistSkipElo(int32 PlaylistId)
{
	for (const FPlaylistItem& PlaylistEntry : Playlist)
	{
		if (PlaylistEntry.PlaylistId == PlaylistId)
		{
			return PlaylistEntry.bSkipEloChecks;
		}
	}

	return false;
}

bool UUTPlaylistManager::GetTeamEloRatingForPlaylist(int32 PlaylistId, FString& TeamEloRating)
{
	for (const FPlaylistItem& PlaylistEntry : Playlist)
	{
		if (PlaylistEntry.PlaylistId == PlaylistId)
		{
			TeamEloRating = PlaylistEntry.TeamEloRating;
			return true;
		}
	}

	return false;
}

int32 UUTPlaylistManager::GetBotDifficulty(int32 PlaylistId)
{
	for (const FPlaylistItem& PlaylistEntry : Playlist)
	{
		if (PlaylistEntry.PlaylistId == PlaylistId)
		{
			return PlaylistEntry.BotDifficulty;
		}
	}

	return 0;
}

bool UUTPlaylistManager::AreBotsAllowed(int32 PlaylistId)
{
	for (const FPlaylistItem& PlaylistEntry : Playlist)
	{
		if (PlaylistEntry.PlaylistId == PlaylistId)
		{
			return PlaylistEntry.bAllowBots;
		}
	}

	return false;
}

bool UUTPlaylistManager::GetPlaylistId(int32 PlaylistIndex, int32& PlaylistId)
{
	if (Playlist.IsValidIndex(PlaylistIndex))
	{
		PlaylistId = Playlist[PlaylistIndex].PlaylistId;
		return true;
	}

	return false;
}

bool UUTPlaylistManager::GetGameModeForPlaylist(int32 PlaylistId, FString& GameMode)
{
	FUTGameRuleset* Ruleset = GetRuleset(PlaylistId);
	if (Ruleset)
	{
		GameMode = Ruleset->GameMode;
		return true;
	}

	return false;
}

bool UUTPlaylistManager::GetURLForPlaylist(int32 PlaylistId, FString& URL)
{
	FUTGameRuleset* Ruleset = GetRuleset(PlaylistId);
	if (Ruleset)
	{
		// Get a list of usable maps
		TArray<FString> MapList;
		Ruleset->EpicMaps.ParseIntoArray(MapList,TEXT(","), true);

		bool bIsRanked = IsPlaylistRanked(PlaylistId);

		FString StartingMap = MapList[FMath::RandRange(0, MapList.Num() - 1)];
		URL = Ruleset->GenerateURL(StartingMap, AreBotsAllowed(PlaylistId), GetBotDifficulty(PlaylistId), bIsRanked);
		URL += TEXT("?MatchmakingSession=1");
		URL += (bIsRanked) ? TEXT("?Ranked=1") : TEXT("?QuickMatch=1");

		return true;
	}

	return false;
}

void UUTPlaylistManager::UpdatePlaylistFromMCP(const FPlaylistItemStorage& MCPPlaylist)
{
	Playlist = MCPPlaylist.NewItems;
	Playlist.Sort([&](const FPlaylistItem &A, const FPlaylistItem &B)
			{
				return A.SortWeight < B.SortWeight;
			}
	);


}

FName UUTPlaylistManager::GetPlaylistSlateBadge(int32 PlaylistId)
{
	for (FPlaylistItem& PlaylistEntry : Playlist)
	{
		if (PlaylistEntry.PlaylistId == PlaylistId && !PlaylistEntry.SlateBadgeName.IsEmpty() )
		{
			return FName(*PlaylistEntry.SlateBadgeName);
		}
	}

	return NAME_None;
}

void UUTPlaylistManager::GetPlaylist(bool bRanked, TArray<FPlaylistItem*>& outList)
{
	for (int32 i=0; i < Playlist.Num(); i++)
	{
		if (Playlist[i].bRanked == bRanked) 
		{
			outList.Add(&Playlist[i]);
		}
	}
}



// TODO: Look at caching the counts 
int32 UUTPlaylistManager::HowManyRanked()
{
	int32 Cnt = 0;
	for (FPlaylistItem& PlaylistEntry : Playlist)
	{
		if (PlaylistEntry.bRanked) Cnt++;
	}
	return Cnt;
}

int32 UUTPlaylistManager::HowManyQuickPlay()
{
	return Playlist.Num() - HowManyRanked();
}
