// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTProjectile.h"
#include "UTProj_Lightning.generated.h"

UCLASS(Abstract)
class UNREALTOURNAMENT_API AUTProj_Lightning : public AUTProjectile
{
	GENERATED_UCLASS_BODY()

public:
	virtual bool ShouldIgnoreHit_Implementation(AActor* OtherActor, UPrimitiveComponent* OtherComp) override;
};
