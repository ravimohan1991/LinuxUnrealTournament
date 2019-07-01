// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Styling/SlateBrush.h"
#include "UTUITypes.h"
#include "UTTextBlock.h"

#include "UTRichTextStyleData.generated.h"

struct FUTAttributeViewItem;

USTRUCT()
struct FUTKeywordIcon
{
	GENERATED_BODY()

public:
	// For UObject system - don't use
	FUTKeywordIcon()
		: Icon(nullptr)
	{}

	// Use me!
	FUTKeywordIcon(UTexture2D* InIcon, const float Sizes[EUTWidgetStyleSize::MAX]);

	UPROPERTY()
	UTexture2D* Icon;

	FSlateBrush* Brushes[EUTWidgetStyleSize::MAX];
};

UCLASS( hidecategories = Object, MinimalAPI, BlueprintType )
class UUTRichTextStyleData : public UDataAsset
{
	GENERATED_UCLASS_BODY()

public:
	/** Subtags used for style sizes */
	UPROPERTY(VisibleAnywhere, Category = Display)
	FString StyleSizeSubtags[EUTWidgetStyleSize::MAX];

	/** Subtags used for style colors */
	UPROPERTY(VisibleAnywhere, Category = Display)
	FString ColorTypeSubtags[EUTTextColor::Custom];

	/** This is the tag that UTRichText identifies keywords with */
	UPROPERTY(VisibleAnywhere, Category = Keywords)
	FString KeywordMarkupTag;

	/** The style to apply to text tagged as a keyword */
	UPROPERTY(EditAnywhere, Category = Keywords)
	TSubclassOf<UUTTextStyle> KeywordStyle;

	/** The brush size of the inline rich text icons at each style size */
	UPROPERTY(EditAnywhere, Category = InlineIcons)
	float InlineIconSizes[EUTWidgetStyleSize::MAX];

	/** The baseline of the inline rich text icons at each style size */
	UPROPERTY(EditAnywhere, Category = InlineIcons)
	float InlineIconBaselines[EUTWidgetStyleSize::MAX];

	/** The tag UTRichText will use to identify status effects with */
	UPROPERTY(VisibleAnywhere, Category = Effects)
	FString StatusEffectMarkupTag;

	/** The base markup tag for currencies */
	UPROPERTY(VisibleAnywhere, Category = Currency)
	FString CurrencyMarkupTag;

	/** The markup subtags for each type of currency */
	UPROPERTY(VisibleAnywhere, Category = Currency)
	FString CurrencyTypeIDs[(uint8)EUTCurrencyType::MAX];

	/** The markup subtags for each type of currency */
	UPROPERTY(EditAnywhere, Category = Currency)
	FText CurrencyNames[(uint8)EUTCurrencyType::MAX];

	/** The markup subtags for each type of currency */
	UPROPERTY(EditAnywhere, Category = Currency)
	UTexture2D* CurrencyIcons[(uint8)EUTCurrencyType::MAX];

	/** The markup tag for an inline input visualization. Assumes an action unless the specific key subtag is also present. */
	UPROPERTY(VisibleAnywhere, Category = Input)
	FString InputMarkupTag;

	/** The markup tag for an inline input axis visualization. */
	UPROPERTY(VisibleAnywhere, Category = Input)
	FString InputAxisMarkupTag;

	/** The subtag to use when identifying a specific key instead of an action */
	UPROPERTY(VisibleAnywhere, Category = Input)
	FString SpecificKeyMarkupSubtag;

	const FSlateBrush* GetIconBrush(const FString& Keyword, UTexture2D* IconTexture, EUTWidgetStyleSize::Type Size) const;

#if WITH_EDITOR
	virtual void PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
	// A map of the various brushes for each icon used in UTRichText organized by keyword
	UPROPERTY(Transient)
	mutable TMap<FString, FUTKeywordIcon> IconsByKeyword;
};

UCLASS()
class UNREALTOURNAMENT_API UUTRichTextHelperLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = RichTextHelper)
	static FString FormatAsKeyword(const FString& Text);

	UFUNCTION(BlueprintPure, Category = RichTextHelper)
	static void ConvertToKeyword(UPARAM(ref) FText& Text);

	UFUNCTION(BlueprintPure, Category = RichTextHelper)
	static FString FormatAsKeywordSpecific(const FString& Text, TEnumAsByte<EUTWidgetStyleSize::Type> Size, TEnumAsByte<EUTTextColor::Type> Color);

	UFUNCTION(BlueprintPure, Category = RichTextHelper)
	static void ConvertToKeywordSpecific(UPARAM(ref) FText& Text, TEnumAsByte<EUTWidgetStyleSize::Type> Size, TEnumAsByte<EUTTextColor::Type> Color);

	UFUNCTION(BlueprintPure, Category = RichTextHelper)
	static FString GetCurrencyIconTag(EUTCurrencyType Currency);

	UFUNCTION(BlueprintPure, Category = RichTextHelper)
	static FString GetCurrencyIconTagSpecific(EUTCurrencyType Currency, EUTWidgetStyleSize::Type Size);

	UFUNCTION(BlueprintPure, Category = RichTextHelper)
	static FText FormatAsCurrency(EUTCurrencyType Currency, FText DisplayValue, EUTWidgetStyleSize::Type Size = EUTWidgetStyleSize::MAX);
};