// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTRewardImage.h"

#include "UTLazyImage.h"
#include "UTMcpTypeRewardImage.h"

#if WITH_PROFILE
#include "UtMcpTokenDefinition.h"
#else
#include "GithubStubs.h"
#endif

UUTRewardImage::UUTRewardImage(const FObjectInitializer& Initializer)
	: Super(Initializer)
	, ItemDef(nullptr)
{
}

void UUTRewardImage::SetData_Implementation(UObject* InData)
{
#if WITH_PROFILE
	ItemDef = Cast<UUtMcpDefinition>(InData);

	LazyImage_HeroPortrait->SetBrushFromItemDefinition(ItemDef);

	ResetInternal();

	FString ItemName;
	EUtItemType UTItemType = UUtMcpDefinition::GetItemTypeFromPersistentName(ItemDef->GetTemplateId(), &ItemName);

	Reward_ImageSwitcher->SetImageWithDefinition(*ItemDef);

	if (UUtMcpProfile* McpProfileAccount = GetMcpProfileAccount())
	{
		// custom behavior on whether or not the owned check appears on the reward tile

		if (ItemName.Equals(TEXT("LootCrateKey")))
		{
			// Do not showed owned status for keys, not necessary on the reward tile itself
			Border_EyebrowText->SetVisibility(ESlateVisibility::Collapsed);

			UUtMcpTokenDefinition* TokenDef = Cast<UUtMcpTokenDefinition>(InData);
			if (TokenDef && !TokenDef->LootCrateDisplayAsset.ToString().IsEmpty())
			{
				LazyImage_HeroPortrait->SetBrushFromLazyTexture(TokenDef->LootCrateDisplayAsset);
			}
		}
		else if (McpProfileAccount->IsItemOwned(ItemDef->GetTemplateId()) && !ItemName.Equals(TEXT("UnlockPVP")))
		{
			// UnlockPVP is our current default token for fake display rewards. These rewards cannot be owned, which is represented by them all sharing the same token
			Border_EyebrowText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		}
		else if (UTItemType == EUtItemType::Token)
		{

		}
	}
#endif
}

void UUTRewardImage::Reset_Implementation()
{
	ResetInternal();
}

void UUTRewardImage::ResetInternal()
{
	if (bGenerateTooltip)
	{
		// Update the tooltip widget
		UUTRewardTooltip* RewardTooltip = Cast<UUTRewardTooltip>(CustomTooltipWidget);
		if (!RewardTooltip && RewardTooltipClass)
		{
			RewardTooltip = CreateWidget<UUTRewardTooltip>(GetOwningPlayer(), RewardTooltipClass);
			SetUTTooltip(RewardTooltip);
		}

		if (RewardTooltip)
		{
			FString LootTierInfo;
			RewardTooltip->SetRewardInfo(ItemDef, LootTierInfo);
			RewardTooltip->SetRewardContext(FText::GetEmpty());
		}
	}

	Border_EyebrowText->SetVisibility(ESlateVisibility::Collapsed);
}
