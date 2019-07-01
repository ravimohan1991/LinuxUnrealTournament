// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UMG.h"
#if WITH_PROFILE
#include "UTMcpProfile.h"
#else
#include "GithubStubs.h"
#endif
#include "UTUserWidgetBase.generated.h"

/** 
 * Base class for UT user widgets that enables the ability to consume all input
 * and to perform an in-editor PreConstruct to allow the designer to be more WYSIWYG
 */
UCLASS( BlueprintType, Blueprintable, meta = (Category = "UT UI") )
class UNREALTOURNAMENT_API UUTUserWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	UUTUserWidgetBase( const FObjectInitializer& ObjectInitializer );

#if WITH_EDITOR
	virtual void PreEditChange(UProperty* PropertyAboutToChange) override;
#endif
protected:
	virtual bool Initialize() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void OnWidgetRebuilt() override;
	virtual bool NativeIsInteractable() const override;

	/** Use this instead of Initialize() if you need to do fancy nonsense in child classes. */
	virtual void InitializeUTWidget() {};
	
	class UUTLocalPlayer* GetUTLocalPlayer() const;
	
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDoubleClick(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnTouchGesture(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchStarted(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchMoved(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;
	virtual FReply NativeOnTouchEnded(const FGeometry& InGeometry, const FPointerEvent& InGestureEvent) override;

	/**
	 * Set the anchored tooltip to display when this widget is hovered
	 */
	UFUNCTION(BlueprintCallable, Category = UTUserWidgetBase)
	void SetUTTooltip(UWidget* InTooltipWidget);

	/** Sets the text to display in a basic tooltip (overridden by custom tooltip widgets) */
	UFUNCTION(BlueprintCallable, Category = UTUserWidgetBase)
	void SetBasicTooltipInfo(FText NewTooltipTitle, FText NewTooltipText);

	/** Sets the text to display in a basic tooltip (overridden by custom tooltip widgets) */
	UFUNCTION(BlueprintCallable, Category = UTUserWidgetBase)
	void SetBasicTooltipTitle(const FText& NewTooltipTitle);

	/** Sets the text to display in a basic tooltip (overridden by custom tooltip widgets) */
	UFUNCTION(BlueprintCallable, Category = UTUserWidgetBase)
	void SetBasicTooltipText(const FText& NewTooltipText);

	UFUNCTION(BlueprintCallable, Category = UTUserWidgetBase)
	const FText& GetBasicTooltipText() const { return UTTooltipText; }

#if WITH_PROFILE
	/** Gets the McpProfileAccount of the owning player */
	template <typename ProfileAccountT = UUtMcpProfile, typename = typename TEnableIf<TIsDerivedFrom<ProfileAccountT, UUtMcpProfile>::IsDerived, ProfileAccountT>::Type>
	ProfileAccountT* GetMcpProfileAccount(bool bChecked = false) const
	{
		UUtMcpProfile* Profile = GetUTLocalPlayer()->GetMcpProfileManager()->GetMcpProfileAs<UUtMcpProfile>(EUtMcpProfile::Profile);
		return bChecked ? CastChecked<ProfileAccountT>(Profile, ECastCheckedType::NullAllowed) : Cast<ProfileAccountT>(Profile);
	}
#endif

	UWidget* GetCurrentTooltip() const;
	
	/** Set this to true if you don't want any pointer (mouse and touch) input to bubble past this widget */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UT Widget|Input")
	bool bConsumePointerInput;

	/** True if this widget might EVER have a tooltip (lets us avoid creating an unnecessary wrapper around widgets that we know will NEVER have a tooltip) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UT Widget|Tooltip")
	bool bEnableUTTooltip;

	/** Purely optional title to display in the basic tooltip (only used if UTTooltipText is set) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UT Widget|Tooltip", meta=(EditCondition="bEnableUTTooltip"))
	FText UTTooltipTitleText;

	/** Where the anchored tooltip for this widget should appear */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UT Widget|Tooltip", meta = (EditCondition = "bEnableUTTooltip"))
	TEnumAsByte<EMenuPlacement> TooltipAnchorPlacement;
	
	/** The specific tag used to identify this widget. "None" is perfectly valid - only give an ID to widgets that need them. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UT Widget|ID")
	FName WidgetId;

	/** The location to anchor any attached messages (for instance, if used in a tutorial) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UT Widget|ID")
	TEnumAsByte<EMenuPlacement> AttachedMessagePlacement;

	/** If set and no custom UT tooltip is provided by SetUTTooltip, this will show a generic text-only UT tooltip */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(MultiLine="true", EditCondition="bEnableUTTooltip"), Category = "UT Widget|Tooltip")
	FText UTTooltipText;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "UT Widget|Tooltip")
	UWidget* CustomTooltipWidget;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UT Widget|Tooltip", meta = (EditCondition = "bEnableUTTooltip"))
	bool bAutoOpenTooltip;

private:
	UFUNCTION()
	UWidget* DynamicGetTooltipContent();
	void HideTooltip();
	void ShowTooltip();

	void InitBasicTooltip();
	void UpdateBasicTooltipInternal();

	/** Every UT widget is wrapped inside of a menu anchor that is used to display anchored tooltips. */
	TWeakObjectPtr<class UMenuAnchor> RootTooltipAnchor;

	UPROPERTY(Transient)
	class UUTBasicTooltipWidget* BasicTooltipWidget;
};