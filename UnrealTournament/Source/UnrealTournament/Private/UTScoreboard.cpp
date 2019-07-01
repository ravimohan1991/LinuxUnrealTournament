// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTScoreboard.h"
#include "UTHUDWidget_Spectator.h"
#include "StatNames.h"
#include "UTPickupWeapon.h"
#include "UTWeapon.h"
#include "UTWeap_Enforcer.h"
#include "UTWeap_ImpactHammer.h"
#include "UTWeap_Translocator.h"
#include "UTDemoRecSpectator.h"
#include "UTLineUpHelper.h"
#include "UTBot.h"
#include "UTLocalPlayer.h"

UUTScoreboard::UUTScoreboard(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DesignedResolution = 1920.f;
	Position = FVector2D(0.f, 0.f);
	Size = FVector2D(1920.0f, 1080.0f);
	ScreenPosition = FVector2D(0.f, 0.f);
	Origin = FVector2D(0.f, 0.f);
	bScaleByDesignedResolution = false;

	ColumnHeaderPlayerX = 0.1f;
	ColumnHeaderScoreX = 0.63f;
	ColumnHeaderDeathsX = 0.79f;
	ColumnHeaderPingX = 0.95f;
	ColumnHeaderY = 6.f;
	ColumnY = 12.f;
	ColumnMedalX = 0.55f;
	CellHeight = 32.f;
	CellWidth = 530.f;
	EdgeWidth = 420.f;
	FlagX = 0.01f;
	MinimapCenter = FVector2D(0.75f, 0.5f);

	KillsColumn = 0.4f;
	DeathsColumn = 0.52f;
	ShotsColumn = 0.72f;
	AccuracyColumn = 0.84f;
	ValueColumn = 0.5f;
	ScoreColumn = 0.85f;
	bHighlightStatsLineTopValue = false;

	static ConstructorHelpers::FObjectFinder<USoundBase> OtherSpreeSoundFinder(TEXT("SoundCue'/Game/RestrictedAssets/Audio/UI/ScoreUpdate_Cue.ScoreUpdate_Cue'"));
	ScoreUpdateSound = OtherSpreeSoundFinder.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> XPGainedSoundFinder(TEXT("SoundWave'/Game/RestrictedAssets/Audio/Stingers/XPGained.XPGained'"));
	XPGainedSound = XPGainedSoundFinder.Object;

	static ConstructorHelpers::FObjectFinder<USoundBase> LevelUpSoundFinder(TEXT("/Game/RestrictedAssets/Audio/Gameplay/A_Gameplay_CTF_CaptureSound02.A_Gameplay_CTF_CaptureSound02"));
	LevelUpSound = LevelUpSoundFinder.Object;

	static ConstructorHelpers::FObjectFinder<UTexture2D> HighlightIconFinder(TEXT("/Game/RestrictedAssets/UI/RankBadges/UT_RankedPlatinum_128x128.UT_RankedPlatinum_128x128"));
	HighlightIcon = HighlightIconFinder.Object;

	GameMessageText = NSLOCTEXT("UTScoreboard", "ScoreboardHeader", "{GameName}{Difficulty} in {MapName}");
	CH_PlayerName = NSLOCTEXT("UTScoreboard", "ColumnHeader_PlayerName", "Player");
	CH_Score = NSLOCTEXT("UTScoreboard", "ColumnHeader_PlayerScore", "Score");
	CH_Kills = NSLOCTEXT("UTScoreboard", "ColumnHeader_PlayerKills", "K");
	CH_Deaths = NSLOCTEXT("UTScoreboard", "ColumnHeader_PlayerDeaths", "D");
	CH_Skill = NSLOCTEXT("UTScoreboard", "ColumnHeader_BotSkill", "SKILL");
	CH_Ping = NSLOCTEXT("UTScoreboard", "ColumnHeader_PlayerPing", "Ping");
	CH_Ready = NSLOCTEXT("UTScoreboard", "ColumnHeader_Ready", "");
	OneSpectatorWatchingText = NSLOCTEXT("UTScoreboard", "OneSpectator", "1 spectator is watching this match");
	SpectatorsWatchingText = NSLOCTEXT("UTScoreboard", "SpectatorFormat", "{0} spectators are watching this match");
	PingFormatText = NSLOCTEXT("UTScoreboard", "PingFormatText", "{0}ms");
	PositionFormatText = NSLOCTEXT("UTScoreboard", "PositionFormatText", "{0}.");
	TeamSwapText = NSLOCTEXT("UTScoreboard", "TEAMSWITCH", "TEAM SWAP");
	ReadyText = NSLOCTEXT("UTScoreboard", "READY", "READY");
	NotReadyText = NSLOCTEXT("UTScoreboard", "NOTREADY", "");
	WarmupText = NSLOCTEXT("UTScoreboard", "WARMUP", "WARMUP");
	InteractiveText = NSLOCTEXT("UTScoreboard", "InteractiveText", "Press [ESC] to toggle interactive scoreboard.");
	InteractiveStandaloneText = NSLOCTEXT("UTScoreboard", "InteractiveStandaloneText", "Press [ESC] to toggle interactive scoreboard and pause.");
	InteractiveHintA = NSLOCTEXT("UTScoreboard", "InteractiveHintA", "Right click on player name to see interactive options.");
	DifficultyText[0] = NSLOCTEXT("UTScoreboard", "Normal", " vs. Normal AI");
	DifficultyText[1] = NSLOCTEXT("UTScoreboard", "Hard", " vs. Hard AI");
	DifficultyText[2] = NSLOCTEXT("UTScoreboard", "Inhuman", " vs. Inhuman AI");

	ReadyColor = FLinearColor::White;
	ReadyScale = 1.f;
	bDrawMinimapInScoreboard = true;
	MinimapPadding = 12.f;
	bShouldKickBack = false;
}

void UUTScoreboard::PreDraw(float DeltaTime, AUTHUD* InUTHUDOwner, UCanvas* InCanvas, FVector2D InCanvasCenter)
{
	Super::PreDraw(DeltaTime, InUTHUDOwner, InCanvas, InCanvasCenter);

	ActualPlayerCount=0;
	if (UTGameState)
	{
		for (int32 i = 0; i < UTGameState->PlayerArray.Num(); i++)
		{
			if (UTGameState->PlayerArray[i] && !UTGameState->PlayerArray[i]->bOnlySpectator)
			{
				ActualPlayerCount++;
			}
		}
	}

	RenderScale = Canvas->ClipX / DesignedResolution;
	RenderScale *= GetDrawScaleOverride();

	// Apply any scaling
	RenderSize.Y = Size.Y * RenderScale;
	if (Size.X > 0.f)
	{
		RenderSize.X = (bMaintainAspectRatio ? RenderSize.Y * AspectScale : RenderSize.X * RenderScale);
	}
	ColumnY = 12.f *RenderScale;
	ScaledEdgeSize = EdgeWidth*RenderScale;
	ScaledCellWidth = RenderScale * CellWidth;
	FooterPosY = 1032.f * RenderScale;
}

void UUTScoreboard::Draw_Implementation(float RenderDelta)
{
	Super::Draw_Implementation(RenderDelta);

	float YOffset = 48.f*RenderScale;
	DrawGamePanel(RenderDelta, YOffset);
	DrawTeamPanel(RenderDelta, YOffset);

	if (UTHUDOwner && UTHUDOwner->bDisplayMatchSummary && !bIsInteractive)
	{
		DrawMatchSummary(RenderDelta);
	}
	else
	{
		if (UTGameState != nullptr && UTGameState->GetMatchState() != MatchState::CountdownToBegin && UTGameState->GetMatchState() != MatchState::PlayerIntro)
		{
			DrawScorePanel(RenderDelta, YOffset);
		}
		if (ShouldDrawScoringStats())
		{
			DrawScoringStats(RenderDelta, YOffset);
		}
		else
		{
			DrawCurrentLifeStats(RenderDelta, YOffset);
		}
		DrawMinimap(RenderDelta);
	}
}

