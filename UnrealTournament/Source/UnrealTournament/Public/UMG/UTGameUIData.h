// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTUITypes.h"
#include "UTRewardDisplayData.h"
#include "UTRichTextStyleData.h"
#include "UTInputDisplayData.h"
#include "UTGameUIData.generated.h"

/** 
 * Contains all UI-related static game data. Things like colors, localized text, images, etc. that are across multiple widgets. 
 * Primarily created to disconnect Dev-UI from Dev-General (since DG is authoritative on DefaultGameData).
 * 
 * We rely heavily on this data to exist throughout the game, so note the pattern of returning const asset references, not pointers.
 * This assures users that there's no need for validity checks, but it does mean that care must be taken when adding to this asset!
 * If you need to reference an asset immediately on load, use an object finder in the ctor, otherwise just take care to establish the asset reference immediately.
 */
UCLASS(config=Game)
class UNREALTOURNAMENT_API UUTGameUIData : public UDataAsset
{
	GENERATED_UCLASS_BODY()
		
	const UUTInputDisplayData& GetInputDisplayData() const { return *InputDisplayData; }
	const UUTRewardDisplayData& GetRewardDisplayData() const { return *RewardDisplayData; }
	const UUTRichTextStyleData& GetRichTextStyleData() const { return *RichTextStyleData; }

public:
	static UUTGameUIData& Get();
	
	/** The default icon to display whenever we don't have anything else to show */
	UPROPERTY(EditDefaultsOnly, Category = Defaults)
	UTexture2D* DefaultIcon;

	/** The brush to use when loading anything */
	UPROPERTY(EditAnywhere, Category = Loading)
	FSlateBrush LoadingSpinner;

private:

	UPROPERTY(EditDefaultsOnly, Category = DisplayData)
	UUTInputDisplayData* InputDisplayData;

	UPROPERTY(EditDefaultsOnly, Category = Styles)
	UUTRichTextStyleData* RichTextStyleData;

	UPROPERTY(EditDefaultsOnly, Category = DisplayData)
	UUTRewardDisplayData* RewardDisplayData;

};