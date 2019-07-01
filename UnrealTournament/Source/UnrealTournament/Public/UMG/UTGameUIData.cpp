// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTGameUIData.h"
#include "UTGlobals.h"


UUTGameUIData::UUTGameUIData(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

UUTGameUIData& UUTGameUIData::Get()
{
	// If there's no globals or UI data, something is very wrong!
	return *UUTGlobals::Get().GetGameUIData();
}