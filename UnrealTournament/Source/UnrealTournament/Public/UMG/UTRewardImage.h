// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTUserWidgetBase.h"
#include "UTObjectListItem.h"
#include "UTRewardTooltip.h"

#include "UTRewardImage.generated.h"

class UUtMcpDefinition;
class UUTMcpTypeRewardImage;
class UUTLazyImage;

UCLASS()
class UUTRewardImage : public UUTUserWidgetBase, public IUTObjectListItem
{
	GENERATED_BODY()

public:
	UUTRewardImage(const FObjectInitializer& Initializer);

	// IUTListItem interface
    virtual UObject* GetData_Implementation() const { return ItemDef; }
    virtual void SetData_Implementation(UObject* InData);
    virtual void Reset_Implementation();
	virtual void SetSelected_Implementation(bool bInSelected) override {}
	virtual void SetIndexInList_Implementation(int32 InIndexInList) override {}
	virtual bool IsItemExpanded_Implementation() const override { return false; }
	virtual void ToggleExpansion_Implementation() override {}
	virtual int32 GetIndentLevel_Implementation() const override { return INDEX_NONE; }
	virtual int32 DoesItemHaveChildren_Implementation() const override { return INDEX_NONE; }
	virtual ESelectionMode::Type GetSelectionMode_Implementation() const override { return ESelectionMode::Single; }
	virtual void Private_OnExpanderArrowShiftClicked_Implementation() override {}
	virtual void RegisterOnClicked_Implementation(const FOnItemClicked& Callback) override {}
	// ~IUTListItem

protected:

	UPROPERTY(EditAnywhere, Category = RewardImage)
	TSubclassOf<UUTRewardTooltip> RewardTooltipClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = RewardImage)
	bool bGenerateTooltip;

	void ResetInternal();

private:

	UPROPERTY()
	UUtMcpDefinition* ItemDef;

	UPROPERTY(meta = (BindWidget))
	UUTLazyImage* LazyImage_HeroPortrait;

	UPROPERTY(meta = (BindWidget))
	UBorder* Border_EyebrowText;

	UPROPERTY(meta = (BindWidget))
	UUTMcpTypeRewardImage* Reward_ImageSwitcher;
};
