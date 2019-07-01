// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTProgressionStorage.h"
#include "UTProfileSettings.h"

UUTProgressionStorage::UUTProgressionStorage(const FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	bNeedsUpdate = false;
}

void UUTProgressionStorage::VersionFixup()
{
	TokensCommit(); // just in case any achievements failed to unlock previously due to bug
}

bool UUTProgressionStorage::HasTokenBeenPickedUpBefore(FName TokenUniqueID)
{
	return FoundTokenUniqueIDs.Contains(TokenUniqueID);
}

void UUTProgressionStorage::TokenPickedUp(FName TokenUniqueID)
{
	bNeedsUpdate = true;
	TempFoundTokenUniqueIDs.AddUnique(TokenUniqueID);
}

void UUTProgressionStorage::TokenRevoke(FName TokenUniqueID)
{
	bNeedsUpdate = true;
	TempFoundTokenUniqueIDs.Remove(TokenUniqueID);
}

bool UUTProgressionStorage::CheckTokens(FString TokenId, int32 MaxNumberOfTokens)
{
	TArray<FName> List;
	FNumberFormattingOptions Options;
	Options.MinimumIntegralDigits = 3;

	for (int32 i = 0; i < MaxNumberOfTokens; i++)
	{
		List.Add(FName(*FString::Printf(TEXT("%s_token_%s"), *TokenId, *FText::AsNumber(i, &Options).ToString())));
	}

	bool bCompletedTutorial = true;
	for (FName TestToken : List)
	{
		if (!FoundTokenUniqueIDs.Contains(TestToken))
		{
			bCompletedTutorial = false;
			break;
		}
	}

	return bCompletedTutorial;
}

bool UUTProgressionStorage::HasAnyTokens(FString TokenId, int32 MaxNumberOfTokens)
{
	TArray<FName> List;
	FNumberFormattingOptions Options;
	Options.MinimumIntegralDigits = 3;

	for (int32 i = 0; i < MaxNumberOfTokens; i++)
	{
		List.Add(FName(*FString::Printf(TEXT("%s_token_%s"), *TokenId, *FText::AsNumber(i, &Options).ToString())));
	}

	bool bCompletedTutorial = false;
	for (FName TestToken : List)
	{
		if (FoundTokenUniqueIDs.Contains(TestToken))
		{
			bCompletedTutorial = true;
			break;
		}
	}

	return bCompletedTutorial;
}


void UUTProgressionStorage::TokensCommit()
{
	for (FName ID : TempFoundTokenUniqueIDs)
	{
		FoundTokenUniqueIDs.AddUnique(ID);
	}

	// see if all achievement tokens have been picked up
	if (!Achievements.Contains(AchievementIDs::TutorialComplete))
	{
		bool bCompletedTutorial = true;
		static TArray<FName, TInlineAllocator<70>> TutorialTokens = []()
		{
			TArray<FName, TInlineAllocator<70>> List;
			FNumberFormattingOptions Options;
			Options.MinimumIntegralDigits = 3;
			for (int32 i = 0; i < 15; i++)
			{
				List.Add(FName(*FString::Printf(TEXT("movementtraining_token_%s"), *FText::AsNumber(i, &Options).ToString())));
			}
			for (int32 i = 0; i < 19; i++)
			{
				List.Add(FName(*FString::Printf(TEXT("weapontraining_token_%s"), *FText::AsNumber(i, &Options).ToString())));
			}
			for (int32 i = 0; i < 10; i++)
			{
				List.Add(FName(*FString::Printf(TEXT("pickuptraining_token_%s"), *FText::AsNumber(i, &Options).ToString())));
			}
			for (int32 i = 0; i < 5; i++)
			{
				List.Add(FName(*FString::Printf(TEXT("FFA_token_%s"), *FText::AsNumber(i, &Options).ToString())));
			}
			for (int32 i = 0; i < 5; i++)
			{
				List.Add(FName(*FString::Printf(TEXT("TDM_token_%s"), *FText::AsNumber(i, &Options).ToString())));
			}
			for (int32 i = 0; i < 5; i++)
			{
				List.Add(FName(*FString::Printf(TEXT("CTF_token_%s"), *FText::AsNumber(i, &Options).ToString())));
			}
			for (int32 i = 0; i < 5; i++)
			{
				List.Add(FName(*FString::Printf(TEXT("Duel_token_%s"), *FText::AsNumber(i, &Options).ToString())));
			}
			for (int32 i = 0; i < 5; i++)
			{
				List.Add(FName(*FString::Printf(TEXT("SD_token_%s"), *FText::AsNumber(i, &Options).ToString())));
			}
			for (int32 i = 0; i < 5; i++)
			{
				List.Add(FName(*FString::Printf(TEXT("FR_token_%s"), *FText::AsNumber(i, &Options).ToString())));
			}
			return List;
		}();
		for (FName TestToken : TutorialTokens)
		{
			if (!FoundTokenUniqueIDs.Contains(TestToken))
			{
				bCompletedTutorial = false;
				break;
			}
		}
		if (bCompletedTutorial)
		{
			Achievements.Add(AchievementIDs::TutorialComplete);
			UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(GEngine->GetFirstGamePlayer(GWorld));	
			if (LP)
			{
				LP->ShowToast(NSLOCTEXT("UT", "ItemRewardVise", "You earned Visse - The Armor of Sacrifice!"));
			}
		}
	}

	bNeedsUpdate = true;
	TempFoundTokenUniqueIDs.Empty();
}

