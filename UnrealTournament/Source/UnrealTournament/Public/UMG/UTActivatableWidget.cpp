// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTActivatableWidget.h"

UUTActivatableWidget::UUTActivatableWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bAutoActivate(false)
	, bIsActive(false)
{
}

void UUTActivatableWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (bAutoActivate)
	{
		UE_LOG(UT, Verbose, TEXT("[%s] auto-activated"), *GetName());
		ActivateWidget();
	}
}

void UUTActivatableWidget::NativeDestruct()
{
	DeactivateWidget();
	OnWidgetActivated.Clear();
	OnWidgetActivatedNative.Clear();
	OnWidgetDeactivated.Clear();
	OnWidgetDeactivatedNative.Clear();
	Super::NativeDestruct();
}

void UUTActivatableWidget::ActivateWidget()
{
	if (!bIsActive && GetPlayerContext().IsValid())
	{
		/*
		if (UUTUIManagerWidget* UIManager = UUTUIManagerWidget::GetUIManagerWidget(GetWorld()))
		{
			UIManager->PushActivatableWidget(this);
		}
		*/

		bIsActive = true;

		NativeOnActivated();
	}
}

void UUTActivatableWidget::DeactivateWidget()
{
	if (bIsActive && GetPlayerContext().IsValid())
	{
		/*
		if (UUTUIManagerWidget* UIManager = UUTUIManagerWidget::GetUIManagerWidget(GetWorld()))
		{
			UIManager->PopActivatableWidget(this);
		}
		*/

		bIsActive = false;
			
		NativeOnDeactivated();		
	}
}

void UUTActivatableWidget::NativeOnActivated()
{
	OnActivated();
	OnWidgetActivatedNative.Broadcast();
	OnWidgetActivated.Broadcast();
}

void UUTActivatableWidget::NativeOnDeactivated()
{
	OnDeactivated();
	OnWidgetDeactivatedNative.Broadcast();
	OnWidgetDeactivated.Broadcast();
}

bool UUTActivatableWidget::OnHandleBackAction_Implementation()
{
	UE_LOG(UT, Verbose, TEXT("[%s] handled back with default implementation"), *GetName());

	// Default behavior is unconditional deactivation, override to do something else
	DeactivateWidget();

	return true;
}