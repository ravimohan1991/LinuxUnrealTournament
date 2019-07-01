// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTUserWidgetBase.h"

#include "UTMcpTypeRewardImage.generated.h"

class UUTLazyImage;

UCLASS()
class UUTMcpTypeRewardImage : public UUTUserWidgetBase
{
	GENERATED_BODY()

public:

	virtual void NativeConstruct() override;

	void SetImageWithDefinition(UUtMcpDefinition& ItemDef);

private:

	UPROPERTY(meta = (BindWidget))
	UUTLazyImage* Image_RewardImage;
};
