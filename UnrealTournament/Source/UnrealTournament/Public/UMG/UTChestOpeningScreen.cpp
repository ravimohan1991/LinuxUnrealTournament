// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTChestOpeningScreen.h"
#include "UTTextBlock.h"
#include "UTLazyImage.h"
#include "UTGlowingRarityText.h"
#include "UTMcpTypeRewardImage.h"
#include "UTChestOpeningScreen.h"
#include "UTGameUIData.h"

#if WITH_PROFILE
#include "UtMcpTokenDefinition.h"
#else
#include "GithubStubs.h"
#endif

#if !UE_BUILD_SHIPPING
static FAutoConsoleVariable CVarLootCrateRewardOverride(
	TEXT("LootCrate.RewardOverride"),
	TEXT(""),
	TEXT("Override the visual display for loot crate reward"),
	ECVF_Default
);
#endif

UUTChestOpeningScreen::UUTChestOpeningScreen(const FObjectInitializer& Initializer)
	: Super(Initializer)
	, PictureFlipRate(0.1f)
	, PictureSlowDownRate(0.1f)
	, PictureFinalStopRate(0.6f)
	, EpicMegaRareCount(1)
	, UltraRareCount(2)
	, RareCount(3)
	, CommonCount(5)
	, SpinnerImageIdx(0)
	, SpinnerState(EUTSpinState::Inactive)
{
}

void UUTChestOpeningScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	
	/*
	if (UStoreContext* StoreContext = GetContext<UStoreContext>())
	{
		StoreContext->ToggleCurrencyUpdate(false);
	}

	if (UMenuContext* MenuContext = GetContext<UMenuContext>())
	{
		if (auto PurchaseConfirmation = MenuContext->GetPurchaseConfirmation())
		{
			PurchaseConfirmation->OnPurchaseSuccess.AddUniqueDynamic(this, &UUTChestOpeningScreen::NativeReset);
		}
	}
	*/

	RefreshOwnedStatus();
	NativeReset();
	LoadFluffRewards();
}

void UUTChestOpeningScreen::NativeOnDeactivated()
{
	Super::NativeOnDeactivated();

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(SpinnerTimerHandle);
	}
	/*
	if (UStoreContext* StoreContext = GetContext<UStoreContext>())
	{
		StoreContext->ToggleCurrencyUpdate(true);
	}

	if (UMenuContext* MenuContext = GetContext<UMenuContext>())
	{
		if (auto PurchaseConfirmation = MenuContext->GetPurchaseConfirmation())
		{
			PurchaseConfirmation->OnPurchaseSuccess.RemoveAll(this);
		}
	}
	*/
}

void UUTChestOpeningScreen::NativeReset()
{
	BP_FullReset();

	Image_RotatingReward->SetBrushFromLazyTexture(LazyDefaultTreasureImage);
	SpinnerState = EUTSpinState::Inactive;
	PictureSlowDownRate = PictureFlipRate;

	LoadPotentialRewards();
}

void UUTChestOpeningScreen::RefreshOwnedStatus()
{
	Reward_EpicMegaRare->RefreshOwnedStatus();
	Reward_UltraRare->RefreshOwnedStatus();
	Reward_Rare->RefreshOwnedStatus();
	Reward_Common->RefreshOwnedStatus();
}


void UUTChestOpeningScreen::ShuffleSpinnerRewardArray()
{
	for (int32 FisherIdx = SpinnerItemArray.Num() - 1; FisherIdx > 0; FisherIdx--)
	{
		int32 ReplacementIdx = FMath::RandRange(0, FisherIdx);
		SpinnerItemArray.Swap(ReplacementIdx, FisherIdx);
	}
}

