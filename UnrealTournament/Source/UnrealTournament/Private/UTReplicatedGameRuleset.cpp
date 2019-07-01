// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTReplicatedGameRuleset.h"
#include "UTReplicatedMapInfo.h"
#include "Net/UnrealNetwork.h"

#if !UE_SERVER
#include "SUWindowsStyle.h"
#endif

AUTReplicatedGameRuleset::AUTReplicatedGameRuleset(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
	SetRemoteRoleForBackwardsCompat(ROLE_SimulatedProxy);
	bReplicates = true;
	bAlwaysRelevant = true;
	bReplicateMovement = false;
	bNetLoadOnClient = false;
}

void AUTReplicatedGameRuleset::GetLifetimeReplicatedProps(TArray< FLifetimeProperty > & OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AUTReplicatedGameRuleset, Data);
	DOREPLIFETIME(AUTReplicatedGameRuleset, MapList);
	DOREPLIFETIME(AUTReplicatedGameRuleset, OptimalPlayers);
	DOREPLIFETIME(AUTReplicatedGameRuleset, bCustomRuleset);
}

void AUTReplicatedGameRuleset::OnReceiveData()
{
	BuildSlateBadge();
}

int32 AUTReplicatedGameRuleset::AddMapAssetToMapList(const FAssetData& Asset)
{
	AUTGameState* GameState = GetWorld()->GetGameState<AUTGameState>();
	if (GameState)
	{
		AUTReplicatedMapInfo* MapInfo = GameState->CreateMapInfo(Asset);
		return MapList.Add(MapInfo);
	}
	return INDEX_NONE;
}

