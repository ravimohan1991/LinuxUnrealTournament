// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTWidgetData.h"
#include "UTBasicTooltipWidget.h"
#include "ITooltipWidget.h"
#include "UTUserWidgetBase.h"


UUTUserWidgetBase::UUTUserWidgetBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bConsumePointerInput(false)
	, bEnableUTTooltip(true)
	, TooltipAnchorPlacement(EMenuPlacement::MenuPlacement_CenteredBelowAnchor)
	, WidgetId(NAME_None)
	, AttachedMessagePlacement(EMenuPlacement::MenuPlacement_MenuRight)
	, CustomTooltipWidget(nullptr)
	, bAutoOpenTooltip(true)
	, BasicTooltipWidget(nullptr)
{
	// The vast majority of UT widgets should not be focusable and this solves a bunch of issues
	bSupportsKeyboardFocus_DEPRECATED = false;
	bIsFocusable = false;

	Visibility = ESlateVisibility::SelfHitTestInvisible;
    //Visiblity_DEPRECATED = ESlateVisibility::SelfHitTestInvisible;
}

#if WITH_EDITOR
void UUTUserWidgetBase::PreEditChange(UProperty* PropertyAboutToChange)
{
	// Prevent a crash in the editor due to PreConstruct
	if (HasAnyFlags(RF_ClassDefaultObject) && !IsPendingKill())
	{
		Super::PreEditChange(PropertyAboutToChange);
	}
}
#endif

bool UUTUserWidgetBase::Initialize()
{
	const bool bInitializedThisCall = Super::Initialize();

	if (bInitializedThisCall)
	{
		InitializeUTWidget();

		if (bEnableUTTooltip)
		{
			// Wrap this widget inside a menu anchor that we'll use for tooltips
			UMenuAnchor* RootTooltipAnchorRaw = WidgetTree->ConstructWidget<UMenuAnchor>(UMenuAnchor::StaticClass(), FName(TEXT("InternalMenuAnchor")));
			RootTooltipAnchorRaw->Placement = TooltipAnchorPlacement;
			RootTooltipAnchorRaw->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			RootTooltipAnchorRaw->UseApplicationMenuStack = false;
			RootTooltipAnchor = RootTooltipAnchorRaw;

			if (WidgetTree->RootWidget)
			{
				RootTooltipAnchorRaw->AddChild(WidgetTree->RootWidget);
				WidgetTree->RootWidget = RootTooltipAnchorRaw;

				RootTooltipAnchor->OnGetMenuContentEvent.BindDynamic(this, &UUTUserWidgetBase::DynamicGetTooltipContent);
			}
		}
	}

	return bInitializedThisCall;
}

TSharedRef<SWidget> UUTUserWidgetBase::RebuildWidget()
{
	TSharedRef<SWidget> Content = Super::RebuildWidget();

	// todo PLK - hook this up when we get a UI manager
	/*
	// If this widget is marked for use in a tutorial, wrap it
	if (!IsDesignTime() && WidgetId != NAME_None)
	{
		return SNew(SUTTaggedContainer, GetUIManager())
			.WidgetId(WidgetId)
			.MessagePlacement(AttachedMessagePlacement)
			[
				Content
			];
	}
	*/

	return Content;
}

void UUTUserWidgetBase::OnWidgetRebuilt()
{
	// this is expected to call construct in non-design cases
	Super::OnWidgetRebuilt();

	if (!IsDesignTime() && !UTTooltipText.IsEmpty())
	{
		InitBasicTooltip();
	}
}

