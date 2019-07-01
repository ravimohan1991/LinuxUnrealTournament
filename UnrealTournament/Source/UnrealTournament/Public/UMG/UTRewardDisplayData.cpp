// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTRewards.h"
#include "UTRewardDisplayData.h"
#include "UTUITypes.h"

#include "UTGlobals.h"

#if WITH_PROFILE
#include "UtMcpTokenDefinition.h"
#else
#include "GithubStubs.h"
#endif

//////////////////////////////////////////////////////////////////////////
// UUTRewardDisplayData
//////////////////////////////////////////////////////////////////////////

#define LOCTEXT_NAMESPACE "UTRewardDisplayData"

UUTRewardDisplayData::UUTRewardDisplayData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Default setup
	RewardSourceTextByNameString.Add(UTRewardConstants::MatchComplete, LOCTEXT("MatchComplete", "Match Complete"));
	RewardSourceTextByNameString.Add(UTRewardConstants::Victory, LOCTEXT("Victory", "Match Victory"));
	RewardSourceTextByNameString.Add(UTRewardConstants::Boost, LOCTEXT("Boost", "Boost"));
	RewardSourceTextByNameString.Add(UTRewardConstants::AccountLevel, LOCTEXT("Account Level", "Account Level Up"));
	RewardSourceTextByNameString.Add(UTRewardConstants::FirstWin, LOCTEXT("First Game", "First Game Today"));
	RewardSourceTextByNameString.Add(UTRewardConstants::DailyReward, LOCTEXT("Daily Reward", "Daily Reward"));
	RewardSourceTextByNameString.Add(UTRewardConstants::RandomReward, LOCTEXT("Random Reward", "Random Reward"));
	RewardSourceTextByNameString.Add(UTRewardConstants::SpecialEvent, LOCTEXT("Event", "Special Event"));
}

void UUTRewardDisplayData::PostLoad()
{
	Super::PostLoad();

	RebuildMaps();
}

#if WITH_EDITOR
void UUTRewardDisplayData::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	RebuildMaps();
}

#endif

void UUTRewardDisplayData::GetRewardDisplayInfo(const FLoginRewardData& LoginReward, FText& DisplayName, FText& DisplayDesc, UTexture2D*& DisplayIcon) const
{
	const UUtMcpDefinition* RewardItem = LoginReward.SpecificItem.Get();
	if (RewardItem)
	{
		GetSpecificItemDisplayInfo(RewardItem, LoginReward.Quantity, DisplayName, DisplayDesc, DisplayIcon);
	}
	else
	{
		// Otherwise, look up the the info on the loot tier group this reward rolls on
		if (const FUTLootTierRewardInfo* LootTierRewardInfo = GetLootTierRewardInfo(LoginReward.LootTierGroup))
		{
			DisplayName = LootTierRewardInfo->DisplayName;
			DisplayDesc = LootTierRewardInfo->ShortDescription;
			DisplayIcon = LootTierRewardInfo->DisplayIcons[EUTWidgetStyleSize::Medium];
		}
		else
		{
			const FText RandomRewardText = NSLOCTEXT("UTUI", "Random Reward", "Random Reward");
			DisplayName = RandomRewardText;
			DisplayDesc = RandomRewardText;

			DisplayIcon = nullptr;
		}
	}

	if (!DisplayIcon)
	{
		DisplayIcon = DefaultRewardIcon;
	}
}

