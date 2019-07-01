// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "WidgetFactory.h"

#include "UTUITypes.h"

#include "UTActivatableWidget.h"

#include "UTChestRewardCategory.h"
#include "UTChestOpeningScreen.generated.h"

#if WITH_PROFILE
#include "McpSharedTypes.h"
#else
#include "GithubStubs.h"
#endif

class UUTTextBlock;
class UUTDefaultButton;
class UUTListView;
class UUTChestRewardCategory;
class UUTGlowingRarityText;
class UUTLazyImage;
class UUTMcpTypeRewardImage;

UENUM(BlueprintType)
enum class EUTSpinState : uint8
{
	Inactive,
	Opening,
	Spinning,
	SlowingDown
};

USTRUCT()
struct FChestResultEntryRow
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UUtMcpDefinition* RewardItem;

	FString TemplateId;

	int32 Quantity;

	EUTItemRarity RewardRarity;

	FChestResultEntryRow() 
		: RewardItem(nullptr)
		, Quantity(0)
		, RewardRarity(EUTItemRarity::Basic)
	{}
};

UCLASS()
class UUTChestOpeningScreen : public UUTActivatableWidget
{
	GENERATED_BODY()

public:
	UUTChestOpeningScreen(const FObjectInitializer& Initializer);

	virtual void NativeOnActivated() override;
	virtual void NativeOnDeactivated() override;

protected:
	
	UFUNCTION(BlueprintImplementableEvent, Category = LootCrate)
	void BP_FullReset();

	UFUNCTION(BlueprintCallable, Category = LootCrate)
	void NativeReset();

	UFUNCTION(BlueprintCallable, Category = LootCrate)
	void BeginOpen();

	UFUNCTION(BlueprintCallable, Category = LootCrate)
	void FireSpinner(const UUtMcpCardPackItem* PackToOpen, const TArray<FMcpItemIdAndQuantity> RewardArray);

	UFUNCTION(BlueprintCallable, Category = LootCrate)
	void BeginSpinnerSlowdown();

	UFUNCTION(BlueprintImplementableEvent, Category = LootCrate)
	void FinalSpinnerStop(EUTItemRarity ItemRarity);

	UFUNCTION(BlueprintImplementableEvent, Category = LootCrate)
	void SetItemRarities(EUTItemRarity ItemRarity);

	UFUNCTION(BlueprintImplementableEvent, Category = LootCrate)
	void OnNextItemShown(EUTItemRarity ItemRarity, EUTSpinState SpinState, bool bIsFinal);

	UFUNCTION(BlueprintCallable, Category = LootCrate)
	void SetScreenState(EUTSpinState NewState);

	virtual bool OnHandleBackAction_Implementation() override;

	void LoadPotentialRewards();

	UPROPERTY(EditAnywhere)
	TAssetPtr<UTexture2D> LazyDefaultTreasureImage;

	UPROPERTY(EditAnywhere)
	float PictureFlipRate;

	float PictureSlowDownRate;
	float PictureFinalStopRate;

	// adjust the number of bogus rewards that are spinning
	UPROPERTY(EditAnywhere, Category = LootCrate)
	int32 EpicMegaRareCount;

	UPROPERTY(EditAnywhere, Category = LootCrate)
	int32 UltraRareCount;

	UPROPERTY(EditAnywhere, Category = LootCrate)
	int32 RareCount;

	UPROPERTY(EditAnywhere, Category = LootCrate)
	int32 CommonCount;

	FChestResultEntryRow ActualReward;

	UPROPERTY()
	TArray<FChestResultEntryRow> PotentialRewardStruct;

	UPROPERTY()
	TArray<FChestResultEntryRow> SpinnerItemArray;

	/** Used to avoid GC after loading images into spinner. */
	UPROPERTY(Transient)
	TArray<UObject*> LoadedImages;

	FTimerHandle SpinnerTimerHandle;

	int32 SpinnerImageIdx;

	EUTSpinState SpinnerState;

	void ShowNextImage();

	void LoadFluffRewards();
	void LoadFluffRewardsInternal(TArray<FChestResultEntryRow>& RewardArray, int32 RewardCount);
	void RemoveOwnedRewards();
	void SortRewardsIntoArrays(TArray<FChestResultEntryRow>& CommonRewards, TArray<FChestResultEntryRow>& RareRewards, 
		TArray<FChestResultEntryRow>& UltraRareRewards, TArray<FChestResultEntryRow>& EpicMegaRareRewards);

	void RefreshOwnedStatus();
	void ShuffleSpinnerRewardArray();

private:
	void ProcessNewCurrentlySelectedReward(FChestResultEntryRow& CurrentlySelectedReward);

private: // bound widgets

	UPROPERTY(meta = (BindWidget))
	UScrollBox* ScrollBox_Rewards;

	UPROPERTY(meta = (BindWidget))
	UUTLazyImage* Image_RotatingReward;

	UPROPERTY(meta = (BindWidget))
	UUTMcpTypeRewardImage* Reward_RotatingSubImage;

	UPROPERTY(meta = (BindWidget))
	UUTLazyImage* Image_FinalResult;

	UPROPERTY(meta = (BindWidget))
	UUTGlowingRarityText* GlowingText_RotatingReward;

	UPROPERTY(meta = (BindWidget))
	UUTTextBlock* Text_RewardName;

	UPROPERTY(meta = (BindWidget))
	UUTTextBlock* Text_RewardType;

	UPROPERTY(meta = (BindWidget))
	UUTGlowingRarityText* GlowingText_FinalReward;

	UPROPERTY(meta = (BindWidget))
	UUTTextBlock* Text_FinalName;

	UPROPERTY(meta = (BindWidget))
	UUTTextBlock* Text_FinalType;

	UPROPERTY(meta = (BindWidget))
	UUTChestRewardCategory* Reward_EpicMegaRare;

	UPROPERTY(meta = (BindWidget))
	UUTChestRewardCategory* Reward_UltraRare;

	UPROPERTY(meta = (BindWidget))
	UUTChestRewardCategory* Reward_Rare;

	UPROPERTY(meta = (BindWidget))
	UUTChestRewardCategory* Reward_Common;

	UPROPERTY(meta = (BindWidget))
	UUTMcpTypeRewardImage* Reward_FinalSubImage;
};
