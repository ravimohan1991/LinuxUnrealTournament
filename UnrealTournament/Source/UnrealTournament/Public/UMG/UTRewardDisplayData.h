// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTUITypes.h"
#include "UTRewards.h"
#include "UTRewardDisplayData.generated.h"

namespace UTRewardConstants
{
	const FText QuantityFormat = NSLOCTEXT("UT UI", "Quantity Format", "{0}x {1}");
	const FText CurrencyQuantityFormat = NSLOCTEXT("UT UI", "Currency Quantity Format", "{0} {1}");
	const FText RandomRewardReasonFormat = NSLOCTEXT("UT UI", "Random Reward Reason Format", "{0} ({1})");

	// XP Source Ids in the MatchCompleteNotification
	static const FString MatchComplete(TEXT("MatchComplete"));
	static const FString Victory(TEXT("Victory"));
	static const FString HeroOwnership(TEXT("HeroOwnership"));
	static const FString Boost(TEXT("Boost"));
	static const FString AccountLevel(TEXT("AccountLevel"));
	static const FString HeroLevel(TEXT("HeroLevel"));
	static const FString FirstWin(TEXT("FirstWin"));
	static const FString DailyReward(TEXT("DailyReward"));
	static const FString RandomReward(TEXT("RandomReward"));
	static const FString SpecialEvent(TEXT("SpecialEvent"));
}

/** Fundamental display info about a type of reward */
USTRUCT()
struct UNREALTOURNAMENT_API FUTRewardTypeInfo
{
	GENERATED_BODY()

public:
	/** The icon associated with this reward type */
	UPROPERTY(EditAnywhere)
	UTexture2D* DisplayIcons[EUTWidgetStyleSize::MAX];

	/** The localized name of this type of reward */
	UPROPERTY(EditAnywhere)
	FText DisplayName;

	/** The localized short description of this type of reward */
	UPROPERTY(EditAnywhere, meta = (MultiLine = "true"))
	FText ShortDescription;

	/** The localized full description of this type of reward */
	UPROPERTY(EditAnywhere, meta = (MultiLine = "true"))
	FText FullDescription;

	FUTRewardTypeInfo() {}
};

/** Display info about a specific item reward */
USTRUCT()
struct UNREALTOURNAMENT_API FUTItemRewardInfo : public FUTRewardTypeInfo
{
	GENERATED_BODY()

public:
	/** The type of mcp item this corresponds to */
	UPROPERTY(EditAnywhere)
	TSubclassOf<UUtMcpDefinition> McpItemType;

	FUTItemRewardInfo()
		: FUTRewardTypeInfo()
		, McpItemType(nullptr)
	{}
};

/** Display info about a loot tier reward */
USTRUCT()
struct UNREALTOURNAMENT_API FUTLootTierRewardInfo : public FUTRewardTypeInfo
{
	GENERATED_BODY()

public:
	/** The name of the loot tier group that this is describing */
	UPROPERTY(EditAnywhere)
	FString LootTierGroupName;

	/** 
	 * The type/subtitle of this loot tier roll. If random and empty, will be "Random Reward". 
	 * If non-random and empty, will stay empty. 
	 */
	UPROPERTY(EditAnywhere)
	FText LootTypeName;

	/** True if this is a random roll. False if the rewards are known. */
	UPROPERTY(EditAnywhere)
	bool bIsRandomRoll;

	FUTLootTierRewardInfo()
		: FUTRewardTypeInfo()
		, bIsRandomRoll(false)
	{}
};

/** Display info about a loot source */
USTRUCT()
struct UNREALTOURNAMENT_API FUTRewardSourceInfo
{
	GENERATED_BODY()

public:
	/** The name of the loot tier group that this is describing */
	UPROPERTY(EditAnywhere)
	FName SourceType;

	UPROPERTY(EditAnywhere)
	FText Title;

	UPROPERTY(EditAnywhere)
	FText Description;

	UPROPERTY(EditAnywhere)
	bool bShowInChest;
};

/** Display info for coin rewards in the loot crate spinner screen */
/** NOTE: The potential reward list is separate, dictated by fake token items*/
USTRUCT()
struct UNREALTOURNAMENT_API FUTCoinLootCrateRewards
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FString RealNameForReward;

	UPROPERTY(EditAnywhere)
	FString ReplacementName;

	UPROPERTY(EditAnywhere)
	UUtMcpDefinition* ReplacementReward;

	UPROPERTY(EditAnywhere)
	EUTItemRarity CoinAmountRarity;

	UPROPERTY(EditAnywhere)
	UTexture2D* RewardIcon;

	UPROPERTY(EditAnywhere)
	int32 MinRangeInclusive;

	UPROPERTY(EditAnywhere)
	int32 MaxRangeInclusive;
};

USTRUCT()
struct UNREALTOURNAMENT_API FUTLootCrateRewardDisplay
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere)
	FString ReplacementString;

	UPROPERTY(EditAnywhere)
	UUtMcpDefinition* ReplacementReward;

	FUTLootCrateRewardDisplay()
		: ReplacementReward(nullptr)
	{}
};

