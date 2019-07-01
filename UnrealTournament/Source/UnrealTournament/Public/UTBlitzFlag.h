// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTFlag.h"
#include "UTBlitzFlag.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTBlitzFlag : public AUTFlag
{
	GENERATED_UCLASS_BODY()

	virtual	void SendHomeWithNotify() override;
	virtual void Drop(AController* Killer) override;

};
