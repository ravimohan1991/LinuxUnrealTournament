// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTUMGWidget.h"
#include "UTUMGWidget_Toast.generated.h"

UCLASS()
class UNREALTOURNAMENT_API UUTUMGWidget_Toast : public UUTUMGWidget
{
	GENERATED_UCLASS_BODY()

	// The Message to be displayed
	UPROPERTY(BlueprintReadWrite, category = Toast)
	FText Message;

	// How long to display the toast for
	UPROPERTY(BlueprintReadWrite, category = UMG)
	float Duration;

	// Returns true if this toast is top on the stack
	UFUNCTION(BlueprintCallable, category = UMG)
	bool IsTopmostToast();

	virtual void CloseWidget();

	// TODO: Add the ability to have a custom toast icon...
};
