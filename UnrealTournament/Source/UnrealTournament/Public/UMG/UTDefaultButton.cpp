// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"

#include "UTDefaultButton.h"
#include "UTTextBlock.h"
#include "UTInputVisualizer.h"

UUTDefaultButton::UUTDefaultButton( const FObjectInitializer& Initializer )
    : Super( Initializer )
	, MinContentSpacing(48.f)
	, WrapLabelTextAt(0)
	, NormalTextColor(EUTTextColor::Emphasis)
	, HoveredTextColor(EUTTextColor::Light)
	, TextAlignment_NoIcon(HAlign_Center)
	, TextAlignment_WithIcon(HAlign_Left)
	, IconAlignment(HAlign_Right)
	, ContentAlignment(HAlign_Fill)
	, bIconOnRight(true)
{
	// todo PLK - hook up when button styles come online
	//static ConstructorHelpers::FClassFinder<UUTButtonStyle> DefaultStyleClassFinder(TEXT("/Game/UI/Button/Styles/ButtonStyle-Outline"));
	//Style = DefaultStyleClassFinder.Class;

	IconSizes[EUTWidgetStyleSize::Small] = 48.f;
	IconSizes[EUTWidgetStyleSize::Medium] = 60.f;
	IconSizes[EUTWidgetStyleSize::Large] = 72.f;

	IconBrush.DrawAs = ESlateBrushDrawType::NoDrawType;
}

void UUTDefaultButton::BindClickToKey(const FKey& KeyToBind)
{
	Super::BindClickToKey(KeyToBind);

	Input_BoundKey->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	Input_BoundKey->ShowSpecificKey(KeyToBind);
}

void UUTDefaultButton::Init(const FText& InText, UTexture2D* Texture, EUTWidgetStyleSize::Type InStyleSize, int32 InMinWidth, int32 InMinHeight)
{
	Text = InText;
	Text_Label->SetText(InText);

	Image_Icon->SetBrushFromTexture(Texture);
	Image_Icon->Brush.ImageSize.X = Image_Icon->Brush.ImageSize.Y = IconSizes[InStyleSize];
	Image_Icon->Brush.DrawAs = Texture ? ESlateBrushDrawType::Image : ESlateBrushDrawType::NoDrawType;

	StyleSize = InStyleSize;
	MinWidth = InMinWidth;
	MinHeight = InMinHeight;

	Input_BoundKey->SetSize(InStyleSize);
	Text_Label->SetSize(InStyleSize);
	RefreshLayout();
}

void UUTDefaultButton::NativePreConstruct()
{
	Super::NativePreConstruct();

	bool bIsValid = !IsDesignTime();
	if (!bIsValid)
	{
		// Take extra precaution at design time to make sure these widgets are all bound
		bIsValid = SizeBox_Container && HBox_Content && Image_Icon && Text_Label && Input_BoundKey;
	}
	
	if (bIsValid)
	{
		if (TSubclassOf<UUTTextStyle> StyleClass = GetCurrentTextStyleClass())
		{
			Text_Label->SetProperties(StyleClass, StyleSize, NormalTextColor, WrapLabelTextAt);
			SetContentColor(NormalTextColor);
		}

		if (bIconOnRight)
		{
			Image_Icon->RemoveFromParent();
			HBox_Content->AddChildToHorizontalBox(Image_Icon);
		}

		Input_BoundKey->ShowSpecificKey(BoundKey);
		Input_BoundKey->SetVisibility(BoundKey.IsValid() ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

		Text_Label->SetText(Text);
		UpdateIcon(IconBrush);
	}
}

void UUTDefaultButton::NativeOnSelected(bool bBroadcast)
{
	if (TSubclassOf<UUTTextStyle> StyleClass = GetCurrentTextStyleClass())
	{
		Text_Label->SetStyle(StyleClass);
	}

	Super::NativeOnSelected(bBroadcast);
}

void UUTDefaultButton::NativeOnDeselected(bool bBroadcast)
{
	if (TSubclassOf<UUTTextStyle> StyleClass = GetCurrentTextStyleClass())
	{
		Text_Label->SetStyle(StyleClass);
	}

	Super::NativeOnDeselected(bBroadcast);
}

void UUTDefaultButton::NativeOnStyleSizeChanged()
{
	Super::NativeOnStyleSizeChanged();
	RefreshLayout();
}

void UUTDefaultButton::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (IsInteractionEnabled())
	{
		SetContentColor(HoveredTextColor);
	}

	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
}

void UUTDefaultButton::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	SetContentColor(NormalTextColor);

	Super::NativeOnMouseLeave(InMouseEvent);
}