void UUTChestOpeningScreen::ProcessNewCurrentlySelectedReward(FChestResultEntryRow& CurrentlySelectedReward)
{
#if WITH_PROFILE
	if (CurrentlySelectedReward.Quantity > 1)
	{
		Text_RewardName->SetText(FText::Format(NSLOCTEXT("ChestOpeningScreen", "FinalCount", "{0} x {1}"), CurrentlySelectedReward.RewardItem->GetDisplayName(), FText::AsNumber(CurrentlySelectedReward.Quantity)));
	}
	else
	{
		Text_RewardName->SetText(CurrentlySelectedReward.RewardItem->GetDisplayName());
	}

	UUtMcpTokenDefinition* TokenItemDefinition = Cast<UUtMcpTokenDefinition>(CurrentlySelectedReward.RewardItem);
	if (TokenItemDefinition && !TokenItemDefinition->LootCrateDisplayAsset.ToString().IsEmpty())
	{
		Image_RotatingReward->SetBrushFromLazyTexture(TokenItemDefinition->LootCrateDisplayAsset);
	}
	else
	{
		Image_RotatingReward->SetBrushFromItemDefinition(CurrentlySelectedReward.RewardItem);
	}
	GlowingText_RotatingReward->SetRarityBP(CurrentlySelectedReward.RewardRarity, CurrentlySelectedReward.RewardRarity == EUTItemRarity::Common);
	SetItemRarities(CurrentlySelectedReward.RewardRarity);

	Reward_RotatingSubImage->SetImageWithDefinition(*CurrentlySelectedReward.RewardItem);

	switch (CurrentlySelectedReward.RewardItem->GetItemType())
	{
	case EUtItemType::Card:
		Text_RewardType->SetText(NSLOCTEXT("ChestOpeningScreen", "Card", "Card"));
		break;
	case EUtItemType::Skin:
		Text_RewardType->SetText(NSLOCTEXT("ChestOpeningScreen", "Hero Skin", "Hero Skin"));
		break;
	case EUtItemType::Boost:
		Text_RewardType->SetText(NSLOCTEXT("ChestOpeningScreen", "Boost", "Boost"));
		break;
	case EUtItemType::Currency:
		Text_RewardType->SetText(NSLOCTEXT("ChestOpeningScreen", "Coins", "Coins"));
		break;
	case EUtItemType::Token:
		if (CurrentlySelectedReward.TemplateId.Contains("MTX"))
		{
			Text_RewardType->SetText(NSLOCTEXT("ChestOpeningScreen", "Coins", "Coins"));
		}
		else if (CurrentlySelectedReward.TemplateId.Contains("Token.BS"))
		{
			Text_RewardType->SetText(NSLOCTEXT("ChestOpeningScreen", "Banner", "Banner"));
		}
		else
		{
			Text_RewardType->SetText(NSLOCTEXT("ChestOpeningScreen", "Reward", "Reward"));
		}
		break;
	default:
		Text_RewardType->SetText(NSLOCTEXT("ChestOpeningScreen", "Reward", "Reward"));
		break;
	}
#endif
}

void UUTChestOpeningScreen::BeginOpen()
{
	SpinnerState = EUTSpinState::Opening;
}

