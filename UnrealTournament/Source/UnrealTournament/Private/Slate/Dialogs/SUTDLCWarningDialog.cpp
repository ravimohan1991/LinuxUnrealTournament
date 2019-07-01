// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLocalPlayer.h"
#include "SUTDLCWarningDialog.h"
#include "../SUTStyle.h"
#include "../SUWindowsStyle.h"
#include "UTAnalytics.h"
#include "UTOnlineGameSettingsBase.h"

#if !UE_SERVER

void SUTDLCWarningDialog::Construct(const FArguments& InArgs)
{
	// Let the Dialog construct itself.
	SUTDialogBase::Construct(SUTDialogBase::FArguments()
							.PlayerOwner(InArgs._PlayerOwner)
							.DialogTitle(NSLOCTEXT("SUTDLCWarningDialog","Title","Downloadable Content Warning!"))
							.DialogSize(InArgs._DialogSize)
							.bDialogSizeIsRelative(InArgs._bDialogSizeIsRelative)
							.DialogPosition(InArgs._DialogPosition)
							.DialogAnchorPoint(InArgs._DialogAnchorPoint)
							.ContentPadding(InArgs._ContentPadding)
							.ButtonMask(UTDIALOG_BUTTON_YES + UTDIALOG_BUTTON_NO)
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
				SNew(SRichTextBlock)
				.TextStyle(SUTStyle::Get(), "UT.Font.NormalText.Medium")
				.Justification(ETextJustify::Center)
				.DecoratorStyleSet(&SUTStyle::Get())
				.AutoWrapText(true)
				.Text(InArgs._MessageText)
			]
		];
	}
}

TSharedRef<SWidget> SUTDLCWarningDialog::BuildCustomButtonBar()
{
	return 
		SNew(SBox)
		.HeightOverride(48)
		[
			SNew(SButton)
			.HAlign(HAlign_Center)
			.ButtonStyle(SUWindowsStyle::Get(), "UT.BottomMenu.Button")
			.ContentPadding(FMargin(5.0f, 5.0f, 5.0f, 5.0f))
			.Text(NSLOCTEXT("SUTDLCWarningDialog","Report","Report server to Epic..."))
			.TextStyle(SUWindowsStyle::Get(), "UT.TopMenu.Button.SmallTextStyle")
			.OnClicked(this, &SUTDLCWarningDialog::ReportClicked)
		];
}

FReply SUTDLCWarningDialog::ReportClicked()
{
	if (FUTAnalytics::IsAvailable())
	{

		// Look at the trust level of the current session
		IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
		if (OnlineSub)
		{
			IOnlineSessionPtr SessionInterface = OnlineSub->GetSessionInterface();
			FOnlineSessionSettings* Settings = SessionInterface->GetSessionSettings(TEXT("Game"));

			if (Settings != nullptr)
			{
				int32 TrustLevel;
				Settings->Get(SETTING_TRUSTLEVEL, TrustLevel);

				FString ServerName;
				Settings->Get(SETTING_SERVERNAME, ServerName);

				FString InstanceGUID;
				Settings->Get(SETTING_SERVERINSTANCEGUID, InstanceGUID);

				FString ConnectionString;
				SessionInterface->GetResolvedConnectString(TEXT("Game"), ConnectionString);


				TArray<FAnalyticsEventAttribute> ParamArray;
				ParamArray.Add(FAnalyticsEventAttribute(TEXT("TrustLevel"), FString::Printf(TEXT("%i"),TrustLevel)));
				ParamArray.Add(FAnalyticsEventAttribute(TEXT("ServerName"), ServerName));
				ParamArray.Add(FAnalyticsEventAttribute(TEXT("InstanceGUID"), InstanceGUID));
				ParamArray.Add(FAnalyticsEventAttribute(TEXT("ConnectionString"), ConnectionString));
				FUTAnalytics::SetClientInitialParameters(Cast<AUTBasePlayerController>(PlayerOwner->PlayerController), ParamArray, false);
				FUTAnalytics::GetProvider().RecordEvent( TEXT("ServerAbuseReport"), ParamArray );
			}
		}
	}

	PlayerOwner->ReturnToMainMenu();
	return OnButtonClick(UTDIALOG_BUTTON_CANCEL);
}


#endif