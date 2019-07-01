// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UnrealTournament.h"

#include "UTLiftMeshComponent.generated.h"

UCLASS(CustomConstructor, ClassGroup = Rendering, editinlinenew, meta = (BlueprintSpawnableComponent = ""))
class UNREALTOURNAMENT_API UUTLiftMeshComponent : public UStaticMeshComponent
{
	GENERATED_UCLASS_BODY()

public:

	UUTLiftMeshComponent(const FObjectInitializer& OI)
		: Super(OI)
	{}

	UFUNCTION(BlueprintCallable, Category="Rendering")
	void SetIndirectLightingCacheQuality(EIndirectLightingCacheQuality Quality)
	{
		IndirectLightingCacheQuality = Quality;
		if (!IsRunningDedicatedServer())
		{
			DestroyRenderState_Concurrent();
			FlushRenderingCommands();
			CreateRenderState_Concurrent();
		}
	}
};