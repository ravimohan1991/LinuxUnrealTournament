// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"

#include "UMG.h"
#include "UTBaseButton.h"
#include "UTTextBlock.h"

//////////////////////////////////////////////////////////////////////////
// UUTButtonStyle
//////////////////////////////////////////////////////////////////////////

void UUTButtonStyle::GetButtonPadding(EUTWidgetStyleSize::Type Size, FMargin& OutButtonPadding) const
{
	OutButtonPadding = ButtonPadding[Size];
}

void UUTButtonStyle::GetCustomPadding(EUTWidgetStyleSize::Type Size, FMargin& OutCustomPadding) const
{
	OutCustomPadding = CustomPadding[Size];
}

UUTTextStyle* UUTButtonStyle::GetNormalTextStyle() const
{
	if (NormalTextStyle)
	{
		if (UUTTextStyle* TextStyle = Cast<UUTTextStyle>(NormalTextStyle->ClassDefaultObject))
		{
			return TextStyle;
		}
	}
	return nullptr;
}

UUTTextStyle* UUTButtonStyle::GetSelectedTextStyle() const
{
	if (SelectedTextStyle)
	{
		if (UUTTextStyle* TextStyle = Cast<UUTTextStyle>(SelectedTextStyle->ClassDefaultObject))
		{
			return TextStyle;
		}
	}
	return nullptr;
}

UUTTextStyle* UUTButtonStyle::GetDisabledTextStyle() const
{
	if (DisabledTextStyle)
	{
		if (UUTTextStyle* TextStyle = Cast<UUTTextStyle>(DisabledTextStyle->ClassDefaultObject))
		{
			return TextStyle;
		}
	}
	return nullptr;
}

void UUTButtonStyle::GetNormalBaseBrush(EUTWidgetStyleSize::Type Size, FSlateBrush& Brush) const
{
	Brush = NormalBase[Size];
}

void UUTButtonStyle::GetNormalHoveredBrush(EUTWidgetStyleSize::Type Size, FSlateBrush& Brush) const
{
	Brush = NormalHovered[Size];
}

void UUTButtonStyle::GetNormalPressedBrush(EUTWidgetStyleSize::Type Size, FSlateBrush& Brush) const
{
	Brush = NormalPressed[Size];
}

void UUTButtonStyle::GetSelectedBaseBrush(EUTWidgetStyleSize::Type Size, FSlateBrush& Brush) const
{
	Brush = SelectedBase[Size];
}

void UUTButtonStyle::GetSelectedHoveredBrush(EUTWidgetStyleSize::Type Size, FSlateBrush& Brush) const
{
	Brush = SelectedHovered[Size];
}

void UUTButtonStyle::GetSelectedPressedBrush(EUTWidgetStyleSize::Type Size, FSlateBrush& Brush) const
{
	Brush = SelectedPressed[Size];
}

void UUTButtonStyle::GetDisabledBrush(EUTWidgetStyleSize::Type Size, FSlateBrush& Brush) const
{
	Brush = Disabled[Size];
}

//////////////////////////////////////////////////////////////////////////
// SUTButtonForUMG
//////////////////////////////////////////////////////////////////////////
/** 
 * Lets us disable clicking on a button without disabling hit-testing
 * Needed because NativeOnMouseEnter is not received by disabled widgets, 
 * but that also disables our anchored tooltips.
 */
class SUTButtonForUMG : public SButton
{
public:
	SLATE_BEGIN_ARGS(SUTButtonForUMG)
		: _Content()
		, _HAlign(HAlign_Fill)
		, _VAlign(VAlign_Fill)
		, _ClickMethod(EButtonClickMethod::DownAndUp)
		, _TouchMethod(EButtonTouchMethod::DownAndUp)
		, _PressMethod(EButtonPressMethod::DownAndUp)
		, _IsFocusable(true)
		, _IsInteractionEnabled(true)
	{}
		SLATE_DEFAULT_SLOT(FArguments, Content)
		SLATE_STYLE_ARGUMENT(FButtonStyle, ButtonStyle)
		SLATE_ARGUMENT(EHorizontalAlignment, HAlign)
		SLATE_ARGUMENT(EVerticalAlignment, VAlign)
		SLATE_EVENT(FOnClicked, OnClicked)
		SLATE_EVENT(FSimpleDelegate, OnPressed)
		SLATE_EVENT(FSimpleDelegate, OnReleased)
		SLATE_ARGUMENT(EButtonClickMethod::Type, ClickMethod)
		SLATE_ARGUMENT(EButtonTouchMethod::Type, TouchMethod)
		SLATE_ARGUMENT(EButtonPressMethod::Type, PressMethod)
		SLATE_ARGUMENT(bool, IsFocusable)

