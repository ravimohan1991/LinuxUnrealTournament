// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTUITypes.h"
#include "UTUserWidgetBase.h"
#include "IUTInputHandlerWidget.h"

#include "UTBaseButton.generated.h"

class UUTTextStyle;

/* ---- All properties must be EditDefaultsOnly, BlueprintReadOnly !!! -----
 *       we return the CDO to blueprints, so we cannot allow any changes (blueprint doesn't support const variables)
 */
UCLASS(Abstract, Blueprintable, ClassGroup = UI, meta = (Category = "UT UI"))
class UNREALTOURNAMENT_API UUTButtonStyle : public UObject
{
	GENERATED_BODY()

public:
	/** The normal (un-selected) brush to apply to each size of this button */
	UPROPERTY(EditDefaultsOnly, Category = "Normal")
	FSlateBrush NormalBase[EUTWidgetStyleSize::MAX];

	/** The normal (un-selected) brush to apply to each size of this button when hovered */
	UPROPERTY(EditDefaultsOnly, Category = "Normal")
	FSlateBrush NormalHovered[EUTWidgetStyleSize::MAX];

	/** The normal (un-selected) brush to apply to each size of this button when pressed */
	UPROPERTY(EditDefaultsOnly, Category = "Normal")
	FSlateBrush NormalPressed[EUTWidgetStyleSize::MAX];

	/** The selected brush to apply to each size of this button */
	UPROPERTY(EditDefaultsOnly, Category = "Selected")
	FSlateBrush SelectedBase[EUTWidgetStyleSize::MAX];

	/** The selected brush to apply to each size of this button when hovered */
	UPROPERTY(EditDefaultsOnly, Category = "Selected")
	FSlateBrush SelectedHovered[EUTWidgetStyleSize::MAX];

	/** The selected brush to apply to each size of this button when pressed */
	UPROPERTY(EditDefaultsOnly, Category = "Selected")
	FSlateBrush SelectedPressed[EUTWidgetStyleSize::MAX];

	/** The disabled brush to apply to each size of this button */
	UPROPERTY(EditDefaultsOnly, Category = "Disabled")
	FSlateBrush Disabled[EUTWidgetStyleSize::MAX];

	/** The button content padding to apply for each size */
	UPROPERTY(EditDefaultsOnly, Category = "Properties")
	FMargin ButtonPadding[EUTWidgetStyleSize::MAX];
	
	/** The custom padding of the button to use for each size */
	UPROPERTY(EditDefaultsOnly, Category = "Properties")
	FMargin CustomPadding[EUTWidgetStyleSize::MAX];

	/** The text style to use when un-selected */
	UPROPERTY(EditDefaultsOnly, Category = "Properties")
	TSubclassOf<UUTTextStyle> NormalTextStyle;

	/** The text style to use when selected */
	UPROPERTY(EditDefaultsOnly, Category = "Properties")
	TSubclassOf<UUTTextStyle> SelectedTextStyle;

	/** The text style to use when disabled */
	UPROPERTY(EditDefaultsOnly, Category = "Properties")
	TSubclassOf<UUTTextStyle> DisabledTextStyle;

	/** The sound to play when the button is pressed */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Properties", meta = (DisplayName = "Pressed Sound"))
	FSlateSound PressedSlateSound;

	/** The sound to play when the button is hovered */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Properties", meta = (DisplayName = "Hovered Sound"))
	FSlateSound HoveredSlateSound;

	UFUNCTION(BlueprintCallable, Category = "UT ButtonStyle|Getters")
	void GetButtonPadding(EUTWidgetStyleSize::Type Size, FMargin& OutButtonPadding) const;

	UFUNCTION(BlueprintCallable, Category = "UT ButtonStyle|Getters")
	void GetCustomPadding(EUTWidgetStyleSize::Type Size, FMargin& OutCustomPadding) const;

	UFUNCTION(BlueprintCallable, Category = "UT ButtonStyle|Getters")
	UUTTextStyle* GetNormalTextStyle() const;

	UFUNCTION(BlueprintCallable, Category = "UT ButtonStyle|Getters")
	UUTTextStyle* GetSelectedTextStyle() const;

	UFUNCTION(BlueprintCallable, Category = "UT ButtonStyle|Getters")
	UUTTextStyle* GetDisabledTextStyle() const;

	UFUNCTION(BlueprintCallable, Category = "UT ButtonStyle|Getters")
	void GetNormalBaseBrush(EUTWidgetStyleSize::Type Size, FSlateBrush& Brush) const;