void UUTChestOpeningScreen::FireSpinner(const UUtMcpCardPackItem* PackToOpen, const TArray<FMcpItemIdAndQuantity> RewardArray)
{
#if WITH_PROFILE
	if (SpinnerState == EUTSpinState::Opening)
	{
		SpinnerState = EUTSpinState::Spinning;

		Text_RewardName->SetText(FText::GetEmpty());

		// currently assuming one reward from loot crate!
#if !UE_BUILD_SHIPPING
		FString LootCrateRewardOverride = CVarLootCrateRewardOverride->GetString();

		if (!LootCrateRewardOverride.IsEmpty())
		{
			FMcpItemIdAndQuantity Reward;
			Reward.ItemId = LootCrateRewardOverride;
			Reward.Quantity = 1;

			UUtMcpDefinition* ItemDef = nullptr;

			const UUTRewardDisplayData& RewardData = UUTGameUIData::Get().GetRewardDisplayData();
			RewardData.FindLootCrateRewardById(Reward, ItemDef);

			ActualReward.RewardItem = ItemDef;
			ActualReward.Quantity = Reward.Quantity;
			ActualReward.TemplateId = Reward.ItemId;
			ActualReward.RewardRarity = ItemDef->GetItemRarity();

			TAssetPtr<UObject> IconAsset = ItemDef->GetIconAsset();
			UUTGlobals::Get().StreamableManager.RequestAsyncLoad(IconAsset.ToStringReference(),
				[this, IconAsset]
				{
					this->LoadedImages.AddUnique(IconAsset.Get());
				},
				TEnumValue(EUTAsyncLoadingPriority::HighPriorityForGameFlow));

			UUtMcpTokenDefinition* TokenItemDef = Cast<UUtMcpTokenDefinition>(ItemDef);
			if (TokenItemDef && !TokenItemDef->LootCrateDisplayAsset.ToString().IsEmpty())
			{
				UUTGlobals::Get().StreamableManager.RequestAsyncLoad(TokenItemDef->LootCrateDisplayAsset.ToStringReference(),
				[this, TokenItemDef]
				{
					this->LoadedImages.AddUnique(TokenItemDef->LootCrateDisplayAsset.Get());
				},
				TEnumValue(EUTAsyncLoadingPriority::HighPriorityForGameFlow));
			}

			SpinnerItemArray.Add(ActualReward);
		}
		else
#endif
		if (RewardArray.IsValidIndex(0))
		{
			FMcpItemIdAndQuantity Reward = RewardArray[0];

			UUtMcpDefinition* ItemDef = nullptr;

			const UUTRewardDisplayData& RewardData = UUTGameUIData::Get().GetRewardDisplayData();
			RewardData.FindLootCrateRewardById(Reward, ItemDef);

			ActualReward.RewardItem = ItemDef;
			ActualReward.Quantity = Reward.Quantity;
			ActualReward.TemplateId = Reward.ItemId;
			ActualReward.RewardRarity = ItemDef->GetItemRarity();

			TAssetPtr<UObject> IconAsset = ItemDef->GetIconAsset();
			UUTGlobals::Get().StreamableManager.RequestAsyncLoad(IconAsset.ToStringReference(),
				[this, IconAsset]
				{
					this->LoadedImages.AddUnique(IconAsset.Get());
				},
				TEnumValue(EUTAsyncLoadingPriority::HighPriorityForGameFlow));

			UUtMcpTokenDefinition* TokenItemDef = Cast<UUtMcpTokenDefinition>(ItemDef);
			if (TokenItemDef && !TokenItemDef->LootCrateDisplayAsset.ToString().IsEmpty())
			{
				UUTGlobals::Get().StreamableManager.RequestAsyncLoad(TokenItemDef->LootCrateDisplayAsset.ToStringReference(),
				[this, TokenItemDef]
				{
					this->LoadedImages.AddUnique(TokenItemDef->LootCrateDisplayAsset.Get());
				},
				TEnumValue(EUTAsyncLoadingPriority::HighPriorityForGameFlow));
			}

			SpinnerItemArray.Add(ActualReward);
		}

		ShuffleSpinnerRewardArray();

		GetWorld()->GetTimerManager().SetTimer(SpinnerTimerHandle, FTimerDelegate::CreateUObject(this, &UUTChestOpeningScreen::ShowNextImage), PictureFlipRate, true, 0.0f);
	}
#endif
}

void UUTChestOpeningScreen::BeginSpinnerSlowdown()
{
	if (SpinnerState == EUTSpinState::Spinning)
	{
		SpinnerState = EUTSpinState::SlowingDown;
	}
}

void UUTChestOpeningScreen::SetScreenState(EUTSpinState NewState)
{
	SpinnerState = NewState;
}

void UUTChestOpeningScreen::ShowNextImage()
{
#if WITH_PROFILE
	const int32 ImageCount = SpinnerItemArray.Num();
	FChestResultEntryRow* CurrentlySelectedReward = &SpinnerItemArray[SpinnerImageIdx];

	if (SpinnerState == EUTSpinState::Spinning)
	{	
		ProcessNewCurrentlySelectedReward(*CurrentlySelectedReward);

		SpinnerImageIdx = (SpinnerImageIdx + 1) % ImageCount;

		OnNextItemShown(CurrentlySelectedReward->RewardRarity, SpinnerState, false);
	}
	else if (SpinnerState == EUTSpinState::SlowingDown)
	{
		if (PictureSlowDownRate >= PictureFinalStopRate)
		{
			CurrentlySelectedReward = &ActualReward;
		}

		ProcessNewCurrentlySelectedReward(*CurrentlySelectedReward);

		auto& TimerManager = GetWorld()->GetTimerManager();

		TimerManager.ClearTimer(SpinnerTimerHandle);

		if (PictureSlowDownRate < PictureFinalStopRate)
		{

			SpinnerImageIdx = FMath::RandRange(0, ImageCount - 1);

			PictureSlowDownRate += 0.05f;

			TimerManager.SetTimer(SpinnerTimerHandle, FTimerDelegate::CreateUObject(this, &UUTChestOpeningScreen::ShowNextImage), PictureSlowDownRate, false);

			OnNextItemShown(CurrentlySelectedReward->RewardRarity, SpinnerState, false);
		}
		else
		{
			// final stop; fill the final reward data in

			GlowingText_FinalReward->SetRarityBP(CurrentlySelectedReward->RewardRarity, CurrentlySelectedReward->RewardRarity == EUTItemRarity::Common);

			if (ActualReward.Quantity > 1)
			{
				Text_FinalName->SetText(FText::Format(NSLOCTEXT("ChestOpeningScreen", "FinalCount", "{0} x {1}"), CurrentlySelectedReward->RewardItem->GetDisplayName(), FText::AsNumber(ActualReward.Quantity)));
			}
			else
			{
				Text_FinalName->SetText(ActualReward.RewardItem->GetDisplayName());
			}

			Text_FinalType->SetText(Text_RewardType->GetText());
			Reward_FinalSubImage->SetImageWithDefinition(*ActualReward.RewardItem);

			UUtMcpTokenDefinition* TokenItemDefinition = Cast<UUtMcpTokenDefinition>(ActualReward.RewardItem);
			if (TokenItemDefinition && TokenItemDefinition->LootCrateDisplayAsset.IsValid())
			{
				Image_FinalResult->SetBrushFromLazyTexture(TokenItemDefinition->LootCrateDisplayAsset);
			}
			else
			{
				Image_FinalResult->SetBrushFromItemDefinition(ActualReward.RewardItem);
			}


			SpinnerState = EUTSpinState::Inactive;

			OnNextItemShown(ActualReward.RewardRarity, SpinnerState, true);
			FinalSpinnerStop(ActualReward.RewardRarity);

			RefreshOwnedStatus();

			// Finally, load a new set of fluff rewards
			LoadFluffRewards();
		}
	}
#endif
}

