// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTFlag.h"
#include "UTCTFFlag.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTCTFFlag : public AUTFlag
{
	GENERATED_UCLASS_BODY()
public:

		/** used to trigger the capture effect */
	UPROPERTY(ReplicatedUsing = PlayCaptureEffect)
		FVector_NetQuantize CaptureEffectLoc;

	virtual void PreNetReceive() override;
	virtual void PostNetReceiveLocationAndRotation() override;
	virtual void PlayCaptureEffect() override;
	virtual void Drop(AController* Killer) override;
};