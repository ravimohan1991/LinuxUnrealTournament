// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTHeartBeatManager.generated.h"

UCLASS()
class UUTHeartbeatManager : public UObject
{
	GENERATED_UCLASS_BODY()

public:

	void StartManager(AUTBasePlayerController* UTPlayerController);

	void StopManager();

private:

	void DoMinuteEvents();

	void DoFiveSecondEvents();

	void DoMinuteEventsServer();

	void DoMinuteEventsLocal();

	void DoFiveSecondEventsServer();

	void DoFiveSecondEventsLocal();

	void SendPlayerContextLocationPerMinute();

	FTimerHandle DoMinuteEventsTimerHandle;
	FTimerHandle DoFiveSecondEventsTimerHandle;

	AUTBasePlayerController* UTPC;
};
