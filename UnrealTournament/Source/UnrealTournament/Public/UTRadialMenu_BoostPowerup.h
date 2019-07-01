// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once 

#include "UTHUDWidget.h"
#include "UTRadialMenu.h"
#include "UTRadialMenu_BoostPowerup.generated.h"

UCLASS()
class UNREALTOURNAMENT_API UUTRadialMenu_BoostPowerup : public UUTRadialMenu
{
	GENERATED_UCLASS_BODY()

public:
	virtual void InitializeWidget(AUTHUD* Hud);
	virtual void BecomeInteractive() override
	{
		Super::BecomeInteractive();
		CurrentSegment = INDEX_NONE;
	}
	virtual void TriggerPowerup();

	int32 SelectedPowerup;
	float RotationRemaining;

protected:
	virtual void Execute() override;
	virtual void DrawMenu(FVector2D ScreenCenter, float RenderDelta) override;
	virtual void Draw_Implementation(float DeltaTime) override;
	
	virtual bool ShouldDraw_Implementation(bool bShowScores) override;
};