	UFUNCTION(BlueprintCallable, Category = "UT ButtonStyle|Getters")
	void GetNormalHoveredBrush(EUTWidgetStyleSize::Type Size, FSlateBrush& Brush) const;

	UFUNCTION(BlueprintCallable, Category = "UT ButtonStyle|Getters")
	void GetNormalPressedBrush(EUTWidgetStyleSize::Type Size, FSlateBrush& Brush) const;

	UFUNCTION(BlueprintCallable, Category = "UT ButtonStyle|Getters")
	void GetSelectedBaseBrush(EUTWidgetStyleSize::Type Size, FSlateBrush& Brush) const;

	UFUNCTION(BlueprintCallable, Category = "UT ButtonStyle|Getters")
	void GetSelectedHoveredBrush(EUTWidgetStyleSize::Type Size, FSlateBrush& Brush) const;

	UFUNCTION(BlueprintCallable, Category = "UT ButtonStyle|Getters")
	void GetSelectedPressedBrush(EUTWidgetStyleSize::Type Size, FSlateBrush& Brush) const;

	UFUNCTION(BlueprintCallable, Category = "UT ButtonStyle|Getters")
	void GetDisabledBrush(EUTWidgetStyleSize::Type Size, FSlateBrush& Brush) const;
};

/** Custom UButton override that allows us to disable clicking without disabling the widget entirely */
UCLASS(Experimental)	// "Experimental" to hide it in the designer
class UNREALTOURNAMENT_API UUTButtonInternal : public UButton
{
	GENERATED_UCLASS_BODY()

public:
	void SetInteractionEnabled(bool bInIsInteractionEnabled);
    bool IsHovered() const;
	bool IsPressed() const;

	void SetMinDesiredHeight(int32 InMinHeight);
	void SetMinDesiredWidth(int32 InMinWidth);

protected:
	/** The minimum width of the button */
	UPROPERTY()
	int32 MinWidth;

	/** The minimum height of the button */
	UPROPERTY()
	int32 MinHeight;

	/** If true, this button can be interacted with it normally. Otherwise, it will not react to being hovered or clicked. */
	UPROPERTY()
	bool bInteractionEnabled;
	
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	/** Cached pointer to the underlying slate button owned by this UWidget */
	TSharedPtr<SBox> MyBox;

	/** Cached pointer to the underlying slate button owned by this UWidget */
	TSharedPtr<class SUTButtonForUMG> MyUTButton;
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FUTSelectedStateChanged, class UUTBaseButton*, Button, bool, Selected);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FUTBaseButtonClicked, class UUTBaseButton*, Button);

UCLASS(Abstract, Blueprintable, ClassGroup = UI, meta = (Category = "UT UI"))
class UNREALTOURNAMENT_API UUTBaseButton : public UUTUserWidgetBase, public IUTInputHandlerWidget
{
	GENERATED_UCLASS_BODY()

public:
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual bool NativeIsInteractable() const override;
	
    virtual void HandleRequestedInput_Implementation(FKey Key, EInputEvent EventType);

	/** Associates this button at its priority with the given key */
	virtual void BindClickToKey(const FKey& KeyToBind);

	/** Enables this button (use instead of SetIsEnabled) */
	UFUNCTION(BlueprintCallable, Category = "UT Button|Setters")
	void EnableButton();

	/** Disables this button (use instead of SetIsEnabled) */
	UFUNCTION(BlueprintCallable, Category = "UT Button|Setters")
	void DisableButton();
	
	/** Disables this button with a reason (use instead of SetIsEnabled) */
	UFUNCTION(BlueprintCallable, Category = "UT Button|Setters")
	void DisableButtonWithReason(const FText& DisabledReason);

	/** Is this button currently interactable? (use instead of GetIsEnabled) */
	UFUNCTION(BlueprintCallable, Category = "UT Button|Getters")
	bool IsInteractionEnabled() const;

	/** Is this button currently hovered? */
    //UFUNCTION(BlueprintCallable, Category = "UT Button|Getters")
    virtual bool IsHovered() const override;

	/** Is this button currently pressed? */
	UFUNCTION(BlueprintCallable, Category = "UT Button|Getters")
	bool IsPressed() const;

	/** Change whether this widget is selectable at all. If false and currently selected, will deselect. */
	UFUNCTION(BlueprintCallable, Category = "UT Button|Setters")
	void SetIsSelectable(bool bInIsSelectable);

