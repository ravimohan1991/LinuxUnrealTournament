// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BackgroundBlurWidget.h"
#include "UTBlurWidget.generated.h"

/** This just exists so we can easily set a default fallback image */
UCLASS()
class UNREALTOURNAMENT_API UUTBlurWidget : public UBackgroundBlurWidget
{
	GENERATED_UCLASS_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = UTBlur)
	void SetColorAndOpacity(FLinearColor InColorAndOpacity);
	
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;


protected:
	
	//virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void SynchronizeProperties() override;

private:
	UPROPERTY(EditAnywhere, meta = (ClampMin=0, ClampMax=1))
	FLinearColor ColorAndOpacity;

	TSharedPtr<class SUTBackgroundBlur> MyUTBlur;
};