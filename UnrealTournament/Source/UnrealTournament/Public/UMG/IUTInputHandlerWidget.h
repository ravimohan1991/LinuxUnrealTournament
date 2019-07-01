// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "IUTInputHandlerWidget.generated.h"

UINTERFACE()
class UUTInputHandlerWidget : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class IUTInputHandlerWidget
{
	GENERATED_IINTERFACE_BODY()
public:

	UFUNCTION(BlueprintNativeEvent, Category = IUTInputHandlerWidget)
	void HandleRequestedInput(FKey Key, EInputEvent EventType);
};