	/** Change the selected state manually */
	UFUNCTION(BlueprintCallable, Category = "UT Button|Setters")
	void SetIsSelected(bool InSelected, bool bFromClick = false);

	/** @returns True if the button is currently in a selected state, False otherwise */
	UFUNCTION(BlueprintCallable, Category = "UT Button|Getters")
	bool GetSelected() const;

	UFUNCTION( BlueprintCallable, Category = "UT Button" )
	void ClearSelection();

	/** Sets the style of this button, rebuilds the internal styling */
	UFUNCTION(BlueprintCallable, Category = "UT Button|Setters")
	void SetStyle(TSubclassOf<UUTButtonStyle> InStyle = nullptr);

	/** @Returns Current button style*/
	UFUNCTION(BlueprintCallable, Category = "UT Button|Getters")
	UUTButtonStyle* GetStyle() const;

	/** Sets the style size and rebuilds the internally managed styles */
	UFUNCTION(BlueprintCallable, Category = "UT Button|Setters")
	void SetStyleSize(EUTWidgetStyleSize::Type InSize);

	/** @return The current button padding that corresponds to the current size and selection state */
	UFUNCTION(BlueprintCallable, Category = "UT Button|Getters")
	void GetCurrentButtonPadding(FMargin& OutButtonPadding) const;

	/** @return The custom padding that corresponds to the current size and selection state */
	UFUNCTION(BlueprintCallable, Category = "UT Button|Getters")
	void GetCurrentCustomPadding(FMargin& OutCustomPadding) const;
	
	/** @return The text style that corresponds to the current size and selection state */
	UFUNCTION(BlueprintCallable, Category = "UT Button|Getters")
	UUTTextStyle* GetCurrentTextStyle() const;

	/** @return The class of the text style that corresponds to the current size and selection state */
	UFUNCTION(BlueprintCallable, Category = "UT Button|Getters")
	TSubclassOf<UUTTextStyle> GetCurrentTextStyleClass() const;

	/** Sets the minimum dimensions of this button */
	UFUNCTION(BlueprintCallable, Category = "UT Button|Setters")
	void SetMinDimensions(int32 InMinWidth, int32 InMinHeight);

	DECLARE_EVENT(UUTBaseButton, FOnBaseButtonEvent);
	FOnBaseButtonEvent& OnClicked() { return OnClickedEvent; };
	FOnBaseButtonEvent& OnHovered() { return OnHoveredEvent; };
	FOnBaseButtonEvent& OnUnhovered() { return OnUnhoveredEvent; };

	DECLARE_EVENT_OneParam(UUTBaseButton, FOnBaseButtonSelectedChanged, bool);
	FOnBaseButtonSelectedChanged& OnSelectedChanged() { return OnSelectedChangedEvent; }

protected:
	virtual void InitializeUTWidget() override;
	virtual void SynchronizeProperties() override;
	//virtual void GetAnalyticsParameters(TArray<FAnalyticsEventAttribute>& ParamArray) override;

	/** Handler function registered to the underlying buttons click. */
	UFUNCTION()
	void HandleButtonClicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "UT Button", meta = (DisplayName = "On Selected"))
	void BP_OnSelected();
	virtual void NativeOnSelected(bool bBroadcast);

	UFUNCTION(BlueprintImplementableEvent, Category = "UT Button", meta = (DisplayName = "On Deselected"))
	void BP_OnDeselected();
	virtual void NativeOnDeselected(bool bBroadcast);

	UFUNCTION(BlueprintImplementableEvent, Category = "UT Button", meta = (DisplayName = "On Hovered"))
	void BP_OnHovered();
	virtual void NativeOnHovered();

	UFUNCTION(BlueprintImplementableEvent, Category = "UT Button", meta = (DisplayName = "On Unhovered"))
	void BP_OnUnhovered();
	virtual void NativeOnUnhovered();

	UFUNCTION(BlueprintImplementableEvent, Category = "UT Button", meta = (DisplayName = "On Clicked"))
	void BP_OnClicked();
	virtual void NativeOnClicked();

	UFUNCTION(BlueprintImplementableEvent, Category = "UT Button", meta = (DisplayName = "On Enabled"))
	void BP_OnEnabled();
	virtual void NativeOnEnabled();

	UFUNCTION(BlueprintImplementableEvent, Category = "UT Button", meta = (DisplayName = "On Disabled"))
	void BP_OnDisabled();
	virtual void NativeOnDisabled();

	/** Allows derived classes to take action when the style size has been changed */
	UFUNCTION(BlueprintImplementableEvent, meta=(BlueprintProtected="true"), Category = "UT Button")
	void OnStyleSizeChanged();
	virtual void NativeOnStyleSizeChanged();

	/** Internal method to allow the selected state to be set regardless of selectability or toggleability */
	UFUNCTION(BlueprintCallable, meta=(BlueprintProtected="true"), Category = "UT Button")
	void SetSelectedInternal(bool bInSelected, bool bAllowSound = true, bool bBroadcast = true);

