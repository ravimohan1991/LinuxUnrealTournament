// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTAnalytics.h"
#include "Runtime/Analytics/Analytics/Public/AnalyticsEventAttribute.h"
#include "Runtime/Analytics/Analytics/Public/Interfaces/IAnalyticsProvider.h"
#include "UTMatchmakingStats.h"
#include "UTOnlineGameSettings.h"

#define STATS_VERSION 5
#define ATTEMPT_RECORDS_CONSTRAINT 100
#define ATTEMPT_SEND_CONSTRAINT 5

// Common attribution
const FString FUTMatchmakingStats::MMStatsSessionGuid = TEXT("MMStats_SessionId");
const FString FUTMatchmakingStats::MMStatsVersion = TEXT("MMStats_Version");

// Events
const FString FUTMatchmakingStats::MMStatsCompleteSearchEvent = TEXT("MMStats_CompleteSearchEvent");
const FString FUTMatchmakingStats::MMStatsSingleSearchEvent = TEXT("MMStats_SingleSearchEvent");

// Matchmaking total stats
const FString FUTMatchmakingStats::MMStatsSearchTimestamp = TEXT("MMStats_Timestamp");
const FString FUTMatchmakingStats::MMStatsSearchUserId = TEXT("MMStats_UserId");
const FString FUTMatchmakingStats::MMStatsTotalSearchTime = TEXT("MMStats_TotalSearchTime");
const FString FUTMatchmakingStats::MMStatsSearchAttemptCount = TEXT("MMStats_TotalAttemptCount");
const FString FUTMatchmakingStats::MMStatsTotalSearchResults = TEXT("MMStats_TotalSearchResultsCount");
const FString FUTMatchmakingStats::MMStatsIniSearchType = TEXT("MMStats_InitialSearchType");
const FString FUTMatchmakingStats::MMStatsEndSearchType = TEXT("MMStats_EndSearchType");
const FString FUTMatchmakingStats::MMStatsEndSearchResult = TEXT("MMStats_EndSearchResult");
const FString FUTMatchmakingStats::MMStatsEndQosDatacenterId = TEXT("MMStats_EndQosDatacenterId");
const FString FUTMatchmakingStats::MMStatsPartyMember = TEXT("MMStats_PartyMember");
const FString FUTMatchmakingStats::MMStatsSearchTypeNoneCount = TEXT("MMStats_SearchTypeNone_Count");
const FString FUTMatchmakingStats::MMStatsSearchTypeEmptyServerCount = TEXT("MMStats_EmptyServer_Count");
const FString FUTMatchmakingStats::MMStatsSearchTypeExistingSessionCount = TEXT("MMStats_SearchTypeExistingSession_Count");
const FString FUTMatchmakingStats::MMStatsSearchTypeJoinPresenceCount = TEXT("MMStats_SearchTypeJoinPresence_Count");
const FString FUTMatchmakingStats::MMStatsSearchTypeJoinInviteCount = TEXT("MMStats_SearchTypeJoinInvite_Count");

// Matchmaking attempt stats
const FString FUTMatchmakingStats::MMStatsSearchType = TEXT("MMStats_SearchType");
const FString FUTMatchmakingStats::MMStatsSearchAttemptTime = TEXT("MMStats_AttemptTime");
const FString FUTMatchmakingStats::MMStatsSearchAttemptEndResult = TEXT("MMStats_AttemptEndResult");
const FString FUTMatchmakingStats::MMStatsAttemptSearchResultCount = TEXT("MMStats_AttemptSearchResultCount");

// Qos stats
const FString FUTMatchmakingStats::MMStatsQosDatacenterId = TEXT("MMStats_QosDatacenterId");
const FString FUTMatchmakingStats::MMStatsQosTotalTime = TEXT("MMStats_QosTotalTime");
const FString FUTMatchmakingStats::MMStatsQosNumResults = TEXT("MMStats_QosNumResults");
const FString FUTMatchmakingStats::MMStatsQosSearchResultDetails = TEXT("MMStats_QosSearchDetails");