		/** Is interaction enabled? */
		SLATE_ARGUMENT(bool, IsInteractionEnabled)
		SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		SButton::Construct(SButton::FArguments()
			.ButtonStyle(InArgs._ButtonStyle)
			.HAlign(InArgs._HAlign)
			.VAlign(InArgs._VAlign)
			.ClickMethod(InArgs._ClickMethod)
			.TouchMethod(InArgs._TouchMethod)
			.PressMethod(InArgs._PressMethod)
			.OnClicked(InArgs._OnClicked)
			.OnPressed(InArgs._OnPressed)
			.OnReleased(InArgs._OnReleased)
			.IsFocusable(InArgs._IsFocusable)
			.Content()
			[
				InArgs._Content.Widget
			]);

		bIsInteractionEnabled = InArgs._IsInteractionEnabled;
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return bIsInteractionEnabled ? SButton::OnMouseButtonDown(MyGeometry, MouseEvent) : FReply::Handled();
	}
	
	virtual FReply OnMouseButtonDoubleClick(const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent) override
	{
		return OnMouseButtonDown(InMyGeometry, InMouseEvent);
	}

	virtual FReply OnMouseButtonUp(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		FReply Reply = FReply::Handled();
		if (!bIsInteractionEnabled)
		{
			if (HasMouseCapture())
			{
				// It's conceivable that interaction was disabled while this button had mouse capture
				// If that's the case, we want to release it (without acknowledging the click)
				Release();
				Reply.ReleaseMouseCapture();
			}
		}
		else
		{
			Reply = SButton::OnMouseButtonUp(MyGeometry, MouseEvent);
		}
		
		return Reply;
	}
	
	virtual bool IsHovered() const override
	{
		return bIsInteractionEnabled ? SButton::IsHovered() : false;
	}

	virtual bool IsPressed() const override
	{
		return bIsInteractionEnabled ? SButton::IsPressed() : false;
	}

	void SetIsInteractionEnabled(bool bInIsInteractionEnabled)
	{
		bIsInteractionEnabled = bInIsInteractionEnabled;
	}

private:

	/** True if clicking is disabled, to allow for things like double click */
	bool bIsInteractionEnabled;
};


//////////////////////////////////////////////////////////////////////////
// UUTButton
//////////////////////////////////////////////////////////////////////////

UUTButtonInternal::UUTButtonInternal(const FObjectInitializer& Initializer)
	: Super(Initializer)
	, bInteractionEnabled(true)
{}

void UUTButtonInternal::SetInteractionEnabled(bool bInIsInteractionEnabled)
{
	bInteractionEnabled = bInIsInteractionEnabled;
	if (MyUTButton.IsValid())
	{
		MyUTButton->SetIsInteractionEnabled(bInIsInteractionEnabled);
	}
}

bool UUTButtonInternal::IsHovered() const
{
	if (MyUTButton.IsValid())
	{
		return MyUTButton->IsHovered();
	}
	return false;
}

bool UUTButtonInternal::IsPressed() const
{
	if (MyUTButton.IsValid())
	{
		return MyUTButton->IsPressed();
	}
	return false;
}

void UUTButtonInternal::SetMinDesiredHeight(int32 InMinHeight)
{
	MinHeight = InMinHeight;
	if (MyBox.IsValid())
	{
		MyBox->SetMinDesiredHeight(InMinHeight);
	}
}