void UUTDefaultButton::UpdateText(const FText& InText)
{
	Text = InText;
	Text_Label->SetText(InText);
	
	RefreshLayout();
}

void UUTDefaultButton::UpdateIcon(const FSlateBrush& InIconBrush, bool bRetainBrushImageSize)
{
	// Set the brush and override the size
	Image_Icon->SetBrush(InIconBrush);
	
	if (!bRetainBrushImageSize)
	{
		Image_Icon->Brush.ImageSize.X = Image_Icon->Brush.ImageSize.Y = IconSizes[StyleSize];
	}

	RefreshLayout();
}

void UUTDefaultButton::UpdateIconFromTexture(UTexture2D* Texture)
{
	Image_Icon->SetBrushFromTexture(Texture);
	Image_Icon->Brush.ImageSize.X = Image_Icon->Brush.ImageSize.Y = IconSizes[StyleSize];
	Image_Icon->Brush.DrawAs = Texture ? ESlateBrushDrawType::Image : ESlateBrushDrawType::NoDrawType;
	RefreshLayout();
}

void UUTDefaultButton::UpdateSpecificImageSize(EUTWidgetStyleSize::Type Type, float InSize)
{
	if (Type < EUTWidgetStyleSize::MAX)
	{
		IconSizes[Type] = InSize;

		if (Image_Icon->Brush.GetResourceObject())
		{
			UpdateIconFromTexture(Cast<UTexture2D>(Image_Icon->Brush.GetResourceObject()));
		}
	}
}

void UUTDefaultButton::RefreshLayout()
{
	const bool bHasIcon = Image_Icon->Brush.DrawAs != ESlateBrushDrawType::NoDrawType;
	const bool bHasText = !Text.IsEmpty();

	Image_Icon->SetVisibility(bHasIcon ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	Text_Label->SetVisibility(bHasText ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);

	if (UHorizontalBoxSlot* TextSlot = Cast<UHorizontalBoxSlot>(Text_Label->Slot))
	{
		TextSlot->SetVerticalAlignment(VAlign_Center);
		TextSlot->SetHorizontalAlignment(bHasIcon ? TextAlignment_WithIcon : TextAlignment_NoIcon);
		TextSlot->SetSize(FSlateChildSize(bHasText ? ESlateSizeRule::Fill : ESlateSizeRule::Automatic));
		
		if (bHasIcon)
		{
			TextSlot->SetPadding(FMargin(bIconOnRight ? 0.f : MinContentSpacing, 0.f, bIconOnRight ? MinContentSpacing : 0.f, 0.f));
		}
		else
		{
			TextSlot->SetPadding(FMargin(0.f));
		}
	}
	if (UHorizontalBoxSlot* ImageSlot = Cast<UHorizontalBoxSlot>(Image_Icon->Slot))
	{
		ImageSlot->SetVerticalAlignment(VAlign_Center);
		ImageSlot->SetHorizontalAlignment(IconAlignment);
		ImageSlot->SetSize(FSlateChildSize(bHasText ? ESlateSizeRule::Automatic : ESlateSizeRule::Fill));
	}

	if (USizeBoxSlot* ContentSlot = Cast<USizeBoxSlot>(HBox_Content->Slot))
	{
		FMargin CustomPadding;
		GetCurrentCustomPadding(CustomPadding);

		ContentSlot->SetHorizontalAlignment(ContentAlignment);
		ContentSlot->SetPadding(CustomPadding);
	}

	Input_BoundKey->SetSize(StyleSize);

	RefreshDimensions();
}

void UUTDefaultButton::SetNormalContentColor(EUTTextColor::Type InNormalTextColor)
{
	NormalTextColor = InNormalTextColor;
	if (!IsHovered())
	{
		SetContentColor(InNormalTextColor);
	}
}

void UUTDefaultButton::SetHoveredContentColor(EUTTextColor::Type InHoveredTextColor)
{
	HoveredTextColor = InHoveredTextColor;
	if (IsHovered())
	{
		SetContentColor(InHoveredTextColor);
	}
}

void UUTDefaultButton::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	if (IsDesignTime())
	{
		NativePreConstruct();
	}
}

void UUTDefaultButton::SetContentColor(EUTTextColor::Type ColorType)
{
	Text_Label->SetColorType(ColorType);
	
	if (UUTTextStyle* TextStyle = GetCurrentTextStyle())
	{
		FLinearColor TextColor;
		TextStyle->GetColor(ColorType, TextColor);
		Image_Icon->SetColorAndOpacity(TextColor);
	}
}
