// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UMG.h"
#include "UTUITypes.h"

#include "UTRichText.generated.h"

// Various ways that we display inline icon that have an icon-name association
UENUM(BlueprintType)
enum class ERichTextInlineIconDisplayMode : uint8
{
	// Only show the icon - use when space is limited
	IconOnly,
	// Only show the text - use seldom if ever
	TextOnly,
	// Show both the icon and the text - use whenever there is space
	IconAndText,
	MAX
};

class UUTTextStyle;
class FUTRichTextLayoutMarshaller;

UCLASS(ClassGroup = UI, meta = (Category = "UT UI", DisplayName = "UT Rich Text"))
class UNREALTOURNAMENT_API UUTRichText : public UTextLayoutWidget
{
	GENERATED_UCLASS_BODY()
public:

	/** The text content for this widget */
	UPROPERTY(EditAnywhere, Category=Content, meta=( MultiLine="true" ))
	FText Text;

	/** The amount of blank space left around the edges of text area. 
		This is different to Padding because this area is still considered part of the text area, and as such, can still be interacted with */
	UPROPERTY(EditAnywhere, Category=Appearance)
	FMargin TextMargin;

	/** True to display icons associated with keywords */
	UPROPERTY(EditAnywhere, Category=Appearance)
	ERichTextInlineIconDisplayMode InlineIconDisplayMode;

public:
	UFUNCTION(BlueprintCallable, Category = UTRichText)
	void SetText(const FText& InText);

	UFUNCTION(BlueprintCallable, Category = UTRichText)
	void SetInlineIconDisplayMode(ERichTextInlineIconDisplayMode DisplayMode);

	UFUNCTION(BlueprintCallable, Category = UTRichText)
	void SetSize(EUTWidgetStyleSize::Type Size);

	UFUNCTION(BlueprintCallable, Category = UTRichText)
	void SetColorType(EUTTextColor::Type Color);

	UFUNCTION(BlueprintCallable, Category = UTRichText)
	EUTWidgetStyleSize::Type GetStyleSize();

	UFUNCTION(BlueprintCallable, Category = UTRichText)
	EUTTextColor::Type GetColorType();

	UFUNCTION(BlueprintCallable, Category = UTRichText)
	EUTTextColor::Type GetInlineIconColorType();

	UFUNCTION(BlueprintCallable, Category = UTRichText)
	void Refresh();

protected:
	virtual void PostLoad() override;

	/** UWidget interface */
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	void UpdateInlineIconTextStyle();
	void BuildDefaultTextStyle(FTextBlockStyle& Style);
	void UpdateDefaultTextStyle();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Appearance, meta = (ExposeOnSpawn = true))
	TSubclassOf<UUTTextStyle> NormalTextStyle;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Appearance, meta = (ExposeOnSpawn = true))
	TSubclassOf<UUTTextStyle> InlineIconTextStyle;

	/** The style size to use when extracting style information from the assigned style */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Appearance, meta = (ExposeOnSpawn = true))
	TEnumAsByte<EUTWidgetStyleSize::Type> StyleSize;

	/** The color type to use when extracting color from the assigned style */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Appearance, meta = (ExposeOnSpawn = true))
	TEnumAsByte<EUTTextColor::Type> ColorType;

	/** The color type to use when extracting color from the inline icon text style */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Appearance, meta = (ExposeOnSpawn = true))
	TEnumAsByte<EUTTextColor::Type> InlineIconColorType;

	/** The minimum desired size for the text */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Appearance)
	float MinDesiredWidth;

protected:

	void BindToInputChangeDelegates();
	void OnUsingGamepad(bool bUsingGamepad);
	void OnControlsChanged(EUTInputType ControlsType);

	TSharedPtr<FUTRichTextLayoutMarshaller> Marshaller;
	TSharedPtr<SRichTextBlock> MyRichText;
};