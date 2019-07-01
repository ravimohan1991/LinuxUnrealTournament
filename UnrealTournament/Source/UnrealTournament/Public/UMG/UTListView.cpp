// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTListView.h"
#include "UTGlobals.h"
#include "UTObjectListItem.h"
#include "UTBaseButton.h"

#include "SScissorRectBox.h"

namespace ListViewText
{
	static const FText skFailedToGenerate =
	    NSLOCTEXT( "UT.ListView", "FailedToGenerate",
	               "The widget you've provided doesn't implement the List Item interface or your data is invalid." );
}

UUTListView::UUTListView()
	: ItemHeight(128.f)
	, ListItemClass(nullptr)
	, SelectionMode(ESelectionMode::Single)
	, ConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible)
	, bClearSelectionOnClick(false)
	, NumPreAllocatedEntries(1)
{
	Visibility = ESlateVisibility::Visible;
}

TSharedRef<SWidget> UUTListView::RebuildWidget()
{
	DataProvider = NewShared<TDataProvider<UObject*>>();

	if (!IsDesignTime())
	{
		UE_CLOG(!ListItemClass, UT, Error, TEXT("[%s] has no row widget class specified!"), *GetName());

		UClass* ItemClass = ListItemClass != nullptr ? *ListItemClass : UUserWidget::StaticClass();
		ItemWidgets = TWidgetFactory<UUserWidget>(
			ItemClass,
			[this]() -> UGameInstance*
			{
				return GetWorld()->GetGameInstance();
			});

		ItemWidgets.PreConstruct(NumPreAllocatedEntries);
	}

	TSharedPtr<SScissorRectBox> ScissorRectBox = SNew(SScissorRectBox)
		[
			RebuildListWidget()
		];

	return BuildDesignTimeWidget(ScissorRectBox.ToSharedRef());
}

void UUTListView::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	Clear();
	MyListView.Reset();

	ItemWidgets.Reset(bReleaseChildren);
}

void UUTListView::BeginDestroy()
{
	Super::BeginDestroy();

	if (MyListView.IsValid())
	{
		MyListView.Reset();
	}

	DataProvider.Reset();
}


void UUTListView::AddItem(UObject* Item) const
{
	UObject*& NewObject = DataProvider->Add();
	NewObject = Item;
	
	if (MyListView.IsValid())
	{
		MyListView->RequestListRefresh();
	}
}

UObject* UUTListView::GetItemAt(int32 Index) const
{
	return DataProvider->IsValidIndex(Index) ? DataProvider->At(Index) : nullptr;
}

int32 UUTListView::GetIndexForItem( UObject* Item ) const
{
	for ( int32 ItemIdx = 0; ItemIdx < DataProvider->Num(); ++ItemIdx )
	{
		if ( Item == DataProvider->At(ItemIdx) )
		{
			return ItemIdx;
		}
	}

	return INDEX_NONE;
}

void UUTListView::SetItemHeight(float NewHeight)
{
	ItemHeight = NewHeight;
	if ( MyListView.IsValid() )
	{
		MyListView->SetItemHeight( GetTotalItemHeight() );
	}
}

void UUTListView::SetDesiredItemPadding(const FMargin& DesiredPadding)
{
	DesiredItemPadding = DesiredPadding;
	
	if (MyListView.IsValid())
	{
		MyListView->RequestListRefresh();
	}
}

int32 UUTListView::GetNumItemsSelected() const
{
	return MyListView.IsValid() ? MyListView->GetNumItemsSelected() : 0;
}

UObject* UUTListView::GetSelectedItem() const
{
	if (MyListView.IsValid())
	{
		TArray<UObject*> SelectedItems = MyListView->GetSelectedItems();
		return SelectedItems.Num() > 0 ? SelectedItems[0] : nullptr;
	}

	return nullptr;
}

bool UUTListView::GetSelectedItems(TArray<UObject*>& Items) const
{
	if (MyListView.IsValid())
	{
		Items = MyListView->GetSelectedItems();
		return true;
	}

	return false;
}

bool UUTListView::IsItemVisible(UObject* Item) const
{
	return MyListView.IsValid() ? MyListView->IsItemVisible(Item) : false;
}

void UUTListView::ScrollIntoView( UObject* Item )
{
	if ( MyListView.IsValid() )
	{
		MyListView->RequestScrollIntoView( Item );
	}
}

void UUTListView::ScrollIntoView( const UObject* Item )
{
	ScrollIntoView(const_cast<UObject*>(Item));
}

bool UUTListView::SetSelectedItem(UObject* Item, bool bWaitIfPendingRefresh)
{
	if (MyListView.IsValid() && DataProvider->AsArray().Contains(Item))
	{
		if (bWaitIfPendingRefresh)
		{
			// Take the slow route to make sure we wait for any pending refresh to complete
			ItemToSelectAfterRefresh = Item;
			SelectItemAfterRefresh();
		}
		else
		{
			// Just go for it
			SetSelectedItemInternal(Item);
		}
		
		return true;
	}

	return false;
}

bool UUTListView::SetSelectedItem(const UObject* Item, bool bWaitIfPendingRefresh)
{
	return SetSelectedItem(const_cast<UObject*>(Item), bWaitIfPendingRefresh);
}

bool UUTListView::SetSelectedIndex(int32 Index)
{
	return SetSelectedItem( GetItemAt(Index) );
}

void UUTListView::SetItemSelection( UObject* Item, bool bSelected )
{
	if ( MyListView.IsValid() && DataProvider->AsArray().Contains( Item ) )
	{
		MyListView->SetItemSelection(Item, bSelected);
	}
}