void AUTReplicatedGameRuleset::SetRules(const FUTGameRuleset& NewRules, const TArray<FAssetData>& MapAssets)
{
	Data = NewRules;

	// First add the Epic maps.
	if (!NewRules.EpicMaps.IsEmpty())
	{
		TArray<FString> EpicMapList;
		NewRules.EpicMaps.ParseIntoArray(EpicMapList,TEXT(","), true);
		for (int32 i = 0 ; i < EpicMapList.Num(); i++)
		{
			FString MapName = EpicMapList[i];
			if ( !MapName.IsEmpty() )
			{
				if ( FPackageName::IsShortPackageName(MapName) )
				{
					FPackageName::SearchForPackageOnDisk(MapName, &MapName); 
				}

				// Look for the map in the asset registry...

				for (const FAssetData& Asset : MapAssets)
				{
					FString AssetPackageName = Asset.PackageName.ToString();
					if ( MapName.Equals(AssetPackageName, ESearchCase::IgnoreCase) )
					{
						// Found the asset data for this map.  Build the FMapListInfo.
						AddMapAssetToMapList(Asset);
						break;
					}
				}
			}
		}
	}

	// Now add the custom maps..
	for (int32 i = 0; i < NewRules.CustomMapList.Num(); i++)
	{
		FString MapPackageName = NewRules.CustomMapList[i];
		if ( FPackageName::IsShortPackageName(MapPackageName) )
		{
			if (!FPackageName::SearchForPackageOnDisk(MapPackageName, &MapPackageName))
			{
				UE_LOG(UT,Log,TEXT("Failed to find package for shortname '%s'"), *MapPackageName);
			}
		}

		// Look for the map in the asset registry...
		for (const FAssetData& Asset : MapAssets)
		{
			FString AssetPackageName = Asset.PackageName.ToString();
			if ( MapPackageName.Equals(AssetPackageName, ESearchCase::IgnoreCase) )
			{
				// Found the asset data for this map.  Build the FMapListInfo.
				int32 Idx = AddMapAssetToMapList(Asset);

				AUTBaseGameMode* DefaultBaseGameMode = GetWorld()->GetAuthGameMode<AUTBaseGameMode>();
				if (DefaultBaseGameMode)
				{
					// Look to see if there are redirects for this map
					
					FPackageRedirectReference Redirect;
					if ( DefaultBaseGameMode->FindRedirect(NewRules.CustomMapList[i], Redirect) )
					{
						MapList[Idx]->Redirect = Redirect;
					}
				}
				break;
			}
		}
	}

	if (MapList.Num() == 0)
	{
		// Last ditch.. I DO NOT LIKE THIS.. but if there are no maps, fill it with all of the maps from the
		// Asset list.
	
		for (int32 i=0; i < NewRules.MapPrefixes.Num(); i++)
		{
			if ( NewRules.MapPrefixes[i] == TEXT("") ) continue;

			for (const FAssetData& Asset : MapAssets)
			{
				FString AssetPackageName = Asset.PackageName.ToString();
				if ( Asset.AssetName.ToString().StartsWith(NewRules.MapPrefixes[i], ESearchCase::IgnoreCase) )
				{
					// Found the asset data for this map.  Build the FMapListInfo.
					int32 Idx = AddMapAssetToMapList(Asset);

					AUTBaseGameMode* DefaultBaseGameMode = GetWorld()->GetAuthGameMode<AUTBaseGameMode>();
					if (DefaultBaseGameMode)
					{
						// Look to see if there are redirects for this map
					
						FPackageRedirectReference Redirect;
						if ( DefaultBaseGameMode->FindRedirect(AssetPackageName, Redirect) )
						{
							MapList[Idx]->Redirect = Redirect;
						}
					}
				}
			}	
		}
	}

	BuildSlateBadge();

	// Fix up the Description
	TArray<FString> PropertyLookups;
	int32 Left = Data.Description.Find(TEXT("%"),ESearchCase::IgnoreCase, ESearchDir::FromStart, 0);
	while (Left != INDEX_NONE)
	{
		int32 Right = Data.Description.Find(TEXT("%"),ESearchCase::IgnoreCase, ESearchDir::FromStart, Left + 1);
		if (Right > Left)
		{
			FString PropertyString = Data.Description.Mid(Left, Right-Left + 1);
			if (PropertyLookups.Find(PropertyString) == INDEX_NONE)
			{
				PropertyLookups.Add(PropertyString);
			}
			Left = Data.Description.Find(TEXT("%"),ESearchCase::IgnoreCase, ESearchDir::FromStart, Right +1);	
		}
		else
		{
			break;
		}
	}

	AUTGameMode* DefaultGameObject = GetDefaultGameModeObject();
	for (int32 i = 0; i < PropertyLookups.Num(); i++)
	{
		if ( PropertyLookups[i].Equals(TEXT("%maxplayers%"),ESearchCase::IgnoreCase) )		
		{
			Data.Description = Data.Description.Replace(*PropertyLookups[i], *FString::FromInt(Data.MaxPlayers), ESearchCase::IgnoreCase);
		}
		else
		{
			// Grab the prop name.
			FString PropName = PropertyLookups[i].LeftChop(1).RightChop(1);	// Remove the %
			FString Value = TEXT("");

			// First search the url for PropName=
			if (UGameplayStatics::HasOption(Data.GameOptions, *PropName))
			{
				Value = UGameplayStatics::ParseOption(Data.GameOptions,  PropName);
			}
			else if (DefaultGameObject)
			{
				for (TFieldIterator<UProperty> It(DefaultGameObject->GetClass()); It; ++It)
				{
					UProperty* Prop = *It;
					if ( Prop->GetName().Equals(PropName,ESearchCase::IgnoreCase) )
					{
						uint8* ObjData = (uint8*)DefaultGameObject;
						Prop->ExportText_InContainer(0, Value, DefaultGameObject, DefaultGameObject, DefaultGameObject, PPF_IncludeTransient);
						break;
					}
				}
			}

			if (Value.Equals(TEXT("true"), ESearchCase::IgnoreCase))
			{
				Value = TEXT("On");
			}
			else if (Value.Equals(TEXT("false"), ESearchCase::IgnoreCase))
			{
				Value = TEXT("Off");
			}

			Data.Description = Data.Description.Replace(*PropertyLookups[i], *Value, ESearchCase::IgnoreCase);
		}
	}
}

FString AUTReplicatedGameRuleset::Fixup(FString OldText)
{
	FString Final = OldText.Replace(TEXT("\\n"), TEXT("\n"), ESearchCase::IgnoreCase);
	Final = Final.Replace(TEXT("\\n"), TEXT("\n"), ESearchCase::IgnoreCase);

	return Final;
}

