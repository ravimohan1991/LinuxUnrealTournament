// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTRichTextStyleData.h"
#include "UTGlobals.h"

FUTKeywordIcon::FUTKeywordIcon(UTexture2D* InIcon, const float Sizes[EUTWidgetStyleSize::MAX])
{
	Icon = InIcon;

	for (int32 SizeIdx = 0; SizeIdx < EUTWidgetStyleSize::MAX; SizeIdx++)
	{
		FSlateBrush* NewIconBrush = new FSlateBrush();
		NewIconBrush->SetResourceObject(Icon);
		NewIconBrush->ImageSize.X = NewIconBrush->ImageSize.Y = Sizes[SizeIdx];

		Brushes[SizeIdx] = NewIconBrush;
	}
}

UUTRichTextStyleData::UUTRichTextStyleData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, KeywordMarkupTag(TEXT("key"))
	, StatusEffectMarkupTag(TEXT("status"))
	, CurrencyMarkupTag(TEXT("cur"))
	, InputMarkupTag(TEXT("input"))
	, InputAxisMarkupTag(TEXT("inputaxis"))
	, SpecificKeyMarkupSubtag(TEXT("specific"))
{
	StyleSizeSubtags[EUTWidgetStyleSize::Small] = TEXT("sm");
	StyleSizeSubtags[EUTWidgetStyleSize::Medium] = TEXT("md");
	StyleSizeSubtags[EUTWidgetStyleSize::Large] = TEXT("lg");
	
	ColorTypeSubtags[EUTTextColor::Light] = TEXT("lt");
	ColorTypeSubtags[EUTTextColor::Dark] = TEXT("dk");
	ColorTypeSubtags[EUTTextColor::Black] = TEXT("bk");
	ColorTypeSubtags[EUTTextColor::Emphasis] = TEXT("em");
	
	CurrencyTypeIDs[TEnumValue(EUTCurrencyType::TimeCurrency)] = TEXT("time");
	CurrencyTypeIDs[TEnumValue(EUTCurrencyType::MtxCurrency)] = TEXT("mtx");
	CurrencyTypeIDs[TEnumValue(EUTCurrencyType::RealMoney)] = TEXT("cash");
}

const FSlateBrush* UUTRichTextStyleData::GetIconBrush(const FString& Keyword, UTexture2D* IconTexture, EUTWidgetStyleSize::Type Size) const
{
	FUTKeywordIcon* KeywordIcon = IconsByKeyword.Find(Keyword);
	if (!KeywordIcon && IconTexture)
	{
		// Create a new brush set for the icon
		FUTKeywordIcon NewKeywordIcon(IconTexture, InlineIconSizes);
		KeywordIcon = &IconsByKeyword.Add(Keyword, NewKeywordIcon);
	}

	if (KeywordIcon)
	{
		return KeywordIcon->Brushes[Size];
	}

	return nullptr;
}

#if WITH_EDITOR
void UUTRichTextStyleData::PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// When the asset gets updated, refresh any currently cached icons
	for (auto& IconPair : IconsByKeyword)
	{
		for (int32 SizeIdx = 0; SizeIdx < EUTWidgetStyleSize::MAX; SizeIdx++)
		{
			FSlateBrush* Brush = IconPair.Value.Brushes[SizeIdx];
			Brush->ImageSize.X = Brush->ImageSize.Y = InlineIconSizes[SizeIdx];
		}
	}
}
#endif

//////////////////////////////////////////////////////////////////////////
// UUTRichTextHelperLibrary
//////////////////////////////////////////////////////////////////////////

FString UUTRichTextHelperLibrary::FormatAsKeyword(const FString& Text)
{
	const UUTRichTextStyleData& RichTextStyle = UUTGameUIData::Get().GetRichTextStyleData();

	return FString::Printf(TEXT("<%s>%s</>"), *RichTextStyle.KeywordMarkupTag, *Text);
}

void UUTRichTextHelperLibrary::ConvertToKeyword(UPARAM(ref) FText& Text)
{
	Text = FText::FromString(FormatAsKeyword(Text.ToString()));
}

FString UUTRichTextHelperLibrary::FormatAsKeywordSpecific(const FString& Text, TEnumAsByte<EUTWidgetStyleSize::Type> Size, TEnumAsByte<EUTTextColor::Type> Color)
{
	const UUTRichTextStyleData& RichTextStyle = UUTGameUIData::Get().GetRichTextStyleData();

	return FString::Printf(TEXT("<%s.%s.%s>%s</>"), *RichTextStyle.KeywordMarkupTag, *RichTextStyle.StyleSizeSubtags[Size], *RichTextStyle.ColorTypeSubtags[Color], *Text);
}

void UUTRichTextHelperLibrary::ConvertToKeywordSpecific(UPARAM(ref) FText& Text, TEnumAsByte<EUTWidgetStyleSize::Type> Size, TEnumAsByte<EUTTextColor::Type> Color)
{
	Text = FText::FromString(FormatAsKeywordSpecific(Text.ToString(), Size, Color));
}

FString UUTRichTextHelperLibrary::GetCurrencyIconTag(EUTCurrencyType Currency)
{
	const UUTRichTextStyleData& RichTextStyle = UUTGameUIData::Get().GetRichTextStyleData();

	return FString::Printf(TEXT("<%s id=\"%s\"/>"), *RichTextStyle.CurrencyMarkupTag, *RichTextStyle.CurrencyTypeIDs[TEnumValue(Currency)]);
}

FString UUTRichTextHelperLibrary::GetCurrencyIconTagSpecific(EUTCurrencyType Currency, EUTWidgetStyleSize::Type Size)
{
	const UUTRichTextStyleData& RichTextStyle = UUTGameUIData::Get().GetRichTextStyleData();

	return FString::Printf(TEXT("<%s.%s id=\"%s\"/>"), *RichTextStyle.CurrencyMarkupTag, *RichTextStyle.StyleSizeSubtags[Size], *RichTextStyle.CurrencyTypeIDs[TEnumValue(Currency)]);
}

FText UUTRichTextHelperLibrary::FormatAsCurrency(EUTCurrencyType Currency, FText DisplayValue, EUTWidgetStyleSize::Type Size)
{
	static const FText CurrencyFormatText = NSLOCTEXT("UTUI", "Currency Format", "{0}{1}");

	if (Size == EUTWidgetStyleSize::MAX)
	{
		return FText::Format(CurrencyFormatText, FText::FromString(GetCurrencyIconTag(Currency)), DisplayValue);
	}
	else
	{
		return FText::Format(CurrencyFormatText, FText::FromString(GetCurrencyIconTagSpecific(Currency, Size)), DisplayValue);
	}
}