void UUTScoreboard::DrawMinimap(float RenderDelta)
{
	if (bDrawMinimapInScoreboard && UTGameState && UTHUDOwner && !UTHUDOwner->IsPendingKillPending() && !UTGameState->HasMatchEnded() && (UTGameState->GetMatchState() != MatchState::CountdownToBegin) && (UTGameState->GetMatchState() != MatchState::PlayerIntro))
	{
		float MapScaleX = (UTHUDOwner->MinimapScaleX > 0.f) ? UTHUDOwner->MinimapScaleX : 1.f;
		float MapSize = (UTGameState && UTGameState->bTeamGame)
				? (Canvas->ClipX - 2.f*ScaledEdgeSize - 2.f*ScaledCellWidth - MinimapPadding*RenderScale)/MapScaleX 
				: 0.5f*Canvas->ClipX;

		MapSize = FMath::Min(MapSize, 0.93f*Canvas->ClipY - 200.f * RenderScale);
		float MapYPos = FMath::Max(200.f*RenderScale, MinimapCenter.Y*Canvas->ClipY - 0.5f*MapSize);
		FVector2D LeftCorner = FVector2D(MinimapCenter.X*Canvas->ClipX - 0.5f*MapSize, MapYPos);
		float BackgroundScale = 1.f - 0.75f*(1.f - MapScaleX);
		float BackgroundX = LeftCorner.X + MapSize * 0.5f * (1.f - BackgroundScale);
		DrawTexture(UTHUDOwner->ScoreboardAtlas, BackgroundX, LeftCorner.Y, MapSize*BackgroundScale, MapSize, 149, 138, 32, 32, 0.5f, FLinearColor::Black);
		LeftCorner.X += MapSize * 0.5f * (1.f - MapScaleX);
		UTHUDOwner->DrawMinimap(FColor(192, 192, 192, 220), MapSize, LeftCorner);
	}
}