void AUTReplicatedGameRuleset::BuildSlateBadge()
{
#if !UE_SERVER
	SlateBadge = nullptr;
	if (!Data.DisplayTexture.IsEmpty())
	{
		BadgeTexture = LoadObject<UTexture2D>(nullptr, *Data.DisplayTexture, nullptr, LOAD_None, nullptr);
		if (BadgeTexture)
		{
			SlateBadge = new FSlateDynamicImageBrush(BadgeTexture, FVector2D(256.0f, 256.0f), NAME_None);
		}
	}
#endif
}

#if !UE_SERVER
const FSlateBrush* AUTReplicatedGameRuleset::GetSlateBadge() const
{
	return (SlateBadge != nullptr) ? SlateBadge : SUWindowsStyle::Get().GetBrush("UWindows.Lobby.MatchBadge");	
}
#endif


AUTGameMode* AUTReplicatedGameRuleset::GetDefaultGameModeObject()
{
	if (!Data.GameMode.IsEmpty())
	{
		FString LongGameModeClassname = UGameMapsSettings::GetGameModeForName(Data.GameMode);
		UClass* GModeClass = LoadClass<AUTGameMode>(NULL, *LongGameModeClassname, NULL, LOAD_NoWarn | LOAD_Quiet, NULL);
		if (GModeClass)
		{
			AUTGameMode* DefaultGameModeObject = GModeClass->GetDefaultObject<AUTGameMode>();
			return DefaultGameModeObject;
		}
	}
	else
	{
		UE_LOG(UT, Warning, TEXT("%s Empty GameModeClass for Ruleset %s"), *GetName(), *Data.Title);
	}
	return NULL;
}

FString AUTReplicatedGameRuleset::GetDescription()
{
	return Data.Description;
}

void AUTReplicatedGameRuleset::MakeJsonReport(TSharedPtr<FJsonObject> JsonObject)
{
	JsonObject->SetStringField(TEXT("Title"), Data.Title);
	JsonObject->SetStringField(TEXT("GameMode"), Data.GameMode);
	JsonObject->SetStringField(TEXT("GameOptions"), Data.GameOptions);

	JsonObject->SetNumberField(TEXT("MaxPlayers"), Data.MaxPlayers);
	JsonObject->SetNumberField(TEXT("OptimalPlayers"), OptimalPlayers);

	JsonObject->SetBoolField(TEXT("bCompetitiveMatch"), Data.bCompetitiveMatch);
	JsonObject->SetBoolField(TEXT("bTeamGame"), Data.bTeamGame);

	JsonObject->SetStringField(TEXT("DefaultMap"), Data.DefaultMap);

	TArray<TSharedPtr<FJsonValue>> MapArray;
	for (int32 i=0; i < MapList.Num(); i++)
	{
		TSharedPtr<FJsonObject> MapJson = MakeShareable(new FJsonObject);
		MapJson->SetStringField(TEXT("MapName"), MapList[i]->MapPackageName);
		MapArray.Add( MakeShareable( new FJsonValueObject( MapJson )));			
	}

	JsonObject->SetArrayField(TEXT("MapLIst"), MapArray);

	if (Data.RequiredPackages.Num() > 0)
	{
		TArray<TSharedPtr<FJsonValue>> ReqArray;
		for (int32 i=0; i < Data.RequiredPackages.Num(); i++)
		{
			TSharedPtr<FJsonObject> ReqJson = MakeShareable(new FJsonObject);
			ReqJson->SetStringField(TEXT("Package"), Data.RequiredPackages[i]);
			ReqArray.Add( MakeShareable( new FJsonValueObject( ReqJson )));			
		}

		JsonObject->SetArrayField(TEXT("RequiredPackages"), ReqArray);
	}
}

FString AUTReplicatedGameRuleset::GenerateURL(const FString& StartingMap, bool bAllowBots, int32 BotDifficulty, bool bRequireFilled)
{
	return Data.GenerateURL(StartingMap, bAllowBots, BotDifficulty, bRequireFilled);
}
