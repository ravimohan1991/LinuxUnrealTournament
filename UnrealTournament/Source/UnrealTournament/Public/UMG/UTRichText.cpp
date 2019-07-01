// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTRichText.h"
#include "UTGlobals.h"
#include "RichTextLayoutMarshaller.h"
#include "SlateTextLayout.h"
#include "SlateGameResources.h"

#include "UTGameUIData.h"
#include "UTTextBlock.h"
#include "UTRichTextStyleData.h"

class FUTRichTextStyle
{
public:
	static void Initialize()
	{
		if (!UTRichTextStyleInstance.IsValid())
		{
			TSharedRef<FSlateStyleSet> StyleRef = FSlateGameResources::New(FUTRichTextStyle::GetStyleSetName(), "/Game/UI/Styles", "/Game/UI/Styles");

			UTRichTextStyleInstance = StyleRef;

			UpdateStyleData();

			FSlateStyleRegistry::RegisterSlateStyle(*UTRichTextStyleInstance);
		}
	}

	static void Shutdown()
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*UTRichTextStyleInstance);

		ensure(UTRichTextStyleInstance.IsUnique());
		UTRichTextStyleInstance.Reset();
	}

	static const ISlateStyle& Get()
	{
		if (!UTRichTextStyleInstance.IsValid())
		{
			Initialize();
		}
		return *UTRichTextStyleInstance;
	}
	
	static FName GetStyleSetName()
	{
		static FName StyleSetName(TEXT("UTRichTextStyle"));
		return StyleSetName;
	}

	static void UpdateStyleData()
	{
		if (UTRichTextStyleInstance.IsValid())
		{
			const UUTRichTextStyleData& StyleData = UUTGameUIData::Get().GetRichTextStyleData();

			if (const UUTTextStyle* KeywordStyle = Cast<UUTTextStyle>(StyleData.KeywordStyle->ClassDefaultObject))
			{
				// Keywords
				FTextBlockStyle KeywordTextStyle;
				SetMultiSizeTextStyle(KeywordTextStyle, KeywordStyle, StyleData.KeywordMarkupTag, true);
			}
		}
	}

private:
	
	static void SetMultiSizeTextStyle(const FTextBlockStyle& InTextStyle, const UUTTextStyle* UTStyleCDO, const FString& BaseTag, bool bIncludeColor)
	{
		const UUTRichTextStyleData& StyleData = UUTGameUIData::Get().GetRichTextStyleData();

		static FString Period(TEXT("."));

		FTextBlockStyle TextStyle = InTextStyle;
		for (int32 SizeIdx = 0; SizeIdx < EUTWidgetStyleSize::MAX; SizeIdx++)
		{
			FString StyleIDString = BaseTag;

			TextStyle.SetFont(UTStyleCDO->Font[SizeIdx]);
			StyleIDString += Period + StyleData.StyleSizeSubtags[SizeIdx];

			if (bIncludeColor)
			{
				StyleIDString += Period;
				for (int32 ColorIdx = 0; ColorIdx < EUTTextColor::Custom; ColorIdx++)
				{
					TextStyle.SetColorAndOpacity(UTStyleCDO->Color[ColorIdx]);
					FString FinalIDString = StyleIDString + StyleData.ColorTypeSubtags[ColorIdx];

					UTRichTextStyleInstance->Set(*FinalIDString, TextStyle);
				}
			}
			else
			{
				UTRichTextStyleInstance->Set(*StyleIDString, TextStyle);
			}
		}
	}

	static TSharedPtr<FSlateStyleSet> UTRichTextStyleInstance;
};

TSharedPtr<FSlateStyleSet> FUTRichTextStyle::UTRichTextStyleInstance = nullptr;

class FUTRichTextLayoutMarshaller : public FRichTextLayoutMarshaller
{
public:
	static TSharedRef<FUTRichTextLayoutMarshaller> Create(ERichTextInlineIconDisplayMode DisplayMode,
		TArray<TSharedRef<ITextDecorator>> InDecorators, 
		const ISlateStyle* const InDecoratorStyleSet,
		const FLocalPlayerContext& InPlayerContext)
	{
		return MakeShareable(new FUTRichTextLayoutMarshaller(DisplayMode, MoveTemp(InDecorators), InDecoratorStyleSet, InPlayerContext));
	}

	virtual ~FUTRichTextLayoutMarshaller() {};