void UUTChestOpeningScreen::LoadFluffRewards()
{
	TArray<FChestResultEntryRow> CommonRewards;
	TArray<FChestResultEntryRow> RareRewards;
	TArray<FChestResultEntryRow> UltraRareRewards;
	TArray<FChestResultEntryRow> EpicMegaRareRewards;

	LoadedImages.Empty();
	SpinnerItemArray.Empty();
	SpinnerImageIdx = 0;

	RemoveOwnedRewards();

	SortRewardsIntoArrays(CommonRewards, RareRewards, UltraRareRewards, EpicMegaRareRewards);

	LoadFluffRewardsInternal(CommonRewards, CommonCount);
	LoadFluffRewardsInternal(RareRewards, RareCount);
	LoadFluffRewardsInternal(UltraRareRewards, UltraRareCount);
	LoadFluffRewardsInternal(EpicMegaRareRewards, EpicMegaRareCount);

}

void UUTChestOpeningScreen::LoadFluffRewardsInternal(TArray<FChestResultEntryRow>& RewardArray, int32 RewardCount)
{
#if WITH_PROFILE
	const int32 ArraySize = RewardArray.Num();

	// pull some random rewards in assuming there is at least one reward in this reward tier
	if (ArraySize > 0)
	{
		for (int32 CountIdx = 0; CountIdx < RewardCount; CountIdx++)
		{
			int32 RandomIdx = FMath::RandRange(0, ArraySize - 1);

			TAssetPtr<UObject> IconAsset = RewardArray[RandomIdx].RewardItem->GetIconAsset();
			UUTGlobals::Get().StreamableManager.RequestAsyncLoad(IconAsset.ToStringReference(),
				[this, IconAsset]
				{
					this->LoadedImages.AddUnique(IconAsset.Get());
				}, 
				TEnumValue(EUTAsyncLoadingPriority::HighPriorityForGameFlow));

			UUtMcpTokenDefinition* TokenItemDef = Cast<UUtMcpTokenDefinition>(RewardArray[RandomIdx].RewardItem);
			if (TokenItemDef && !TokenItemDef->LootCrateDisplayAsset.ToString().IsEmpty())
			{
				UUTGlobals::Get().StreamableManager.RequestAsyncLoad(TokenItemDef->LootCrateDisplayAsset.ToStringReference(),
				[this, TokenItemDef]
				{
					this->LoadedImages.AddUnique(TokenItemDef->LootCrateDisplayAsset.Get());
				},
				TEnumValue(EUTAsyncLoadingPriority::HighPriorityForGameFlow));
			}

			SpinnerItemArray.Add(RewardArray[RandomIdx]);
		}

		ShuffleSpinnerRewardArray();
	}
#endif
}

