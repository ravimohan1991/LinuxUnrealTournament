// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "ITooltipWidget.generated.h"

UINTERFACE()
class UNREALTOURNAMENT_API UTooltipWidget : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class UNREALTOURNAMENT_API ITooltipWidget
{
	GENERATED_IINTERFACE_BODY()
public:

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = ITooltipWidget)
	void SetAdditionalContent(UWidget* Widget);

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = ITooltipWidget)
	void Show();

	UFUNCTION(BlueprintImplementableEvent, BlueprintCallable, Category = ITooltipWidget)
	void Hide();
};