void UUTButtonInternal::SetMinDesiredWidth(int32 InMinWidth)
{
	MinWidth = InMinWidth;
	if (MyBox.IsValid())
	{
		MyBox->SetMinDesiredWidth(InMinWidth);
	}
}

TSharedRef<SWidget> UUTButtonInternal::RebuildWidget()
{
	MyButton = MyUTButton = SNew(SUTButtonForUMG)
		.OnClicked(BIND_UOBJECT_DELEGATE(FOnClicked, SlateHandleClicked))
		.OnPressed(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandlePressed))
		.OnReleased(BIND_UOBJECT_DELEGATE(FSimpleDelegate, SlateHandleReleased))
		.ButtonStyle(&WidgetStyle)
		.ClickMethod(ClickMethod)
		.TouchMethod(TouchMethod)
		.IsFocusable(IsFocusable)
		.IsInteractionEnabled(bInteractionEnabled);

	MyBox = SNew(SBox)
		.MinDesiredWidth(MinWidth)
		.MinDesiredHeight(MinHeight)
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			MyUTButton.ToSharedRef()
		];

	if (GetChildrenCount() > 0)
	{
		Cast<UButtonSlot>(GetContentSlot())->BuildSlot(MyUTButton.ToSharedRef());
	}

	return MyBox.ToSharedRef();
}

void UUTButtonInternal::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyUTButton.Reset();
	MyBox.Reset();
}

//////////////////////////////////////////////////////////////////////////
// UUTBaseButton
//////////////////////////////////////////////////////////////////////////

UUTBaseButton::UUTBaseButton(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, MinWidth(0)
	, MinHeight(0)
	, bApplyAlphaOnDisable(true)
	, bSelectable(false)
	, bToggleable(false)
	, bListenForInput(true)
//	, ClickedTrackingLevel(EUTUIAnalyticsTrackingLevel::Normal)
//	, SelectionChangedTrackingLevel(EUTUIAnalyticsTrackingLevel::Verbose)
	, bSelected(false)
	, bInteractionEnabled(true)
{
	//MouseEnteredTrackingLevel = EUTUIAnalyticsTrackingLevel::Verbose;
	//MouseLeftTrackingLevel = EUTUIAnalyticsTrackingLevel::Verbose;
}

void UUTBaseButton::InitializeUTWidget()
{
	UUTButtonInternal* RootButtonRaw = WidgetTree->ConstructWidget<UUTButtonInternal>(UUTButtonInternal::StaticClass(), FName(TEXT("InternalRootButton")));
	RootButtonRaw->ClickMethod = ClickMethod;
	RootButtonRaw->IsFocusable = bSupportsKeyboardFocus_DEPRECATED;
	RootButtonRaw->SetInteractionEnabled(bInteractionEnabled);
	RootButton = RootButtonRaw;

	if (WidgetTree->RootWidget)
	{
		UButtonSlot* ButtonSlot = Cast<UButtonSlot>(RootButtonRaw->AddChild(WidgetTree->RootWidget));
		ButtonSlot->SetPadding(FMargin());
		ButtonSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
		ButtonSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
		WidgetTree->RootWidget = RootButtonRaw;

		RootButton->OnClicked.AddUniqueDynamic(this, &UUTBaseButton::HandleButtonClicked);
	}

	BuildStyles();
}

void UUTBaseButton::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	RefreshDimensions();
	BuildStyles();
}

void UUTBaseButton::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	if (UButton* ButtonPtr = RootButton.Get())
	{
		ButtonPtr->SetIsEnabled(GetIsEnabled());
	}
	Super::NativeTick(MyGeometry, InDeltaTime);
}

void UUTBaseButton::NativeConstruct()
{
	Super::NativeConstruct();

	if (BoundKey.IsValid())
	{
		BindClickToKey(BoundKey);
	}

	BuildStyles();
}

void UUTBaseButton::NativeDestruct()
{
	// todo PLK: when we get a UI manager widget
	/*
	UUTUIManagerWidget* UIManager = GetUIManager();
	if (UIManager && BoundKey.IsValid())
	{
		UIManager->UnregisterInputHandlerWidget(this, BoundKey);
	}
	*/

	Super::NativeDestruct();
}

