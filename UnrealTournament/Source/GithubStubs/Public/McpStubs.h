// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "McpStubs.generated.h"

USTRUCT()
struct GITHUBSTUBS_API FMcpLootEntry
{
	GENERATED_USTRUCT_BODY()

	/** The PersistentName of the item asset this represents */
	UPROPERTY()
	FString ItemType;

	/** (optional) Guid for the item that was created. World items will never have this field. Also persistent items that were rolled up into another stack may be missing this field. */
	UPROPERTY()
	FString ItemGuid;

	/** The stack count for this item entry. */
	UPROPERTY()
	int32 Quantity;

	FMcpLootEntry()
		: Quantity(1)
	{
	}

	FMcpLootEntry(const FString& InItemType, int32 InQuantity = 1)
		: ItemType(InItemType)
		, Quantity(InQuantity)
	{
	}
};

USTRUCT()
struct GITHUBSTUBS_API FMcpLootResult
{
	GENERATED_USTRUCT_BODY()

	/** The loot tier group that was used to generate this loot in the form "<Name>:<TierNum>" */
	UPROPERTY()
	FString TierGroupName;

	/** Items which have already been created by MCP. Should already be in the inventory before this notification is processed. */
	UPROPERTY()
	TArray<FMcpLootEntry> Items;
};

UCLASS(ABSTRACT, BlueprintType)
class GITHUBSTUBS_API UUtMcpDefinition : public UObject
{
	GENERATED_UCLASS_BODY()
};

UCLASS(ABSTRACT, BlueprintType)
class GITHUBSTUBS_API UUtMcpCardPackItem : public UObject
{
	GENERATED_UCLASS_BODY()
};

USTRUCT()
struct GITHUBSTUBS_API FMcpItemIdAndQuantity
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	FString ItemId;

	UPROPERTY()
	int32 Quantity;
};

/** The different rarities of persistent items */
UENUM(BlueprintType)
enum class EUTItemRarity : uint8
{
	Basic,
	Common,
	Rare,
	UltraRare,
	EpicMegaRare
};

UENUM(BlueprintType)
enum class EUtItemType : uint8
{
	Card,
	CardPack,
	GiftBox,
	Currency,
	Taunt,
	Skin,
	WeaponSkin,
	Token,
	CodeToken,
	Item, // Classic items
	Boost,

	Max_None		UMETA(DisplayName = "None")
};

FString GITHUBSTUBS_API EUtItemType_ToString(EUtItemType ItemType);

UENUM()
namespace EUTObjectLibrary
{
	enum Type
	{
		// Items and persistent assets. These are never loaded, but metadata is used at runtime
		BoostData,
		CardData,
		CardPackData,
		GiftBoxData,
		SkinData,
		TokenData,
		CodeTokenData,
		ItemData,

		Max_None
	};
}