	void SetDisplayMode(ERichTextInlineIconDisplayMode InDisplayMode)
	{
		DisplayMode = InDisplayMode;
	}

	void SetInlineIconTextStyle(const FTextBlockStyle& InTextStyle)
	{
		InlineIconTextStyle = InTextStyle;
	}

	void SetSize(EUTWidgetStyleSize::Type InSize)
	{
		DefaultStyleSize = InSize;
	}

	void SetColorType(EUTTextColor::Type InColorType)
	{
		ColorType = InColorType;
	}

	bool GetHasInputWidget() const
	{
		return bHasInputWidget;
	}

protected:
	FUTRichTextLayoutMarshaller(ERichTextInlineIconDisplayMode InDisplayMode,
		TArray<TSharedRef<ITextDecorator>> InDecorators,
		const ISlateStyle* const InDecoratorStyleSet,
		const FLocalPlayerContext& InPlayerContext)
		: FRichTextLayoutMarshaller(InDecorators, InDecoratorStyleSet)
		, DisplayMode(InDisplayMode)
		, PlayerContext()
	{
		if (InPlayerContext.IsValid())
		{
			PlayerContext = InPlayerContext;
		}
	}

	/** Internal helper enum for creating input widgets */
	enum class EInputWidgetType
	{
		SpecificKey,
		InputAction,
		InputAxis
	};

	/** Helper function that creates an input widget */
	TSharedPtr<SWidget> CreateInputWidget(EInputWidgetType WidgetType, FName TagID)
	{
		const UUTInputDisplayData& InputDisplayData = UUTGameUIData::Get().GetInputDisplayData();
		APlayerController* PC = PlayerContext.IsValid() ? PlayerContext.GetPlayerController() : nullptr;
		TSharedPtr<SWidget> KeyWidget;
		if (PC && PC->PlayerInput)
		{
			if (WidgetType == EInputWidgetType::SpecificKey)
			{
				KeyWidget = InputDisplayData.CreateWidgetForKey(FKey(TagID), DefaultStyleSize);
			}
			else if (WidgetType == EInputWidgetType::InputAction)
			{
				KeyWidget = InputDisplayData.CreateWidgetForAction(TagID, PC, DefaultStyleSize);
			}
			else if (WidgetType == EInputWidgetType::InputAxis)
			{
				KeyWidget = InputDisplayData.CreateWidgetForAxis(TagID, PC, DefaultStyleSize);
			}
		}
#if WITH_EDITOR
		else
		{
			if (WidgetType == EInputWidgetType::InputAxis)
			{
				KeyWidget = InputDisplayData.CreateWidgetForKey(EKeys::Gamepad_LeftStick_Up, DefaultStyleSize);
			}
			else
			{
				KeyWidget = InputDisplayData.CreateWidgetForKey(EKeys::Gamepad_FaceButton_Bottom, DefaultStyleSize);
			}
		}
#endif

		return KeyWidget;
	}

