// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#if WITH_PROFILE
#include "UtMcpDefinition.h"
#else
#include "GithubStubs.h"
#endif

#include "UTRewards.generated.h"

USTRUCT()
struct FRewardToSource : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere)
	FString PersistentName;
	UPROPERTY(EditAnywhere)
	FDataTableRowHandle RewardLocHandle;
};
USTRUCT()
struct FRewardSourceLoc : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()
public:
	UPROPERTY(EditAnywhere)
	FText Description;
};

/** Structure that defines a level up table entry */
USTRUCT(BlueprintType)
struct UNREALTOURNAMENT_API FUTLevelUpData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()
public:
	FUTLevelUpData()
		: Index(0)
		, XP(0)
		, Quantity(0)
		, MasterQuantity(0)
	{
		SpecificItem.Reset();
		MasterSpecificItem.Reset();
	}

	/** 'Name' is Level*/
	UPROPERTY(EditAnywhere, Category = LevelUp)
	int32 Index;

	/** The hero this reward table is for*/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LevelUp)
	FString HeroName;

	/** This was the old property name (represented total XP) TODO: remove this when the build has been pushed out for a while */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LevelUp)
	int32 XP;

	// FREE REWARDS

	/** Quantity for given loot */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LevelUp)
	int32 Quantity;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LevelUp)
	TAssetPtr<UUtMcpDefinition> SpecificItem;

	/** Loot table roll is used when specific information isn't given */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LevelUp)
	FString LootTierGroup;

	// OWNERSHIP REWARDS
	
	/** Quantity for given loot */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LevelUp)
	int32 MasterQuantity;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LevelUp)
	TAssetPtr<UUtMcpDefinition> MasterSpecificItem;

	/** Loot table roll is used when specific information isn't given */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LevelUp)
	FString MasterLootTierGroup;
};

/** For associating a given LevelUpData row with an actual level */
USTRUCT()
struct UNREALTOURNAMENT_API FUTLevelUpRewardInfo
{
	GENERATED_BODY()

public:
	// Don't use (for UObject system)
	FUTLevelUpRewardInfo()
		: Level(0)
	{}

	/** Use this constructor */
	FUTLevelUpRewardInfo(int32 InLevel, const FUTLevelUpData& InRowData)
		: Level(InLevel)
		, LevelUpRowData(InRowData)
	{}

	/** The level that this is associated with */
	int32 Level;

	/** The level up data from a given row */
	UPROPERTY(Transient)
	FUTLevelUpData LevelUpRowData;

	const UUtMcpDefinition* GetSpecificItem() const;
	const UUtMcpDefinition* GetSpecificItem(int32& Quantity) const;
	const FString& GetLootTierGroup() const;
};

/** Structure that defines a level up table entry */
USTRUCT(BlueprintType)
struct UNREALTOURNAMENT_API FLoginRewardData : public FTableRowBase
{
	GENERATED_USTRUCT_BODY()
public:
	FLoginRewardData()
		: Quantity(0)
		, LootTierGroup()
	{
		SpecificItem.Reset();
	}

	/** 'Name' is Level*/

	/** Quantity for given loot */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Reward)
	int32 Quantity;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Reward)
	TAssetPtr<UUtMcpDefinition> SpecificItem;

	/** Loot table roll is used when specific information isn't given */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = LevelUp)
	FString LootTierGroup;
};

/** For associating a given FLoginRewardData row with an actual level */
USTRUCT()
struct FUTLoginRewardInfo
{
	GENERATED_BODY()

public:
	// Don't use (for UObject system)
	FUTLoginRewardInfo()
		: Level(0)
	{}

	/** Use this constructor */
	FUTLoginRewardInfo(int32 InLevel, const FLoginRewardData& InRowData)
		: Level(InLevel)
		, LoginRewardRowData(InRowData)
	{}

	/** The level that this is associated with */
	UPROPERTY()
	int32 Level;

	/** The reward data from a given row */
	UPROPERTY()
	FLoginRewardData LoginRewardRowData;
};