// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTTooltipBase.h"

#include "UTGameUIData.h"

UUTTooltipBase::UUTTooltipBase(const FObjectInitializer& Initializer) 
	: Super(Initializer)
	, bIsFading(false)
{
	bEnableUTTooltip = false;
	Visibility = ESlateVisibility::HitTestInvisible;
}

void UUTTooltipBase::NativeConstruct()
{
	Super::NativeConstruct();

	FadeSequence.AddCurve(0.f, 0.15f);
	UpdateFadeAnim();
}

void UUTTooltipBase::NativeDestruct()
{
	Super::NativeDestruct();

	bIsFading = false;
	FadeSequence = FCurveSequence();
}

void UUTTooltipBase::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bIsFading)
	{
		UpdateFadeAnim();
		bIsFading = FadeSequence.IsPlaying();
	}
}

void UUTTooltipBase::Show()
{
	bIsFading = true;
	FadeSequence.Play(GetCachedWidget().ToSharedRef());

	NativeOnShow();
}

void UUTTooltipBase::Hide()
{
	bIsFading = false;
	FadeSequence.JumpToStart();
	FadeSequence.Pause();
	UpdateFadeAnim();

	NativeOnHide();
}

void UUTTooltipBase::NativeOnShow()
{
	OnShow();
}

void UUTTooltipBase::NativeOnHide()
{
	OnHide();
}

void UUTTooltipBase::UpdateFadeAnim()
{
	const float FadeAlpha = FadeSequence.GetLerp();

	FLinearColor MyColor = ColorAndOpacity;
	MyColor.A = FadeAlpha;
	SetColorAndOpacity(MyColor);

	SetRenderTranslation(FVector2D(0.f, 75.f - (75.f * FadeAlpha)));
}

void UUTTooltipBase::ShowImmediatelyForExport()
{
	FadeSequence.JumpToEnd();
	SetColorAndOpacity(FLinearColor::White);
	SetRenderTranslation(FVector2D::ZeroVector);
}

