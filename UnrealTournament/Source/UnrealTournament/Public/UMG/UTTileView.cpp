// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTTileView.h"

#include "UTObjectListItem.h"

UUTTileView::UUTTileView()
	: ItemAlignment(EItemAlignment::EvenlyDistributed)
	, ItemWidth(128.f)
{
}

void UUTTileView::ReleaseSlateResources( bool bReleaseChildren )
{
	Super::ReleaseSlateResources( bReleaseChildren );

	MyTileView.Reset();
}

void UUTTileView::BeginDestroy()
{
	Super::BeginDestroy();

	if ( MyTileView.IsValid() )
	{
		MyTileView.Reset();
	}
}

void UUTTileView::SetItemWidth(float NewWidth)
{
	ItemWidth = NewWidth;
	if (MyTileView.IsValid())
	{
		MyTileView->SetItemWidth(GetTotalItemWidth());
	}
}

TSharedRef<SListView<UObject*>> UUTTileView::RebuildListWidget()
{
	MyListView = MyTileView = SNew(STileView<UObject*>)
		.HandleGamepadEvents(false)
		.SelectionMode(SelectionMode)
		.ListItemsSource(&DataProvider->AsArray())
		.ClearSelectionOnClick(bClearSelectionOnClick)
		.ItemWidth(GetTotalItemWidth())
		.ItemHeight(GetTotalItemHeight())
		.ConsumeMouseWheel(ConsumeMouseWheel)
		.ItemAlignment(AsListItemAlignment(ItemAlignment))
		.OnGenerateTile_UObject(this, &ThisClass::HandleGenerateRow)
		.OnMouseButtonClick_UObject(this, &ThisClass::HandleItemClicked)
		.OnSelectionChanged_UObject(this, &ThisClass::HandleSelectionChanged)
		.OnTileReleased_UObject(this, &ThisClass::HandleRowReleased);

	return MyTileView.ToSharedRef();
}

float UUTTileView::GetTotalItemWidth() const
{
	return ItemWidth + DesiredItemPadding.Left + DesiredItemPadding.Right;
}
