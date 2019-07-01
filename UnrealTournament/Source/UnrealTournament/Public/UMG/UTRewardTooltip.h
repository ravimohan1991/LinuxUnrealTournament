// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTTooltipBase.h"
#include "UTUITypes.h"
#include "UTRewardTooltip.generated.h"

class UUTTextBlock;
class UUtMcpDefinition;

struct FUTLevelUpRewardInfo;
struct FUTRewardTypeInfo;

UCLASS(BlueprintType, Blueprintable)
class UUTRewardTooltipItem : public UUserWidget
{
	GENERATED_BODY()

public:
	UUTRewardTooltipItem(const FObjectInitializer& Initializer);

	void SetRewardInfo(const UUtMcpDefinition* SpecificItem, const FString& LootTierGroup, int32 Quantity = 1, EUTWidgetStyleSize::Type ImageSize = EUTWidgetStyleSize::Large);
	void SetRewardInfo(const FUTRewardTypeInfo& RewardInfo, const FUTRewardTypeInfo& RewardTypeInfo, EUTWidgetStyleSize::Type ImageSize = EUTWidgetStyleSize::Large);
	void SetRewardInfo(EUTCurrencyType Currency, int32 Amount, EUTWidgetStyleSize::Type ImageSize = EUTWidgetStyleSize::Large);

private:	// Bound Widgets
	UPROPERTY(meta = (BindWidget))
	UUTTextBlock* Text_RewardName;

	UPROPERTY(meta = (BindWidget))
	UUTTextBlock* Text_RewardDesc;

	UPROPERTY(meta = (BindWidget))
	UUTTextBlock* Text_RewardType;

	UPROPERTY(meta = (BindWidget))
	UImage* Image_RewardIcon;
};

UCLASS(BlueprintType, Blueprintable)
class UUTRewardTooltip : public UUTTooltipBase
{
	GENERATED_BODY()

public:
	UUTRewardTooltip(const FObjectInitializer& Initializer);

	void SetRewardContext(const FText& Context);
	
	void SetRewardInfo(const UUtMcpDefinition* SpecificItem, const FString& LootTierGroup, int32 Quantity = 1, bool bInIsEarned = true, EUTWidgetStyleSize::Type ImageSize = EUTWidgetStyleSize::Large);
	void SetRewardInfo(const FUTRewardTypeInfo& RewardInfo, const FUTRewardTypeInfo& RewardTypeInfo, bool bInIsEarned = true, EUTWidgetStyleSize::Type ImageSize = EUTWidgetStyleSize::Large);
	void SetRewardInfo(EUTCurrencyType Currency, int32 Amount, bool bInIsEarned = true, EUTWidgetStyleSize::Type ImageSize = EUTWidgetStyleSize::Large);

	void SetIsEarned(bool bInIsEarned);

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = RewardTooltip)
	FLinearColor UnearnedContentTint;

	UPROPERTY(BlueprintReadOnly, Category = RewardTooltip)
	bool bIsEarned;

protected:	// Bound Widgets
	UPROPERTY(meta = (BindWidget))
	UUTTextBlock* Text_RewardContext;
	
	UPROPERTY(meta = (BindWidget))
	UUTRewardTooltipItem* Item_Reward;
};
