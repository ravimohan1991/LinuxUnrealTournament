// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTMcpTypeRewardImage.h"

#include "UTLazyImage.h"

void UUTMcpTypeRewardImage::NativeConstruct()
{
	Image_RewardImage->SetVisibility(ESlateVisibility::Collapsed);
}

void UUTMcpTypeRewardImage::SetImageWithDefinition(UUtMcpDefinition& ItemDef)
{
	Image_RewardImage->SetVisibility(ESlateVisibility::Collapsed);
}
