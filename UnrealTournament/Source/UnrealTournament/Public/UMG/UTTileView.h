// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTListView.h"
#include "Widgets/Views/STileView.h"

#include "UTTileView.generated.h"

UENUM(BlueprintType)
enum class EItemAlignment : uint8
{
	EvenlyDistributed UMETA(DisplayName = "Even"),
	LeftAligned UMETA(DisplayName = "Left"),
	RightAligned UMETA(DisplayName = "Right"),
	CenterAligned UMETA(DisplayName = "Center"),
	Fill,
};

static FORCEINLINE EListItemAlignment AsListItemAlignment(EItemAlignment Align)
{
	switch (Align)
	{
	default:
	case EItemAlignment::EvenlyDistributed: return EListItemAlignment::EvenlyDistributed;
	case EItemAlignment::LeftAligned: return EListItemAlignment::LeftAligned;
	case EItemAlignment::RightAligned: return EListItemAlignment::RightAligned;
	case EItemAlignment::CenterAligned: return EListItemAlignment::CenterAligned;
	case EItemAlignment::Fill: return EListItemAlignment::Fill;
	}
}

UCLASS(DisplayName = "UT Tile View")
class UUTTileView : public UUTListView
{
	GENERATED_BODY()

public:
	UUTTileView();

	virtual void ReleaseSlateResources( bool bReleaseChildren ) override;
	virtual void BeginDestroy() override;

	/** Sets the width of every tile in the list */
	UFUNCTION(BlueprintCallable, Category = TileView)
	void SetItemWidth(float NewWidth);

protected:
	virtual TSharedRef<SListView<UObject*>> RebuildListWidget() override;

	float GetTotalItemWidth() const;

	UPROPERTY(EditInstanceOnly, Category = ItemLayout, meta = (DesignerRebuild = true))
	EItemAlignment ItemAlignment;

	UPROPERTY(EditInstanceOnly, Category = ItemLayout, meta = (DesignerRebuild = true))
	float ItemWidth;

private:
	TSharedPtr<STileView<UObject*>> MyTileView;
};