// Single search pass stats
const FString FUTMatchmakingStats::MMStatsSearchPassTime = TEXT("MMStats_SearchPassTime");
const FString FUTMatchmakingStats::MMStatsTotalDedicatedCount = TEXT("MMStats_NumDedicatedResults");
const FString FUTMatchmakingStats::MMStatsSearchPassNumResults = TEXT("MMStats_NumSearchPassResults");
const FString FUTMatchmakingStats::MMStatsTotalJoinAttemptTime = TEXT("MMStats_TotalJoinAttemptTime");

// Single search join attempt stats
const FString FUTMatchmakingStats::MMStatsSearchResultDetails = TEXT("MMStats_SearchResultDetails");
const FString FUTMatchmakingStats::MMStatsSearchJoinOwnerId = TEXT("MMStats_JoinOwnerId");
const FString FUTMatchmakingStats::MMStatsSearchJoinIsDedicated = TEXT("MMStats_JoinIsDedicated");
const FString FUTMatchmakingStats::MMStatsSearchJoinAttemptTime = TEXT("MMStats_JoinAttemptTime");
const FString FUTMatchmakingStats::MMStatsSearchJoinAttemptResult = TEXT("MMStats_JoinAttemptResult");

const FString FUTMatchmakingStats::MMStatsJoinAttemptNone_Count = TEXT("MMStats_JoinAttemptNone_Count");
const FString FUTMatchmakingStats::MMStatsJoinAttemptSucceeded_Count = TEXT("MMStats_JoinAttemptSucceeded_Count");
const FString FUTMatchmakingStats::MMStatsJoinAttemptFailedFull_Count = TEXT("MMStats_JoinAttemptFailedFull_Count");
const FString FUTMatchmakingStats::MMStatsJoinAttemptFailedTimeout_Count = TEXT("MMStats_JoinAttemptFailedTimeout_Count");
const FString FUTMatchmakingStats::MMStatsJoinAttemptFailedDenied_Count = TEXT("MMStats_JoinAttemptFailedDenied_Count");
const FString FUTMatchmakingStats::MMStatsJoinAttemptFailedDuplicate_Count = TEXT("MMStats_JoinAttemptFailedDuplicate_Count");
const FString FUTMatchmakingStats::MMStatsJoinAttemptFailedOther_Count = TEXT("MMStats_JoinAttemptFailedOther_Count");

/**
 * Debug output for the contents of a recorded stats event
 *
 * @param StatsEvent event type recorded
 * @param Attributes attribution of the event
 */
inline void PrintEventAndAttributes(const FString& StatsEvent, const TArray<FAnalyticsEventAttribute>& Attributes)
{
	UE_LOG(LogUTAnalytics, Display, TEXT("Event: %s"), *StatsEvent);
	for (int32 AttrIdx=0; AttrIdx<Attributes.Num(); AttrIdx++)
	{
		const FAnalyticsEventAttribute& Attr = Attributes[AttrIdx];
        UE_LOG(LogUTAnalytics, Display, TEXT("\t%s : %s"), *Attr.AttrName, *Attr.AttrValueString);
	}
}

FUTMatchmakingStats::FUTMatchmakingStats() :
	StatsVersion(STATS_VERSION),
	bAnalyticsInProgress(false),
	ControllerId(INVALID_CONTROLLERID)
{
}

void FUTMatchmakingStats::StartTimer(MMStats_Timer& Timer)
{
	Timer.MSecs = FPlatformTime::Seconds();
	Timer.bInProgress = true;
}

void FUTMatchmakingStats::StopTimer(MMStats_Timer& Timer)
{
	if (Timer.bInProgress)
	{
		Timer.MSecs = (FPlatformTime::Seconds() - Timer.MSecs) * 1000;
		Timer.bInProgress = false;
	}
}

void FUTMatchmakingStats::StartMatchmaking(EMatchmakingSearchType::Type InSearchType, TArray<FPlayerReservation>& PartyMembers, int32 LocalControllerId)
{
	FDateTime UTCTime = FDateTime::UtcNow();
	CompleteSearch.Timestamp = UTCTime.ToString();
	CompleteSearch.IniSearchType = InSearchType;
	ControllerId = LocalControllerId;

	for (int32 PlayerIdx = 0; PlayerIdx < PartyMembers.Num(); PlayerIdx++)
	{
		new (CompleteSearch.Members) MMStats_PartyMember(PartyMembers[PlayerIdx].UniqueId.GetUniqueNetId());
	}

	StartTimer(CompleteSearch.SearchTime);
	bAnalyticsInProgress = true;
}

