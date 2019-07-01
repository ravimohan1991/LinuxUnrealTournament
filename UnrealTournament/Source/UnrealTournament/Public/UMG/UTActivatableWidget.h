// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTUserWidgetBase.h"
#include "UTActivatableWidget.generated.h"

DECLARE_MULTICAST_DELEGATE(FOnWidgetActivationChanged);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWidgetActivationChangedDynamic);

/** 
 * The base for UT widgets that are capable of being "activated" during their lifetime.
 * By default, an activatable widget:
 *	- Is not automatically activated
 *	- Does not register to receive back actions
 *	- If classified as a back handler, is automatically deactivated (but not destroyed) when it receives a back action
 * 
 * Custom behavior can be provided to activate, deactivate, and remove the widget where appropriate.
 * 
 * Note that removing an activatable widget from the UI will always deactivate it, even if the underlying UWidget is not destroyed.
 * Re-constructing the widget will only re-activate it if auto-activate is enabled.
 * 
 * Use-cases abound in the front end. 
 * For examples and complimentary widgets see UUTActivatableWidgetSwitcher, UUTLightBox, and DeckBuilder (blueprint)
 */
UCLASS( BlueprintType, Blueprintable, meta = (Category = "UT UI") )
class UNREALTOURNAMENT_API UUTActivatableWidget : public UUTUserWidgetBase
{
	GENERATED_UCLASS_BODY()

public:

	// UUserWidget interface
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	// End UUserWidget interface
	
	/** @return True if this widget is currently activated */
	bool IsActivated() const { return bIsActive; }

	/** Activates the widget and registers it to receive back actions */
	UFUNCTION(BlueprintCallable, Category = ActivatableWidget)
	void ActivateWidget();

	/** Unregisters the panel and either removes it from the UI or allows an external bind to handle the deactivation */
	UFUNCTION(BlueprintCallable, Category = ActivatableWidget)
	void DeactivateWidget();

	/** Fires when the widget is activated. */
	UPROPERTY(BlueprintAssignable)
	FOnWidgetActivationChangedDynamic OnWidgetActivated;
	FOnWidgetActivationChanged OnWidgetActivatedNative;

	/** Fires when the widget is deactivated. */
	UPROPERTY(BlueprintAssignable)
	FOnWidgetActivationChangedDynamic OnWidgetDeactivated;
	FOnWidgetActivationChanged OnWidgetDeactivatedNative;
	
	/**
	 *	Override in BP implementations to provide custom behavior when receiving a back action.
	 *	@return True if the back action was handled
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = ActivatableWidget, meta = (BlueprintProtected=true))
	bool OnHandleBackAction();

protected:
	
	/** If true, the panel is automatically activated on construction */
	UPROPERTY(EditAnywhere, Category = ActivatableWidget)
	bool bAutoActivate;
	
	/** True if the panel is currently active and receiving actions */
	UPROPERTY(BlueprintReadOnly, Category = ActivatableWidget, meta = (AllowPrivateAccess = true))
	bool bIsActive;

	/** Allows the BP to take action when activated */
	UFUNCTION(BlueprintImplementableEvent, Category = ActivatableWidget)
	void OnActivated();
	virtual void NativeOnActivated();

	/** Allows the BP to take action when deactivated */
	UFUNCTION(BlueprintImplementableEvent, Category = ActivatableWidget)
	void OnDeactivated();
	virtual void NativeOnDeactivated();
};