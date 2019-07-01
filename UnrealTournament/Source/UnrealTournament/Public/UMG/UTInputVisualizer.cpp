// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTInputVisualizer.h"

#include "UTGameUIData.h"
#include "UTInputDisplayData.h"

#include "UTGlobals.h"

UUTInputVisualizer::UUTInputVisualizer(const FObjectInitializer& Initializer)
	: Super(Initializer)
	, StyleSize(EUTWidgetStyleSize::Medium)
	, ActionName(NAME_None)
	, SpecificKey(EKeys::Invalid)
	, bShowKeyBorder(true)
	, bPreferSecondaryIcon(false)
{
	bIsVariable = false;
}

TSharedRef<SWidget> UUTInputVisualizer::RebuildWidget()
{
	MyKeyBox = SNew(SBox);
	return MyKeyBox.ToSharedRef();
}

void UUTInputVisualizer::ReleaseSlateResources(bool bReleaseChildren)
{
	/*
	UUTGlobals::Get().OnUsingGamepadChanged().RemoveAll(this);
	*/
	MyKeyBox.Reset();
}

void UUTInputVisualizer::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (MyKeyBox.IsValid() && IsDesignTime())
	{
		const UUTInputDisplayData& InputData = UUTGameUIData::Get().GetInputDisplayData();
		MyKeyBox->SetContent(InputData.CreateWidgetForKey(SpecificKey, StyleSize, bShowKeyBorder, bPreferSecondaryIcon));

		//@todo DanH: SBox::SetContent should invalidate shouldn't it?
		MyKeyBox->Invalidate(EInvalidateWidget::LayoutAndVolatility);
	}
}

void UUTInputVisualizer::SetSize(EUTWidgetStyleSize::Type InSize)
{
	if (InSize != StyleSize.GetValue())
	{
		StyleSize = InSize;
	}
}

void UUTInputVisualizer::ShowInputAction(FName InActionName)
{
	ActionName = InActionName;
	ListenForUsingGamepadChangedIfNeeded();

	if (MyKeyBox.IsValid())
	{
		UWorld* World = GetWorld();
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (PC && PC->PlayerInput)
		{
			SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			const UUTInputDisplayData& InputData = UUTGameUIData::Get().GetInputDisplayData();
			MyKeyBox->SetContent(InputData.CreateWidgetForAction(InActionName, PC, StyleSize, bShowKeyBorder, bPreferSecondaryIcon));

			//@todo DanH: SBox::SetContent should invalidate shouldn't it?
			MyKeyBox->Invalidate(EInvalidateWidget::LayoutAndVolatility);
		}
	}
}

void UUTInputVisualizer::ShowSpecificKey(FKey Key)
{
	SpecificKey = Key;
	ListenForUsingGamepadChangedIfNeeded();

	if (MyKeyBox.IsValid())
	{
		// Set the content for the key
		const UUTInputDisplayData& InputData = UUTGameUIData::Get().GetInputDisplayData();
		MyKeyBox->SetContent(InputData.CreateWidgetForKey(Key, StyleSize, bShowKeyBorder, bPreferSecondaryIcon));
		
		//@todo DanH: SBox::SetContent should invalidate shouldn't it?
		MyKeyBox->Invalidate(EInvalidateWidget::LayoutAndVolatility);

		// The determine if we should be showing it right now
		bool bShowKey = SpecificKey.IsValid() && SpecificKey.IsGamepadKey() == UUTGlobals::Get().GetIsUsingGamepad();
#if WITH_EDITOR
		bShowKey |= IsDesignTime();
#endif
		SetVisibility(bShowKey ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UUTInputVisualizer::ShowInputAxis(FName InAxisName)
{
	AxisName = InAxisName;
	ListenForUsingGamepadChangedIfNeeded();

	if (MyKeyBox.IsValid())
	{
		UWorld* World = GetWorld();
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (PC && PC->PlayerInput)
		{
			SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			const UUTInputDisplayData& InputData = UUTGameUIData::Get().GetInputDisplayData();
			MyKeyBox->SetContent(InputData.CreateWidgetForAxis(AxisName, PC, StyleSize, bShowKeyBorder, bPreferSecondaryIcon));

			//@todo DanH: SBox::SetContent should invalidate shouldn't it?
			MyKeyBox->Invalidate(EInvalidateWidget::LayoutAndVolatility);
		}
	}
}

#if WITH_EDITOR
void UUTInputVisualizer::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	//grab the property name
	const FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;

	//zero out the other two identifying ways to set an action, depending on the action
	if (PropertyName == GET_MEMBER_NAME_CHECKED(UUTInputVisualizer, ActionName))
	{
		AxisName = NAME_None;
	}
	else if (PropertyName == GET_MEMBER_NAME_CHECKED(UUTInputVisualizer, AxisName))
	{
		ActionName = NAME_None;
	}
}
#endif

void UUTInputVisualizer::OnWidgetRebuilt()
{
	const bool bIsDesignTime = IsDesignTime();
	if (AxisName != NAME_None && !bIsDesignTime)
	{
		ShowInputAxis(AxisName);
	}
	else if (ActionName != NAME_None && !bIsDesignTime)
	{
		ShowInputAction(ActionName);
	}
	else
	{
		ShowSpecificKey(SpecificKey);
	}

	ListenForUsingGamepadChangedIfNeeded();
}

void UUTInputVisualizer::ListenForUsingGamepadChangedIfNeeded()
{
	if (!IsDesignTime() && (ActionName != NAME_None || SpecificKey.IsValid() || AxisName != NAME_None))
	{
		/*
		UUTGlobals& Globals = UUTGlobals::Get();
		if (!Globals.OnUsingGamepadChanged().IsBoundToObject(this))
		{
			Globals.OnUsingGamepadChanged().AddUObject(this, &UUTInputVisualizer::HandleUsingGamepadChanged);
		}
		*/
		/*
		UWorld* World = GetWorld();
		if (UPlayerContext* PlayCon = World ? UPlayerContext::GetCurrent(World) : nullptr)
		{
			PlayCon->OnClientGameplaySettingsChanged.RemoveAll(this);
			PlayCon->OnClientGameplaySettingsChanged.AddDynamic(this, &ThisClass::HandleControlsChanged);
		}*/
	}
}

void UUTInputVisualizer::HandleControlsChanged()
{
	HandleUsingGamepadChanged(UUTGlobals::Get().GetIsUsingGamepad());
}

void UUTInputVisualizer::HandleUsingGamepadChanged(bool bUsingGamepad)
{
	if (AxisName != NAME_None)
	{
		ShowInputAxis(AxisName);
	}
	else if (ActionName != NAME_None)
	{
		// Refresh the action to match the current input mode
		ShowInputAction(ActionName);
	}
	else
	{
		// Toggle whether we're showing the specific key based on the active input method
		bool bShowKey = SpecificKey.IsValid() && SpecificKey.IsGamepadKey() == bUsingGamepad;
#if WITH_EDITOR
		bShowKey |= IsDesignTime();
#endif 
		SetVisibility(bShowKey ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}