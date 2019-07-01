// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
// Uses the GameSingleton object on GEngine so that UI can backdoor into UObjects

#pragma once

#include "UnrealTournament.h"
#include "UTRewardDisplayData.h"
#include "UTGameUIData.h"
#include "UTRewards.h"
#include "Engine/ObjectLibrary.h"

#include "UTGlobals.generated.h"

/** Each object library is stored here */
USTRUCT()
struct FUTObjectLibraryEntry
{
	GENERATED_USTRUCT_BODY()

	/** The ObjectLibrary we are wrapping */
	UPROPERTY()
	UObjectLibrary *Library;

	/** What type of item is stored here. May be None */
	UPROPERTY()
	EUtItemType ItemType;

	/** Paths that were used to load this libary */
	UPROPERTY()
	TArray<FString> LoadedPaths;

	FUTObjectLibraryEntry() : Library(NULL), ItemType(EUtItemType::Max_None) {}
};

/**
 * This is a simple wrapper for MakeSharable( new T );
 *
 * Ex:
 * TSharedPtr< FClassType > = NewShared< FClassType >( Arg1, Arg2, ArgN... );
 */
template <typename DataType, ESPMode ModeType = ESPMode::Fast, typename... TS>
FORCEINLINE TSharedPtr<DataType, ModeType> NewShared( TS&&... Arguments )
{
	return MakeShareable( new DataType( Forward<TS>( Arguments )... ) );
}

template <typename T>
FORCEINLINE CONSTEXPR typename TEnableIf<TIsEnum<T>::Value, __underlying_type(T)>::Type TEnumValue(T Value)
{
	return static_cast<__underlying_type(T)>(Value);
}

/** Centralization of various priority "bands" used for loading different types of data */
enum class EUTAsyncLoadingPriority : int32
{
	Default = FStreamableManager::DefaultAsyncLoadPriority,

	Background = Default - 1,		// For non-critical loads that can be postponed when required
	HighPriorityForGameFlow = Default + 1		// FE/Game flow will depend on these loads so they need to happen immediately
};

UCLASS(config = Engine)
class UNREALTOURNAMENT_API UUTGlobals : public UObject
{
	GENERATED_UCLASS_BODY()
	
	static FORCEINLINE UUTGlobals& Get()
	{
		return *static_cast<UUTGlobals*>(GEngine->GameSingleton);
	}
	
	/** A helper function to load a TAssetPtr */
	template<typename AssetType>
	AssetType* SynchronouslyLoadAsset(const TAssetPtr<AssetType>& AssetPointer)
	{
		const FStringAssetReference& StringReference = AssetPointer.ToStringReference();
		if (StringReference.IsValid())
		{
			SCOPE_LOG_TIME_IN_SECONDS(*FString::Printf(TEXT("Loaded UTGlobals asset %s"), *StringReference.ToString()), nullptr)
				FStreamableManager& Streamable = UUTGlobals::Get().StreamableManager;
			AssetType* LoadedAsset = Cast<AssetType>(Streamable.SynchronousLoad(StringReference));
			if (ensureMsgf(LoadedAsset, TEXT("Unable to load asset %s"), *StringReference.ToString()))
			{
				return LoadedAsset;
			}
		}

		return NULL;
	}

	const UUTRewardDisplayData& GetRewardDisplayData() const { return *RewardDisplayData; }

	void GetRewardSources(const FString& PersistentName, TArray<FRewardSourceLoc*> &OutRewardSources) const;
	void GetRewardsFromSameSource(const FString& SourceName, TArray<FString> &OutSimilarRewards) const;

	/** Search all MCP item lists for an MCP Items based on its persistent name */
	UUtMcpDefinition* FindItemInAssetLists(const FString& Ident);

	/** Handles async streaming of various data objects */
	FStreamableManager StreamableManager;

	UUTGameUIData* GetGameUIData();

	class UUTWidgetData* GetWidgetData();

	/** This function handles simple assetdata-only dictionaries, and will reload the library if required. Returns number loaded */
	int32 GetSimpleAssetData(EUTObjectLibrary::Type LibraryType, TArray<class FAssetData>& AssetList, bool bInHasBlueprintClasses = false);

	/** Gets the list of MCP Items depending on type */
	void GetItemData(EUtItemType ItemType, TArray<FAssetData> &AssetList);

	bool GetIsUsingGamepad() const { return false; }

	/** Asset to load as the default WidgetData */
	UPROPERTY(config, EditAnywhere, Category = "Widget Data", meta = (AllowedClasses = "UTWidgetData", DisplayName = "WidgetDataName"))
	FStringAssetReference WidgetDataNameRef;

	/** Holds all the UUserWidgets that can be accessed from Native Code */
	UPROPERTY()
	UUTWidgetData* WidgetData;
	
	/*
	 * Helper function to setup a new object library from a set of given paths
	 * @param	LibraryType				object library to load objects into
	 * @param	BaseClass				base class type of objects to load
	 * @param	bInHasBlueprintClasses	if true will load only blueprint assets
	 * @param	bFullyLoad				if true assets will be loaded immediately
	 * @param	ItemType				item type
	 * @param	Path					main path to load (this is appended to the list of given paths)
	 * @param	RootPaths				Paths to load objects from
	 */
	void LoadObjectLibrary(EUTObjectLibrary::Type LibraryType, UClass *BaseClass, bool bInHasBlueprintClasses, bool bFullyLoad, EUtItemType ItemType, const FString& Path, const TArray<FString>& RootPaths);

	/** Helper function to setup a new object library from several paths */
	void LoadObjectLibrary(EUTObjectLibrary::Type LibraryType, UClass* BaseClass, bool bInHasBlueprintClasses, bool bFullyLoad, EUtItemType ItemType, const TArray<FString>& Paths);

	/** UT GameData: this contains balance sheet references and other properties. */
	UPROPERTY(config, EditAnywhere, Category = "UT Game Data", meta=(AllowedClasses="UTGameData", DisplayName="Game Data"))
	FStringAssetReference GameDataRef;

	/** UT GameUIData: contains references and properties needed for the UI. */
	UPROPERTY(config, EditAnywhere, Category = "UT Game UI Data", meta=(AllowedClasses="UTGameUIData", DisplayName="Game UI Data"))
	FStringAssetReference GameUIDataRef;

	/** Holds the table of all rewards to how to get them*/
	UPROPERTY()
	class UDataTable* RewardToSourceTable;
	/** Holds the table of reward sources to their localization data*/
	UPROPERTY()
	class UDataTable* RewardSourceLocTable;

	/** Global UI Data */
	UPROPERTY()
	UUTGameUIData* GameUIData;

	UPROPERTY(EditDefaultsOnly, Category = DisplayData)
	UUTRewardDisplayData* RewardDisplayData;
	
	// The actual object libraries
	UPROPERTY()
	FUTObjectLibraryEntry ObjectLibraries[EUTObjectLibrary::Max_None];
};