void UUTProgressionStorage::TokensReset()
{
	bNeedsUpdate = true;
	TempFoundTokenUniqueIDs.Empty();
}

void UUTProgressionStorage::TokensClear()
{
	bNeedsUpdate = true;
	TempFoundTokenUniqueIDs.Empty();
	FoundTokenUniqueIDs.Empty();
}

bool UUTProgressionStorage::GetBestTime(FName TimingName, float& OutBestTime)
{
	OutBestTime = 0;

	float* BestTime = BestTimes.Find(TimingName);
	if (BestTime)
	{
		OutBestTime = *BestTime;
		return true;
	}

	return false;
}

void UUTProgressionStorage::SetBestTime(FName TimingName, float InBestTime)
{
	if (!BestTimes.Contains(TimingName) || BestTimes[TimingName] > InBestTime)
	{
		BestTimes.Add(TimingName, InBestTime);
		bNeedsUpdate = true;

		// hacky halloween reward implementation
		if (TimingName == AchievementIDs::FacePumpkins)
		{
			if (InBestTime >= 6666.0f)
			{
				for (TObjectIterator<UUTLocalPlayer> It; It; ++It)
				{
					if (It->GetProgressionStorage() == this)
					{
						It->AwardAchievement(AchievementIDs::FacePumpkins);
					}
				}
			}

			if (InBestTime >= 5000.0f)
			{
				for (TObjectIterator<UUTLocalPlayer> It; It; ++It)
				{
					if (It->GetProgressionStorage() == this)
					{
						It->AwardAchievement(AchievementIDs::PumpkinHead2015Level3);
					}
				}
			}

			if (InBestTime >= 1000.0f)
			{
				for (TObjectIterator<UUTLocalPlayer> It; It; ++It)
				{
					if (It->GetProgressionStorage() == this)
					{
						It->AwardAchievement(AchievementIDs::PumpkinHead2015Level2);
					}
				}
			}

			if (InBestTime >= 200.0f)
			{
				for (TObjectIterator<UUTLocalPlayer> It; It; ++It)
				{
					if (It->GetProgressionStorage() == this)
					{
						It->AwardAchievement(AchievementIDs::PumpkinHead2015Level1);
					}
				}
			}
		}
	}
}

