// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Runtime/Core/Public/Features/IModularFeature.h"

class UTVoiceChatTokenFeature : public IModularFeature
{
public:
	virtual void GenerateClientLoginToken(const FString& PlayerName, FString& Token) = 0;
	virtual void GenerateClientJoinToken(const FString& PlayerName, const FString& RoomId, FString& Token) = 0;
};