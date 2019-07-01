// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTListItem.h"
#include "STableRow.h"
#include "Slate/SObjectWidget.h"

template <typename ItemType>
class SObjectTableRowUT : public SObjectWidget, public ITableRow
{
public:
    SLATE_BEGIN_ARGS( SObjectTableRowUT )
		{	
		}

		SLATE_ATTRIBUTE( FMargin, Padding )
		SLATE_DEFAULT_SLOT( FArguments, Content )
			

	SLATE_END_ARGS()

public:
	void Construct( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView, UUserWidget* InChildWidget )
	{
		WidgetObject = InChildWidget;

		ConstructInternal( InArgs, InOwnerTableView );
		ConstructChildren( InOwnerTableView->TableViewMode, InArgs._Padding, InArgs._Content.Widget );
	}

	void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override
	{
		SObjectWidget::Tick( AllottedGeometry, InCurrentTime, InDeltaTime );

		TSharedRef<ITypedTableView<ItemType>> OwnerWidget = OwnerTablePtr.Pin().ToSharedRef();
		const bool bIsActive = OwnerWidget->AsWidget()->HasKeyboardFocus();
		const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );

		// True if this is the item having it's selection modified actively
		bool bActive = ( bIsActive && OwnerWidget->Private_UsesSelectorFocus() && OwnerWidget->Private_HasSelectorFocus( *MyItem ) );

		// True if this item is selected at all
		bool bSelected = OwnerWidget->Private_IsItemSelected( *MyItem );