void FUTMatchmakingStats::StartSearchAttempt(EMatchmakingSearchType::Type CurrentSearchType)
{
	if (bAnalyticsInProgress)
	{
		//stop any active timer in last search attempt
		if (CompleteSearch.SearchAttempts.Num() > 0)
		{
			FinalizeSearchAttempt(CompleteSearch.SearchAttempts.Last());
		}

		new (CompleteSearch.SearchAttempts) MMStats_Attempt();
		CompleteSearch.SearchAttempts.Last().SearchType = CurrentSearchType;
		StartTimer(CompleteSearch.SearchAttempts.Last().AttemptTime);
		CompleteSearch.TotalSearchAttempts++;

		if (CompleteSearch.SearchTypeCount.IsValidIndex(CurrentSearchType))
		{
			CompleteSearch.SearchTypeCount[CurrentSearchType]++;
		}

		//prevent size of attempts array goes too big
		if (CompleteSearch.SearchAttempts.Num() > ATTEMPT_RECORDS_CONSTRAINT)
		{
			CompleteSearch.SearchAttempts.RemoveAt(0);
		}
	}
}

void FUTMatchmakingStats::EndMatchmakingAttempt(EMatchmakingAttempt::Type Result)
{
	if (bAnalyticsInProgress)
	{
		if (CompleteSearch.SearchAttempts.Num() > 0)
		{
			MMStats_Attempt& SearchAttempt = CompleteSearch.SearchAttempts.Last();
			StopTimer(SearchAttempt.AttemptTime);
			SearchAttempt.AttemptResult = Result;
			CompleteSearch.TotalSearchResults += SearchAttempt.SearchPass.SearchResults.Num();
		}
		//use the latest result to update the complete search result
		CompleteSearch.EndResult = Result;
	}
}

void FUTMatchmakingStats::StartQosPass()
{
	if (bAnalyticsInProgress && CompleteSearch.SearchAttempts.Num() > 0)
	{
		StartTimer(CompleteSearch.SearchAttempts.Last().QosPass.SearchTime);
	}
}

void FUTMatchmakingStats::EndQosPass(const FUTOnlineSessionSearchBase& SearchSettings, const FString& DatacenterId)
{
	if (bAnalyticsInProgress && CompleteSearch.SearchAttempts.Num() > 0)
	{
		MMStats_Attempt& SearchAttempt = CompleteSearch.SearchAttempts.Last();
		StopTimer(SearchAttempt.QosPass.SearchTime);
		SearchAttempt.QosPass.BestDatacenterId = DatacenterId;
		for (int32 SearchIdx = 0; SearchIdx < SearchSettings.SearchResults.Num(); SearchIdx++)
		{
			const FOnlineSessionSearchResult& SearchResult = SearchSettings.SearchResults[SearchIdx];

			MMStats_QosSearchResult& NewSearchResult = *new (SearchAttempt.QosPass.SearchResults) MMStats_QosSearchResult();
			NewSearchResult.OwnerId = SearchResult.Session.OwningUserId;
			NewSearchResult.PingInMs = SearchResult.PingInMs;
			NewSearchResult.bIsValid = SearchResult.IsValid();

			FString TmpRegion;
			if (SearchResult.Session.SessionSettings.Get(SETTING_REGION, TmpRegion))
			{
				NewSearchResult.DatacenterId = TmpRegion;
			}
		}
	}
}

FUTMatchmakingStats::MMStats_SearchPass& FUTMatchmakingStats::GetCurrentSearchPass()
{
	check(CompleteSearch.SearchAttempts.Num() > 0);
	MMStats_Attempt& SearchAttempt = CompleteSearch.SearchAttempts.Last();
	return SearchAttempt.SearchPass;
}

void FUTMatchmakingStats::StartSearchPass()
{
	if (bAnalyticsInProgress && CompleteSearch.SearchAttempts.Num() > 0 )
	{
		MMStats_SearchPass& NewSearch = GetCurrentSearchPass();
		StartTimer(NewSearch.SearchTime);
	}
}

