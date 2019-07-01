// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTBlurWidget.h"
#include "Slate/SlateBrushAsset.h"

// todo PLK: waiting on next engine merge
/*
class SUTBackgroundBlur : public SBackgroundBlurWidget
{
public:
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyClippingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		if (IsUsingLowQualityFallbackBrush() && LowQualityFallbackBrush)
		{
			// Exceedingly hacky workaround to prevent showing the fallback brush when the blur strength is 0
			const_cast<FSlateBrush*>(LowQualityFallbackBrush)->DrawAs = (BlurStrength.Get() <= 0.f) ? ESlateBrushDrawType::NoDrawType : ESlateBrushDrawType::Image;
		}

		FWidgetStyle AdjustedStyle = InWidgetStyle;
		AdjustedStyle.BlendColorAndOpacityTint(ColorAndOpacity);
		return SBackgroundBlur::OnPaint(Args, AllottedGeometry, MyClippingRect, OutDrawElements, LayerId, AdjustedStyle, bParentEnabled);
	}

	void SetColorAndOpacity(const FLinearColor& InColorAndOpacity)
	{
		ColorAndOpacity = InColorAndOpacity;
	}

private:
	FLinearColor ColorAndOpacity;
};
*/

UUTBlurWidget::UUTBlurWidget(const FObjectInitializer& Initializer)
	: Super(Initializer)
	, ColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 1.f))
{
	// todo PLK - add default blur brush
	/*
	// Set the default fallback brush
	static ConstructorHelpers::FObjectFinder<USlateBrushAsset> DefaultFallbackBrushFinder(TEXT("/Game/RestrictedAssets/UI/Textures/BlurFrame_Default_LowBrush"));
	if (DefaultFallbackBrushFinder.Succeeded())
	{
		LowQualityFallbackBrush = DefaultFallbackBrushFinder.Object->Brush;
	}
	*/
}

void UUTBlurWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	MyUTBlur.Reset();
}

// todo PLK: Waiting for the latest background blur widget in our depot
/*
TSharedRef<SWidget> UUTBlurWidget::RebuildWidget()
{
	// Identical to BackgroundBlur::RebuildWidget, except that we're making an UTBackgroundBlur
	MyBackgroundBlur = MyUTBlur = SNew(SUTBackgroundBlur);

	if (GetChildrenCount() > 0)
	{
		//Cast<UBackgroundBlurSlot>(GetContentSlot())->BuildSlot(MyBackgroundBlur.ToSharedRef());
		MyBackgroundBlur->SetContent(GetContentSlot()->Content ? GetContentSlot()->Content->TakeWidget() : SNullWidget::NullWidget);
	}

	return BuildDesignTimeWidget(MyBackgroundBlur.ToSharedRef());
}
*/

void UUTBlurWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	/*
	if (MyUTBlur.IsValid())
	{
		MyUTBlur->SetColorAndOpacity(ColorAndOpacity);
	}
	*/
}

void UUTBlurWidget::SetColorAndOpacity(FLinearColor InColorAndOpacity)
{
	ColorAndOpacity = InColorAndOpacity;
	/*
	if (MyUTBlur.IsValid())
	{
		MyUTBlur->SetColorAndOpacity(InColorAndOpacity);
	}*/
}