void UUTScoreboard::DrawMatchSummary(float RenderDelta)
{
	AUTPlayerState* ViewedPS = UTHUDOwner ? UTHUDOwner->GetScorerPlayerState() : nullptr;
	AUTGameState* GS = GetWorld()->GetGameState<AUTGameState>();
	if (ViewedPS && GS)
	{
		float SummaryTime = GetWorld()->GetTimeSeconds() - UTHUDOwner->MatchSummaryTime;
		int32 NumHighlights = 0;
		for (int32 i = 0; i < 5; i++)
		{
			if (ViewedPS->MatchHighlights[i] != NAME_None)
			{
				NumHighlights++;
			}
		}
		int32 NumHighlightsToShow = FMath::Min(NumHighlights, int32(2.f * SummaryTime));
		if (NumHighlightsToShow != FMath::Min(NumHighlights, int32(2.f * (SummaryTime - RenderDelta))))
		{
			UTHUDOwner->UTPlayerOwner->ClientPlaySound(ScoreUpdateSound);
		}
		const float HighlightWidth = 300.f*RenderScale;
		const float HighlightSpace = 100.f*RenderScale;
		const float HighlightHeight = 0.3f * Canvas->ClipY;
		const float HighlightY = 0.25f * Canvas->ClipY;
		float HighlightPosX = 0.5f*(Canvas->ClipX - NumHighlights*HighlightWidth - FMath::Max(0, NumHighlights-1) * HighlightSpace);
		FLinearColor ShadowColor = FLinearColor::Black;
		ShadowColor.A = 0.8f;
		FLinearColor HighlightTextColor = FLinearColor::White;
		HighlightTextColor.A = 0.8f;
		NumHighlightsToShow = FMath::Min(NumHighlightsToShow, NumHighlights);
		for (int32 i = 0; i < NumHighlightsToShow; i++)
		{
			if (ViewedPS->MatchHighlights[i] != NAME_None)
			{
				DrawFramedBackground(HighlightPosX, HighlightY, HighlightWidth, HighlightHeight);

				float TextXL, TextYL;
				Canvas->TextSize(UTHUDOwner->LargeFont, GS->ShortPlayerHighlightText(ViewedPS, i).ToString(), TextXL, TextYL, 1.0f, 1.0f);
				float TinyXL, TinyYL;
				Canvas->TextSize(UTHUDOwner->SmallFont, GS->FormatPlayerHighlightText(ViewedPS, i).ToString(), TinyXL, TinyYL, 1.0f, 1.0f);

				float MaxTextWidth = 0.98f*HighlightWidth;
				FUTCanvasTextItem ShortHighlightTextItem(FVector2D(HighlightPosX + 0.5f*HighlightWidth - 0.5f*FMath::Min(MaxTextWidth,TextXL*RenderScale), HighlightY + HighlightHeight - TextYL*RenderScale - TinyYL*RenderScale), GS->ShortPlayerHighlightText(ViewedPS, i), UTHUDOwner->LargeFont, HighlightTextColor, NULL);
				ShortHighlightTextItem.Scale = FVector2D(RenderScale * FMath::Min(1.f, MaxTextWidth /FMath::Max(1.f, TextXL*RenderScale)), RenderScale);
				ShortHighlightTextItem.BlendMode = SE_BLEND_Translucent;
				ShortHighlightTextItem.EnableShadow(ShadowColor);
				ShortHighlightTextItem.FontRenderInfo = Canvas->CreateFontRenderInfo(true, false);
				Canvas->DrawItem(ShortHighlightTextItem);

				FUTCanvasTextItem HighlightTextItem(FVector2D(HighlightPosX + 0.5f*HighlightWidth - 0.5f*FMath::Min(MaxTextWidth, TinyXL*RenderScale), HighlightY + HighlightHeight - 1.1f*TinyYL*RenderScale), GS->FormatPlayerHighlightText(ViewedPS, i), UTHUDOwner->SmallFont, HighlightTextColor, NULL);
				HighlightTextItem.Scale = FVector2D(RenderScale * FMath::Min(1.f, MaxTextWidth / FMath::Max(1.f, TinyXL*RenderScale)), RenderScale);
				HighlightTextItem.BlendMode = SE_BLEND_Translucent;
				HighlightTextItem.EnableShadow(ShadowColor);
				HighlightTextItem.FontRenderInfo = Canvas->CreateFontRenderInfo(true, false);
				Canvas->DrawItem(HighlightTextItem);

				DrawTexture(HighlightIcon, HighlightPosX + 0.5f*(HighlightWidth - 128.f*RenderScale), HighlightY + 48.f*RenderScale, 128.f*RenderScale, 128.f*RenderScale, 0, 0, 128, 128, 1.f, FLinearColor::White);
				HighlightPosX += HighlightWidth + HighlightSpace;
			}
		}
		if (NumHighlights > NumHighlightsToShow)
		{
			// draw in box
			float BoxScale = 2.f - (2.f*SummaryTime - int32(2.f*SummaryTime));
			if (BoxScale < 1.8f)
			{
				DrawFramedBackground(HighlightPosX - 0.5f*(BoxScale - 1.f)*HighlightWidth, HighlightY - 0.5f*(BoxScale - 1.f)*HighlightHeight, BoxScale*HighlightWidth, BoxScale*HighlightHeight);
			}
		}

		UUTLocalPlayer* LP = UTHUDOwner->UTPlayerOwner ? Cast<UUTLocalPlayer>(UTHUDOwner->UTPlayerOwner->GetLocalPlayer()) : nullptr;
		if (LP && LP->IsEarningXP()) 
		{
			float XPUpdateStart = 1.5f + 0.5f*NumHighlights;
			float LevelUpdateStart = XPUpdateStart + 2.f;
			bool bGiveReward = (SummaryTime > LevelUpdateStart);
			float XPWidth = 0.6 * Canvas->ClipX;
			float XPHeight = 0.3f * Canvas->ClipY;
			float XPBoxX = 0.5f * (Canvas->ClipX - XPWidth);
			float XPBoxY = 0.6f*Canvas->ClipY;
			DrawFramedBackground(XPBoxX, XPBoxY, XPWidth, XPHeight);

			FUTCanvasTextItem XPTextItem(FVector2D(XPBoxX + 0.1f*Canvas->ClipX, XPBoxY + 0.01f*Canvas->ClipY), NSLOCTEXT("UTScoreboard", "XP", "XP"), UTHUDOwner->LargeFont, HighlightTextColor, NULL);
			XPTextItem.Scale = FVector2D(RenderScale, RenderScale);
			XPTextItem.BlendMode = SE_BLEND_Translucent;
			XPTextItem.EnableShadow(ShadowColor);
			XPTextItem.FontRenderInfo = Canvas->CreateFontRenderInfo(true, true);
			Canvas->DrawItem(XPTextItem);

			float XPBarWidth = 0.8f * XPWidth;
			float XPBarX = XPBoxX + 0.5f*(XPWidth - XPBarWidth);
			float XPBarY = XPBoxY + 0.3f * XPHeight;
			float XPBarHeight = 0.1f * XPHeight;
			DrawTexture(UTHUDOwner->HUDAtlas, XPBarX, XPBarY, XPBarWidth, XPBarHeight, 185.f, 400.f, 4.f, 4.f, 1.0f, FLinearColor::Black);

			int32 CurrentXP = LP->GetOnlineXP();
			FXPBreakdown XPBreakdown = UTHUDOwner->UTPlayerOwner->XPBreakdown;
			int32 NewXP = XPBreakdown.Total();  //28 FIXMESTEVE
			if (CurrentXP > UTPlayerOwner->UTPlayerState->GetPrevXP())
			{
				CurrentXP -= NewXP;
			}
		
			int32 Level = GetLevelForXP(CurrentXP);
			int32 LevelXPStart = GetXPForLevel(Level);
			int32 LevelXPEnd = FMath::Max(GetXPForLevel(Level + 1), LevelXPStart+1);
			int32 LevelXP = LevelXPEnd - LevelXPStart;
			int32 DisplayedXP = CurrentXP - LevelXPStart;
			float CurrentXPWidth = XPBarWidth*FMath::Min(1.f, float(DisplayedXP) / float(LevelXP)) - 4.f*RenderScale;
			DrawTexture(UTHUDOwner->HUDAtlas, XPBarX + 2.f*RenderScale, XPBarY + 2.f*RenderScale, CurrentXPWidth, XPBarHeight - 4.f*RenderScale, 185.f, 400.f, 4.f, 4.f, 1.0f, FLinearColor::Green);

			FUTCanvasTextItem XPValueItem(FVector2D(XPBarX, XPBarY + 0.75f*XPBarHeight), FText::AsNumber(LevelXPStart), UTHUDOwner->TinyFont, HighlightTextColor, NULL);
			XPValueItem.Scale = FVector2D(RenderScale, RenderScale);
			XPValueItem.BlendMode = SE_BLEND_Translucent;
			XPValueItem.EnableShadow(ShadowColor);
			XPValueItem.FontRenderInfo = XPTextItem.FontRenderInfo;
			Canvas->DrawItem(XPValueItem);

			XPValueItem.Text = FText::AsNumber(LevelXPEnd);
			XPValueItem.Position.X = XPBarX + XPBarWidth;
			Canvas->DrawItem(XPValueItem);

			int32 NewLevel = GetLevelForXP(LP->GetOnlineXP());
			FFormatNamedArguments Args;
			Args.Add("LevelNum", FText::AsNumber(bGiveReward ? NewLevel : Level));
			FText LevelText = FText::Format(NSLOCTEXT("UTScoreboard", "Level", "Level {LevelNum}"), Args);
			FUTCanvasTextItem LevelTextItem(FVector2D(XPBarX, XPBarY+1.5f*XPBarHeight), LevelText, UTHUDOwner->MediumFont, HighlightTextColor, NULL);
			LevelTextItem.Scale = FVector2D(RenderScale, RenderScale);
			LevelTextItem.BlendMode = SE_BLEND_Translucent;
			LevelTextItem.EnableShadow(ShadowColor);
			LevelTextItem.FontRenderInfo = XPTextItem.FontRenderInfo;
			Canvas->DrawItem(LevelTextItem);

			// FIXMESTEVE TEMP copied from UpdateBackendContentCommandlet
			TArray<FString> LevelUpRewards;
			LevelUpRewards.AddZeroed(51);
			LevelUpRewards[2] = FString(TEXT("/Game/RestrictedAssets/ProfileItems/BeanieBlack.BeanieBlack"));
			LevelUpRewards[3] = FString(TEXT("/Game/RestrictedAssets/ProfileItems/Sunglasses.Sunglasses"));
			LevelUpRewards[4] = FString(TEXT("/Game/RestrictedAssets/ProfileItems/HockeyMask.HockeyMask"));
			LevelUpRewards[5] = FString(TEXT("/Game/RestrictedAssets/ProfileItems/ThundercrashMale05.ThundercrashMale05"));
			LevelUpRewards[7] = FString(TEXT("/Game/RestrictedAssets/ProfileItems/NecrisMale01.NecrisMale01"));
			LevelUpRewards[8] = FString(TEXT("/Game/RestrictedAssets/ProfileItems/ThundercrashMale03.ThundercrashMale03"));
			LevelUpRewards[10] = FString(TEXT("/Game/RestrictedAssets/ProfileItems/NecrisHelm01.NecrisHelm01"));
			LevelUpRewards[12] = FString(TEXT("/Game/RestrictedAssets/ProfileItems/ThundercrashBeanieGreen.ThundercrashBeanieGreen"));
			LevelUpRewards[14] = FString(TEXT("/Game/RestrictedAssets/ProfileItems/HockeyMask02.HockeyMask02"));
			LevelUpRewards[17] = FString(TEXT("/Game/RestrictedAssets/ProfileItems/ThundercrashMale02.ThundercrashMale02"));
			LevelUpRewards[20] = FString(TEXT("/Game/RestrictedAssets/ProfileItems/NecrisFemale02.NecrisFemale02"));
			LevelUpRewards[23] = FString(TEXT("/Game/RestrictedAssets/ProfileItems/BeanieWhite.BeanieWhite"));
			LevelUpRewards[26] = FString(TEXT("/Game/RestrictedAssets/ProfileItems/NecrisHelm02.NecrisHelm02"));
			LevelUpRewards[30] = FString(TEXT("/Game/RestrictedAssets/ProfileItems/SkaarjMale01.SkaarjMale01"));
			LevelUpRewards[34] = FString(TEXT("/Game/RestrictedAssets/ProfileItems/BeanieGrey.BeanieGrey"));
			LevelUpRewards[39] = FString(TEXT("/Game/RestrictedAssets/ProfileItems/ThundercrashBeanieRed.ThundercrashBeanieRed"));
			LevelUpRewards[40] = FString(TEXT("/Game/RestrictedAssets/ProfileItems/SkaarjMale02.SkaarjMale02"));
			LevelUpRewards[45] = FString(TEXT("/Game/RestrictedAssets/ProfileItems/ThundercrashBeret.ThundercrashBeret"));
			LevelUpRewards[50] = FString(TEXT("/Game/RestrictedAssets/ProfileItems/NecrisMale04.NecrisMale04"));

			int32 OldLevel = Level;
			if (SummaryTime > XPUpdateStart)
			{
				if (SummaryTime - RenderDelta <= XPUpdateStart)
				{
					UTHUDOwner->UTPlayerOwner->ClientPlaySound(XPGainedSound);
				}

				Args.Add("XPGained", FText::AsNumber(NewXP));
				XPValueItem.Text = FText::Format(NSLOCTEXT("UTScoreboard", "XPGained", "+{XPGained}"), Args);
				XPValueItem.SetColor(FLinearColor::Yellow);
				XPValueItem.Position.X = XPBarX + FMath::Max(CurrentXPWidth, 0.1f*XPBarWidth);
				Canvas->DrawItem(XPValueItem);

				float GlowTime = SummaryTime - XPUpdateStart;
				float XPSize = XPBarWidth* float(NewXP) / float(LevelXP);
				DrawTexture(UTHUDOwner->HUDAtlas, XPBarX + 2.f*RenderScale + CurrentXPWidth, XPBarY + 2.f*RenderScale, FMath::Min(XPSize, XPBarWidth - CurrentXPWidth - 4.f*RenderScale), XPBarHeight - 4.f*RenderScale, 185.f, 400.f, 4.f, 4.f, 1.0f, FLinearColor::Yellow);

				FLinearColor XPGlow = FLinearColor::Yellow;
				XPGlow.A = 0.3f;
				float GlowScale = (GlowTime < 0.1f) ? (1.f + 30.f*GlowTime) : FMath::Max(1.f, 4.f - 5.f*(GlowTime - 0.1f));
				DrawTexture(UTHUDOwner->HUDAtlas, XPBarX + 2.f*RenderScale + CurrentXPWidth - 0.5f * XPSize * (GlowScale - 1.f), XPBarY + 2.f*RenderScale - 0.5f * XPBarHeight * (GlowScale - 1.f), XPSize * GlowScale, (XPBarHeight*GlowScale) - 4.f*RenderScale, 185.f, 400.f, 4.f, 4.f, 0.2f, XPGlow);

				OldLevel = GetLevelForXP(CurrentXP);
				if (bGiveReward && (OldLevel != NewLevel))
				{
					if (SummaryTime - RenderDelta <= LevelUpdateStart)
					{
						UTHUDOwner->UTPlayerOwner->ClientPlaySound(LevelUpSound);
					}

					// draw level up with Scale in
					FText LevelUpText = NSLOCTEXT("UTScoreboard", "LEVELUP", "LEVEL UP!");
					float LevelUpXL, LevelUpYL;
					Canvas->TextSize(UTHUDOwner->LargeFont, LevelUpText.ToString(), LevelUpXL, LevelUpYL, 1.0f, 1.0f);
					float UpScale = FMath::Max(1.f, 3.f - 6.f * (SummaryTime - LevelUpdateStart));
					FLinearColor UpColor = FLinearColor::Yellow;
					UpColor.A = FMath::Min(1.f, 6.f*(SummaryTime - LevelUpdateStart));
					FUTCanvasTextItem LevelUpTextItem(FVector2D(XPBoxX + 0.5f * (XPWidth - LevelUpXL*UpScale*RenderScale), XPBoxY + 0.01f*Canvas->ClipY), LevelUpText, UTHUDOwner->LargeFont, UpColor, NULL);
					LevelUpTextItem.Scale = FVector2D(RenderScale*UpScale, RenderScale*UpScale);
					LevelUpTextItem.BlendMode = SE_BLEND_Translucent;
					LevelUpTextItem.EnableShadow(ShadowColor);
					LevelUpTextItem.FontRenderInfo = XPTextItem.FontRenderInfo;
					Canvas->DrawItem(LevelUpTextItem);
				}
			}

			int32 RewardLevelNum = 0;
			FText RewardText = FText::GetEmpty();
			UTexture* RewardImage = nullptr;
			if (Level < LevelUpRewards.Num() - 2)
			{
				for (int32 i = OldLevel + 1; i < LevelUpRewards.Num(); i++)
				{
					if (!LevelUpRewards[i].IsEmpty())
					{
						RewardLevelNum = i;
						UUTProfileItem* ProfileItem = LoadObject<UUTProfileItem>(NULL, *LevelUpRewards[i], NULL, LOAD_NoWarn | LOAD_Quiet);
						if (ProfileItem)
						{
							RewardText = ProfileItem->DisplayName;
							RewardImage = ProfileItem->Image.Texture;
						}
						break;
					}
				}

				if (RewardLevelNum > 0)
				{
					Args.Add("RewardLevelNum", FText::AsNumber(RewardLevelNum));
					Args.Add("NextReward", RewardText);
					FText RewardLevelText = FText::Format(NSLOCTEXT("UTScoreboard", "RewardLevel", "Unlock '{NextReward}' when you reach Level {RewardLevelNum}"), Args);
					if (bGiveReward && (RewardLevelNum <= NewLevel))
					{
						RewardLevelText = FText::Format(NSLOCTEXT("UTScoreboard", "RewardAchieved", "You unlocked '{NextReward}'!"), Args);
					}
					FUTCanvasTextItem RewardLevelTextItem(FVector2D(XPBarX + 128.f*RenderScale, XPBarY + 3.f*XPBarHeight), RewardLevelText, UTHUDOwner->MediumFont, FLinearColor::White, NULL);
					RewardLevelTextItem.Scale = FVector2D(RenderScale, RenderScale);
					RewardLevelTextItem.BlendMode = SE_BLEND_Translucent;
					RewardLevelTextItem.EnableShadow(ShadowColor);
					RewardLevelTextItem.FontRenderInfo = XPTextItem.FontRenderInfo;
					Canvas->DrawItem(RewardLevelTextItem);

					if (RewardImage)
					{ 
						DrawTexture(RewardImage, XPBarX, XPBarY + 3.f*XPBarHeight, 128.f*RenderScale, 128.f*RenderScale, 0, 0, 128, 128, 1.f, FLinearColor::White);
					}
				}
			}
		}
	}
}