void FUTMatchmakingStats::EndSearchPass(const FUTOnlineSessionSearchBase& SearchSettings)
{
	if (bAnalyticsInProgress && CompleteSearch.SearchAttempts.Num() > 0 )
	{
		MMStats_SearchPass& CurrentSearchPass = GetCurrentSearchPass();
		StopTimer(CurrentSearchPass.SearchTime);

		for (int32 SearchIdx = 0; SearchIdx < SearchSettings.SearchResults.Num(); SearchIdx++)
		{
			const FOnlineSessionSearchResult& SearchResult = SearchSettings.SearchResults[SearchIdx];

			MMStats_SearchResult& NewSearchResult = *new (CurrentSearchPass.SearchResults) MMStats_SearchResult();
			NewSearchResult.OwnerId = SearchResult.Session.OwningUserId;
			NewSearchResult.bIsDedicated = SearchResult.Session.SessionSettings.bIsDedicated;
			NewSearchResult.bIsValid = SearchResult.IsValid();
		}
	}
}

void FUTMatchmakingStats::EndSearchPass(const FOnlineSessionSearchResult& SearchResult)
{
	if (bAnalyticsInProgress && CompleteSearch.SearchAttempts.Num() > 0)
	{
		MMStats_SearchPass& CurrentSearchPass = GetCurrentSearchPass();
		StopTimer(CurrentSearchPass.SearchTime);

		MMStats_SearchResult& NewSearchResult = *new (CurrentSearchPass.SearchResults) MMStats_SearchResult();
		NewSearchResult.OwnerId = SearchResult.Session.OwningUserId;
		NewSearchResult.bIsDedicated = SearchResult.Session.SessionSettings.bIsDedicated;
		NewSearchResult.bIsValid = SearchResult.IsValid();
	}
}

void FUTMatchmakingStats::StartJoinAttempt(int32 SessionIndex)
{
	if (bAnalyticsInProgress && CompleteSearch.SearchAttempts.Num() > 0)
	{
		MMStats_Attempt& SearchAttempt = CompleteSearch.SearchAttempts.Last();
		if (SearchAttempt.SearchPass.SearchResults.IsValidIndex(SessionIndex))
		{
			MMStats_SearchResult& SearchResult = SearchAttempt.SearchPass.SearchResults[SessionIndex];
			StartTimer(SearchResult.JoinActionTime);
		}
	}
}

void FUTMatchmakingStats::EndJoinAttempt(int32 SessionIndex, EPartyReservationResult::Type Result)
{
	if (bAnalyticsInProgress && CompleteSearch.SearchAttempts.Num() > 0)
	{
		MMStats_Attempt& SearchAttempt = CompleteSearch.SearchAttempts.Last();
		if (SearchAttempt.SearchPass.SearchResults.IsValidIndex(SessionIndex))
		{
			MMStats_SearchResult& SearchResult = SearchAttempt.SearchPass.SearchResults[SessionIndex];
			StopTimer(SearchResult.JoinActionTime);
			switch (Result)
			{
			case EPartyReservationResult::PartyLimitReached:
				SearchResult.JoinResult = EMatchmakingJoinAction::JoinFailed_Full;
				break;
			case EPartyReservationResult::RequestTimedOut:
				SearchResult.JoinResult = EMatchmakingJoinAction::JoinFailed_Timeout;
				break;
			case EPartyReservationResult::ReservationAccepted:
				SearchResult.JoinResult = EMatchmakingJoinAction::JoinSucceeded;
				break;
			case EPartyReservationResult::ReservationDenied:
				SearchResult.JoinResult = EMatchmakingJoinAction::JoinFailed_Denied;
				break;
			case EPartyReservationResult::ReservationDenied_Banned:
				SearchResult.JoinResult = EMatchmakingJoinAction::JoinFailed_Banned;
				break;
			case EPartyReservationResult::ReservationDuplicate:
				SearchResult.JoinResult = EMatchmakingJoinAction::JoinFailed_Duplicate;
				break;
			case EPartyReservationResult::ReservationRequestCanceled:
				SearchResult.JoinResult = EMatchmakingJoinAction::JoinFailed_Canceled;
				break;
			case EPartyReservationResult::GeneralError:
			case EPartyReservationResult::BadSessionId:
			case EPartyReservationResult::IncorrectPlayerCount:
			case EPartyReservationResult::ReservationNotFound:
			case EPartyReservationResult::ReservationInvalid:
			default:
				SearchResult.JoinResult = EMatchmakingJoinAction::JoinFailed_Other;
				break;
			}
		}
	}
}