void UUTUserWidgetBase::SetUTTooltip(UWidget* InTooltipWidget)
{
	if (bEnableUTTooltip)
	{
		HideTooltip();

		CustomTooltipWidget = InTooltipWidget;
		if (CustomTooltipWidget)
		{
			// Tooltips should never be be hit testable and nobody should be setting a collapsed tooltip
			CustomTooltipWidget->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}
}

void UUTUserWidgetBase::SetBasicTooltipInfo(FText NewTooltipTitle, FText NewTooltipText)
{
	UTTooltipTitleText = NewTooltipTitle;
	UTTooltipText = NewTooltipText;
	UpdateBasicTooltipInternal();
}

void UUTUserWidgetBase::SetBasicTooltipTitle(const FText& NewTooltipTitle)
{
	UTTooltipTitleText = NewTooltipTitle;
	UpdateBasicTooltipInternal();
}

void UUTUserWidgetBase::SetBasicTooltipText(const FText& NewTooltipText)
{
	UTTooltipText = NewTooltipText;
	UpdateBasicTooltipInternal();
}

void UUTUserWidgetBase::InitBasicTooltip()
{
	if (!BasicTooltipWidget && bEnableUTTooltip && GetOwningPlayer())
	{
		// Create the basic tooltip
		if (TSubclassOf<UUTBasicTooltipWidget> DefaultTooltipClass = UUTWidgetData::FindWidget<UUTBasicTooltipWidget>(TEXT("DefaultTooltip")))
		{
			UUTBasicTooltipWidget* NewBasicTooltip = CreateWidget<UUTBasicTooltipWidget>(GetWorld()->GetGameInstance(), DefaultTooltipClass);
			BasicTooltipWidget = NewBasicTooltip;

			BasicTooltipWidget->UpdateTitleText(UTTooltipTitleText);
			BasicTooltipWidget->UpdateTooltipText(UTTooltipText);
		}
	}
}

void UUTUserWidgetBase::UpdateBasicTooltipInternal()
{
	if (bEnableUTTooltip)
	{
		if (BasicTooltipWidget)
		{
			BasicTooltipWidget->UpdateTitleText(UTTooltipTitleText);
			BasicTooltipWidget->UpdateTooltipText(UTTooltipText);
		}
		else if (!UTTooltipText.IsEmpty())
		{
			InitBasicTooltip();
		}
	}
}

bool UUTUserWidgetBase::NativeIsInteractable() const
{
	if (BasicTooltipWidget || CustomTooltipWidget)
	{
		// If we've got a tooltip, we want virtual cursor friction
		return true;
	}

	return Super::IsInteractable();
}

FReply UUTUserWidgetBase::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Mild hack here to make sure we don't eat back actions
	FReply DefaultResult = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

	return bConsumePointerInput && InMouseEvent.GetEffectingButton() != EKeys::ThumbMouseButton ? FReply::Handled() : DefaultResult;
}

FReply UUTUserWidgetBase::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply DefaultResult = Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
	return bConsumePointerInput ? FReply::Handled() : DefaultResult;
}

FReply UUTUserWidgetBase::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply DefaultResult = Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
	return bConsumePointerInput ? FReply::Handled() : DefaultResult;
}

FReply UUTUserWidgetBase::NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	FReply DefaultResult = Super::NativeOnMouseButtonDoubleClick(InGeometry, InMouseEvent);
	return bConsumePointerInput ? FReply::Handled() : DefaultResult;
}

FReply UUTUserWidgetBase::NativeOnTouchGesture(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	FReply DefaultResult = Super::NativeOnTouchGesture(InGeometry, InGestureEvent);
	return bConsumePointerInput ? FReply::Handled() : DefaultResult;
}

FReply UUTUserWidgetBase::NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	FReply DefaultResult = Super::NativeOnTouchStarted(InGeometry, InGestureEvent);
	return bConsumePointerInput ? FReply::Handled() : DefaultResult;
}

FReply UUTUserWidgetBase::NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	FReply DefaultResult = Super::NativeOnTouchMoved(InGeometry, InGestureEvent);
	return bConsumePointerInput ? FReply::Handled() : DefaultResult;
}

FReply UUTUserWidgetBase::NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent)
{
	FReply DefaultResult = Super::NativeOnTouchEnded(InGeometry, InGestureEvent);
	return bConsumePointerInput ? FReply::Handled() : DefaultResult;
}

UWidget* UUTUserWidgetBase::DynamicGetTooltipContent()
{
	return GetCurrentTooltip();
}

UWidget* UUTUserWidgetBase::GetCurrentTooltip() const
{
	return CustomTooltipWidget ? CustomTooltipWidget : !UTTooltipText.IsEmpty() ? BasicTooltipWidget : nullptr;
}

void UUTUserWidgetBase::HideTooltip()
{
	if (RootTooltipAnchor.IsValid() && RootTooltipAnchor->IsOpen())
	{
		RootTooltipAnchor->Close();

		UWidget* CurrentTooltip = GetCurrentTooltip();
		if (CurrentTooltip && CurrentTooltip->Implements<UTooltipWidget>())
		{
			ITooltipWidget::Execute_Hide(CurrentTooltip);
		}
		else if (UUTTooltipBase* TooltipBase = Cast<UUTTooltipBase>(CurrentTooltip))
		{
			TooltipBase->Hide();
		}
	}
}

void UUTUserWidgetBase::ShowTooltip()
{
	UWidget* CurrentTooltip = GetCurrentTooltip();
	if (CurrentTooltip && RootTooltipAnchor.IsValid())
	{
		RootTooltipAnchor->Open(false);

		if (CurrentTooltip->Implements<UTooltipWidget>())
		{
			ITooltipWidget::Execute_Show(CurrentTooltip);
		}
		else if (UUTTooltipBase* TooltipBase = Cast<UUTTooltipBase>(CurrentTooltip))
		{
			TooltipBase->Show();
		}
	}
}

UUTLocalPlayer* UUTUserWidgetBase::GetUTLocalPlayer() const
{
	return Cast<UUTLocalPlayer>(GetOwningLocalPlayer());
}