void UUTScoreboard::GetTitleMessageArgs(FFormatNamedArguments& Args) const
{
	FText MapName = UTHUDOwner ? FText::FromString(UTHUDOwner->GetWorld()->GetMapName().ToUpper()) : FText::GetEmpty();
	FText GameName = FText::GetEmpty();
	FText AIText = FText::GetEmpty();
	if (UTGameState && UTGameState->GameModeClass)
	{
		AUTGameMode* DefaultGame = UTGameState->GameModeClass->GetDefaultObject<AUTGameMode>();
		if (DefaultGame)
		{
			GameName = FText::FromString(DefaultGame->DisplayName.ToString().ToUpper());
		}
		if (UTGameState->AIDifficulty > 0)
		{
			int32 ClampedIndex = FMath::Clamp(int32(UTGameState->AIDifficulty-1), 0, 2);
			AIText = DifficultyText[ClampedIndex];
		}
	}
	Args.Add("GameName", FText::AsCultureInvariant(GameName));
	Args.Add("MapName", FText::AsCultureInvariant(MapName));
	Args.Add("Difficulty", FText::AsCultureInvariant(AIText));
}

void UUTScoreboard::DrawGamePanel(float RenderDelta, float& YOffset)
{
	float MessageX = 0.f;
	float MessageY = 0.f;
	FText GameMessage = FText::GetEmpty();
	if (UTHUDOwner->ScoreMessageText.IsEmpty())
	{
		FFormatNamedArguments Args;
		GetTitleMessageArgs(Args);
		GameMessage = FText::Format(GameMessageText, Args);
		Canvas->StrLen(UTHUDOwner->MediumFont, GameMessage.ToString(), MessageX, MessageY);
		MessageX *= RenderScale;
	}
	else
	{
		MessageX = UTHUDOwner->DrawWinConditions(Canvas, UTHUDOwner->MediumFont, 220.f*RenderScale, YOffset + 2.f*RenderScale, Canvas->ClipX, RenderScale, true, true);
		float NameX;
		Canvas->StrLen(UTHUDOwner->MediumFont, TEXT("TEST"), NameX, MessageY);
	}

	if (!UTGameState || !UTGameState->IsLineUpActive() || (UTGameState && UTGameState->HasMatchStarted()))
	{
		DrawText((UTHUDOwner->GetNetMode() == NM_Standalone) ? InteractiveStandaloneText : InteractiveText, 0.5f*Canvas->ClipX, YOffset - 24.f*RenderScale, UTHUDOwner->SmallFont, RenderScale, 1.f, FLinearColor::White, ETextHorzPos::Center, ETextVertPos::Center);
	}

	// Draw the Background
	float TimerX = DrawGameOptions(RenderDelta, YOffset, 0.f, true);
	float Width = FMath::Clamp(MessageX + (TimerX + 32.f)*RenderScale, 520.f*RenderScale, Canvas->ClipX);
	float LeftEdge = 0.5f*(Canvas->ClipX - Width);
	DrawTexture(UTHUDOwner->ScoreboardAtlas,LeftEdge - 16.f*RenderScale,YOffset, Width + 32.f*RenderScale, 42.f*RenderScale, 4.f, 2.f, 124.f, 128.f, 1.f);

	if (UTHUDOwner->ScoreMessageText.IsEmpty())
	{ 
		DrawText(GameMessage, LeftEdge, YOffset + 16.f*RenderScale, UTHUDOwner->MediumFont, RenderScale, 1.f, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Center);
	}
	else
	{
		UTHUDOwner->DrawWinConditions(Canvas, UTHUDOwner->MediumFont, LeftEdge, YOffset - 0.24f*MessageY*RenderScale, Canvas->ClipX, RenderScale, false);
	}

	DrawGameOptions(RenderDelta, YOffset, LeftEdge + Width);
	YOffset += 42.f*RenderScale;	// The size of this zone.
}

float UUTScoreboard::DrawGameOptions(float RenderDelta, float& YOffset, float RightEdge, bool bGetLengthOnly)
{
	float Length = 0.f;
	if (UTGameState)
	{
		bool bShouldDrawTime = (!UTGameState->IsLineUpActive() || UTGameState->HasMatchStarted());
		float DisplayedTime = UTGameState ? UTGameState->GetClockTime() : 0.f;
		FText Timer = UTHUDOwner->ConvertTime(FText::GetEmpty(), FText::GetEmpty(), DisplayedTime, false, true, true);
		FText StatusText = UTGameState->GetGameStatusText(true);
		if (bGetLengthOnly)
		{
			float XL, YL;
			if (bShouldDrawTime)
			{
				Canvas->StrLen(UTHUDOwner->NumberFont, Timer.ToString(), XL, YL);
				Length = XL;
			}
			if (!StatusText.IsEmpty())
			{
				Canvas->StrLen(UTHUDOwner->SmallFont, StatusText.ToString(), XL, YL);
				Length += XL;
			}
			else if (UTGameState->GoalScore > 0)
			{
				// Draw Game Text
				FText Score = FText::Format(UTGameState->GoalScoreText, FText::AsNumber(UTGameState->GoalScore));
				Canvas->StrLen(UTHUDOwner->SmallFont, Score.ToString(), XL, YL);
				Length += XL;
			}
		}
		else
		{
			FVector2D TimeSize = bShouldDrawTime ? DrawText(Timer, RightEdge, YOffset + 21.f*RenderScale, UTHUDOwner->NumberFont, RenderScale, 1.f, FLinearColor::White, ETextHorzPos::Right, ETextVertPos::Center) : FVector2D(0.f, 0.f);
			FVector2D StatusSize(0.f, 0.f);
			RightEdge = RightEdge - (TimeSize.X + 8.f)*RenderScale;
			if (!StatusText.IsEmpty())
			{
				StatusSize = DrawText(StatusText, RightEdge, YOffset + 17.f*RenderScale, UTHUDOwner->SmallFont, RenderScale, 1.f, FLinearColor::Yellow, ETextHorzPos::Right, ETextVertPos::Center);
			}
			else if (UTGameState->GoalScore > 0)
			{
				// Draw Game Text
				FText Score = FText::Format(UTGameState->GoalScoreText, FText::AsNumber(UTGameState->GoalScore));
				StatusSize = DrawText(Score, RightEdge, YOffset + 21.f*RenderScale, UTHUDOwner->SmallFont, RenderScale, 1.f, FLinearColor::Yellow, ETextHorzPos::Right, ETextVertPos::Center);
			}
			Length = TimeSize.X + StatusSize.X;
		}
	}
	return Length;
}

