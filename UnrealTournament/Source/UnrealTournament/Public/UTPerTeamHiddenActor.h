// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTWorldSettings.h"
#include "UTTeamInterface.h"
#include "UTPerTeamHiddenActor.generated.h"

/** base class of states that fire the weapon and live in the weapon's FiringState array */
UCLASS(CustomConstructor)
class UNREALTOURNAMENT_API AUTPerTeamHiddenActor : public AActor, public IUTTeamInterface
{
	GENERATED_UCLASS_BODY()

	AUTPerTeamHiddenActor(const FObjectInitializer& OI)
		: Super(OI)
	{}

	void BeginPlay() override
	{
		Super::BeginPlay();

		if (!IsPendingKillPending())
		{
			AUTWorldSettings* WS = Cast<AUTWorldSettings>(GetWorld()->GetWorldSettings());
			if (WS != NULL)
			{
				WS->PerTeamHiddenActors.Add(this);
			}
		}
	}

	void EndPlay(const EEndPlayReason::Type EndPlayReason) override
	{
		Super::EndPlay(EndPlayReason);

		AUTWorldSettings* WS = Cast<AUTWorldSettings>(GetWorld()->GetWorldSettings());
		if (WS != NULL)
		{
			WS->PerTeamHiddenActors.Remove(this);
		}
	}

	virtual void SetTeamForSideSwap_Implementation(uint8 NewTeamNum) override
	{}

	virtual uint8 GetTeamNum() const override
	{
		static FName NAME_ScriptGetTeamNum(TEXT("ScriptGetTeamNum"));
		UFunction* GetTeamNumFunc = GetClass()->FindFunctionByName(NAME_ScriptGetTeamNum);
		if (GetTeamNumFunc != NULL && GetTeamNumFunc->Script.Num() > 0)
		{
			return IUTTeamInterface::Execute_ScriptGetTeamNum(const_cast<AUTPerTeamHiddenActor*>(this));
		}
		return 0;
	}
};