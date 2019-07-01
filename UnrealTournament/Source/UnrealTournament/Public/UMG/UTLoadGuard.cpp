// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTLoadGuard.h"
#include "UTGameUIData.h"

#include "UTTextBlock.h"

UUTLoadGuard::UUTLoadGuard(const FObjectInitializer& Initializer)
	: Super(Initializer)
	, ThrobberAlignment(EHorizontalAlignment::HAlign_Center)
	, LoadingText(NSLOCTEXT("LoadGuard", "Loading", "Loading..."))
	, TextStyle(nullptr)
	, TextStyleSize(EUTWidgetStyleSize::Small)
	, TextColorType(EUTTextColor::Light)
	, bIsLoading(false)
{
	// todo PLK - setup text styles
	//static ConstructorHelpers::FClassFinder<UUTTextStyle> BaseTextStyleClassFinder(TEXT("/Game/UI/Text/TextStyle-Body"));
	//TextStyle = BaseTextStyleClassFinder.Class;

	Visibility = ESlateVisibility::SelfHitTestInvisible;

#if WITH_EDITORONLY_DATA
	bShowLoading = false;
#endif
}

void UUTLoadGuard::SetLoadingText(const FText& InLoadingText)
{
	LoadingText = InLoadingText;
	RefreshText();
}

void UUTLoadGuard::SetIsLoading(bool bInIsLoading)
{
	SetIsLoadingInternal(bInIsLoading, false);
}

bool UUTLoadGuard::IsLoading() const
{
	return bIsLoading;
}

void UUTLoadGuard::BP_GuardAndLoadAsset(const TAssetPtr<UObject>& InLazyAsset, const FOnAssetLoaded& OnAssetLoaded)
{
	GuardAndLoadAsset<UObject>(InLazyAsset, 
		[OnAssetLoaded] (UObject* Asset) 
		{
			OnAssetLoaded.ExecuteIfBound(Asset); 
		});
}

void UUTLoadGuard::SetContent(const TSharedRef<SWidget>& Content)
{
	if (MyContentBox.IsValid())
	{
		MyContentBox->SetContent(Content);
	}
}

TSharedRef<SWidget> UUTLoadGuard::RebuildWidget()
{
	MyContentBox = SNew(SBox);
	if (GetChildrenCount() > 0)
	{
		Cast<USizeBoxSlot>(GetContentSlot())->BuildSlot(MyContentBox.ToSharedRef());
	}
	
	Text_LoadingText = NewObject<UUTTextBlock>(this);
	RefreshText();

	MyGuardOverlay = 
		SNew(SOverlay)
		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			MyContentBox.ToSharedRef()
		]

		+ SOverlay::Slot()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Fill)
		[
			SAssignNew(MyGuardBox, SBox)
			.HAlign(ThrobberAlignment.GetValue())
			.VAlign(VAlign_Center)
			.Padding(ThrobberPadding)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Left)
				.VAlign(VAlign_Center)
				.Padding(0.f, 0.f, Text_LoadingText ? 18.f : 0.f, 0.f)
				[
					SNew(SImage)
					.Image(&UUTGameUIData::Get().LoadingSpinner)
				]

				+ SHorizontalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.AutoWidth()
				[
					Text_LoadingText ? Text_LoadingText->TakeWidget() : SNullWidget::NullWidget
				]
			]
		];

	SetIsLoadingInternal(bIsLoading);

    //return BuildDesignTimeWidget(MyGuardOverlay.ToSharedRef());
#if WITH_EDITOR
    return CreateDesignerOutline(MyGuardOverlay.ToSharedRef());
#else
    return MyGuardOverlay.ToSharedRef();
#endif
}

void UUTLoadGuard::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	RefreshText();

#if WITH_EDITORONLY_DATA
	SetIsLoadingInternal(bShowLoading);
#endif
}

UClass* UUTLoadGuard::GetSlotClass() const
{
	return USizeBoxSlot::StaticClass();
}

void UUTLoadGuard::OnSlotAdded(UPanelSlot* InSlot)
{
	if (MyContentBox.IsValid())
	{
		Cast<USizeBoxSlot>(InSlot)->BuildSlot(MyContentBox.ToSharedRef());
	}
}

void UUTLoadGuard::OnSlotRemoved(UPanelSlot* InSlot)
{
	if (MyContentBox.IsValid())
	{
		MyContentBox->SetContent(SNullWidget::NullWidget);
	}
}

#if WITH_EDITOR
const FText UUTLoadGuard::GetPaletteCategory()
{
	return FText::FromString(TEXT("UT"));
}
#endif

void UUTLoadGuard::SetIsLoadingInternal(bool bInIsLoading, bool bForce)
{
	if (bForce || (bInIsLoading != bIsLoading))
	{
		bIsLoading = bInIsLoading;

		if (OnLoadingStateChanged.IsBound())
		{
			OnLoadingStateChanged.Broadcast(bInIsLoading);
		}

		if (MyGuardOverlay.IsValid())
		{
			MyGuardOverlay->SetVisibility(bIsLoading ? EVisibility::HitTestInvisible : EVisibility::SelfHitTestInvisible);
		}
		if (MyGuardBox.IsValid())
		{
			MyGuardBox->SetVisibility(bIsLoading ? EVisibility::SelfHitTestInvisible : EVisibility::Collapsed);
		}

		if (MyContentBox.IsValid())
		{
			MyContentBox->SetVisibility(bIsLoading ? EVisibility::Hidden : EVisibility::SelfHitTestInvisible);
		}
	}
}

void UUTLoadGuard::RefreshText()
{
	if (Text_LoadingText)
	{
		if (LoadingText.IsEmpty())
		{
			Text_LoadingText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			Text_LoadingText->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			Text_LoadingText->SetProperties(TextStyle, TextStyleSize, TextColorType);
			Text_LoadingText->SetText(LoadingText);
		}
	}
}

void UUTLoadGuard::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	LazyAsset.Reset();

	MyGuardOverlay.Reset();
	MyGuardBox.Reset();
	MyContentBox.Reset();

	if (Text_LoadingText)
	{
		Text_LoadingText->ReleaseSlateResources(bReleaseChildren);
		Text_LoadingText = nullptr;
	}
}
