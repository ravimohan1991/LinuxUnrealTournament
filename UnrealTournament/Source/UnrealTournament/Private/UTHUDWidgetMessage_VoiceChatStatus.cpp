// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTHUDWidgetMessage.h"
#include "UTHUDWidgetMessage_VoiceChatStatus.h"
#include "UTPlayerState.h"
#include "UTGameViewportClient.h"

UUTHUDWidgetMessage_VoiceChatStatus::UUTHUDWidgetMessage_VoiceChatStatus(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	ManagedMessageArea = FName(TEXT("VoiceChatStatus"));
	Position = FVector2D(0.0f, 0.0f);
	ScreenPosition = FVector2D(0.01f, 0.82f);
	LineupPositionOffset = FVector2D(0.f, -0.2f);
	Size = FVector2D(0.0f, 0.0f);
	Origin = FVector2D(0.0f, 0.0f);
	NumVisibleLines = 4;
	LargeShadowDirection = FVector2D(1.f, 1.f);
	SmallShadowDirection = FVector2D(1.f, 1.f);
}

// @TODO FIXMESTEVE temp - need smaller font
float UUTHUDWidgetMessage_VoiceChatStatus::GetDrawScaleOverride()
{
	return UTHUDOwner->GetHUDWidgetScaleOverride();
}

void UUTHUDWidgetMessage_VoiceChatStatus::DrawMessages(float DeltaTime)
{
	Canvas->Reset();

	float XL, SmallYL;
	Canvas->TextSize(UTHUDOwner->GetFontFromSizeIndex(-1), "TEST", XL, SmallYL, RenderScale, RenderScale);

	AUTGameState* GS = nullptr;
	if (UTPlayerOwner && UTPlayerOwner->GetLocalPlayer())
	{
		UUTGameViewportClient* UTGVC = Cast<UUTGameViewportClient>(UTPlayerOwner->GetLocalPlayer()->ViewportClient);
		if (UTGVC)
		{
			UWorld* RealWorld = UTGVC->GetWorldNoActiveWorldOverride();
			if (RealWorld)
			{
				GS = RealWorld->GetGameState<AUTGameState>();
			}
		}
	}

	if (GS)
	{
		int32 NumTalking = 0;
		float Y = 0;
		for (int i = 0; i < GS->PlayerArray.Num() && NumTalking < NumVisibleLines; i++)
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(GS->PlayerArray[i]);
			if (PS && PS->bIsTalking)
			{
                Canvas->TextSize(UTHUDOwner->GetFontFromSizeIndex(-1), PS->GetPlayerName(), XL, SmallYL, RenderScale, RenderScale);
				XL += 28 * RenderScale;
                DrawTalker(PS->GetPlayerName(), PS->Team ? PS->Team->GetTeamNum() : 0, 0, Y, XL, SmallYL);
				NumTalking++;
				Y -= SmallYL;
				Y -= 2;
			}
		}
	}
}

void UUTHUDWidgetMessage_VoiceChatStatus::DrawTalker(const FString& PlayerName, int32 TeamNum, float X, float Y, float XL, float YL)
{
	if (bScaleByDesignedResolution)
	{
		X *= RenderScale;
		Y *= RenderScale;
	}
	FVector2D RenderPos = FVector2D(RenderPosition.X + X, RenderPosition.Y + Y);
	float TextScaling = bScaleByDesignedResolution ? RenderScale : 1.0f;

	FFontRenderInfo FontRenderInfo = FFontRenderInfo();
	
	if (TeamNum == 1)
	{
		Canvas->DrawColor = FColor(7,22,188);
	}
	else
	{
		Canvas->DrawColor = FColor(188,7,7);
	}
	
	FCanvasTileItem TileItem(RenderPos, GWhiteTexture, FVector2D(XL, YL), FVector2D(0, 0), FVector2D(1, 1), Canvas->DrawColor);
	TileItem.BlendMode = ESimpleElementBlendMode::SE_BLEND_Translucent;
	Canvas->DrawItem(TileItem);

	Canvas->DrawColor = FColor::White;

	FTextureUVs ChatIconUVs = FTextureUVs(497.0f, 965.0f, 35.0f, 31.0f);
	float U = ChatIconUVs.U / UTHUDOwner->HUDAtlas->Resource->GetSizeX();
	float V = ChatIconUVs.V / UTHUDOwner->HUDAtlas->Resource->GetSizeY();
	float UL = U + (ChatIconUVs.UL / UTHUDOwner->HUDAtlas->Resource->GetSizeX());
	float VL = V + (ChatIconUVs.VL / UTHUDOwner->HUDAtlas->Resource->GetSizeY());
	FVector2D ChatIconRenderPos = RenderPos;
	ChatIconRenderPos.X += 2;
	ChatIconRenderPos.Y += 2;
	FCanvasTileItem ImageItem(ChatIconRenderPos, UTHUDOwner->HUDAtlas->Resource, FVector2D(23 * RenderScale, 20 * RenderScale), FVector2D(U, V), FVector2D(UL, VL), FLinearColor::White);
	ImageItem.BlendMode = ESimpleElementBlendMode::SE_BLEND_Translucent;
	Canvas->DrawItem(ImageItem);

	RenderPos.X += 26 * RenderScale;

	FUTCanvasTextItem TextItem(RenderPos, FText::FromString(PlayerName), UTHUDOwner->GetFontFromSizeIndex(-1), Canvas->DrawColor, WordWrapper);
	TextItem.FontRenderInfo = FontRenderInfo;
	TextItem.Scale = FVector2D(TextScaling, TextScaling);
	TextItem.EnableShadow(ShadowColor);
	Canvas->DrawItem(TextItem);
}
