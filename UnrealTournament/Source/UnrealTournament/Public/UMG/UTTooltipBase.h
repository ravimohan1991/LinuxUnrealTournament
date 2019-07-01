// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTUserWidgetBase.h"
//#include "QuickChatTypes.h"
#include "UTTooltipBase.generated.h"

//class UUTQuickChatWrapper;

/** 
 * The base for all UT tooltips that works in conjunction with UUTUserWidgetBase.
 * Takes care of fading the tooltip in and providing API for show/hide events, but makes no assumptions about layout (beyond the content border).
 * Additionally capable of wrapping itself with quick-chat options when desired.
 */
UCLASS(BlueprintType, Blueprintable, meta = (Category = "UT UI"))
class UNREALTOURNAMENT_API UUTTooltipBase : public UUTUserWidgetBase
{
	GENERATED_UCLASS_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** Enables quick chat on this widget for the given topic and subject. */
	//void InitQuickChat(EUTQuickChatTopic Topic, const FUTQuickChatSubjectInfo& SubjectInfo);

	/** 
	 * Enables quick chat on this widget for the given topic with multiple subject options,
	 * as different messages in a topic may pertain to different subjects. 
	 */
	//void InitQuickChat(EUTQuickChatTopic Topic, const TArray<FUTQuickChatSubjectInfo>& Subjects);

	/** Sets the quick chat topic for this widget */
	//void SetQuickChatTopic(EUTQuickChatTopic Topic);

	UFUNCTION(BlueprintCallable, Category = UTTooltip)
	void Show();

	UFUNCTION(BlueprintCallable, Category = UTTooltip)
	void Hide();

protected:
	UFUNCTION(BlueprintImplementableEvent, Category = UTTooltip)
	void OnShow();
	virtual void NativeOnShow();

	UFUNCTION(BlueprintImplementableEvent, Category = UTTooltip)
	void OnHide();
	virtual void NativeOnHide();

	/** The border surrounding all tooltips, will not be optional forever */
	UPROPERTY(meta = (BindWidget, OptionalWidget=true))
	UBorder* Border_Contents;

private:
	friend struct FWebExporter;
	// Shows the tooltip immediately (for web exporting)
	void ShowImmediatelyForExport();

	//UPROPERTY(Transient)
	//UUTQuickChatWrapper* MyQuickChatWrapper;

	void UpdateFadeAnim();
	FCurveSequence FadeSequence;
	bool bIsFading;
};