void UUTRewardDisplayData::GetRewardDisplayInfo(const FUTLevelUpData& LevelUpData, FText& DisplayName, FText& DisplayDesc, UTexture2D*& DisplayIcon) const
{
	// Otherwise, look up the the info on the loot tier group this reward rolls on
	const UUtMcpDefinition* RewardItem = LevelUpData.SpecificItem.Get();
	int32 ItemQuantity = LevelUpData.Quantity;
	if (!RewardItem)
	{
		RewardItem = LevelUpData.MasterSpecificItem.Get();
		ItemQuantity = LevelUpData.MasterQuantity;
	}

	// If this grants a specific item, use that
	if (RewardItem)
	{
		GetSpecificItemDisplayInfo(RewardItem, ItemQuantity, DisplayName, DisplayDesc, DisplayIcon);
	}
	else
	{
		const bool bMasterReward = !LevelUpData.MasterLootTierGroup.IsEmpty();
		const FString& LootTierGroupName = bMasterReward ? LevelUpData.MasterLootTierGroup : LevelUpData.LootTierGroup;

		if (const FUTLootTierRewardInfo* LootTierRewardInfo = GetLootTierRewardInfo(LootTierGroupName))
		{
			DisplayName = LootTierRewardInfo->DisplayName;
			DisplayDesc = LootTierRewardInfo->ShortDescription;
			DisplayIcon = LootTierRewardInfo->DisplayIcons[EUTWidgetStyleSize::Medium];
		}
		else
		{
			DisplayIcon = nullptr;

			const FText RandomRewardText = NSLOCTEXT("UTUI", "Random Reward", "Random Reward");
			DisplayName = RandomRewardText;
			DisplayDesc = RandomRewardText;
		}
	}

	if (DisplayDesc.IsEmpty())
	{
		DisplayDesc = DisplayName;
	}
	if (!DisplayIcon)
	{
		DisplayIcon = DefaultRewardIcon;
	}
}

void UUTRewardDisplayData::GetSpecificItemDisplayInfo(const UUtMcpDefinition* SpecificItem, int32	Quantity, FText& DisplayName, FText& DisplayDesc, UTexture2D*& DisplayIcon) const
{
#if WITH_PROFILE
	bool bIsCurrency = false;
	if (SpecificItem->IsA<UUtMcpTokenDefinition>())
	{
		const FString TokenName = SpecificItem->GetPersistentName();
		if (TokenName.Contains(TEXT("Currency.Mtx")) ||
			(TokenName.Contains(TEXT("TimeCurrency")) && !TokenName.Contains(TEXT("Sub"))))
		{
			bIsCurrency = true;
		}
	}

	FText ItemName = SpecificItem->GetDisplayName(true);
	if (bIsCurrency)
	{
		DisplayName = FText::Format(UTRewardConstants::CurrencyQuantityFormat, FText::AsNumber(Quantity), ItemName);
	}
	else
	{
		DisplayName = Quantity > 1 ? FText::Format(UTRewardConstants::QuantityFormat, FText::AsNumber(Quantity), ItemName) : ItemName;
	}


	DisplayDesc = SpecificItem->GetDescription();

	TAssetPtr<UObject> ItemDisplayAsset = SpecificItem->GetIconAsset();
	if (!ItemDisplayAsset.IsValid() && !ItemDisplayAsset.IsNull())
	{
		DisplayIcon = Cast<UTexture2D>(UUTGlobals::Get().SynchronouslyLoadAsset(ItemDisplayAsset));
	}
#endif
}

void UUTRewardDisplayData::GetAllLootCrateRewardDisplays(TArray<FUTLootCrateRewardDisplay>& AllLootCrateRewards) const
{
#if WITH_PROFILE
	TArray<FString> LootRewards;
	UUTGlobals::Get().GetRewardsFromSameSource(TEXT("LootCrate"), LootRewards);

	for (const FString& Reward : LootRewards)
	{
		UUtMcpDefinition* ItemDef = nullptr;

		// if there's a fake coin reward on the list, replace it with the appropriate reward from the reward display data
		for (const FUTCoinLootCrateRewards& RewardDisplay : LootCrateCoinRewards)
		{
			if (Reward.Equals(RewardDisplay.ReplacementName))
			{
				ItemDef = RewardDisplay.ReplacementReward;
				break;
			}
		}

		if (!ItemDef)
		{
			ItemDef = UUTGlobals::Get().FindItemInAssetLists(Reward);
		}

		if (ItemDef)
		{
			FUTLootCrateRewardDisplay RewardDisplay;
			RewardDisplay.ReplacementReward = ItemDef;
			RewardDisplay.ReplacementString = Reward;

			AllLootCrateRewards.Add(RewardDisplay);
		}
	}
#endif
}

