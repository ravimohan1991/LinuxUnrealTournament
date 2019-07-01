// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#include "UnrealTournament.h"
#include "UTHUD_InstantReplay.h"

AUTHUD_InstantReplay::AUTHUD_InstantReplay(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	InstantReplayText = NSLOCTEXT("UTHUD", "InstantReplay", "INSTANT REPLAY");
}

EInputMode::Type AUTHUD_InstantReplay::GetInputMode_Implementation() const
{
	return EInputMode::EIM_GameOnly;
}

void AUTHUD_InstantReplay::DrawHUD()
{
	Super::DrawHUD();

	float RenderScale = Canvas->ClipX / 1920.0f;
	float XL, YL;
	Canvas->DrawColor = FColor(255, 255, 255, 255);
	Canvas->TextSize(LargeFont, InstantReplayText.ToString(), XL, YL, 1.f, 1.f);
	Canvas->DrawText(LargeFont, InstantReplayText, 0.5f*Canvas->ClipX - 0.5f*XL*RenderScale, 0.1f*Canvas->ClipY, RenderScale, RenderScale);
}