	virtual void AppendRunsForText(
		const int32 LineIndex,
		const FTextRunParseResults& TextRun,
		const FString& ProcessedString,
		const FTextBlockStyle& DefaultTextStyle,
		const TSharedRef<FString>& InOutModelText,
		FTextLayout& TargetTextLayout,
		TArray<TSharedRef<IRun>>& Runs,
		TArray<FTextLineHighlight>& LineHighlights,
		TMap<const FTextBlockStyle*, TSharedPtr<FSlateTextUnderlineLineHighlighter>>& CachedUnderlineHighlighters
		) override
	{
		//reset this each time
		bHasInputWidget = false;

		const FString Period(TEXT("."));
		const UUTRichTextStyleData& RichTextStyleData = UUTGameUIData::Get().GetRichTextStyleData();

		FString TagID;
		if (TextRun.MetaData.Contains(TEXT("id")))
		{
			const FTextRange& IdRange = TextRun.MetaData[TEXT("id")];
			TagID = ProcessedString.Mid(IdRange.BeginIndex, IdRange.EndIndex - IdRange.BeginIndex);
		}

		// If an id was supplied, check for special tag markup
		if (!TagID.IsEmpty())
		{
			//@todo this should work for all inline icons!
			EUTWidgetStyleSize::Type ImageSize = DefaultStyleSize;
			TArray<FString> Tags;
			TextRun.Name.ParseIntoArray(Tags, *Period);
			if (Tags.Num() > 1)
			{
				const FString& SizeString = Tags[1];
				if (SizeString == RichTextStyleData.StyleSizeSubtags[EUTWidgetStyleSize::Small])
				{
					ImageSize = EUTWidgetStyleSize::Small;
				}
				else if (SizeString == RichTextStyleData.StyleSizeSubtags[EUTWidgetStyleSize::Medium])
				{
					ImageSize = EUTWidgetStyleSize::Medium;
				}
				else if (SizeString == RichTextStyleData.StyleSizeSubtags[EUTWidgetStyleSize::Large])
				{
					ImageSize = EUTWidgetStyleSize::Large;
				}
			}

			// CURRENCY
			if (TextRun.Name.Contains(RichTextStyleData.CurrencyMarkupTag))
			{
				EUTCurrencyType CurrencyType = EUTCurrencyType::MAX;
				for (int32 CurrencyIdx = 0; CurrencyIdx < TEnumValue(EUTCurrencyType::MAX); ++CurrencyIdx)
				{
					if (RichTextStyleData.CurrencyTypeIDs[CurrencyIdx] == TagID)
					{
						CurrencyType = (EUTCurrencyType)CurrencyIdx;
						break;
					}
				}

				if (CurrencyType != EUTCurrencyType::MAX)
				{
					uint8 CurrencyByte = TEnumValue(CurrencyType);
					AppendInlineIconBlock(TagID, RichTextStyleData.CurrencyIcons[CurrencyByte], RichTextStyleData.CurrencyNames[CurrencyByte], ImageSize, InOutModelText, Runs);
				}
			}
			else //INPUT
			{
				TSharedPtr<SWidget> KeyWidget;
				if (TextRun.Name.Contains(RichTextStyleData.InputAxisMarkupTag))
				{
					KeyWidget = CreateInputWidget(EInputWidgetType::InputAxis, *TagID);
				}
				else if (TextRun.Name.Contains(RichTextStyleData.InputMarkupTag))
				{
					if (TextRun.Name.Contains(RichTextStyleData.SpecificKeyMarkupSubtag))
					{
						KeyWidget = CreateInputWidget(EInputWidgetType::SpecificKey, *TagID);
					}
					else
					{
						KeyWidget = CreateInputWidget(EInputWidgetType::InputAction, *TagID);
					}
				}

				if (KeyWidget.IsValid())
				{
					//we have an input widget if we get here
					bHasInputWidget = true;

					FTextRange ModelRange;
					ModelRange.BeginIndex = InOutModelText->Len();
					*InOutModelText += TEXT('\x00A0'); // No break space
					ModelRange.EndIndex = InOutModelText->Len();

					Runs.Add(FSlateWidgetRun::Create(
						TargetTextLayout.AsShared(),
						FTextRunInfo(TagID, FText::FromString(TagID)),
						InOutModelText,
						FSlateWidgetRun::FWidgetRunInfo(KeyWidget.ToSharedRef(), RichTextStyleData.InlineIconBaselines[DefaultStyleSize]),
						ModelRange
					));
				}
			}
		}
		else
		{if (TextRun.Name == RichTextStyleData.KeywordMarkupTag)
			{
				// keyword.size.color
				const_cast<FTextRunParseResults*>(&TextRun)->Name += Period + RichTextStyleData.StyleSizeSubtags[DefaultStyleSize] + Period + RichTextStyleData.ColorTypeSubtags[ColorType];
			}

			// Handle other runs as normal rich text
			FRichTextLayoutMarshaller::AppendRunsForText(LineIndex, TextRun, ProcessedString, DefaultTextStyle, InOutModelText, TargetTextLayout, Runs, LineHighlights, CachedUnderlineHighlighters);
		}
	}

private:
	// Potentially the icon, the icon and text, or just the text
	void AppendInlineIconBlock(const FString& TagID,
		UTexture2D* IconTexture,
		const FText& DisplayName,
		EUTWidgetStyleSize::Type Size,
		const TSharedRef<FString>& InOutModelText,
		TArray<TSharedRef<IRun>>& Runs)
	{
		const UUTRichTextStyleData& RichTextStyleData = UUTGameUIData::Get().GetRichTextStyleData();
		bool bShowIcon = DisplayMode == ERichTextInlineIconDisplayMode::IconAndText || DisplayMode == ERichTextInlineIconDisplayMode::IconOnly;
		const bool bShowText = DisplayMode == ERichTextInlineIconDisplayMode::IconAndText || DisplayMode == ERichTextInlineIconDisplayMode::TextOnly;

		if (bShowIcon)
		{	
			if (const FSlateBrush* IconBrush = RichTextStyleData.GetIconBrush(TagID, IconTexture, Size))
			{
				AppendImageRun(*IconBrush, Size, InOutModelText, Runs);
			}
			else
			{
				bShowIcon = false;
			}
		}

		if (bShowText)
		{
			FTextRange ModelRange;
			ModelRange.BeginIndex = InOutModelText->Len();

			if (bShowIcon)
			{
				// If we're showing the icon, put a non-breaking space between the icon and the text
				*InOutModelText += TEXT('\x00A0');
			}
			*InOutModelText += DisplayName.ToString();
			ModelRange.EndIndex = InOutModelText->Len();

			// Create and append a text run for the attribute
			TSharedPtr<ISlateRun> InlineIconTextRun = FSlateTextRun::Create(FRunInfo(), InOutModelText, InlineIconTextStyle, ModelRange);
			Runs.Add(InlineIconTextRun.ToSharedRef());
		}
	}

