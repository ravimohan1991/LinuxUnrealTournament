// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTTooltipBase.h"

#include "UTBasicTooltipWidget.generated.h"

UCLASS()
class UUTBasicTooltipWidget : public UUTTooltipBase
{
	GENERATED_UCLASS_BODY()

public:
	UFUNCTION(BlueprintImplementableEvent, Category = BasicTooltip)
	void UpdateTitleText(const FText& NewTitleText);

	UFUNCTION(BlueprintImplementableEvent, Category = BasicTooltip)
	void UpdateTooltipText(const FText& NewTooltipText);
};