bool UUTBaseButton::NativeIsInteractable() const
{
	// If it's enabled, it's "interactable" from a UMG perspective. 
	// For now this is how we generate friction on the analog cursor, which we still want for disabled buttons since they have tooltips.
	return GetIsEnabled();
}

void UUTBaseButton::HandleRequestedInput_Implementation(FKey Key, EInputEvent EventType)
{
	// Count bound input as just another way to click
	if (EventType == IE_Pressed && IsInteractionEnabled())
	{
		HandleButtonClicked();
	}
}

void UUTBaseButton::BindClickToKey(const FKey& KeyToBind)
{
	if (!bListenForInput)
	{
		return;
	}

	// todo PLK: when we get a UI manager widget
	/*
	if (UUTUIManagerWidget* UIManager = GetUIManager())
	{
		if (BoundKey.IsValid())
		{
			UIManager->UnregisterInputHandlerWidget(this, BoundKey);
		}

		UIManager->RegisterInputHandlerWidget(this, KeyToBind);
	}
	*/

	BoundKey = KeyToBind;
}

void UUTBaseButton::EnableButton()
{
	if (!bInteractionEnabled)
	{
		bInteractionEnabled = true;

		// If this is a selected and not-toggleable button, don't enable root button interaction
		if (!GetSelected() || bToggleable)
		{
			RootButton->SetInteractionEnabled(true);
		}
		
		if (bApplyAlphaOnDisable)
		{
			FLinearColor ButtonColor = RootButton->ColorAndOpacity;
			ButtonColor.A = 1.f;
			RootButton->SetColorAndOpacity(ButtonColor);
		}

		SetBasicTooltipText(EnabledTooltipText);
		NativeOnEnabled();
	}
}

void UUTBaseButton::DisableButton()
{
	if (bInteractionEnabled)
	{
		EnabledTooltipText = UTTooltipText;

		bInteractionEnabled = false;
		RootButton->SetInteractionEnabled(false);

		if (bApplyAlphaOnDisable)
		{
			FLinearColor ButtonColor = RootButton->ColorAndOpacity;
			ButtonColor.A = 0.5f;
			RootButton->SetColorAndOpacity(ButtonColor);
		}

		NativeOnDisabled();
	}
}

void UUTBaseButton::DisableButtonWithReason(const FText& DisabledReason)
{
	DisableButton();

	DisabledTooltipText = DisabledReason;
	SetBasicTooltipText(DisabledTooltipText);
}

bool UUTBaseButton::IsInteractionEnabled() const
{
	return GetIsEnabled() && bInteractionEnabled;
}

bool UUTBaseButton::IsHovered() const
{
	return RootButton.IsValid() && RootButton->IsHovered();
}

bool UUTBaseButton::IsPressed() const
{
	return RootButton.IsValid() && RootButton->IsPressed();
}

void UUTBaseButton::SetIsSelectable(bool bInIsSelectable)
{
	if (bInIsSelectable != bSelectable)
	{
		bSelectable = bInIsSelectable;

		if (bSelected && !bInIsSelectable)
		{
			SetSelectedInternal(false);
		}
	}
}

void UUTBaseButton::SetIsSelected(bool InSelected, bool bFromClick)
{
	if (bSelectable && bSelected != InSelected)
	{
		if (!InSelected && bToggleable)
		{
			SetSelectedInternal(false);
		}
		else if (InSelected)
		{
			// Only allow a sound if we weren't just clicked
			SetSelectedInternal(true, !bFromClick);
		}
	}
}