	void AppendImageRun(const FSlateBrush& Brush, 
		EUTWidgetStyleSize::Type Size,
		const TSharedRef<FString>& InOutModelText,
		TArray<TSharedRef<IRun>>& Runs)
	{
		FTextRange ModelRange;
		ModelRange.BeginIndex = InOutModelText->Len();
		*InOutModelText += TEXT('\x00A0'); // No-break space
		ModelRange.EndIndex = InOutModelText->Len();

		const float Baseline = UUTGameUIData::Get().GetRichTextStyleData().InlineIconBaselines[Size];
		TSharedPtr<ISlateRun> IconRun = FSlateImageRun::Create(FRunInfo(), InOutModelText, &Brush, Baseline, ModelRange);
		Runs.Add(IconRun.ToSharedRef());
	}

	FTextBlockStyle InlineIconTextStyle;
	ERichTextInlineIconDisplayMode DisplayMode;
	EUTWidgetStyleSize::Type DefaultStyleSize;
	EUTTextColor::Type ColorType;
	FLocalPlayerContext PlayerContext;
	bool bHasInputWidget;
};


UUTRichText::UUTRichText(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, StyleSize(EUTWidgetStyleSize::Medium)
	, ColorType(EUTTextColor::Light)
	, InlineIconColorType(EUTTextColor::Light)
{
	static ConstructorHelpers::FClassFinder<UUTTextStyle> BodyTextStyleClassFinder(TEXT("/Game/RestrictedAssets/UI/Text/TextStyle-Body"));
	NormalTextStyle = BodyTextStyleClassFinder.Class;

	static ConstructorHelpers::FClassFinder<UUTTextStyle> KeywordTextStyleClassFinder(TEXT("/Game/RestrictedAssets/UI/Text/TextStyle-Body"));
	InlineIconTextStyle = KeywordTextStyleClassFinder.Class;

	Visiblity_DEPRECATED = Visibility = ESlateVisibility::SelfHitTestInvisible;
}

void UUTRichText::PostLoad()
{
	Super::PostLoad();
}

TSharedRef<SWidget> UUTRichText::RebuildWidget()
{
	TArray<TSharedRef<ITextDecorator>> Decorators;
	
	if (IsDesignTime())
	{
		FUTRichTextStyle::UpdateStyleData();
	}

	if (!Marshaller.IsValid())
	{
		Marshaller = FUTRichTextLayoutMarshaller::Create(InlineIconDisplayMode, Decorators, &FUTRichTextStyle::Get(), IsDesignTime() ? FLocalPlayerContext() : FLocalPlayerContext(GetWorld()->GetFirstLocalPlayerFromController()));
	}

	FTextBlockStyle NormalStyle;
	BuildDefaultTextStyle(NormalStyle);

	TSharedRef<SWidget> RetWidget = SAssignNew(MyRichText, SRichTextBlock)
	.Text(Text)
	.TextStyle(&NormalStyle)
	.Marshaller(Marshaller)
	.WrapTextAt(WrapTextAt)
	.Margin(TextMargin)
	.AutoWrapText(false)
	.MinDesiredWidth(MinDesiredWidth);

	BindToInputChangeDelegates();

	return RetWidget;
}

void UUTRichText::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (Marshaller.IsValid())
	{
		UpdateInlineIconTextStyle();
		Marshaller->SetDisplayMode(InlineIconDisplayMode);
		Marshaller->SetSize(StyleSize);
		Marshaller->SetColorType(ColorType);
	}

	if (MyRichText.IsValid())
	{
		MyRichText->SetText(Text);
	}

	Super::SynchronizeTextLayoutProperties(*MyRichText);
}

