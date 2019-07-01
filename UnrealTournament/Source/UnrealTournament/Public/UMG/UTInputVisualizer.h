// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UMG.h"
#include "UTUITypes.h"
#include "UTInputVisualizer.generated.h"

/** Visualizes input methods (keys, buttons, sticks, whatever) based on the currently active input method */
UCLASS(BlueprintType, Blueprintable)
class UNREALTOURNAMENT_API UUTInputVisualizer : public UWidget
{
	GENERATED_UCLASS_BODY()

public:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	virtual void SynchronizeProperties() override;

	/** Sets the style size of this visualizer. */
	UFUNCTION(BlueprintCallable, Category = InputVisualizer)
	void SetSize(EUTWidgetStyleSize::Type InSize);

	/**
	 * Displays the input binding for the given action with the currently active input method.
	 * Note that this will not quite work for abilities, as their action name varies across input methods.
	 * To visualize the input binding for an ability, use ShowAbilityBinding
	 */
	UFUNCTION(BlueprintCallable, Category = InputVisualizer)
	void ShowInputAction(FName InActionName);
	
	/** Displays a specific key - collapses if the key is not relevant to the currently active input method. */
	UFUNCTION(BlueprintCallable, Category = InputVisualizer)
	void ShowSpecificKey(FKey Key);

	UFUNCTION(BlueprintCallable, Category = InputVisualizer)
	void ShowInputAxis(FName InAxisName);

#if WITH_EDITOR
	// UObject interface
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
	// End of UObject interface
#endif

protected:
	virtual void OnWidgetRebuilt() override;

	// Super long name for a super short function - we just don't want to bind until we're actually showing something
	void ListenForUsingGamepadChangedIfNeeded();

	UFUNCTION()
	void HandleControlsChanged();

private:
	void HandleUsingGamepadChanged(bool bUsingGamepad);
	
	/**	The size to draw this visualizer */
	UPROPERTY(EditAnywhere)
	TEnumAsByte<EUTWidgetStyleSize::Type> StyleSize;

	/**	The name of the action to display the input binding for */
	UPROPERTY(EditAnywhere)
	FName ActionName;

	UPROPERTY(EditAnywhere)
	FName AxisName;

	/**	The specific key to display. Used only if no action name is provided. */
	UPROPERTY(EditAnywhere)
	FKey SpecificKey;

	/**	True to include the key border for keyboard keys */
	UPROPERTY(EditAnywhere)
	bool bShowKeyBorder;

	/** True to prefer the secondary icon over the main/standard one, if one exists */
	UPROPERTY(EditAnywhere)
	bool bPreferSecondaryIcon;

	TSharedPtr<SBox> MyKeyBox;
};