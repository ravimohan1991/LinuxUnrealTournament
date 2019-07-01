// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "AssetData.h"
#include "UTATypes.h"
#include "UTReplicatedGameRuleset.generated.h"


UCLASS()
class UNREALTOURNAMENT_API AUTReplicatedGameRuleset : public AInfo
{
	GENERATED_UCLASS_BODY()

public:

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Ruleset, ReplicatedUsing = OnReceiveData)
	FUTGameRuleset Data;

	// This is the list of maps that are available to this rule
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Ruleset)
	TArray<AUTReplicatedMapInfo*> MapList;
	
	// The number of players that is optimal for this rule.  The game will not display maps who optimal player counts are less than this number.
	UPROPERTY(Replicated, BlueprintReadOnly, Category = Ruleset)
	int32 OptimalPlayers;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = Ruleset)
	uint32 bCustomRuleset:1;

	void SetRules(const FUTGameRuleset& NewRules, const TArray<FAssetData>& MapAssets);

	UPROPERTY()
	UTexture2D*  BadgeTexture;

#if !UE_SERVER
	FSlateDynamicImageBrush* SlateBadge;
	const FSlateBrush* GetSlateBadge() const;
#endif

	// When the material reference arrives, build the slate texture
	UFUNCTION()
	virtual void BuildSlateBadge();

	UFUNCTION()
	virtual AUTGameMode* GetDefaultGameModeObject();

	// Returns the description of this ruleset. 
	UFUNCTION()
	virtual FString GetDescription();

protected:

	UFUNCTION()
	void OnReceiveData();

	FString Fixup(FString OldText);
	int32 AddMapAssetToMapList(const FAssetData& Asset);

public:
	virtual void MakeJsonReport(TSharedPtr<FJsonObject> JsonObject);

	/**
	 *	Generates a URL that can be used to launch a match based on this Ruleset.
	 **/
	virtual FString GenerateURL(const FString& StartingMap, bool bAllowBots, int32 BotDifficulty, bool bRequireFilled);

};