		if ( bShowSelection != bSelected )
		{
			bShowSelection = bSelected;

			IUTListItem::Execute_SetSelected( WidgetObject, bSelected );
		}
	}

	/**
	 * @param InIndexInList  The index of the item for which this widget was generated
	 */
	void SetIndexInList( int32 InIndexInList ) override
	{
		IUTListItem::Execute_SetIndexInList( WidgetObject, InIndexInList );
	}

	/** @return true if the corresponding item is expanded; false otherwise*/
	bool IsItemExpanded() const override
	{
		return IUTListItem::Execute_IsItemExpanded( WidgetObject );
	}

	/** Toggle the expansion of the item associated with this row */
	void ToggleExpansion() override
	{
		IUTListItem::Execute_ToggleExpansion( WidgetObject );
	}

	/** @return how nested the item associated with this row when it is in a TreeView */
	int32 GetIndentLevel() const override
	{
		return IUTListItem::Execute_GetIndentLevel( WidgetObject );
	}

	/** @return Does this item have children? */
	int32 DoesItemHaveChildren() const override
	{
		return IUTListItem::Execute_DoesItemHaveChildren( WidgetObject );
	}

	/** @return this table row as a widget */
	TSharedRef<SWidget> AsWidget() override
	{
		return SharedThis( this );
	}

	/** @return the content of this table row */
	TSharedPtr<SWidget> GetContent() override
	{
		return SNullWidget::NullWidget;
	}

	/** Called when the expander arrow for this row is shift+clicked */
	void Private_OnExpanderArrowShiftClicked() override
	{
		IUTListItem::Execute_Private_OnExpanderArrowShiftClicked( WidgetObject );
	}

	/** SWidget Stuff */
	FReply OnMouseButtonDoubleClick( const FGeometry& InMyGeometry, const FPointerEvent& InMouseEvent ) override
	{
		if ( InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
		{
			TSharedRef<ITypedTableView<ItemType>> OwnerWidget = OwnerTablePtr.Pin().ToSharedRef();

			// Only one item can be double-clicked
			const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );

			// If we're configured to route double-click messages to the owner of the table, then
			// do that here.  Otherwise, we'll toggle expansion.
			const bool bWasHandled = OwnerWidget->Private_OnItemDoubleClicked( *MyItem );
			if ( !bWasHandled )
			{
				ToggleExpansion();
			}

			return FReply::Handled();
		}

		return FReply::Unhandled();
	}

	FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override
	{
		TSharedPtr<ITypedTableView<ItemType>> OwnerWidget = OwnerTablePtr.Pin();
		ChangedSelectionOnMouseDown = false;

		check( OwnerWidget.IsValid() );

		if ( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
		{
			switch ( GetSelectionMode() )
			{
				case ESelectionMode::Single:
				{
					const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );
					const bool bIsSelected = OwnerWidget->Private_IsItemSelected( *MyItem );

					// Select the item under the cursor
					if ( !bIsSelected )
					{
						OwnerWidget->Private_ClearSelection();
						OwnerWidget->Private_SetItemSelection( *MyItem, true, true );

						ChangedSelectionOnMouseDown = true;
					}

					return FReply::Handled()
						.DetectDrag( SharedThis( this ), EKeys::LeftMouseButton )
						.SetUserFocus( OwnerWidget->AsWidget(), EFocusCause::Mouse )
						.CaptureMouse( SharedThis( this ) );
				}
				break;

				case ESelectionMode::SingleToggle:
				{
					const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );
					const bool bIsSelected = OwnerWidget->Private_IsItemSelected( *MyItem );

					if ( !bIsSelected )
					{
						OwnerWidget->Private_ClearSelection();
						OwnerWidget->Private_SetItemSelection( *MyItem, true, true );
						ChangedSelectionOnMouseDown = true;
					}

					return FReply::Handled()
						.DetectDrag( SharedThis( this ), EKeys::LeftMouseButton )
						.SetUserFocus( OwnerWidget->AsWidget(), EFocusCause::Mouse )
						.CaptureMouse( SharedThis( this ) );
				}
				break;

				case ESelectionMode::Multi:
				{
					const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );
					const bool bIsSelected = OwnerWidget->Private_IsItemSelected( *MyItem );

					check( MyItem != nullptr );

					if ( MouseEvent.IsControlDown() )
					{
						OwnerWidget->Private_SetItemSelection( *MyItem, !bIsSelected, true );
						ChangedSelectionOnMouseDown = true;
					}
					else if ( MouseEvent.IsShiftDown() )
					{
						OwnerWidget->Private_SelectRangeFromCurrentTo( *MyItem );
						ChangedSelectionOnMouseDown = true;
					}
					else if ( !bIsSelected )
					{
						OwnerWidget->Private_ClearSelection();
						OwnerWidget->Private_SetItemSelection( *MyItem, true, true );
						ChangedSelectionOnMouseDown = true;
					}

					return FReply::Handled()
						.DetectDrag( SharedThis( this ), EKeys::LeftMouseButton )
						.SetUserFocus( OwnerWidget->AsWidget(), EFocusCause::Mouse )
						.CaptureMouse( SharedThis( this ) );
				}
				break;

				case ESelectionMode::None:
				{
					return SObjectWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
				}
				break;
			}
		}

		return FReply::Unhandled();
	}

	FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override
	{
		TSharedPtr<ITypedTableView<ItemType>> OwnerWidget = OwnerTablePtr.Pin();
		check( OwnerWidget.IsValid() );

		TSharedRef<STableViewBase> OwnerTableViewBase = StaticCastSharedPtr<SListView<ItemType>>( OwnerWidget ).ToSharedRef();

		if ( MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton )
		{
			FReply Reply = FReply::Unhandled().ReleaseMouseCapture();

			if ( ChangedSelectionOnMouseDown )
			{
				Reply = FReply::Handled().ReleaseMouseCapture();
			}

			const bool bIsUnderMouse = MyGeometry.IsUnderLocation( MouseEvent.GetScreenSpacePosition() );
			if ( HasMouseCapture() )
			{
				if ( bIsUnderMouse )
				{
					switch ( GetSelectionMode() )
					{
						case ESelectionMode::SingleToggle:
						{
							if ( !ChangedSelectionOnMouseDown )
							{
								const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );

								OwnerWidget->Private_ClearSelection();
								OwnerWidget->Private_SignalSelectionChanged( ESelectInfo::OnMouseClick );
							}

							Reply = FReply::Handled().ReleaseMouseCapture();
						}
						break;

						case ESelectionMode::Multi:
						{
							if ( !ChangedSelectionOnMouseDown && !MouseEvent.IsControlDown() && !MouseEvent.IsShiftDown() )
							{
								const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );
								check( MyItem != nullptr );

								const bool bIsSelected = OwnerWidget->Private_IsItemSelected( *MyItem );
								if ( bIsSelected && OwnerWidget->Private_GetNumSelectedItems() > 1 )
								{
									// We are mousing up on a previous selected item;
									// deselect everything but this item.

									OwnerWidget->Private_ClearSelection();
									OwnerWidget->Private_SetItemSelection( *MyItem, true, true );
									OwnerWidget->Private_SignalSelectionChanged( ESelectInfo::OnMouseClick );

									Reply = FReply::Handled().ReleaseMouseCapture();
								}
							}
						}
						break;

						case ESelectionMode::None:
						{
							return SObjectWidget::OnMouseButtonUp(MyGeometry, MouseEvent);
						}
						break;
					}
				}

				const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );
				if ( MyItem && OwnerWidget->Private_OnItemClicked( *MyItem ) )
				{
					Reply = FReply::Handled().ReleaseMouseCapture();
				}

				if ( ChangedSelectionOnMouseDown )
				{
					OwnerWidget->Private_SignalSelectionChanged( ESelectInfo::OnMouseClick );
				}

				return Reply;
			}
		}
		else if ( MouseEvent.GetEffectingButton() == EKeys::RightMouseButton && !OwnerTableViewBase->IsRightClickScrolling() )
		{
			// Handle selection of items when releasing the right mouse button, but only if the user isn't actively
			// scrolling the view by holding down the right mouse button.

			switch ( GetSelectionMode() )
			{
				case ESelectionMode::Single:
				case ESelectionMode::SingleToggle:
				case ESelectionMode::Multi:
				{
					// Only one item can be selected at a time
					const ItemType* MyItem = OwnerWidget->Private_ItemFromWidget( this );
					const bool bIsSelected = OwnerWidget->Private_IsItemSelected( *MyItem );

					// Select the item under the cursor
					if ( !bIsSelected )
					{
						OwnerWidget->Private_ClearSelection();
						OwnerWidget->Private_SetItemSelection( *MyItem, true, true );
						OwnerWidget->Private_SignalSelectionChanged( ESelectInfo::OnMouseClick );
					}

					OwnerWidget->Private_OnItemRightClicked( *MyItem, MouseEvent );

					return FReply::Handled();
				}
				break;
			}
		}

		return FReply::Unhandled();
	}

	FReply OnTouchStarted( const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent ) override
	{
		return SObjectWidget::OnTouchStarted( MyGeometry, InTouchEvent );
	}

	FReply OnTouchEnded( const FGeometry& MyGeometry, const FPointerEvent& InTouchEvent ) override
	{
		return SObjectWidget::OnTouchEnded( MyGeometry, InTouchEvent );
	}

	virtual bool SupportsKeyboardFocus() const override { return true; }

