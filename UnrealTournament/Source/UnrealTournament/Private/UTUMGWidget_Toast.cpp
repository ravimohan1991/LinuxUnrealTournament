// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTUMGWidget_Toast.h"

UUTUMGWidget_Toast::UUTUMGWidget_Toast(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	DisplayZOrder = 30000;
	Duration = 1.5f;
}

bool UUTUMGWidget_Toast::IsTopmostToast()
{
	return (UTPlayerOwner && UTPlayerOwner->ToastStack.Num() >0 && UTPlayerOwner->ToastStack[0] == this);
}

void UUTUMGWidget_Toast::CloseWidget()
{
	if (UTPlayerOwner != nullptr)
	{
		UTPlayerOwner->ToastCompleted(this);
	}

	Super::CloseWidget();
}