void UUTScoreboard::DrawTeamPanel(float RenderDelta, float& YOffset)
{
	YOffset += 39.f*RenderScale; // A small gap
}

void UUTScoreboard::DrawScorePanel(float RenderDelta, float& YOffset)
{
	if (bIsInteractive)
	{
		SelectionStack.Empty();
	}
	LastScorePanelYOffset = YOffset;
	if (UTGameState && (!UTGameState->IsLineUpActive() || (UTHUDOwner && UTHUDOwner->bShowScores) || (UTHUDOwner && UTHUDOwner->bDisplayMatchSummary && bIsInteractive)))
	{
		DrawScoreHeaders(RenderDelta, YOffset);
		DrawPlayerScores(RenderDelta, YOffset);

		if (bIsInteractive && (UTGameState->GetNetMode() != NM_Standalone))
		{
			YOffset += 16.f * RenderScale;
			float XL, YL;
			Canvas->TextSize(UTHUDOwner->TinyFont, InteractiveHintA.ToString(), XL, YL, 1.f, 1.f);
			DrawFramedBackground(ScaledEdgeSize, YOffset, ScaledCellWidth, 1.1f*YL);
			DrawText(InteractiveHintA, ScaledEdgeSize + 10.f*RenderScale, YOffset + 0.5f*YL, UTHUDOwner->TinyFont, RenderScale, 1.0f, FLinearColor::White, ETextHorzPos::Left, ETextVertPos::Center);
		}
	}
}

void UUTScoreboard::DrawScoreHeaders(float RenderDelta, float& YOffset)
{
	float Height = 23.f * RenderScale;
	int32 ColumnCnt = ((UTGameState && UTGameState->bTeamGame) || ActualPlayerCount > 16) ? 2 : 1;
	float ColumnHeaderAdjustY = ColumnHeaderY * RenderScale;
	float XOffset = ScaledEdgeSize;
	for (int32 i = 0; i < ColumnCnt; i++)
	{
		// Draw the background Border
		DrawTexture(UTHUDOwner->ScoreboardAtlas, XOffset, YOffset, ScaledCellWidth, Height, 149, 138, 32, 32, 1.0, FLinearColor(0.72f, 0.72f, 0.72f, 0.85f));
		DrawText(CH_PlayerName, XOffset + (ScaledCellWidth * ColumnHeaderPlayerX), YOffset + ColumnHeaderAdjustY, UTHUDOwner->TinyFont, RenderScale, 1.0f, FLinearColor::Black, ETextHorzPos::Left, ETextVertPos::Center);

		if (UTGameState && UTGameState->HasMatchStarted())
		{
			DrawText(CH_Score, XOffset + (ScaledCellWidth * ColumnHeaderScoreX), YOffset + ColumnHeaderAdjustY, UTHUDOwner->TinyFont, RenderScale, 1.0f, FLinearColor::Black, ETextHorzPos::Center, ETextVertPos::Center);
			DrawText(CH_Deaths, XOffset + (ScaledCellWidth * ColumnHeaderDeathsX), YOffset + ColumnHeaderAdjustY, UTHUDOwner->TinyFont, RenderScale, 1.0f, FLinearColor::Black, ETextHorzPos::Center, ETextVertPos::Center);
		}
		else
		{
			DrawText(CH_Ready, XOffset + (ScaledCellWidth * ColumnHeaderScoreX), YOffset + ColumnHeaderAdjustY, UTHUDOwner->TinyFont, RenderScale, 1.0f, FLinearColor::Black, ETextHorzPos::Center, ETextVertPos::Center);
		}
		DrawText((GetWorld()->GetNetMode() == NM_Standalone) ? CH_Skill : CH_Ping, XOffset + (ScaledCellWidth * ColumnHeaderPingX), YOffset + ColumnHeaderAdjustY, UTHUDOwner->TinyFont, RenderScale, 1.0f, FLinearColor::Black, ETextHorzPos::Center, ETextVertPos::Center);
		XOffset = Canvas->ClipX - ScaledCellWidth - ScaledEdgeSize;
	}

	YOffset += Height + 4.f * RenderScale;
}

void UUTScoreboard::DrawPlayerScores(float RenderDelta, float& YOffset)
{
	if (!UTGameState)
	{
		return;
	}

	int32 Place = 1;
	int32 NumSpectators = 0;
	int32 ColumnCnt = ((UTGameState && UTGameState->bTeamGame) || ActualPlayerCount > 16) ? 2 : 1;
	float XOffset = ScaledEdgeSize;
	for (int32 i=0; i<UTGameState->PlayerArray.Num(); i++)
	{
		AUTPlayerState* PlayerState = Cast<AUTPlayerState>(UTGameState->PlayerArray[i]);
		if (PlayerState)
		{
			if (!PlayerState->bOnlySpectator)
			{
				DrawPlayer(Place, PlayerState, RenderDelta, XOffset, YOffset);
				YOffset += CellHeight*RenderScale;
				Place++;
			}
			else if (Cast<AUTDemoRecSpectator>(UTPlayerOwner) == nullptr && !PlayerState->bIsDemoRecording)
			{
				NumSpectators++;
			}
		}
	}
	
	if (UTGameState->PlayerArray.Num() <= 28 && NumSpectators > 0)
	{
		FText SpectatorCount = (NumSpectators == 1) 
			? OneSpectatorWatchingText
			: FText::Format(SpectatorsWatchingText, FText::AsNumber(NumSpectators));
		DrawText(SpectatorCount, 635.f*RenderScale, 765.f*RenderScale, UTHUDOwner->SmallFont, RenderScale, 1.0f, FLinearColor(0.75f, 0.75f, 0.75f, 1.0f), ETextHorzPos::Center, ETextVertPos::Bottom);
	}
}

FLinearColor UUTScoreboard::GetPlayerColorFor(AUTPlayerState* InPS) const
{
	return FLinearColor::White;
}

FLinearColor UUTScoreboard::GetPlayerBackgroundColorFor(AUTPlayerState* InPS) const
{
	if (UTHUDOwner->UTPlayerOwner->UTPlayerState == InPS)
	{
		return FLinearColor::Gray;
	}
	else
	{
		return FLinearColor::Black;
	}
}

FLinearColor UUTScoreboard::GetPlayerHighlightColorFor(AUTPlayerState* InPS) const
{
	//RedTeam
	if (InPS->GetTeamNum() == 0)
	{
		return FLinearColor(0.5f, 0.f, 0.f, 1.f);
	}
	else // Blue, or unknown team
	{
		return FLinearColor(0.f, 0.f, 0.5f, 1.f);
	}
}