protected:
	/** Called to query the selection mode for the row */
	ESelectionMode::Type GetSelectionMode() const override
	{
		return IUTListItem::Execute_GetSelectionMode( WidgetObject );
	}

	void ConstructInternal( const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView )
	{
		SetOwnerTableView( InOwnerTableView );
	}

	void ConstructChildren( ETableViewMode::Type InOwnerTableMode, const TAttribute<FMargin>& InPadding, const TSharedRef<SWidget>& InContent )
	{
		if ( InOwnerTableMode == ETableViewMode::List || InOwnerTableMode == ETableViewMode::Tile )
		{
			// clang-format off
			ChildSlot
				.Padding( InPadding )
				[
					InContent
				];
			// clang-format on
		}
		else
		{
			// clang-format off
			ChildSlot
				[
					SNew( SHorizontalBox )
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.HAlign( HAlign_Right )
					.VAlign( VAlign_Fill )
					[
						SNew( SExpanderArrow, SharedThis( this ) )
					]
					+ SHorizontalBox::Slot()
					.FillWidth( 1 )
					.Padding( InPadding )
					[
						InContent
					]
				];
			// clang-format on
		}
	}

	void SetOwnerTableView( TSharedPtr<STableViewBase> OwnerTableView )
	{
		OwnerTablePtr = StaticCastSharedPtr<SListView<ItemType>>( OwnerTableView );
	}

	/**
	 * ITableRow interface
	 */
	// todo PLK: update when we get ITableRow support
	/*
	virtual FVector2D GetRowSizeForColumn(const FName& InColumnName) const override
	{
		return FVector2D::ZeroVector;
	}
	*/
private:
	bool bShowSelection;

	TWeakPtr<ITypedTableView<ItemType>> OwnerTablePtr;

	bool ChangedSelectionOnMouseDown;
};
