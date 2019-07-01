// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"

#include "UTTextBlock.h"

void UUTTextStyle::GetFont(EUTWidgetStyleSize::Type Size, FSlateFontInfo& OutFont) const
{
	OutFont = Font[Size];
}

void UUTTextStyle::GetColor(EUTTextColor::Type ColorType, FLinearColor& OutColor) const
{
	OutColor = Color[ColorType];
}

void UUTTextStyle::GetMargin(EUTWidgetStyleSize::Type Size, FMargin& OutMargin) const
{
	OutMargin = Margin[Size];
}

float UUTTextStyle::GetLineHeightPercentage(EUTWidgetStyleSize::Type Size) const
{
	return LineHeightPercentage[Size];
}

void UUTTextStyle::GetShadowOffset(EUTWidgetStyleSize::Type Size, FVector2D& OutShadowOffset) const
{
	OutShadowOffset = ShadowOffset[Size];
}

void UUTTextStyle::GetShadowColor(EUTTextColor::Type ColorType, FLinearColor& OutColor) const
{
	OutColor = ShadowColor[ColorType];
}

void UUTTextStyle::ToTextBlockStyle(FTextBlockStyle& OutTextBlockStyle, EUTWidgetStyleSize::Type Size, EUTTextColor::Type ColorType)
{
	OutTextBlockStyle.Font = Font[Size];
	OutTextBlockStyle.ColorAndOpacity = Color[ColorType];
}

UUTTextBlock::UUTTextBlock(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, StyleSize(EUTWidgetStyleSize::Medium)
	, bUseDropShadow(false)
	, bAllCaps(false)
	, bScroll(false)
	, Speed(20.f)
	, StartDelay(2.f)
	, EndDelay(2.f)
	, FadeInDelay(0.5f)
	, FadeOutDelay(0.5f)
	, ActiveState(EStart)
	, TimeElapsed(0.f)
	, ScrollOffset(0.f)
	, FontAlpha(1.f)
	, bPlaying(true)
{
	static ConstructorHelpers::FClassFinder<UUTTextStyle> BaseTextStyleClassFinder(TEXT("/Game/RestrictedAssets/UI/Text/TextStyle-Body"));
	Style = BaseTextStyleClassFinder.Class;

	Visiblity_DEPRECATED = Visibility = ESlateVisibility::SelfHitTestInvisible;
}

void UUTTextBlock::SetWrapTextWidth(int32 InWrapTextAt)
{
	WrapTextAt = InWrapTextAt;
	SynchronizeProperties();
}

void UUTTextBlock::SetStyle(TSubclassOf<UUTTextStyle> InStyle)
{
	Style = InStyle;
	SynchronizeProperties();
}

void UUTTextBlock::SetSize(EUTWidgetStyleSize::Type Size)
{
	StyleSize = Size;
	SynchronizeProperties();
}

void UUTTextBlock::SetColorType(EUTTextColor::Type Color)
{
	ColorType = Color;
	SynchronizeProperties();
}

void UUTTextBlock::SetUseDropShadow(bool bShouldUseDropShadow)
{
	bUseDropShadow = bShouldUseDropShadow;
	SynchronizeProperties();
}

void UUTTextBlock::SetProperties(TSubclassOf<UUTTextStyle> InStyle,
	EUTWidgetStyleSize::Type Size,
	EUTTextColor::Type Color,
	int32 InWrapTextAt,
	bool bShouldUseDropShadow)
{
	Style          = InStyle;
	StyleSize      = Size;
	ColorType      = Color;
	WrapTextAt	   = InWrapTextAt;
	bUseDropShadow = bShouldUseDropShadow;
	SynchronizeProperties();
}

void UUTTextBlock::SynchronizeProperties()
{
	if (const UUTTextStyle* TextStyle = GetStyleCDO())
	{
		// Update our styling to match the assigned style
		Font                 = TextStyle->Font[StyleSize];
		Margin               = TextStyle->Margin[StyleSize];
		LineHeightPercentage = TextStyle->LineHeightPercentage[StyleSize];

		if (ColorType != EUTTextColor::Custom)
		{
			ColorAndOpacity      = TextStyle->Color[ColorType];
		}

		if (bUseDropShadow)
		{
			ShadowOffset          = TextStyle->ShadowOffset[StyleSize];
			ShadowColorAndOpacity = TextStyle->ShadowColor[ColorType];
		}
		else
		{
			ShadowOffset          = FVector2D::ZeroVector;
			ShadowColorAndOpacity = FLinearColor::Transparent;
		}
	}

	Super::SynchronizeProperties();
}

TSharedRef<SWidget> UUTTextBlock::RebuildWidget()
{
	if (!bScroll)
	{
		return Super::RebuildWidget();
	}

	// clang-format off
	ScrollBar = SNew(SScrollBar);
	ScrollBox = SNew(SScrollBox)
		.ScrollBarVisibility(EVisibility::Collapsed)
		.Orientation(Orient_Horizontal)
		.ExternalScrollbar(ScrollBar)
		+ SScrollBox::Slot()
		[
			Super::RebuildWidget()
		];
	// clang-format on

	TickHandle = FTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &ThisClass::OnTick));
	return BuildDesignTimeWidget(ScrollBox.ToSharedRef());
}

