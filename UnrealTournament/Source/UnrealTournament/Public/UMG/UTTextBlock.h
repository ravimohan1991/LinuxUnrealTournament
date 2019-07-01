// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UMG.h"
#include "UTUITypes.h"
#include "Widgets/Layout/SScrollBox.h"

#include "UTTextBlock.generated.h"

/* 
 * ---- All properties must be EditDefaultsOnly, BlueprintReadOnly !!! -----
 * We return the CDO to blueprints, so we cannot allow any changes (blueprint doesn't support const variables)
 */
UCLASS(Abstract, Blueprintable, ClassGroup = UI, meta = (Category = "UT UI"))
class UNREALTOURNAMENT_API UUTTextStyle : public UObject
{
	GENERATED_BODY()

public:
	/** The font to apply at each size */
	UPROPERTY(EditDefaultsOnly, Category = "Font")
	FSlateFontInfo Font[EUTWidgetStyleSize::MAX];

	/** The color of the text */
	UPROPERTY(EditDefaultsOnly, Category = "Color")
	FLinearColor Color[EUTTextColor::MAX];

	/** The offset of the drop shadow at each size */
	UPROPERTY(EditDefaultsOnly, Category = "Shadow")
	FVector2D ShadowOffset[EUTWidgetStyleSize::MAX];

	/** The drop shadow color */
	UPROPERTY(EditDefaultsOnly, Category = "Shadow")
	FLinearColor ShadowColor[EUTTextColor::MAX];

	/** The amount of blank space left around the edges of text area at each size */
	UPROPERTY(EditDefaultsOnly, Category = "Properties")
	FMargin Margin[EUTWidgetStyleSize::MAX];

	/** The amount to scale each lines height by at each size */
	UPROPERTY(EditDefaultsOnly, Category = "Properties")
	float LineHeightPercentage[EUTWidgetStyleSize::MAX];

	UFUNCTION(BlueprintCallable, Category = "UT Text Style|Getters")
	void GetFont(EUTWidgetStyleSize::Type Size, FSlateFontInfo& OutFont) const;

	UFUNCTION(BlueprintCallable, Category = "UT Text Style|Getters")
	void GetColor(EUTTextColor::Type ColorType, FLinearColor& OutColor) const;

	UFUNCTION(BlueprintCallable, Category = "UT Text Style|Getters")
	void GetMargin(EUTWidgetStyleSize::Type Size, FMargin& OutMargin) const;

	UFUNCTION(BlueprintCallable, Category = "UT Text Style|Getters")
	float GetLineHeightPercentage(EUTWidgetStyleSize::Type Size) const;

	UFUNCTION(BlueprintCallable, Category = "UT Text Style|Getters")
	void GetShadowOffset(EUTWidgetStyleSize::Type Size, FVector2D& OutShadowOffset) const;

	UFUNCTION(BlueprintCallable, Category = "UT Text Style|Getters")
	void GetShadowColor(EUTTextColor::Type ColorType, FLinearColor& OutColor) const;

	void ToTextBlockStyle(FTextBlockStyle& OutTextBlockStyle, 
		EUTWidgetStyleSize::Type Size = EUTWidgetStyleSize::Medium, 
		EUTTextColor::Type ColorType = EUTTextColor::Light);
};


UCLASS(ClassGroup = UI, meta = (Category = "UT UI", DisplayName = "UT Text"))
class UNREALTOURNAMENT_API UUTTextBlock : public UTextBlock
{
	GENERATED_UCLASS_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "UT Text")
	void SetWrapTextWidth(int32 InWrapTextAt);

	UFUNCTION(BlueprintCallable, Category = "UT Text")
	void SetStyle(TSubclassOf<UUTTextStyle> InStyle);

	UFUNCTION(BlueprintCallable, Category = "UT Text")
	void SetSize(EUTWidgetStyleSize::Type Size);

	UFUNCTION(BlueprintCallable, Category = "UT Text")
	void SetColorType(EUTTextColor::Type Color);

	UFUNCTION(BlueprintCallable, Category = "UT Text")
	void SetUseDropShadow(bool bShouldUseDropShadow);

	UFUNCTION(BlueprintCallable, Category = "UT Text")
	void SetProperties(TSubclassOf<UUTTextStyle> InStyle, EUTWidgetStyleSize::Type Size, EUTTextColor::Type Color, int32 InWrapTextAt = 0, bool bShouldUseDropShadow = false);

protected:
	// UWidget interface
	virtual void SynchronizeProperties() override;
	// End of UWidget interface

	/** References the button style asset that defines a style in multiple sizes */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UT Text", meta = (ExposeOnSpawn = true))
	TSubclassOf<UUTTextStyle> Style;

	/** The style size to use when extracting style information from the assigned style */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UT Text", meta = (ExposeOnSpawn = true))
	TEnumAsByte<EUTWidgetStyleSize::Type> StyleSize;

	/** The color type to use when extracting color from the assigned style */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UT Text", meta = (ExposeOnSpawn = true))
	TEnumAsByte<EUTTextColor::Type> ColorType;

	/** Whether to apply the drop shadow as specified in the applied text style or hide it completely */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UT Text", meta = (ExposeOnSpawn = true))
	bool bUseDropShadow;

	/** True to always display text in ALL CAPS */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UT Text", meta = (ExposeOnSpawn = true))
	bool bAllCaps;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UT Text|Scrolling", meta = (ExposeOnSpawn = true))
	bool bScroll;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UT Text|Scrolling", meta = (ExposeOnSpawn = true, EditCondition = bScroll))
	float Speed;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UT Text|Scrolling", meta = (ExposeOnSpawn = true, EditCondition = bScroll))
	float StartDelay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UT Text|Scrolling", meta = (ExposeOnSpawn = true, EditCondition = bScroll))
	float EndDelay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UT Text|Scrolling", meta = (ExposeOnSpawn = true, EditCondition = bScroll))
	float FadeInDelay;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UT Text|Scrolling", meta = (ExposeOnSpawn = true, EditCondition = bScroll))
	float FadeOutDelay;

	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual TAttribute<FText> GetDisplayText() override;

private:
	const UUTTextStyle* GetStyleCDO() const;

	enum
	{
		EFadeIn = 0,
		EStart,
		EStartWait,
		EScrolling,
		EStop,
		EStopWait,
		EFadeOut,
	} ActiveState;

	bool OnTick( float Delta );

	float TimeElapsed;
	float ScrollOffset;
	float FontAlpha;
	bool bPlaying;

	TSharedPtr<SScrollBox> ScrollBox;
	TSharedPtr<SScrollBar> ScrollBar;

	FDelegateHandle TickHandle;
};