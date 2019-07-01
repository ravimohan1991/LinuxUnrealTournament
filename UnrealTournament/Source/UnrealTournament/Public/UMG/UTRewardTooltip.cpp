// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTRewardTooltip.h"

#include "UTTextBlock.h"
#include "UTRichText.h"
#include "UTRewardDisplayData.h"
#include "UTGameUIData.h"
#include "UTGlobals.h"

#if WITH_PROFILE
#include "UtMcpDefinition.h"
#include "UtMcpTokenDefinition.h"
#else
#include "GithubStubs.h"
#endif

//////////////////////////////////////////////////////////////////////////
// UUTRewardTooltipItem
//////////////////////////////////////////////////////////////////////////

UUTRewardTooltipItem::UUTRewardTooltipItem(const FObjectInitializer& Initializer)
	: Super(Initializer)
{

}

void UUTRewardTooltipItem::SetRewardInfo(const UUtMcpDefinition* SpecificItem, const FString& LootTierGroup, int32 Quantity /*= 1*/, EUTWidgetStyleSize::Type ImageSize /*= EUTWidgetStyleSize::Large*/)
{
	// If there's no info, just collapse
	SetVisibility(SpecificItem || !LootTierGroup.IsEmpty() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	const UUTRewardDisplayData& RewardData = UUTGameUIData::Get().GetRewardDisplayData();

	FText RewardName;
	FText RewardDesc;
	FText RewardTypeName;
	UTexture2D* RewardIcon = nullptr;
	if (SpecificItem)
	{
#if WITH_PROFILE
		if (SpecificItem->IsA<UUtMcpTokenDefinition>())
		{
			const FString TokenName = SpecificItem->GetPersistentName();
			if (TokenName.Contains(TEXT("TimeCurrency")) &&
				!TokenName.Contains(TEXT("Sub")))
			{
				// Show time currency as a currency
				SetRewardInfo(EUTCurrencyType::TimeCurrency, Quantity);
				return;
			}
			else if (TokenName.Contains(TEXT("Currency.Mtx")))
			{
				// Show MTX as a currency
				SetRewardInfo(EUTCurrencyType::MtxCurrency, Quantity);
				return;
			}
		}
		FText DisplayName = SpecificItem->GetDisplayName(true);
		RewardName = Quantity > 1 ? FText::Format(UTRewardConstants::QuantityFormat, FText::AsNumber(Quantity), DisplayName) : DisplayName;
		RewardDesc = SpecificItem->GetDescription();

		if (const FUTRewardTypeInfo* RewardTypeInfo = RewardData.GetItemRewardTypeInfo(SpecificItem))
		{
			RewardTypeName = RewardTypeInfo->DisplayName;
			RewardIcon = RewardTypeInfo->DisplayIcons[ImageSize];

			if (RewardDesc.IsEmpty())
			{
				RewardDesc = RewardTypeInfo->ShortDescription;
				if (RewardDesc.IsEmpty())
				{
					RewardDesc = RewardTypeInfo->FullDescription;
				}
			}
		}
#endif
	}
	else if (const FUTLootTierRewardInfo* LootRewardInfo = RewardData.GetLootTierRewardInfo(LootTierGroup))
	{
		RewardName = LootRewardInfo->DisplayName;
		RewardDesc = LootRewardInfo->ShortDescription;
		RewardIcon = LootRewardInfo->DisplayIcons[ImageSize];

		if (LootRewardInfo->LootTypeName.IsEmpty() && LootRewardInfo->bIsRandomRoll)
		{
			RewardTypeName = NSLOCTEXT("RewardTooltip", "Random Reward", "Random Reward");
		}
		else
		{
			RewardTypeName = LootRewardInfo->LootTypeName;	
		}
	}

	Text_RewardType->SetText(RewardTypeName);
	Text_RewardName->SetText(RewardName);
	Text_RewardDesc->SetText(RewardDesc);

	Image_RewardIcon->SetBrushFromTexture(RewardIcon ? RewardIcon : RewardData.DefaultRewardIcon);
}

void UUTRewardTooltipItem::SetRewardInfo(const FUTRewardTypeInfo& RewardInfo, const FUTRewardTypeInfo& RewardTypeInfo, EUTWidgetStyleSize::Type ImageSize /*= EUTWidgetStyleSize::Large*/)
{
	Text_RewardType->SetText(RewardTypeInfo.DisplayName);
	Text_RewardName->SetText(RewardInfo.DisplayName);
	Text_RewardDesc->SetText(RewardInfo.ShortDescription);

	Image_RewardIcon->SetBrushFromTexture(RewardInfo.DisplayIcons[ImageSize]);
}

void UUTRewardTooltipItem::SetRewardInfo(EUTCurrencyType Currency, int32 Amount, EUTWidgetStyleSize::Type ImageSize /*= EUTWidgetStyleSize::Large*/)
{
	const FUTRewardTypeInfo& CurrencyRewardInfo = UUTGameUIData::Get().GetRewardDisplayData().Currencies[TEnumValue(Currency)];

	Text_RewardType->SetText(NSLOCTEXT("RewardTooltip", "Currency", "Currency"));

	Text_RewardName->SetText(FText::Format(UTRewardConstants::CurrencyQuantityFormat, FText::AsNumber(Amount), CurrencyRewardInfo.DisplayName));
	Text_RewardDesc->SetText(CurrencyRewardInfo.ShortDescription);
	Image_RewardIcon->SetBrushFromTexture(CurrencyRewardInfo.DisplayIcons[ImageSize]);
}

//////////////////////////////////////////////////////////////////////////
// UUTRewardTooltip
//////////////////////////////////////////////////////////////////////////

UUTRewardTooltip::UUTRewardTooltip(const FObjectInitializer& Initializer)
	: Super(Initializer)
	, UnearnedContentTint(.4f, .4f, .4f, 1.f)
	, bIsEarned(true)
{
}

void UUTRewardTooltip::SetRewardContext(const FText& Context)
{
	Text_RewardContext->SetText(Context);
}

void UUTRewardTooltip::SetRewardInfo(const UUtMcpDefinition* SpecificItem, const FString& LootTierGroup, int32 Quantity, bool bInIsEarned, EUTWidgetStyleSize::Type ImageSize)
{
	SetIsEarned(bInIsEarned);
	Item_Reward->SetRewardInfo(SpecificItem, LootTierGroup, Quantity, ImageSize);
}

void UUTRewardTooltip::SetRewardInfo(const FUTRewardTypeInfo& RewardInfo, const FUTRewardTypeInfo& RewardTypeInfo, bool bInIsEarned, EUTWidgetStyleSize::Type ImageSize)
{
	SetIsEarned(bInIsEarned);
	Item_Reward->SetRewardInfo(RewardInfo, RewardTypeInfo, ImageSize);
}

void UUTRewardTooltip::SetRewardInfo(EUTCurrencyType Currency, int32 Amount, bool bInIsEarned, EUTWidgetStyleSize::Type ImageSize)
{
	SetIsEarned(bInIsEarned);
	Item_Reward->SetRewardInfo(Currency, Amount, ImageSize);
}

void UUTRewardTooltip::SetIsEarned(bool bInIsEarned)
{
	bIsEarned = bInIsEarned;
	//Border_Contents->SetContentColorAndOpacity(bInIsEarned ? FLinearColor::White : UnearnedContentTint);
}