/** Kind of hacky fix up until we move to the ladder system */
void UUTProgressionStorage::FixupBestTimes(int32& inTutorialMask)
{
	// These are converted
	static FName OldMovementTrainingName = FName(TEXT("movementtraining_timingsection"));	
	static FName OldWeaponTrainingName = FName(TEXT("weapontraining_timingsection"));	
	static FName OldPickupTrainingName = FName(TEXT("pickuptraining_timingsection"));	

	// These are just removed
	static FName OldFlagRunTrainingName = FName(TEXT("flagrun_timingsection"));	
	static FName OldDMTrainingName = FName(TEXT("deathmatch_timingsection"));
	static FName OldTDMTrainingName = FName(TEXT("teamdeathmatch_timingsection"));
	static FName OldCTFTrainingName = FName(TEXT("capturetheflag_timingsection"));
	static FName OldDuelTrainingName = FName(TEXT("duel_timingsection"));

	if (BestTimes.Contains(OldMovementTrainingName))
	{
		BestTimes.Add(ETutorialTags::TUTTAG_Movement, BestTimes[OldMovementTrainingName]);
		BestTimes.Remove(OldMovementTrainingName);
		inTutorialMask = inTutorialMask | TUTORIAL_Movement;
	}
		
	if (BestTimes.Contains(OldWeaponTrainingName))
	{
		BestTimes.Add(ETutorialTags::TUTTAG_Weapons, BestTimes[OldWeaponTrainingName]);
		BestTimes.Remove(OldWeaponTrainingName);
		inTutorialMask = inTutorialMask | TUTOIRAL_Weapon;
	}

	if (BestTimes.Contains(OldPickupTrainingName))
	{
		BestTimes.Add(ETutorialTags::TUTTAG_Pickups, BestTimes[OldPickupTrainingName]);
		BestTimes.Remove(OldPickupTrainingName);
		inTutorialMask = inTutorialMask | TUTORIAL_Pickups;
	}

	if (BestTimes.Contains(OldFlagRunTrainingName))
	{
		BestTimes.Remove(OldFlagRunTrainingName);
		inTutorialMask = inTutorialMask | TUTORIAL_FlagRun;
	}

	if (BestTimes.Contains(OldDMTrainingName))
	{
		BestTimes.Remove(OldDMTrainingName);
		inTutorialMask = inTutorialMask | TUTORIAL_DM;
	}

	if (BestTimes.Contains(OldTDMTrainingName))
	{
		BestTimes.Remove(OldTDMTrainingName);
		inTutorialMask = inTutorialMask | TUTORIAL_TDM;
	}

	if (BestTimes.Contains(OldCTFTrainingName))
	{
		BestTimes.Remove(OldCTFTrainingName);
		inTutorialMask = inTutorialMask | TUTORIAL_CTF;
	}

	if (BestTimes.Contains(OldDuelTrainingName) )
	{
		BestTimes.Remove(OldDuelTrainingName);
		inTutorialMask = inTutorialMask | TUTORIAL_Showdown;
	}

	if (CheckTokens(TEXT("movementtraining"),15))
	{
		inTutorialMask = inTutorialMask | TUTORIAL_Movement;
	}
		
	if (CheckTokens(TEXT("weapontraining"),15))
	{
		inTutorialMask = inTutorialMask | TUTOIRAL_Weapon;
	}

	if (CheckTokens(TEXT("pickuptraining"),10))
	{
		inTutorialMask = inTutorialMask | TUTORIAL_Pickups;
	}

	if (CheckTokens(TEXT("FR"),5))
	{
		inTutorialMask = inTutorialMask | TUTORIAL_FlagRun;
	}

	if (CheckTokens(TEXT("FFA"),5))
	{
		inTutorialMask = inTutorialMask | TUTORIAL_DM;
	}

	if (CheckTokens(TEXT("TDM"),5))
	{
		inTutorialMask = inTutorialMask | TUTORIAL_TDM;
	}

	if (CheckTokens(TEXT("CTF"),5))
	{
		inTutorialMask = inTutorialMask | TUTORIAL_CTF;
	}

	if (CheckTokens(TEXT("Duel"),5) )
	{
		inTutorialMask = inTutorialMask | TUTORIAL_Showdown;
	}

}