void UUTRichText::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	MyRichText.Reset();
	Marshaller.Reset();
}

void UUTRichText::SetText(const FText& InText)
{
	Text = InText;
	if (MyRichText.IsValid())
	{
		MyRichText->SetText(InText);
		BindToInputChangeDelegates();
	}
}

void UUTRichText::SetInlineIconDisplayMode(ERichTextInlineIconDisplayMode DisplayMode)
{
	InlineIconDisplayMode = DisplayMode;
	if (Marshaller.IsValid())
	{
		Marshaller->SetDisplayMode(DisplayMode);
		if (MyRichText.IsValid())
		{
			MyRichText->Refresh();
		}
	}
}

void UUTRichText::SetSize(EUTWidgetStyleSize::Type Size)
{
	StyleSize = Size;
	if (Marshaller.IsValid())
	{
		Marshaller->SetSize(Size);

		UpdateDefaultTextStyle();
		UpdateInlineIconTextStyle();
	}
}

void UUTRichText::SetColorType(EUTTextColor::Type Color)
{
	ColorType = Color;
	if (Marshaller.IsValid())
	{
		Marshaller->SetColorType(Color);

		UpdateDefaultTextStyle();
		UpdateInlineIconTextStyle();
	}
}

EUTWidgetStyleSize::Type UUTRichText::GetStyleSize()
{
	return StyleSize;
}

EUTTextColor::Type UUTRichText::GetColorType()
{
	return ColorType;
}

EUTTextColor::Type UUTRichText::GetInlineIconColorType()
{
	return InlineIconColorType;
}

void UUTRichText::Refresh()
{
	if (MyRichText.IsValid() && Marshaller.IsValid())
	{
		MyRichText->Refresh();
	}
}

void UUTRichText::UpdateInlineIconTextStyle()
{
	if (Marshaller.IsValid())
	{
		FTextBlockStyle InlineIconStyle;
		UUTTextStyle* InlineStyleCDO = InlineIconTextStyle ? Cast<UUTTextStyle>(InlineIconTextStyle->ClassDefaultObject) : nullptr;
		if (InlineStyleCDO)
		{
			InlineIconStyle.SetFont(InlineStyleCDO->Font[StyleSize]).SetColorAndOpacity(InlineStyleCDO->Color[InlineIconColorType]);
		}

		Marshaller->SetInlineIconTextStyle(InlineIconStyle);
	}
}

void UUTRichText::BuildDefaultTextStyle(FTextBlockStyle& Style)
{
	FTextBlockStyle NormalStyle;
	UUTTextStyle* NormalStyleCDO = NormalTextStyle ? Cast<UUTTextStyle>(NormalTextStyle->ClassDefaultObject) : nullptr;
	if (NormalStyleCDO)
	{
		Style.SetFont(NormalStyleCDO->Font[StyleSize]).SetColorAndOpacity(NormalStyleCDO->Color[ColorType]);
	}
}

void UUTRichText::UpdateDefaultTextStyle()
{
	if (MyRichText.IsValid())
	{
		FTextBlockStyle NormalStyle;
		BuildDefaultTextStyle(NormalStyle);
		MyRichText->SetTextStyle(NormalStyle);
	}
}

void UUTRichText::BindToInputChangeDelegates()
{
	// if we have a world, and a marshaller, and that marshaller says we have an input widget
	// in the string, then bind to these two delegates
	UWorld* World = GetWorld();
	if (World && !IsDesignTime() && Marshaller.IsValid() && Marshaller->GetHasInputWidget())
	{
		/*
		if (UAnalogCursorContext* AnalogCon = UAnalogCursorContext::GetCurrent(World))
		{
			AnalogCon->NativeOnUsingGamepad.AddUObject(this, &ThisClass::OnUsingGamepad);
		}

		if (UHUDContext* HUDCon = UHUDContext::GetCurrent(World))
		{
			HUDCon->GetOnControlsChangedDelegate().AddUObject(this, &ThisClass::OnControlsChanged);
		}
		*/
	}
}

void UUTRichText::OnUsingGamepad(bool bUsingGamepad)
{
	Refresh();
}

void UUTRichText::OnControlsChanged(EUTInputType ControlsType)
{
	Refresh();
}
