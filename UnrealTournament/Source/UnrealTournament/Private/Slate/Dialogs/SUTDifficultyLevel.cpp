// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "SUTDifficultyLevel.h"
#include "../SUTStyle.h"
#include "../Widgets/SUTButton.h"
#include "../Widgets/SUTImage.h"
#include "SlateBasics.h"
#include "SlateExtras.h"

#if !UE_SERVER

void SUTDifficultyLevel::Construct(const FArguments& InArgs)
{
	// These must be set before the parent Construct is called.

	// Let the Dialog construct itself.
	SUTDialogBase::Construct(SUTDialogBase::FArguments()
							.PlayerOwner(InArgs._PlayerOwner)
							.DialogTitle(FText::GetEmpty())
							.DialogSize(InArgs._DialogSize)
							.bDialogSizeIsRelative(InArgs._bDialogSizeIsRelative)
							.DialogPosition(InArgs._DialogPosition)
							.DialogAnchorPoint(InArgs._DialogAnchorPoint)
							.ContentPadding(InArgs._ContentPadding)
							.ButtonMask(InArgs._ButtonMask)
							.OnDialogResult(InArgs._OnDialogResult)
						);



	// At this point, the DialogContent should be ready to have slots added.
	if (DialogContent.IsValid())
	{
		const float MessageTextPaddingX = 50.0f;
		TSharedPtr<SRichTextBlock> MessageTextBlock;
		DialogContent->AddSlot()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			.AutoHeight().Padding(0.0f,20.0f,0.0f,10.0f)
			[			
				SNew(STextBlock)
				.Text(NSLOCTEXT("SUTDifficultyLevel","DifficultyMsg","Choose the difficulty level of the AI you will fight against."))
				.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Tween")
				.Justification(ETextJustify::Center)
				.AutoWrapText(true)
			]
			+SVerticalBox::Slot()
			.AutoHeight().HAlign(HAlign_Center).AutoHeight().Padding(0.0f,10.0f,0.0f,0.0f)
			[
				SNew(SHorizontalBox)

				// Easy
				+SHorizontalBox::Slot().AutoWidth().Padding(10.0f,0.0f,10.0f,0.0f)
				[
					SNew(SBox).WidthOverride(250).HeightOverride(250).VAlign(VAlign_Fill).HAlign(HAlign_Fill)
					[
						SNew(SUTButton)
						.ButtonStyle(SUTStyle::Get(), "UT.WeaponConfig.Button")
						.bSpringButton(true)
						.OnClicked(this, &SUTDifficultyLevel::PickDifficulty, 0)
						.ContentPadding(FMargin(2.0f, 2.0f, 2.0f, 2.0f))
						[
							SNew(SBorder)
							.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot().HAlign(HAlign_Center).AutoHeight().Padding(0.0f,20.0f,0.0f,0.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("NORMAL")))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")

								]
								+SVerticalBox::Slot().HAlign(HAlign_Center).AutoHeight().Padding(0.0f,20.0f,0.0f,0.0f)
								[
									SNew(SHorizontalBox)
									+SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SUTImage)
										.Image(SUTStyle::Get().GetBrush("UT.Icon.Skull.128x128"))
										.WidthOverride(80)
										.HeightOverride(80)
									]
								]
							]
						]
					]
				]

				// Normal
				+SHorizontalBox::Slot().AutoWidth().Padding(5.0f,0.0f,5.0f,0.0f)
				[
					SNew(SBox).WidthOverride(250).HeightOverride(250).VAlign(VAlign_Fill).HAlign(HAlign_Fill)
					[
						SNew(SUTButton)
						.ButtonStyle(SUTStyle::Get(), "UT.HomePanel.Button")
						.bSpringButton(true)
						.OnClicked(this, &SUTDifficultyLevel::PickDifficulty, 1)
						.ContentPadding(FMargin(2.0f, 2.0f, 2.0f, 2.0f))
						[
							SNew(SBorder)
							.BorderImage(SUTStyle::Get().GetBrush("UT.HeaderBackground.Dark"))
							[
								SNew(SVerticalBox)
								+SVerticalBox::Slot().HAlign(HAlign_Center).AutoHeight().Padding(0.0f,20.0f,0.0f,0.0f)
								[
									SNew(STextBlock)
									.Text(FText::FromString(TEXT("HARD")))
									.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")

								]
								+SVerticalBox::Slot().HAlign(HAlign_Center).AutoHeight().Padding(0.0f,20.0f,0.0f,0.0f)
								[
									SNew(SHorizontalBox)
									+SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SUTImage)
										.Image(SUTStyle::Get().GetBrush("UT.Icon.Skull.128x128"))
										.WidthOverride(80)
										.HeightOverride(80)
									]
									+SHorizontalBox::Slot()
									.AutoWidth()
									[
										SNew(SUTImage)
										.Image(SUTStyle::Get().GetBrush("UT.Icon.Skull.128x128"))
										.WidthOverride(80)
										.HeightOverride(80)
									]
								]
							]
						]
					]
				]
			]
		];
	}
}

FReply SUTDifficultyLevel::PickDifficulty(int32 Value)
{
	FinalDifficulty = Value;
	return OnButtonClick(UTDIALOG_BUTTON_OK);
}


#endif