protected:
	void RefreshDimensions();
	virtual void NativeOnMouseEnter( const FGeometry& InGeometry, const FPointerEvent& InMouseEvent ) override;
	virtual void NativeOnMouseLeave( const FPointerEvent& InMouseEvent ) override;

	/** The minimum width of the button */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Layout, meta = (ClampMin = "0"))
	int32 MinWidth;

	/** The minimum height of the button */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Layout, meta = (ClampMin = "0"))
	int32 MinHeight;

	/** References the button style asset that defines a style in multiple sizes */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Style, meta = (ExposeOnSpawn = true))
	TSubclassOf<UUTButtonStyle> Style;

	/** The style size to use when extracting style information from the assigned style */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Style, meta = (ExposeOnSpawn = true))
	TEnumAsByte<EUTWidgetStyleSize::Type> StyleSize;

	/** The type of mouse action required by the user to trigger the button's 'Click' */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Style, meta = (ExposeOnSpawn = true))
	bool bApplyAlphaOnDisable;

	/** Optional override for the sound to play when this button is pressed */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Sound, meta = (DisplayName = "Pressed Sound Override"))
	FSlateSound PressedSlateSoundOverride;

	/** Optional override for the sound to play when this button is hovered */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Sound, meta = (DisplayName = "Hovered Sound Override"))
	FSlateSound HoveredSlateSoundOverride;
	
	/** True if the button supports being in a "selected" state, which will update the style accordingly. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Selection, meta = (ExposeOnSpawn = true))
	bool bSelectable;

	/** True if the button can be deselected by clicking it when selected */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Selection, meta = (ExposeOnSpawn = true, EditCondition = "bSelectable"))
	bool bToggleable;

	/** The type of mouse action required by the user to trigger the button's 'Click' */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Selection, meta = (ExposeOnSpawn = true))
	TEnumAsByte<EButtonClickMethod::Type> ClickMethod;

	/** True if this button is currently bound to a key */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (ExposeOnSpawn = true))
	FKey BoundKey;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (ExposeOnSpawn = true))
	bool bListenForInput;

	//UPROPERTY(EditAnywhere, Category = "Analytics|Actions", meta = (DisplayName = "Clicked"))
	//EUTUIAnalyticsTrackingLevel ClickedTrackingLevel;

	//UPROPERTY(EditAnywhere, Category = "Analytics|Actions", meta = (DisplayName = "Selection Changed"))
	//EUTUIAnalyticsTrackingLevel SelectionChangedTrackingLevel;

private:
	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess = true))
	FUTSelectedStateChanged OnButtonSelectedChanged;

	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess = true))
	FUTBaseButtonClicked OnButtonClicked;

	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess = true))
	FUTBaseButtonClicked OnButtonHovered;

	UPROPERTY(BlueprintAssignable, Category = "Events", meta = (AllowPrivateAccess = true))
	FUTBaseButtonClicked OnButtonUnhovered;

	friend class UUTBaseButton_Group;
	friend class UPageView;

	const UUTButtonStyle* GetStyleCDO() const;
	void BuildStyles();
	void SetButtonStyle();
	
	FText EnabledTooltipText;
	FText DisabledTooltipText;

	/** Internally managed and applied style to use when not selected */
	FButtonStyle NormalStyle;
	
	/** Internally managed and applied style to use when selected */
	FButtonStyle SelectedStyle;

	/** True if this button is currently selected */
	bool bSelected;

	/** True if interaction with this button is currently enabled */
	bool bInteractionEnabled;

	/**
	 * The actual UButton that we wrap this user widget into. 
	 * Allows us to get user widget customization and built-in button functionality. 
	 */
	TWeakObjectPtr<class UUTButtonInternal> RootButton;

	FOnBaseButtonEvent OnClickedEvent;
	FOnBaseButtonEvent OnHoveredEvent;
	FOnBaseButtonEvent OnUnhoveredEvent;
	FOnBaseButtonSelectedChanged OnSelectedChangedEvent;
};