/** Display info about a hero badge */
USTRUCT()
struct UNREALTOURNAMENT_API FUTBadgeRewardInfo : public FUTRewardTypeInfo
{
	GENERATED_BODY()

public:
	/** The name of the loot tier group that this is describing */
	UPROPERTY(EditAnywhere)
	int32 MinLevel;

	FUTBadgeRewardInfo()
		: FUTRewardTypeInfo()
		, MinLevel(1)
	{}
};

UCLASS(hidecategories = Object, BlueprintType)
class UNREALTOURNAMENT_API UUTRewardDisplayData : public UDataAsset
{
	GENERATED_UCLASS_BODY()

public:
	// Helpers to extract basic display info about a reward
	void GetRewardDisplayInfo(const FUTLevelUpData& LevelUpData, FText& DisplayName, FText& DisplayDesc, UTexture2D*& DisplayIcon) const;
	
	bool GetFreeRewardDisplayInfo(const FUTLevelUpData& LevelUpData, FText& DisplayName, FText& DisplayDesc, UTexture2D*& DisplayIcon) const;
	bool GetMasterRewardDisplayInfo(const FUTLevelUpData& LevelUpData, FText& DisplayName, FText& DisplayDesc, UTexture2D*& DisplayIcon) const;

	void GetRewardDisplayInfo(const FLoginRewardData& LoginReward, FText& DisplayName, FText& DisplayDesc, UTexture2D*& DisplayIcon) const;
	
	void GetSpecificItemDisplayInfo(const UUtMcpDefinition* SpecificItem, int32 Quantity, FText& DisplayName, FText& DisplayDesc, UTexture2D*& DisplayIcon) const;

	void GetAllLootCrateRewardDisplays(TArray<FUTLootCrateRewardDisplay>& AllLootCrateRewards) const;

	void FindLootCrateRewardById(const FMcpItemIdAndQuantity& Reward, UUtMcpDefinition*& OutReward) const;

	virtual void PostLoad() override;
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

	const FUTRewardTypeInfo* GetItemRewardTypeInfo(const UUtMcpDefinition* ItemDefinition) const;
	const FUTLootTierRewardInfo* GetLootTierRewardInfo(const FString& LootTierGroup) const;

	const FUTRewardSourceInfo& GetRewardSource(const FName& SourceType) const;
	
	/** The default reward icon */
	UPROPERTY(EditAnywhere, Category = Default)
	UTexture2D* DefaultRewardIcon;

	/** The default reward source text */
	UPROPERTY(EditAnywhere, Category = Default)
	TMap<FString, FText> RewardSourceTextByNameString;

	/** Info about badge type rewards */
	UPROPERTY(EditAnywhere, Category = Badges)
	FUTRewardTypeInfo BadgeTypeInfo;
	
	/** Information about currencies */
	UPROPERTY(EditAnywhere, Category = Currency)
	FUTRewardTypeInfo Currencies[(uint8)EUTCurrencyType::MAX];

	/** The names used for each type of level */
	UPROPERTY(EditAnywhere, Category = XP)
	FText LevelTypeNames[(uint8)EUTLevelTypes::MAX];

	/** Colors associated with each type of level */
	UPROPERTY(EditAnywhere, Category = XP)
	FLinearColor BaseXPColors[(uint8)EUTLevelTypes::MAX];

	UPROPERTY(EditAnywhere, Category = XP)
	FLinearColor DeltaXPColors[(uint8)EUTLevelTypes::MAX];

	UPROPERTY(EditAnywhere, Category = XP)
	FLinearColor BoostXPColors[(uint8)EUTLevelTypes::MAX];

	UPROPERTY(EditAnywhere, Category = XP)
	FLinearColor FakeBoostXPColors[(uint8)EUTLevelTypes::MAX];

private:
	void RebuildMaps();
	
	/** Titles of the various sources of rewards */
	UPROPERTY(EditAnywhere, Category = Default)
	TArray<FUTRewardSourceInfo> RewardSources;

	UPROPERTY(EditAnywhere, Category = Default)
	FUTRewardSourceInfo DefaultRewardSource;

	/** Type info for specific item rewards */
	UPROPERTY(EditAnywhere, Category = ItemRewards)
	TArray<FUTItemRewardInfo> ItemRewardTypes;
	TMap<FName, const FUTItemRewardInfo*> ItemRewardsByClassName;

	/** Type info for loot table rewards */
	UPROPERTY(EditAnywhere, Category = LootRewards)
	TArray<FUTLootTierRewardInfo> LootRewardTypes;
	TMap<FString, const FUTLootTierRewardInfo*> LootRewardsByTierGroup;

	UPROPERTY(EditAnywhere, Category = LootCrates)
	TArray<FUTCoinLootCrateRewards> LootCrateCoinRewards;
};
