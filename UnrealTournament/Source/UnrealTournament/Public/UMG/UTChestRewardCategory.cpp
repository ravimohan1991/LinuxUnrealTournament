// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTChestRewardCategory.h"

#include "UTTileView.h"
#include "UTGlowingRarityText.h"

UUTChestRewardCategory::UUTChestRewardCategory(const FObjectInitializer& Initializer)
	: Super(Initializer)
{
}

void UUTChestRewardCategory::NativeConstruct()
{
	Super::NativeConstruct();

}

void UUTChestRewardCategory::NativeDestruct()
{
	Super::NativeDestruct();
}

void UUTChestRewardCategory::FillData(EUTItemRarity GroupRarity)
{
	GlowingRarity_Text->SetRarityBP(GroupRarity);
	TileView_Items->SetDataProvider(RewardDataProvider);
}

void UUTChestRewardCategory::RefreshOwnedStatus()
{
	TileView_Items->RebuildListItems();
}