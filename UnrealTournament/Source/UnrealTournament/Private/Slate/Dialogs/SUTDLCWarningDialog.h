// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "SlateBasics.h"
#include "../Base/SUTDialogBase.h"

#if !UE_SERVER

class UNREALTOURNAMENT_API SUTDLCWarningDialog : public SUTDialogBase
{
	SLATE_BEGIN_ARGS(SUTDLCWarningDialog)
	: _DialogSize(FVector2D(700,400))
	, _bDialogSizeIsRelative(false)
	, _DialogPosition(FVector2D(0.5f,0.5f))
	, _DialogAnchorPoint(FVector2D(0.5f,0.5f))
	, _ContentPadding(FVector2D(10.0f, 5.0f))
	
	{}
	SLATE_ARGUMENT(TWeakObjectPtr<class UUTLocalPlayer>, PlayerOwner)			
	SLATE_ARGUMENT(FVector2D, DialogSize)										
	SLATE_ARGUMENT(bool, bDialogSizeIsRelative)									
	SLATE_ARGUMENT(FVector2D, DialogPosition)									
	SLATE_ARGUMENT(FVector2D, DialogAnchorPoint)								
	SLATE_ARGUMENT(FVector2D, ContentPadding)									
	SLATE_EVENT(FDialogResultDelegate, OnDialogResult)		
	SLATE_ARGUMENT(FText, MessageText)

	SLATE_END_ARGS()

	/** needed for every widget */
	void Construct(const FArguments& InArgs);

	virtual TSharedRef<class SWidget> BuildCustomButtonBar() override;
private:

public:
	virtual bool bRemainOpenThroughTravel()
	{
		return false;
	}

	FReply ReportClicked();
};

#endif