void UUTScoreboard::DrawPlayer(int32 Index, AUTPlayerState* PlayerState, float RenderDelta, float XOffset, float YOffset)
{
	if (PlayerState == NULL) return;	

	float BarOpacity = 0.3f;
	bool bIsUnderCursor = false;

	// If we are interactive, store off the bounds of this cell for selection
	if (bIsInteractive)
	{
		FVector4 Bounds = FVector4(RenderPosition.X + XOffset, RenderPosition.Y + YOffset, 
										RenderPosition.X + XOffset + ScaledCellWidth, RenderPosition.Y + YOffset + CellHeight*RenderScale);
		SelectionStack.Add(FSelectionObject(PlayerState, Bounds));
		bIsUnderCursor = (CursorPosition.X >= Bounds.X && CursorPosition.X <= Bounds.Z && CursorPosition.Y >= Bounds.Y && CursorPosition.Y <= Bounds.W);
	}
	PlayerState->ScoreCorner = FVector(RenderPosition.X + XOffset, RenderPosition.Y + YOffset + 0.25f*CellHeight*RenderScale, 0.f);
	if (!PlayerState->Team || (PlayerState->Team->TeamIndex != 1))
	{
		PlayerState->ScoreCorner.X += ScaledCellWidth;
	}

	float NameXL, NameYL;
	float ClanXL = 0.f;
    FString DisplayName = PlayerState->GetPlayerName();
	FString ClanName = PlayerState->ClanName;
	if (!PlayerState->ClanName.IsEmpty())
	{
		ClanName = "[" + ClanName + "]";
		Canvas->TextSize(UTHUDOwner->SmallFont, ClanName, ClanXL, NameYL, 1.f, 1.f);
		ClanXL += 4.f;
	}
	float MaxNameWidth = 0.42f*ScaledCellWidth - (PlayerState->bIsFriend ? 30.f*RenderScale : 0.f);
	Canvas->TextSize(UTHUDOwner->SmallFont, DisplayName, NameXL, NameYL, 1.f, 1.f);
	UFont* NameFont = UTHUDOwner->SmallFont;
	FLinearColor DrawColor = GetPlayerColorFor(PlayerState);

	int32 Ping = PlayerState->Ping * 4;
	if (UTHUDOwner->UTPlayerOwner->UTPlayerState == PlayerState)
	{
		Ping = PlayerState->ExactPing;
		BarOpacity = 0.5f;
	}
	
	// Draw the background border.
	FLinearColor BarColor = GetPlayerBackgroundColorFor(PlayerState);
	float FinalBarOpacity = BarOpacity;
	if (bIsUnderCursor) 
	{
		BarColor = FLinearColor(0.0,0.3,0.0,1.0);
		FinalBarOpacity = 0.75f;
	}
	if (PlayerState == SelectedPlayer) 
	{
		BarColor = FLinearColor(0.0, 0.3, 0.3, 1.0);
		FinalBarOpacity = 0.75f;
	}

	DrawTexture(UTHUDOwner->ScoreboardAtlas, XOffset, YOffset, ScaledCellWidth, 0.9f*CellHeight*RenderScale, 149, 138, 32, 32, FinalBarOpacity, BarColor);	// NOTE: Once I make these interactable.. have a selection color too

	if (PlayerState->KickCount > 0)
	{
		float NumPlayers = 0.0f;

		for (int32 i=0; i < UTGameState->PlayerArray.Num(); i++)
		{
			if (!UTGameState->PlayerArray[i]->bIsSpectator && !UTGameState->PlayerArray[i]->bOnlySpectator && !UTGameState->PlayerArray[i]->bIsABot)		
			{
				if (!UTGameState->bOnlyTeamCanVoteKick || UTGameState->OnSameTeam(PlayerState,UTGameState->PlayerArray[i]) )
				{
					NumPlayers += 1.0f;
				}
			}
		}

		if (NumPlayers > 0.0f)
		{
			float KickPercent = float(PlayerState->KickCount) / NumPlayers;
			float XL, SmallYL;
			Canvas->TextSize(UTHUDOwner->SmallFont, "Kick", XL, SmallYL, RenderScale, RenderScale);
			DrawText(NSLOCTEXT("UTScoreboard", "Kick", "Kick"), XOffset + (ScaledCellWidth * FlagX), YOffset + ColumnY - 0.27f*SmallYL, UTHUDOwner->TinyFont, RenderScale, 1.0f, DrawColor, ETextHorzPos::Left, ETextVertPos::Center);
			FText Kick = FText::Format(NSLOCTEXT("Common", "PercFormat", "{0}%"), FText::AsNumber(int32(KickPercent * 100.0)));
			DrawText(Kick, XOffset + (ScaledCellWidth * FlagX), YOffset + ColumnY + 0.33f*SmallYL, UTHUDOwner->TinyFont, RenderScale, 1.0f, DrawColor, ETextHorzPos::Left, ETextVertPos::Center);
		}
	}
	else
	{
		FTextureUVs FlagUV;
		UTexture2D* NewFlagAtlas = UTHUDOwner->ResolveFlag(PlayerState, FlagUV);
		DrawTexture(NewFlagAtlas, XOffset + (ScaledCellWidth * FlagX), YOffset + 14.f*RenderScale, FlagUV.UL*RenderScale, FlagUV.VL*RenderScale, FlagUV.U, FlagUV.V, 36, 26, 1.0, FLinearColor::White, FVector2D(0.0f, 0.5f));
	}

	FVector2D NameSize;

	float NameScaling = FMath::Min(RenderScale, MaxNameWidth / FMath::Max(NameXL+ClanXL, 1.f));
	if (!PlayerState->EpicAccountName.IsEmpty())
	{
		NameSize = DrawText(FText::FromString(ClanName), XOffset + (ScaledCellWidth * ColumnHeaderPlayerX), YOffset + ColumnY, NameFont, NameScaling, 1.0f, DrawColor, ETextHorzPos::Left, ETextVertPos::Center);
		NameSize += DrawText(FText::FromString(DisplayName), XOffset + NameScaling*ClanXL + (ScaledCellWidth * ColumnHeaderPlayerX), YOffset + ColumnY, NameFont, false, FVector2D(0.f, 0.f), FLinearColor::Black, true, GetPlayerHighlightColorFor(PlayerState), NameScaling, 1.0f, DrawColor, FLinearColor(0.0f, 0.0f, 0.0f, 0.0f), ETextHorzPos::Left, ETextVertPos::Center);
	}
	else
	{
		NameSize = DrawText(FText::FromString(DisplayName), XOffset + (ScaledCellWidth * ColumnHeaderPlayerX), YOffset + ColumnY, NameFont, NameScaling, 1.0f, DrawColor, ETextHorzPos::Left, ETextVertPos::Center);
	}

	if (PlayerState->bIsFriend)
	{
		DrawTexture(UTHUDOwner->ScoreboardAtlas, XOffset + (ScaledCellWidth * ColumnHeaderPlayerX) + NameSize.X*NameScaling + 5.f*RenderScale, YOffset + 18.f*RenderScale, 30.f*RenderScale, 24.f*RenderScale, 236, 136, 30, 24, 1.0, FLinearColor::White, FVector2D(0.0f, 0.5f));
	}
	if (UTGameState && UTGameState->HasMatchStarted())
	{
		if (PlayerState->bPendingTeamSwitch && !PlayerState->bIsABot)
		{
			DrawText(TeamSwapText, XOffset + (ScaledCellWidth * ColumnHeaderScoreX), YOffset + ColumnY, UTHUDOwner->SmallFont, RenderScale, 1.0f, FLinearColor::White, ETextHorzPos::Center, ETextVertPos::Center);
		}
		else
		{
			DrawPlayerScore(PlayerState, XOffset, YOffset, ScaledCellWidth, DrawColor);
		}
	}
	else
	{
		DrawReadyText(PlayerState, XOffset, YOffset, ScaledCellWidth);
	}

	AUTBot* Bot = Cast<AUTBot>(PlayerState->GetOwner());
	if (Bot)
	{
		static const FNumberFormattingOptions SkillValueFormattingOptions = FNumberFormattingOptions()
			.SetMinimumFractionalDigits(1)
			.SetMaximumFractionalDigits(1);

		DrawText(FText::AsNumber(Bot->Skill, &SkillValueFormattingOptions), XOffset + 0.995f*ScaledCellWidth, YOffset + ColumnY, UTHUDOwner->TinyFont, 0.75f*RenderScale, 1.f, DrawColor, ETextHorzPos::Right, ETextVertPos::Center);
	}
	else if (GetWorld()->GetNetMode() != NM_Standalone)
	{
		FText PingText = FText::Format(PingFormatText, FText::AsNumber(Ping));
		DrawText(PingText, XOffset + 0.995f*ScaledCellWidth, YOffset + ColumnY, UTHUDOwner->TinyFont, 0.75f*RenderScale, 1.f, DrawColor, ETextHorzPos::Right, ETextVertPos::Center);
	}

	// Strike out players that are out of lives
	if (PlayerState->bOutOfLives)
	{
		float Height = 8.0f;
		float XL, YL;
        Canvas->TextSize(UTHUDOwner->SmallFont, (PlayerState->GetPlayerName() + PlayerState->ClanName), XL, YL, RenderScale, RenderScale);
		float StrikeWidth = FMath::Min(0.475f*ScaledCellWidth, XL);
		DrawTexture(UTHUDOwner->HUDAtlas, XOffset + (ScaledCellWidth * ColumnHeaderPlayerX), YOffset + ColumnY, StrikeWidth, Height, 185.f, 400.f, 4.f, 4.f, 1.0f, FLinearColor::Red);
	}
	
	// Draw the muted indicator
	if (UTHUDOwner->UTPlayerOwner->IsPlayerGameMuted(PlayerState))
	{
		bool bLeft = (XOffset < Canvas->ClipX * 0.5f);
		float TalkingXOffset = bLeft ? ScaledCellWidth + (10.0f *RenderScale) : (-36.0f * RenderScale);
		FTextureUVs ChatIconUVs = bLeft ? FTextureUVs(497.0f, 965.0f, 35.0f, 31.0f) : FTextureUVs(532.0f, 965.0f, -35.0f, 31.0f);
		DrawTexture(UTHUDOwner->HUDAtlas, XOffset + TalkingXOffset, YOffset + ((CellHeight * 0.5f - 24.0f) * RenderScale), (26 * RenderScale), (23 * RenderScale), ChatIconUVs.U, ChatIconUVs.V, ChatIconUVs.UL, ChatIconUVs.VL, 1.0f);

		FTextureUVs MuteIconUVs = FTextureUVs(410.0f, 942.0f, 64.0f, 64.0f);
		DrawTexture(UTHUDOwner->HUDAtlas, XOffset + TalkingXOffset - 3.f, YOffset + ((CellHeight * 0.5f - 30.0f) * RenderScale), (32 * RenderScale), (32 * RenderScale), MuteIconUVs.U, MuteIconUVs.V, MuteIconUVs.UL, MuteIconUVs.VL, 1.0f, FLinearColor::Red);
	}
	// Draw the talking indicator
	else if ( PlayerState->bIsTalking )
	{
		bool bLeft = (XOffset < Canvas->ClipX * 0.5f);
		float TalkingXOffset = bLeft ? ScaledCellWidth + (10.0f *RenderScale) : (-36.0f * RenderScale);
		FTextureUVs ChatIconUVs =  bLeft ? FTextureUVs(497.0f, 965.0f, 35.0f, 31.0f) : FTextureUVs(532.0f, 965.0f, -35.0f, 31.0f);
		DrawTexture(UTHUDOwner->HUDAtlas, XOffset + TalkingXOffset, YOffset + ( (CellHeight * 0.5f - 24.0f) * RenderScale), (26 * RenderScale),(23 * RenderScale), ChatIconUVs.U, ChatIconUVs.V, ChatIconUVs.UL, ChatIconUVs.VL, 1.0f);
	}
}