void UUTBaseButton::SetSelectedInternal(bool bInSelected, bool bAllowSound /*= true*/, bool bBroadcast /*= true*/)
{
	bool bDidChange = (bInSelected != bSelected);

	bSelected = bInSelected;
	/*
	if (bDidChange)
	{
		FireAnalyticsEvent(SelectionChangedTrackingLevel, TEXT("Button"), TEXT("SelectionChanged"));
	}
	*/
	SetButtonStyle();

	if (bSelected)
	{
		NativeOnSelected(bBroadcast);
		if (!bToggleable)
		{
			// If the button isn't toggleable, then disable interaction with the root button while selected
			// The prevents us getting unnecessary click noises and events
			RootButton->SetInteractionEnabled(false);
		}

		if (bAllowSound)
		{
			// Selection was not triggered by a button click, so play the click sound
			FSlateApplication::Get().PlaySound(NormalStyle.PressedSlateSound);
		}
	}
	else
	{
		// Once deselected, restore the root button interactivity to the desired state
		RootButton->SetInteractionEnabled(bInteractionEnabled);
		
		NativeOnDeselected(bBroadcast);
	}
}

void UUTBaseButton::RefreshDimensions()
{
	RootButton->SetMinDesiredWidth(MinWidth);
	RootButton->SetMinDesiredHeight(MinHeight);
}

void UUTBaseButton::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (IsInteractionEnabled())
	{
		NativeOnHovered();
	}
}

void UUTBaseButton::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	
	if (IsInteractionEnabled())
	{
		NativeOnUnhovered();
	}
}

bool UUTBaseButton::GetSelected() const
{
	return bSelected;
}

void UUTBaseButton::ClearSelection()
{
	SetSelectedInternal( false, false );
}

void UUTBaseButton::SetStyle(TSubclassOf<UUTButtonStyle> InStyle)
{
	if (InStyle && Style != InStyle)
	{
		Style = InStyle;
		BuildStyles();
	}
}

UUTButtonStyle* UUTBaseButton::GetStyle() const
{
	return const_cast<UUTButtonStyle*>(GetStyleCDO());
}

void UUTBaseButton::SetStyleSize(EUTWidgetStyleSize::Type InSize)
{
	if (StyleSize != InSize)
	{
		StyleSize = InSize;
		BuildStyles();
		NativeOnStyleSizeChanged();
	}
}

const UUTButtonStyle* UUTBaseButton::GetStyleCDO() const
{
	if (Style)
	{
		if (const UUTButtonStyle* UTButtonStyle = Cast<UUTButtonStyle>(Style->ClassDefaultObject))
		{
			return UTButtonStyle;
		}
	}
	return nullptr;
}

void UUTBaseButton::GetCurrentButtonPadding(FMargin& OutButtonPadding) const
{
	if (const UUTButtonStyle* UTButtonStyle = GetStyleCDO())
	{
		UTButtonStyle->GetButtonPadding(StyleSize, OutButtonPadding);
	}
}

void UUTBaseButton::GetCurrentCustomPadding(FMargin& OutCustomPadding) const
{
	if (const UUTButtonStyle* UTButtonStyle = GetStyleCDO())
	{
		UTButtonStyle->GetCustomPadding(StyleSize, OutCustomPadding);
	}
}

UUTTextStyle* UUTBaseButton::GetCurrentTextStyle() const
{
	if (const UUTButtonStyle* UTButtonStyle = GetStyleCDO())
	{
		if (GetIsEnabled())
		{
			if (bSelected)
			{
				return UTButtonStyle->GetSelectedTextStyle();
			}
			else
			{
				return UTButtonStyle->GetNormalTextStyle();
			}
		}
		else
		{
			return UTButtonStyle->GetDisabledTextStyle();
		}
	}
	return nullptr;
}

TSubclassOf<UUTTextStyle> UUTBaseButton::GetCurrentTextStyleClass() const
{
	if (UUTTextStyle* CurrentTextStyle = GetCurrentTextStyle())
	{
		return CurrentTextStyle->GetClass();
	}
	return nullptr;
}

void UUTBaseButton::SetMinDimensions(int32 InMinWidth, int32 InMinHeight)
{
	MinWidth = InMinWidth;
	MinHeight = InMinHeight;

	RefreshDimensions();
}

void UUTBaseButton::HandleButtonClicked()
{
	SetIsSelected(!bSelected, true);

	NativeOnClicked();
}

