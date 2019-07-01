// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "SUTWaitingForListenServerDialog.h"
#include "../SUWindowsStyle.h"
#include "SlateBasics.h"
#include "SlateExtras.h"

#if !UE_SERVER

void SUTWaitingForListenServerDialog::Construct(const FArguments& InArgs)
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
			.AutoHeight()
			[			
				SAssignNew(MessageTextBlock, SRichTextBlock)
				.TextStyle(SUWindowsStyle::Get(), *InArgs._MessageTextStyleName)
				.Justification(ETextJustify::Center)
				.DecoratorStyleSet(&SUWindowsStyle::Get())
				.AutoWrapText(true)
				.Text(NSLOCTEXT("SUTWaitingForListenServerDialog","Text","Waiting for your server to start..."))
			]
			+SVerticalBox::Slot()
			.AutoHeight().HAlign(HAlign_Center)
			[
				SNew(SThrobber)
			]
		];
	}
}

#endif