TAttribute<FText> UUTTextBlock::GetDisplayText()
{
	TAttribute<FText> TextBinding = Super::GetDisplayText();
	if (bAllCaps)
	{
		if (TextBinding.IsBound())
		{
			UE_LOG(UT, Warning, TEXT("Text Block '%s' is dynamically bound to text that is set to be transformed each frame. This may have serious performance penalties!"), *GetPathName());

			TAttribute<FText> InternalTextBinding = TextBinding;
			TextBinding.Bind(TAttribute<FText>::FGetter::CreateLambda([InternalTextBinding]()
			{
				return InternalTextBinding.Get(FText::GetEmpty()).ToUpper();
			}));
		}
		else
		{
			TextBinding = TextBinding.Get(FText::GetEmpty()).ToUpper();
		}
	}
	return TextBinding;
}

const UUTTextStyle* UUTTextBlock::GetStyleCDO() const
{
	if (Style)
	{
		if (const UUTTextStyle* TextStyle = Cast<UUTTextStyle>(Style->ClassDefaultObject))
		{
			return TextStyle;
		}
	}
	return nullptr;
}

bool UUTTextBlock::OnTick(float Delta)
{
	if (!bPlaying)
	{
		return true;
	}

	const float ContentSize = ScrollBox->GetDesiredSize().X;
	TimeElapsed += Delta;

	switch (ActiveState)
	{
	case EFadeIn:
	{
		if (!ScrollBar->IsNeeded())
		{
			FontAlpha = 1.f;
			break;
		}
		else
		{
			FontAlpha = FMath::Clamp<float>(TimeElapsed / FadeInDelay, 0.f, 1.f);
			if (TimeElapsed >= FadeInDelay)
			{
				FontAlpha = 1.f;
				TimeElapsed = 0.f;
				ScrollOffset = 0.f;
				ActiveState = EStart;
			}
		}
		break;
	}
	case EStart:
	{
		if (!ScrollBar->IsNeeded())
		{
			break;
		}
		else
		{
			TimeElapsed = 0.f;
			ScrollOffset = 0.f;
			ActiveState = EStartWait;
		}
		break;
	}
	case EStartWait:
	{
		if (TimeElapsed >= StartDelay)
		{
			TimeElapsed = 0.f;
			ScrollOffset = 0.f;
			ActiveState = EScrolling;
		}
		break;
	}
	case EScrolling:
	{
		ScrollOffset += Speed * Delta;
		if (FMath::IsNearlyZero(ScrollBar->DistanceFromBottom()))
		{
			ScrollOffset = ContentSize;
			TimeElapsed  = 0.0f;
			ActiveState  = EStop;
		}
		break;
	}
	case EStop:
	{
		if (!ScrollBar->IsNeeded())
		{
			break;
		}
		else
		{
			TimeElapsed = 0.f;
			ActiveState = EStopWait;
		}
		break;
	}
	case EStopWait:
	{
		if (TimeElapsed >= EndDelay)
		{
			TimeElapsed = 0.f;
			ActiveState = EFadeOut;
		}
		break;
	}
	case EFadeOut:
	{
		if (!ScrollBar->IsNeeded())
		{
			FontAlpha = 1.f;
			break;
		}
		else
		{
			FontAlpha = 1.0f - FMath::Clamp<float>(TimeElapsed / FadeOutDelay, 0.f, 1.f);
			if (TimeElapsed >= FadeOutDelay)
			{
				FontAlpha = 0.0f;
				TimeElapsed = 0.0f;
				ScrollOffset = 0.0f;
				ActiveState = EFadeIn;
			}
		}
		break;
	}
	}

	if (ScrollBox.IsValid())
	{
		ScrollBox->SetScrollOffset(ScrollOffset);
	}

	if (MyTextBlock.IsValid())
	{
		const FLinearColor& Color = ColorAndOpacity.GetSpecifiedColor();
		MyTextBlock->SetColorAndOpacity(FSlateColor(FLinearColor(Color.R, Color.G, Color.B, Color.A * FontAlpha)));
	}

	return true;
}
