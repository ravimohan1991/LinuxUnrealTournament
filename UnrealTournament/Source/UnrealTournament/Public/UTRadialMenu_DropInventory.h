// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once 

#include "UTHUDWidget.h"
#include "UTRadialMenu.h"
#include "UTRadialMenu_DropInventory.generated.h"

class AUTTimedPowerup;

UCLASS()
class UNREALTOURNAMENT_API UUTRadialMenu_DropInventory : public UUTRadialMenu
{
	GENERATED_UCLASS_BODY()

public:
	virtual void InitializeWidget(AUTHUD* Hud);

protected:

	virtual void Execute();
	virtual void DrawMenu(FVector2D ScreenCenter, float RenderDelta);
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RenderObject")
	FHUDRenderObject_Texture FlagIcon;
	FHUDRenderObject_Texture HealthIcon;
	FHUDRenderObject_Texture ArmorIcon;
	FHUDRenderObject_Texture BootsIcon;
	FHUDRenderObject_Texture ShieldIcon;

	FHUDRenderObject_Texture PowerupIcon;

	TArray<AUTTimedPowerup*> Powerups;
	AUTTimedPowerup* SelectedPowerup;

	virtual bool ShouldDraw_Implementation(bool bShowScores)
	{
		return UTHUDOwner && UTHUDOwner->bShowDropMenu;
	}

};

