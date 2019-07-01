// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once
#include "UTHUDWidgetMessage.h"
#include "UTHUDWidgetMessage_VoiceChatStatus.generated.h"

UCLASS()
class UNREALTOURNAMENT_API UUTHUDWidgetMessage_VoiceChatStatus : public UUTHUDWidgetMessage
{
	GENERATED_UCLASS_BODY()

public:
	virtual bool ShouldDraw_Implementation(bool bShowScores) override
	{
		return (!UTHUDOwner->UTPlayerOwner || !UTHUDOwner->UTPlayerOwner->UTPlayerState
			|| !UTHUDOwner->UTPlayerOwner->UTPlayerState->bOnlySpectator || !UTHUDOwner->UTPlayerOwner->bShowCameraBinds);
	}

protected:
	virtual void DrawMessages(float DeltaTime) override;
	virtual void DrawTalker(const FString& PlayerName, int32 TeamNum, float X, float Y, float XL, float YL);
	virtual float GetDrawScaleOverride() override;
};