void UUTChestOpeningScreen::RemoveOwnedRewards()
{
#if WITH_PROFILE
	if (UUtMcpProfile* McpProfileAccount = GetMcpProfileAccount())
	{
		for(int32 RewardIdx = PotentialRewardStruct.Num()-1; RewardIdx >= 0; RewardIdx--)
		{
			if (McpProfileAccount->IsItemOwned(PotentialRewardStruct[RewardIdx].TemplateId))
			{
				// HACK: Do not remove loot crate keys from potential rewards
				if (!PotentialRewardStruct[RewardIdx].TemplateId.Contains(TEXT("LootCrateKey")))
				{
					PotentialRewardStruct.RemoveAt(RewardIdx);
				}
			}
		}
	}
#endif
}

void UUTChestOpeningScreen::SortRewardsIntoArrays(TArray<FChestResultEntryRow>& CommonRewards, TArray<FChestResultEntryRow>& RareRewards, TArray<FChestResultEntryRow>& UltraRareRewards, TArray<FChestResultEntryRow>& EpicMegaRareRewards)
{
	UUTLocalPlayer* LocalPlayer = GetUTLocalPlayer();

	for (auto Item : PotentialRewardStruct)
	{
		TArray<FChestResultEntryRow>* FinalRewardBucket = &CommonRewards;

		switch (Item.RewardRarity)
		{
		case EUTItemRarity::Common:
			FinalRewardBucket = &CommonRewards;
			break;
		case EUTItemRarity::Rare:
			FinalRewardBucket = &RareRewards;
			break;
		case EUTItemRarity::UltraRare:
			FinalRewardBucket = &UltraRareRewards;
			break;
		case EUTItemRarity::EpicMegaRare:
			FinalRewardBucket = &EpicMegaRareRewards;
			break;
		}

		FinalRewardBucket->Add(Item);
	}
}

bool UUTChestOpeningScreen::OnHandleBackAction_Implementation()
{
	if (SpinnerState == EUTSpinState::Inactive)
	{
		return Super::OnHandleBackAction_Implementation();
	}

	return true;
}

void UUTChestOpeningScreen::LoadPotentialRewards()
{
#if WITH_PROFILE
	const UUTRewardDisplayData& RewardData = UUTGameUIData::Get().GetRewardDisplayData();

	// probably only needs to actually do this once
	TArray<FUTLootCrateRewardDisplay> AllLootRewards;
	RewardData.GetAllLootCrateRewardDisplays(AllLootRewards);

	PotentialRewardStruct.Empty();

	for (FUTLootCrateRewardDisplay Reward : AllLootRewards)
	{
		FChestResultEntryRow EntryItem;

		EntryItem.RewardItem = Reward.ReplacementReward;
		EntryItem.Quantity = 1;
		EntryItem.TemplateId = Reward.ReplacementString;
		EntryItem.RewardRarity = Reward.ReplacementReward->GetItemRarity();

		PotentialRewardStruct.Add(EntryItem);
	}

	TArray<FChestResultEntryRow> CommonRewards;
	TArray<FChestResultEntryRow> RareRewards;
	TArray<FChestResultEntryRow> UltraRareRewards;
	TArray<FChestResultEntryRow> EpicMegaRareRewards;

	SortRewardsIntoArrays(CommonRewards, RareRewards, UltraRareRewards, EpicMegaRareRewards);

	Reward_EpicMegaRare->RewardDataProvider.Empty();
	Reward_UltraRare->RewardDataProvider.Empty();
	Reward_Rare->RewardDataProvider.Empty();
	Reward_Common->RewardDataProvider.Empty();

	for (auto& Reward : EpicMegaRareRewards)
	{
		Reward_EpicMegaRare->RewardDataProvider.Add(Reward.RewardItem);
	}

	for (auto& Reward : UltraRareRewards)
	{
		Reward_UltraRare->RewardDataProvider.Add(Reward.RewardItem);
	}

	for (auto& Reward : RareRewards)
	{
		Reward_Rare->RewardDataProvider.Add(Reward.RewardItem);
	}

	for (auto& Reward : CommonRewards)
	{
		Reward_Common->RewardDataProvider.Add(Reward.RewardItem);
	}

	Reward_EpicMegaRare->FillData(EUTItemRarity::EpicMegaRare);
	Reward_UltraRare->FillData(EUTItemRarity::UltraRare);
	Reward_Rare->FillData(EUTItemRarity::Rare);
	Reward_Common->FillData(EUTItemRarity::Common);

	RemoveOwnedRewards();
#endif
}
