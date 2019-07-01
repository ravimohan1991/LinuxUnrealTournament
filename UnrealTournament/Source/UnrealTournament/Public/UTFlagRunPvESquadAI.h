// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UTAsymCTFSquadAI.h"

#include "UTFlagRunPvESquadAI.generated.h"

UCLASS()
class UNREALTOURNAMENT_API AUTFlagRunPvESquadAI : public AUTAsymCTFSquadAI
{
	GENERATED_BODY()
public:
	virtual bool ShouldStartRally(AUTBot* B) override;
};