void UUTListView::ClearSelection()
{
	if ( MyListView.IsValid() )
	{
		MyListView->ClearSelection();
	}
}

void UUTListView::Clear()
{
	if (DataProvider.IsValid())
	{
		DataProvider->Clear();
	}
}

bool UUTListView::IsRefreshPending() const
{
	return MyListView.IsValid() ? MyListView->IsPendingRefresh() : false;
}

void UUTListView::SetDataProvider(const TArray<UObject*>& InDataProvider)
{
	DataProvider->FromArray(InDataProvider);

	if (MyListView.IsValid())
	{
		MyListView->RequestListRefresh();
	}
}

TSharedRef<SListView<UObject*>> UUTListView::RebuildListWidget()
{
	MyListView = SNew(SListView<UObject*>)
		.HandleGamepadEvents(false)
		.SelectionMode(SelectionMode)
		.ListItemsSource(&DataProvider->AsArray())
		.ClearSelectionOnClick(bClearSelectionOnClick)
		.ItemHeight(GetTotalItemHeight())
		.ConsumeMouseWheel(ConsumeMouseWheel)
		.OnGenerateRow_UObject(this, &ThisClass::HandleGenerateRow)
		.OnMouseButtonClick_UObject(this, &ThisClass::HandleItemClicked)
		.OnSelectionChanged_UObject(this, &ThisClass::HandleSelectionChanged)
		.OnRowReleased_UObject(this, &ThisClass::HandleRowReleased);

	return MyListView.ToSharedRef();
}

TSharedRef<ITableRow> UUTListView::HandleGenerateRow(UObject* Item, const TSharedRef<STableViewBase>& OwnerTable)
{
	UUserWidget* Widget = ItemWidgets.Acquire();
	check(Widget);

	// Make sure we take the widget first BEFORE setting data on it
	// This ensures that the widget has had a chance to construct before we set data on it
	TSharedRef<ITableRow> GeneratedRow = ItemWidgets.TakeAndCacheRow<ObjectRowType>(Widget, OwnerTable);

	if (!Widget->Implements<UUTObjectListItem>())
	{
		UE_LOG(UT, Warning, TEXT("[%s] does not implement UTListItem interface, cannot set data."), *Widget->GetClass()->GetName());
	}
	else
	{
		Widget->SetPadding(DesiredItemPadding);
		IUTObjectListItem::Execute_SetData(Widget, Item);

		FOnItemClicked OnClicked;
		OnClicked.BindDynamic(this, &ThisClass::DynamicHandleItemClicked);
		IUTObjectListItem::Execute_RegisterOnClicked(Widget, OnClicked);

		// If the list item is a base button, override the selectability of it
		// The list view is in charge of selection, and it's up to the widget to respond accordingly
		if (UUTBaseButton* BaseButton = Cast<UUTBaseButton>(Widget))
		{
			BaseButton->SetIsSelectable(false);
		}
	}

	return GeneratedRow;
}

void UUTListView::HandleItemClicked(UObject* Item)
{
	OnItemClicked.Broadcast(Item);
}

void UUTListView::HandleSelectionChanged(UObject* Item, ESelectInfo::Type SelectInfo)
{
	const bool bItemSelected = MyListView.IsValid() ? MyListView->Private_IsItemSelected(Item) : false;
	OnItemSelected.Broadcast(Item, bItemSelected);
}

void UUTListView::HandleRowReleased(const TSharedRef<ITableRow>& Row)
{
	// Get the actual UserWidget from the released row
	const TSharedRef<ObjectRowType>& ObjectRow = StaticCastSharedRef<ObjectRowType>(Row);
	UUserWidget* ItemWidget = Cast<UUserWidget>(ObjectRow->GetWidgetObject());

	if (ensure(ItemWidget->Implements<UUTObjectListItem>()))
	{
		IUTObjectListItem::Execute_Reset(ItemWidget);
	}

	ItemWidgets.Release(ItemWidget);
}

float UUTListView::GetTotalItemHeight() const
{
	return ItemHeight + DesiredItemPadding.Bottom + DesiredItemPadding.Top;
}

void UUTListView::DynamicHandleItemClicked(UUserWidget* Widget)
{
	if (Widget && ensure(Widget->Implements<UUTObjectListItem>()))
	{
		UObject* Data = IUTObjectListItem::Execute_GetData(Widget);

		const bool bIsSelected = MyListView->Private_IsItemSelected(Data);
		if (SelectionMode == ESelectionMode::Single)
		{
			MyListView->ClearSelection();
			MyListView->SetItemSelection(Data, !bIsSelected);
		}
		else if (SelectionMode == ESelectionMode::Multi)
		{
			MyListView->SetItemSelection(Data, !bIsSelected);
		}

		HandleItemClicked(Data);
	}
}

void UUTListView::SelectItemAfterRefresh()
{
	if (MyListView.IsValid() && ItemToSelectAfterRefresh.IsValid())
	{
		if (MyListView->IsPendingRefresh())
		{
			if (UWorld* World = GetWorld())
			{
				World->GetTimerManager().SetTimerForNextTick(this, &UUTListView::SelectItemAfterRefresh);
			}
		}
		else
		{
			SetSelectedItemInternal(ItemToSelectAfterRefresh.Get());
		}
	}
	else
	{
		ItemToSelectAfterRefresh.Reset();
	}
}

void UUTListView::SetSelectedItemInternal(UObject* Item)
{
	check(MyListView.IsValid());
	MyListView->SetSelection(Item, ESelectInfo::OnNavigation);
	MyListView->RequestScrollIntoView(Item);

	ItemToSelectAfterRefresh.Reset();
}

