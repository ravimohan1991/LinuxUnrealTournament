// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UMG.h"
#include "UTBaseButton.h"
#include "UTUITypes.h"
#include "UTDefaultButton.generated.h"

class UUTTextBlock;
class UUTInputVisualizer;

UCLASS()
class UNREALTOURNAMENT_API UUTDefaultButton : public UUTBaseButton
{
	GENERATED_BODY()

public:
	UUTDefaultButton(const FObjectInitializer& Initializer);

	virtual void BindClickToKey(const FKey& KeyToBind) override;

	/** Initialize the button with all the necessary info */
	void Init(const FText& InText, UTexture2D* Texture, EUTWidgetStyleSize::Type InStyleSize, int32 InMinWidth = 0, int32 InMinHeight = 0);

	/** Updates the text displayed in the button */
	UFUNCTION(BlueprintCallable, Category = DefaultButton)
	void UpdateText(const FText& InText);

	/** Updates the icon displayed in the button */
	UFUNCTION(BlueprintCallable, Category = DefaultButton)
	void UpdateIcon(const FSlateBrush& Brush, bool bRetainBrushImageSize = false);

	/** Updates the icon displayed in the button from a texture (uses the established icon sizes for the button) */
	UFUNCTION(BlueprintCallable, Category = DefaultButton)
	void UpdateIconFromTexture(UTexture2D* Texture);

	/** Updates the image size for the specific type of style size*/
	UFUNCTION(BlueprintCallable, Category = DefaultButton)
	void UpdateSpecificImageSize(EUTWidgetStyleSize::Type Type, float InSize);

	/** Refreshes the button layout */
	UFUNCTION(BlueprintCallable, Category = DefaultButton)
	void RefreshLayout();

	/** Set the color type of the content normally */
	UFUNCTION(BlueprintCallable, Category = DefaultButton)
	void SetNormalContentColor(EUTTextColor::Type InNormalTextColor);

	/** Set the color type of the content when hovered */
	UFUNCTION(BlueprintCallable, Category = DefaultButton)
	void SetHoveredContentColor(EUTTextColor::Type InHoveredTextColor);

protected:
	virtual void NativePreConstruct() override;
	
	virtual void NativeOnSelected(bool bBroadcast) override;
	virtual void NativeOnDeselected(bool bBroadcast) override;
	virtual void NativeOnStyleSizeChanged() override;

	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	
	virtual void SynchronizeProperties() override;

	void SetContentColor(EUTTextColor::Type ColorType);
	
protected:
	/** The minimum spacing between the text and the icon (if we're showing one) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DefaultButton, meta = (ClampMin = "0"))
	float MinContentSpacing;

	/** The width at which to wrap the text */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DefaultButton, meta = (ClampMin = "0"))
	int32 WrapLabelTextAt;

	/** The text to display (optional) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DefaultButton)
	FText Text;
	
	/** The color type of the text normally */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DefaultButton)
	TEnumAsByte<EUTTextColor::Type> NormalTextColor;
	
	/** The color type of the text when hovered */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DefaultButton)
	TEnumAsByte<EUTTextColor::Type> HoveredTextColor;

	/** The alignment of the text when there is no icon */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DefaultButton)
	TEnumAsByte<EHorizontalAlignment> TextAlignment_NoIcon;

	/** The alignment of the text when there is an icon */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DefaultButton)
	TEnumAsByte<EHorizontalAlignment> TextAlignment_WithIcon;

	/** The alignment of the icon */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DefaultButton)
	TEnumAsByte<EHorizontalAlignment> IconAlignment;

	/** The alignment of the button content as a whole */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DefaultButton)
	TEnumAsByte<EHorizontalAlignment> ContentAlignment;

	/** The icon to display (optional) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = DefaultButton)
	FSlateBrush IconBrush;

	/** The sizes to display the icon when provided */
	UPROPERTY(EditAnywhere, Category = DefaultButton)
	float IconSizes[EUTWidgetStyleSize::MAX];

	/** True to display the icon to the right of the text, false to display on the left */
	UPROPERTY(EditAnywhere, Category = DefaultButton)
	bool bIconOnRight;

private:	// Bound widgets
	
	UPROPERTY(meta = (BindWidget))
	USizeBox* SizeBox_Container;

	// Note: This is expected to be inside a size box
	UPROPERTY(meta = (BindWidget))
	UHorizontalBox* HBox_Content;

	// Note: This is expected to be inside a horizontal box
	UPROPERTY(meta = (BindWidget))
	UImage* Image_Icon;

	// Note: This is expected to be inside a horizontal box
	UPROPERTY(meta = (BindWidget))
	UUTTextBlock* Text_Label;

	UPROPERTY(meta = (BindWidget))
	UUTInputVisualizer* Input_BoundKey;
};