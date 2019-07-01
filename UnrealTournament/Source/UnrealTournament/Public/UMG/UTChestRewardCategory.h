// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTUserWidgetBase.h"
#include "WidgetFactory.h"

#include "UTChestRewardCategory.generated.h"

class UUTTileView;
class UUTGlowingRarityText;

UCLASS()
class UNREALTOURNAMENT_API UUTChestRewardCategory : public UUTUserWidgetBase
{
	GENERATED_BODY()

public:
	UUTChestRewardCategory(const FObjectInitializer& Initializer);

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	void FillData(EUTItemRarity GroupRarity);

	void RefreshOwnedStatus();

	UPROPERTY()
	TArray<UUtMcpDefinition*> RewardDataProvider;

private:
	UPROPERTY(meta = (BindWidget))
	UUTTileView* TileView_Items;

	UPROPERTY(meta = (BindWidget))
	UUTGlowingRarityText* GlowingRarity_Text;
};