void UUTBaseButton::NativeOnSelected(bool bBroadcast)
{
	BP_OnSelected();

	if (bBroadcast)
	{
		OnSelectedChanged().Broadcast(true);
		if (OnButtonSelectedChanged.IsBound())
		{
			OnButtonSelectedChanged.Broadcast(this, true);
		}
	}
}

void UUTBaseButton::NativeOnDeselected(bool bBroadcast)
{
	BP_OnDeselected();

	if (bBroadcast)
	{
		OnSelectedChanged().Broadcast(false);
		if (OnButtonSelectedChanged.IsBound())
		{
			OnButtonSelectedChanged.Broadcast(this, false);
		}
	}
}

void UUTBaseButton::NativeOnHovered()
{
	BP_OnHovered();

	OnHovered().Broadcast();
	if (OnButtonHovered.IsBound())
	{
		OnButtonHovered.Broadcast(this);
	}
}

void UUTBaseButton::NativeOnUnhovered()
{
	BP_OnUnhovered();

	OnUnhovered().Broadcast();
	if (OnButtonUnhovered.IsBound())
	{
		OnButtonUnhovered.Broadcast(this);
	}
}

void UUTBaseButton::NativeOnClicked()
{
//	FireAnalyticsEvent(ClickedTrackingLevel, TEXT("Button"), TEXT("Clicked"));

	BP_OnClicked();

	OnClicked().Broadcast();
	if (OnButtonClicked.IsBound())
	{
		OnButtonClicked.Broadcast(this);
	}
}

void UUTBaseButton::NativeOnEnabled()
{
	BP_OnEnabled();
}

void UUTBaseButton::NativeOnDisabled()
{
	BP_OnDisabled();
}

void UUTBaseButton::NativeOnStyleSizeChanged()
{
	OnStyleSizeChanged();
}

void UUTBaseButton::BuildStyles()
{
	if (const UUTButtonStyle* UTButtonStyle = GetStyleCDO())
	{
		const FMargin& ButtonPadding = UTButtonStyle->ButtonPadding[StyleSize];
		const FSlateBrush& DisabledBrush = UTButtonStyle->Disabled[StyleSize];

		NormalStyle.Normal = UTButtonStyle->NormalBase[StyleSize];
		NormalStyle.Hovered = UTButtonStyle->NormalHovered[StyleSize];
		NormalStyle.Pressed = UTButtonStyle->NormalPressed[StyleSize];
		NormalStyle.Disabled = DisabledBrush;
		NormalStyle.NormalPadding = ButtonPadding;
		NormalStyle.PressedPadding = ButtonPadding;
		NormalStyle.PressedSlateSound = PressedSlateSoundOverride.GetResourceObject() ? PressedSlateSoundOverride : UTButtonStyle->PressedSlateSound;
		NormalStyle.HoveredSlateSound = HoveredSlateSoundOverride.GetResourceObject() ? HoveredSlateSoundOverride : UTButtonStyle->HoveredSlateSound;

		SelectedStyle.Normal = UTButtonStyle->SelectedBase[StyleSize];
		SelectedStyle.Hovered = UTButtonStyle->SelectedHovered[StyleSize];
		SelectedStyle.Pressed = UTButtonStyle->SelectedPressed[StyleSize];
		SelectedStyle.Disabled = DisabledBrush;
		SelectedStyle.NormalPadding = ButtonPadding;
		SelectedStyle.PressedPadding = ButtonPadding;
		SelectedStyle.PressedSlateSound = NormalStyle.PressedSlateSound;
		SelectedStyle.HoveredSlateSound = NormalStyle.HoveredSlateSound;

		SetButtonStyle();
	}
}

void UUTBaseButton::SetButtonStyle()
{
	if (UButton* ButtonPtr = RootButton.Get())
	{
		ButtonPtr->WidgetStyle = bSelected ? SelectedStyle : NormalStyle;
	}
}
/*
void UUTBaseButton::GetAnalyticsParameters(TArray<FAnalyticsEventAttribute>& ParamArray)
{
	Super::GetAnalyticsParameters(ParamArray);

	if (bSelectable)
	{
		ParamArray.Add(FAnalyticsEventAttribute(TEXT("Selected"), bSelected));
	}
}
*/