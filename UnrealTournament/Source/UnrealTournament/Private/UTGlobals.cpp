// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTWidgetData.h"
#include "UTGlobals.h"

UUTGlobals::UUTGlobals(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

class UUTWidgetData* UUTGlobals::GetWidgetData()
{
	if (!WidgetData)
	{
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Loading WidgetData Object"), STAT_WidgetData, STATGROUP_LoadTime);
		if (WidgetDataNameRef.ToString().Len() > 0)
		{
#if WITH_EDITOR
			FScopedSlowTask SlowTask(0, NSLOCTEXT("UTEditor", "BeginLoadingWidgetDataTask", "Loading WidgetData"), true);
#endif
			{
				UE_LOG(UT, Log, TEXT("Loading UTWidgetData: %s ..."), *WidgetDataNameRef.ToString());
				//SCOPE_LOG_TIME_IN_SECONDS(TEXT("    ... UTWidgetData loaded!"), nullptr)
				WidgetData = LoadObject<UUTWidgetData>(NULL, *WidgetDataNameRef.ToString(), NULL, LOAD_None, NULL);
				UE_LOG(UT, Log, TEXT("Finished Loading UTWidgetData: %s ..."), *WidgetDataNameRef.ToString());
			}
		}

		if (!WidgetData)
		{
			// None in ini, so build a default
			WidgetData = NewObject<UUTWidgetData>(UUTWidgetData::StaticClass());
		}
	}
	return WidgetData;
}

UUTGameUIData* UUTGlobals::GetGameUIData()
{
	if (!GameUIData)
	{
		DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Loading GameUIData Object"), STAT_GameUIData, STATGROUP_LoadTime);
		if (GameUIDataRef.ToString().Len() > 0)
		{
#if WITH_EDITOR
			FScopedSlowTask SlowTask(0, NSLOCTEXT("UTEditor", "BeginLoadingGameUIDataTask", "Loading GameUIData"), true);
#endif
			{
				UE_LOG(UT, Log, TEXT("Loading UTGameUIData: %s ..."), *GameUIDataRef.ToString());
				SCOPE_LOG_TIME_IN_SECONDS(TEXT("    ... UTGameUIData loaded!"), nullptr)
					GameUIData = LoadObject<UUTGameUIData>(NULL, *GameUIDataRef.ToString(), NULL, LOAD_None, NULL);
			}
		}
		if (!GameUIData)
		{
			// None in ini, so build a default
			GameUIData = NewObject<UUTGameUIData>(UUTGameUIData::StaticClass());
		}
	}
	return GameUIData;
}

void UUTGlobals::GetRewardsFromSameSource(const FString& SourceName, TArray<FString> &OutSimilarRewards) const
{
	OutSimilarRewards.Empty();

	if (RewardToSourceTable)
	{
        for (auto& RowIter : RewardToSourceTable->GetRowMap())
		{
			const FRewardToSource* RewardToSource = (const FRewardToSource*)RowIter.Value;

			//if names match
			if (RewardToSource->RewardLocHandle.RowName.ToString() == SourceName)
			{
				OutSimilarRewards.AddUnique(RewardToSource->PersistentName);
			}
		}
	}
}

UUtMcpDefinition* UUTGlobals::FindItemInAssetLists(const FString& Ident)
{
#if WITH_PROFILE
	for (int32 TypeIndex = 0; TypeIndex < TEnumValue(EUtItemType::Max_None); ++TypeIndex)
	{
		EUtItemType EachType = static_cast<EUtItemType>(TypeIndex);
		TArray<FAssetData> EachArray;
		GetItemData(EachType, EachArray);
		for (const FAssetData& EachItem : EachArray)
		{
			UUtMcpDefinition* ItemData = Cast<UUtMcpDefinition>(EachItem.GetAsset());
			if (ItemData != nullptr)
			{
				if (ItemData->GetPersistentName() == Ident)
				{
					return ItemData;
				}
			}
		}
	}
#endif
	return nullptr;
}

/** Gets the list of MCP Items depending on type */
void UUTGlobals::GetItemData(EUtItemType ItemType, TArray<FAssetData>& AssetList)
{
	switch (ItemType)
	{
	case EUtItemType::Boost:
		GetSimpleAssetData(EUTObjectLibrary::BoostData, AssetList);
		break;
	case EUtItemType::Card:
		GetSimpleAssetData(EUTObjectLibrary::CardData, AssetList);
		break;
	case EUtItemType::CardPack:
		GetSimpleAssetData(EUTObjectLibrary::CardPackData, AssetList);
		break;
	case EUtItemType::GiftBox:
		GetSimpleAssetData(EUTObjectLibrary::GiftBoxData, AssetList);
		break;
	case EUtItemType::Skin:
		GetSimpleAssetData(EUTObjectLibrary::SkinData, AssetList);
		break;
	case EUtItemType::Token:
		GetSimpleAssetData(EUTObjectLibrary::TokenData, AssetList);
		break;
	case EUtItemType::CodeToken:
		GetSimpleAssetData(EUTObjectLibrary::CodeTokenData, AssetList);
		break;
	default:
		UE_LOG(UT, Log, TEXT("Unsupported Item Type being requested: %s ..."), *EUtItemType_ToString(ItemType));
		break;
	}
}

int32 UUTGlobals::GetSimpleAssetData(EUTObjectLibrary::Type LibraryType, TArray<struct FAssetData> &AssetList, bool bInHasBlueprintClasses)
{
	UObjectLibrary*& ObjectLibrary = ObjectLibraries[LibraryType].Library;

	if (!ObjectLibrary)
	{
		// Can't do anything, wasn't set up initially
		return 0;
	}

	if (ObjectLibrary->GetAssetDataCount() == 0)
	{
		// Library got flushed, reload it
		if (ObjectLibraries[LibraryType].LoadedPaths.Num() != 0)
		{
			LoadObjectLibrary(LibraryType, ObjectLibrary->ObjectBaseClass, bInHasBlueprintClasses, false, ObjectLibraries[LibraryType].ItemType, ObjectLibraries[LibraryType].LoadedPaths);
		}
	}

	ObjectLibrary->GetAssetDataList(AssetList);
	return AssetList.Num();
}

void UUTGlobals::LoadObjectLibrary(EUTObjectLibrary::Type LibraryType, UClass* BaseClass, bool bInHasBlueprintClasses, bool bFullyLoad, EUtItemType ItemType, const FString& Path, const TArray<FString>& RootPaths)
{
	TArray<FString> Paths;
	for (const FString& EachPath : RootPaths)
	{
		FString FullPath = EachPath;
		FullPath.PathAppend(*Path, Path.Len());
		Paths.Add(FullPath);
	}
	LoadObjectLibrary(LibraryType, BaseClass, bInHasBlueprintClasses, bFullyLoad, ItemType, Paths);
}

void UUTGlobals::LoadObjectLibrary(EUTObjectLibrary::Type LibraryType, UClass* BaseClass, bool bInHasBlueprintClasses, bool bFullyLoad, EUtItemType ItemType, const TArray<FString>& Paths)
{
	UObjectLibrary*& ObjectLibrary = ObjectLibraries[LibraryType].Library;
	if (ObjectLibrary && (ObjectLibrary->GetObjectCount() != 0 || ObjectLibrary->GetAssetDataCount() != 0))
	{
		// Already loaded or invalid holder
		return;
	}

	DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Loading Library"), STAT_ObjectLibrary, STATGROUP_LoadTime);
	FScopeCycleCounterUObject AssetTypeScope(BaseClass);

#if WITH_EDITOR
	FFormatNamedArguments Args;
	Args.Add(TEXT("BaseClassName"), FText::FromString(BaseClass->GetName()));
	FScopedSlowTask SlowTask(0, FText::Format(NSLOCTEXT("UTEditor", "BeginLoadingClassLibraryTask", "Loading {BaseClassName} Library"), Args));
	const bool bShowCancelButton = false;
	const bool bAllowInPIE = true;
	SlowTask.MakeDialog(bShowCancelButton, bAllowInPIE);
#endif

	if (!ObjectLibrary)
	{
		ObjectLibrary = UObjectLibrary::CreateLibrary(BaseClass, bInHasBlueprintClasses, GIsEditor && !IsRunningCommandlet());
	}
	ObjectLibraries[LibraryType].ItemType = ItemType;

	FScopeCycleCounterUObject PreloadScope(ObjectLibrary);

	ObjectLibraries[LibraryType].LoadedPaths = Paths;

	const bool bShouldForceSynchronousScan = bFullyLoad;
	if (bInHasBlueprintClasses)
	{
		ObjectLibrary->LoadBlueprintAssetDataFromPaths(Paths, bShouldForceSynchronousScan);
	}
	else
	{
		ObjectLibrary->LoadAssetDataFromPaths(Paths, bShouldForceSynchronousScan);
	}
	if (bFullyLoad)
	{
#if STATS
		FString PerfMessage = FString::Printf(TEXT("Loaded UTGlobals object library: %s in "), *BaseClass->GetName());
		for(const FString& EachPath:Paths)
		{
			PerfMessage.Append(EachPath);
			PerfMessage.AppendChar(*TEXT(","));
		}
		SCOPE_LOG_TIME_IN_SECONDS(*PerfMessage, nullptr)
#endif
		ObjectLibrary->LoadAssetsFromAssetData();
	}
}