void UUTScoreboard::DrawReadyText(AUTPlayerState* PlayerState, float XOffset, float YOffset, float Width)
{
	FText PlayerReady = PlayerState->bIsWarmingUp ? WarmupText : NotReadyText;
	float ReadyX = XOffset + ScaledCellWidth * ColumnHeaderScoreX;
	if (PlayerState && PlayerState->bPendingTeamSwitch)
	{
		PlayerReady = TeamSwapText;
	}
	ReadyColor = FLinearColor::White;
	ReadyScale = 1.f;
	DrawText(PlayerReady, ReadyX, YOffset + ColumnY, UTHUDOwner->SmallFont, ReadyScale * RenderScale, 1.0f, ReadyColor, ETextHorzPos::Center, ETextVertPos::Center);
}

void UUTScoreboard::DrawPlayerScore(AUTPlayerState* PlayerState, float XOffset, float YOffset, float Width, FLinearColor DrawColor)
{
	DrawText(FText::AsNumber(int32(PlayerState->Score)), XOffset + (Width * ColumnHeaderScoreX), YOffset + ColumnY, UTHUDOwner->SmallFont, RenderScale, 1.0f, DrawColor, ETextHorzPos::Center, ETextVertPos::Center);
	DrawText(FText::AsNumber(PlayerState->Deaths), XOffset + (Width * ColumnHeaderDeathsX), YOffset + ColumnY, UTHUDOwner->TinyFont, RenderScale, 1.0f, DrawColor, ETextHorzPos::Center, ETextVertPos::Center);
}

int32 UUTScoreboard::SelectionHitTest(FVector2D InPosition)
{
	if (bIsInteractive)
	{
		for (int32 i = 0; i < SelectionStack.Num(); i++)
		{
			if (InPosition.X >= SelectionStack[i].ScoreBounds.X && InPosition.X <= SelectionStack[i].ScoreBounds.Z &&
				InPosition.Y >= SelectionStack[i].ScoreBounds.Y && InPosition.Y <= SelectionStack[i].ScoreBounds.W && SelectionStack[i].ScoreOwner.IsValid())
			{
				return i;
			}
		}
	}
	return -1;
}

void UUTScoreboard::TrackMouseMovement(FVector2D NewMousePosition)
{
	if (bIsInteractive)
	{
		CursorPosition = NewMousePosition;
	}
}

bool UUTScoreboard::AttemptSelection(FVector2D SelectionPosition)
{
	if (bIsInteractive)
	{
		int32 SelectionIndex = SelectionHitTest(SelectionPosition);
		if (SelectionIndex >=0 && SelectionIndex < SelectionStack.Num())
		{
			SelectedPlayer = SelectionStack[SelectionIndex].ScoreOwner;
			return true;
		}
	}
	return false;
}

void UUTScoreboard::ClearSelection()
{
	SelectedPlayer.Reset();
}

void UUTScoreboard::BecomeInteractive()
{
	bIsInteractive = true;
}

void UUTScoreboard::BecomeNonInteractive()
{
	bIsInteractive = false;
	ClearSelection();
}

void UUTScoreboard::DefaultSelection(AUTGameState* GS, uint8 TeamIndex)
{
	if (GS != NULL)
	{
		for (int32 i=0; i < GS->PlayerArray.Num(); i++)
		{
			AUTPlayerState* PS = Cast<AUTPlayerState>(GS->PlayerArray[i]);
			if (PS && !PS->bIsSpectator && !PS->bOnlySpectator && (TeamIndex == 255 || PS->GetTeamNum() == TeamIndex))
			{
				SelectedPlayer = PS;
				return;
			}
		}
	}
	SelectedPlayer.Reset();
}

void UUTScoreboard::SelectNext(int32 Offset, bool bDoNoWrap)
{
	AUTGameState* GS = UTHUDOwner->GetWorld()->GetGameState<AUTGameState>();
	if (GS == NULL) return;

	GS->SortPRIArray();
	int32 SelectedIndex = GS->PlayerArray.Find(SelectedPlayer.Get());
	
	if (SelectedIndex >= 0 && SelectedIndex < GS->PlayerArray.Num())
	{
		AUTPlayerState* Next = NULL;
		int32 Step = Offset > 0 ? 1 : -1;
		do 
		{
			SelectedIndex += Step;
			if (SelectedIndex < 0) 
			{
				if (bDoNoWrap) return;
				SelectedIndex = GS->PlayerArray.Num() -1;
			}
			if (SelectedIndex >= GS->PlayerArray.Num()) 
			{
				if (bDoNoWrap) return;
				SelectedIndex = 0;
			}

			Next = Cast<AUTPlayerState>(GS->PlayerArray[SelectedIndex]);
			if (Next && !Next->bOnlySpectator && !Next->bIsSpectator)
			{
				// Valid potential player.
				Offset -= Step;
				if (Offset == 0)
				{
					SelectedPlayer = Next;
					return;
				}
			}

		} while (Next != SelectedPlayer.Get());
	}
	else
	{
		DefaultSelection(GS);
	}

}

void UUTScoreboard::SelectionUp()
{
	SelectNext(-1);
}

void UUTScoreboard::SelectionDown()
{
	SelectNext(1);
}

void UUTScoreboard::SelectionLeft()
{
	SelectNext(-16,true);
}

void UUTScoreboard::SelectionRight()
{
	SelectNext(16,true);
}

void UUTScoreboard::SelectionClick()
{
	if (SelectedPlayer.IsValid())
	{
		UUTLocalPlayer* LP = Cast<UUTLocalPlayer>(UTHUDOwner->UTPlayerOwner->Player);
		if (LP)
		{
            LP->ShowPlayerInfo(SelectedPlayer->UniqueId.ToString(), SelectedPlayer->GetPlayerName());
			ClearSelection();
		}
	}
}

void UUTScoreboard::DrawStatsLine(FText StatsName, int32 StatValue, int32 ScoreValue, float DeltaTime, float XOffset, float& YPos, const FStatsFontInfo& StatsFontInfo, float ScoreWidth)
{
	Canvas->SetLinearDrawColor(FLinearColor::White);
	Canvas->DrawText(StatsFontInfo.TextFont, StatsName, XOffset, YPos, RenderScale, RenderScale, StatsFontInfo.TextRenderInfo);

	if (StatValue >= 0)
	{
		Canvas->SetLinearDrawColor((bHighlightStatsLineTopValue && (StatValue > ScoreValue)) ? FLinearColor::Yellow : FLinearColor::White);
		Canvas->DrawText(StatsFontInfo.TextFont, FString::Printf(TEXT(" %i"), StatValue), XOffset + ValueColumn*ScoreWidth, YPos, RenderScale, RenderScale, StatsFontInfo.TextRenderInfo);
	}
	if (ScoreValue >= 0)
	{
		Canvas->SetLinearDrawColor((bHighlightStatsLineTopValue && (ScoreValue > StatValue)) ? FLinearColor::Yellow : FLinearColor::White);
		Canvas->DrawText(StatsFontInfo.TextFont, FString::Printf(TEXT(" %i"), ScoreValue), XOffset + ScoreColumn*ScoreWidth, YPos, RenderScale, RenderScale, StatsFontInfo.TextRenderInfo);
	}
	YPos += StatsFontInfo.TextHeight;
}