void FUTMatchmakingStats::Finalize()
{
	StopTimer(CompleteSearch.SearchTime);
	for (auto& SearchAttempt : CompleteSearch.SearchAttempts)
	{
		FinalizeSearchAttempt(SearchAttempt);
	}
	bAnalyticsInProgress = false;
}

void FUTMatchmakingStats::FinalizeSearchPass(MMStats_SearchPass& SearchPass)
{
	StopTimer(SearchPass.SearchTime);
	for (int32 SessionIdx = 0; SessionIdx < SearchPass.SearchResults.Num(); SessionIdx++)
	{
		MMStats_SearchResult& SearchResult = SearchPass.SearchResults[SessionIdx];
		StopTimer(SearchResult.JoinActionTime);
	}
}

void FUTMatchmakingStats::FinalizeSearchAttempt(MMStats_Attempt& SearchAttempt){
	StopTimer(SearchAttempt.QosPass.SearchTime);
	StopTimer(SearchAttempt.AttemptTime);
	FinalizeSearchPass(SearchAttempt.SearchPass);
}

void FUTMatchmakingStats::ParseSearchAttempt(TSharedPtr<IAnalyticsProvider>& AnalyticsProvider, FGuid& SessionId, MMStats_Attempt& SearchAttempt)
{
	int32 TotalSearchResults = 0;
	int32 TotalDedicated = 0;
	double TotalJoinAttemptTime = 0;
	TArray<int32> JoinActionCount;
	JoinActionCount.AddZeroed(EMatchmakingJoinAction::JoinFailed_Other + 1);
	for (auto& SearchResult : SearchAttempt.SearchPass.SearchResults)
	{
		if (SearchResult.bIsValid)
		{
			JoinActionCount[SearchResult.JoinResult]++;
			TotalDedicated += SearchResult.bIsDedicated ? 1 : 0;
			TotalSearchResults++;
			TotalJoinAttemptTime += SearchResult.JoinActionTime.MSecs;
		}
	}
}

void FUTMatchmakingStats::ParseMatchmakingResult(TSharedPtr<IAnalyticsProvider>& AnalyticsProvider, FGuid& SessionId, const FUniqueNetId& MatchmakingUserId)
{
	//Constraint the result to be sent within the number indicated by ATTEMPT_SEND_CONSTRAINT 
	if (CompleteSearch.SearchAttempts.Num() > ATTEMPT_SEND_CONSTRAINT)
	{
		CompleteSearch.SearchAttempts.RemoveAt(0, CompleteSearch.SearchAttempts.Num() - ATTEMPT_SEND_CONSTRAINT);
	}
	for (auto& SearchAttempt : CompleteSearch.SearchAttempts)
	{
		ParseSearchAttempt(AnalyticsProvider, SessionId, SearchAttempt);
	}

}

void FUTMatchmakingStats::EndMatchmakingAndUpload(TSharedPtr<IAnalyticsProvider>& AnalyticsProvider, const FUniqueNetId& MatchmakingUserId)
{
	if (bAnalyticsInProgress)
	{
		Finalize();

		if (AnalyticsProvider.IsValid())
		{		
			// GUID representing the entire matchmaking attempt
			FGuid MMStatsGuid;
			FPlatformMisc::CreateGuid(MMStatsGuid);

			ParseMatchmakingResult(AnalyticsProvider, MMStatsGuid, MatchmakingUserId);
		}
		else
		{
			UE_LOG(LogUTAnalytics, Log, TEXT("No analytics provider for matchmaking analytics upload."));
		}
	}
}

int32 FUTMatchmakingStats::GetMMLocalControllerId()
{
	return ControllerId;
}
