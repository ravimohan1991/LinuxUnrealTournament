// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTUserWidgetBase.h"

#include "UTGlowingRarityText.generated.h"

UCLASS(Abstract)
class UUTGlowingRarityText : public UUTUserWidgetBase
{
	GENERATED_BODY()
public:
	UUTGlowingRarityText(const FObjectInitializer& Initializer);

	UFUNCTION(BlueprintImplementableEvent, Category = GlowingRarityText)
	void SetRarityBP(EUTItemRarity ItemRarity, bool bHideGlowingText = false);
};