void UUTScoreboard::DrawPlayerStatsLine(FText StatsName, AUTPlayerState* FirstPS, AUTPlayerState* SecondPS, float DeltaTime, float XOffset, float& YPos, const FStatsFontInfo& StatsFontInfo, float ScoreWidth, int32 HighlightIndex)
{
	if ((HighlightIndex == 0) && UTHUDOwner && UTHUDOwner->UTPlayerOwner && UTHUDOwner->UTPlayerOwner->UTPlayerState && !UTHUDOwner->UTPlayerOwner->UTPlayerState->bOnlySpectator)
	{
		if (FirstPS == UTHUDOwner->UTPlayerOwner->UTPlayerState)
		{
			HighlightIndex = 1;
		}
		else if (SecondPS == UTHUDOwner->UTPlayerOwner->UTPlayerState)
		{
			HighlightIndex = 2;
		}
	}
	DrawTextStatsLine(StatsName, GetPlayerNameFor(FirstPS), GetPlayerNameFor(SecondPS), DeltaTime, XOffset, YPos, StatsFontInfo, ScoreWidth, HighlightIndex);
}

void UUTScoreboard::DrawTextStatsLine(FText StatsName, FString StatValue, FString ScoreValue, float DeltaTime, float XOffset, float& YPos, const FStatsFontInfo& StatsFontInfo, float ScoreWidth, int32 HighlightIndex)
{
	Canvas->SetLinearDrawColor(FLinearColor::White);
	Canvas->DrawText(StatsFontInfo.TextFont, StatsName, XOffset, YPos, RenderScale, RenderScale, StatsFontInfo.TextRenderInfo);
	if (!StatValue.IsEmpty())
	{
		float XL, YL;
		Canvas->StrLen(StatsFontInfo.TextFont, StatValue, XL, YL);
		float NameScale = FMath::Clamp(0.96f*ScoreWidth * (ScoreColumn - ValueColumn)/ FMath::Max(XL, 1.f), 0.5f, 1.f);
		Canvas->SetLinearDrawColor((HighlightIndex & 1) ? FLinearColor::Yellow : FLinearColor::White);
		Canvas->DrawText(StatsFontInfo.TextFont, StatValue, XOffset + ValueColumn*ScoreWidth, YPos, NameScale, RenderScale, StatsFontInfo.TextRenderInfo);
	}
	if (!ScoreValue.IsEmpty())
	{
		float XL, YL;
		Canvas->StrLen(StatsFontInfo.TextFont, ScoreValue, XL, YL);
		float NameScale = FMath::Clamp(0.96f*ScoreWidth * (ScoreColumn - ValueColumn) / FMath::Max(XL, 1.f), 0.5f, 1.f);
		Canvas->SetLinearDrawColor((HighlightIndex & 2) ? FLinearColor::Yellow : FLinearColor::White);
		Canvas->DrawText(StatsFontInfo.TextFont, ScoreValue, XOffset + ScoreColumn*ScoreWidth, YPos, NameScale, RenderScale, StatsFontInfo.TextRenderInfo);
	}
	YPos += StatsFontInfo.TextHeight;
}

void UUTScoreboard::DrawCurrentLifeStats(float DeltaTime, float& YPos)
{
	return;

	FVector2D SavedRenderPosition = RenderPosition;
	RenderPosition = FVector2D(0.f, 0.f);
	float TopYPos = YPos;
	float ScoreWidth = 1.2f*ScaledCellWidth;
	float MaxHeight = FooterPosY + SavedRenderPosition.Y - YPos;
	float PageBottom = TopYPos + MaxHeight;

	// draw left side
	float XOffset = ScaledEdgeSize + ScaledCellWidth - ScoreWidth;
	FLinearColor PageColor = FLinearColor::Black;
	PageColor.A = 0.5f;
	DrawTexture(UTHUDOwner->ScoreboardAtlas, XOffset, YPos, ScoreWidth, MaxHeight, 149, 138, 32, 32, 0.5f, PageColor);

	DrawText(NSLOCTEXT("UTScoreboard", "THISLIFE", "This Life"), XOffset, YPos, UTHUDOwner->MediumFont, RenderScale, 1.0f, FLinearColor::White, ETextHorzPos::Center, ETextVertPos::Center);
	float XL, SmallYL;
	Canvas->TextSize(UTHUDOwner->SmallFont, "TEST", XL, SmallYL, RenderScale, RenderScale);
	float TinyYL;
	Canvas->TextSize(UTHUDOwner->TinyFont, "TEST", XL, TinyYL, RenderScale, RenderScale);
	float MedYL;
	Canvas->TextSize(UTHUDOwner->MediumFont, "TEST", XL, MedYL, RenderScale, RenderScale);

	DrawText(NSLOCTEXT("UTScoreboard", "Kills", "Kills"), XOffset, YPos + MedYL, UTHUDOwner->MediumFont, RenderScale, 1.0f, FLinearColor::White, ETextHorzPos::Center, ETextVertPos::Center);
	DrawText( FText::AsNumber(UTHUDOwner->UTPlayerOwner->UTPlayerState->Kills), XOffset + 0.3f*ScoreWidth, YPos + MedYL, UTHUDOwner->MediumFont, RenderScale, 1.0f, FLinearColor::Black, ETextHorzPos::Center, ETextVertPos::Center);
}

void UUTScoreboard::DrawScoringStats(float DeltaTime, float& YPos)
{
	FVector2D SavedRenderPosition = RenderPosition;
	RenderPosition = FVector2D(0.f, 0.f);
	float TopYPos = YPos;
	float ScoreWidth = 1.15f*ScaledCellWidth;
	float MaxHeight = FooterPosY + SavedRenderPosition.Y - YPos;
	float PageBottom = TopYPos + MaxHeight;

	// draw left side
	float XOffset = ScaledEdgeSize + ScaledCellWidth - ScoreWidth;
	DrawStatsLeft(DeltaTime, YPos, XOffset, ScoreWidth, PageBottom);

	// draw right side
	XOffset = Canvas->ClipX - XOffset - ScoreWidth;
	YPos = TopYPos;
	DrawStatsRight(DeltaTime, YPos, XOffset, ScoreWidth, PageBottom);

	RenderPosition = SavedRenderPosition;
}

void UUTScoreboard::DrawStatsLeft(float DeltaTime, float& YPos, float XOffset, float ScoreWidth, float PageBottom)
{
}

void UUTScoreboard::DrawStatsRight(float DeltaTime, float& YPos, float XOffset, float ScoreWidth, float PageBottom)
{
}

float UUTScoreboard::GetDrawScaleOverride()
{
	return 1.f;
}

TWeakObjectPtr<AUTPlayerState> UUTScoreboard::GetSelectedPlayer()
{
	return SelectedPlayer;
}

void UUTScoreboard::GetContextMenuItems_Implementation(TArray<FScoreboardContextMenuItem>& MenuItems)
{
}

bool UUTScoreboard::HandleContextCommand_Implementation(uint8 ContextId, AUTPlayerState* InSelectedPlayer)
{
	return false;
}

void UUTScoreboard::DrawFramedBackground(float XOffset, float YOffset, float Width, float Height)
{
	float FrameWidth = 8.f * RenderScale;
	Canvas->SetLinearDrawColor(FLinearColor::Black);
	DrawTexture(UTHUDOwner->ScoreboardAtlas, XOffset - FrameWidth, YOffset - FrameWidth, Width + 2.f*FrameWidth, Height + 2.f*FrameWidth, 149, 138, 32, 32, 0.75f, FLinearColor::Black);
	Canvas->SetLinearDrawColor(FLinearColor::White);
	float BackAlpha = 0.3f;
	DrawTexture(UTHUDOwner->ScoreboardAtlas, XOffset, YOffset, Width, Height, 149, 138, 32, 32, BackAlpha, FLinearColor::White);
}

bool UUTScoreboard::ShouldDraw_Implementation(bool bShowScores)
{
	if (UTGameState && UTGameState->GetMatchState() == MatchState::MapVoteHappening && GetWorld()->GetNetMode() == NM_Standalone)
	{
		return false;
	}

	return bShowScores;

}