void UUTRewardDisplayData::FindLootCrateRewardById(const FMcpItemIdAndQuantity& Reward, UUtMcpDefinition*& OutReward) const
{
	OutReward = nullptr;

	// First check to see if it's a coin reward, and if so, find the appropriate reward display data to use, depending on amount
	for (const FUTCoinLootCrateRewards& RewardDisplay : LootCrateCoinRewards)
	{
		if (Reward.ItemId.Equals(RewardDisplay.RealNameForReward))
		{
			if (Reward.Quantity >= RewardDisplay.MinRangeInclusive && Reward.Quantity <= RewardDisplay.MaxRangeInclusive)
			{
				OutReward = RewardDisplay.ReplacementReward;
				break;
			}
		}
	}

	if (!OutReward)
	{
		OutReward = UUTGlobals::Get().FindItemInAssetLists(Reward.ItemId);
	}
}

const FUTRewardTypeInfo* UUTRewardDisplayData::GetItemRewardTypeInfo(const UUtMcpDefinition* ItemDefinition) const
{
#if WITH_PROFILE
	if (ItemDefinition)
	{
		if (const FUTItemRewardInfo* const* Info = ItemRewardsByClassName.Find(ItemDefinition->GetClass()->GetFName()))
		{
			return *Info;
		}
		else if (ItemDefinition->IsA<UUtMcpTokenDefinition>() &&
			ItemDefinition->GetPersistentName().Contains(TEXT("TimeCurrency")))
		{
			// For the special case of time currency tokens, use the time currency reward info
			return &Currencies[TEnumValue(EUTCurrencyType::TimeCurrency)];
		}
	}
#endif
	return nullptr;
}

const FUTLootTierRewardInfo* UUTRewardDisplayData::GetLootTierRewardInfo(const FString& LootTierGroup) const
{
	if (const FUTLootTierRewardInfo* const* Info = LootRewardsByTierGroup.Find(LootTierGroup))
	{
		return *Info;
	}
	return nullptr;
}

const FUTRewardSourceInfo& UUTRewardDisplayData::GetRewardSource(const FName& SourceType) const
{
	FString SourceName(SourceType.ToString());

	// Peel off the right-side of the "." to get more generic until you find it.
	// E.G. "HeroLevelUp.Hero.Arcblade" will also match "HeroLevelUp.Hero" or "HeroLevelUp" until one is found
	while (!SourceName.IsEmpty())
	{
		for (const FUTRewardSourceInfo& RewardSource : RewardSources)
		{
			if (RewardSource.SourceType.ToString() == SourceName)
			{
				return RewardSource;
			}
		}

		FString Left, Right;
		SourceName.Split(TEXT("."), &Left, &Right, ESearchCase::IgnoreCase, ESearchDir::FromEnd);

		SourceName = Left;
	}

	return DefaultRewardSource;
}

void UUTRewardDisplayData::RebuildMaps()
{
	ItemRewardsByClassName.Reset();
	for (const FUTItemRewardInfo& ItemRewardInfo : ItemRewardTypes)
	{
		if (ItemRewardInfo.McpItemType)
		{
			ItemRewardsByClassName.Add(ItemRewardInfo.McpItemType->GetFName(), &ItemRewardInfo);
		}
	}

	LootRewardsByTierGroup.Reset();
	for (const FUTLootTierRewardInfo& LootRewardInfo : LootRewardTypes)
	{
		LootRewardsByTierGroup.Add(LootRewardInfo.LootTierGroupName, &LootRewardInfo);
	}
}

#undef LOCTEXT_NAMESPACE