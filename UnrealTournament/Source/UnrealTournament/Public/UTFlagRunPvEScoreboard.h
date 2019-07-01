// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UTFlagRunScoreboard.h"
#include "UTFlagRunPvEGameState.h"
#include "UTLineUpHelper.h"
#include "StatNames.h"
#include "UTGameEngine.h"
#include "UTHUD.h"
#include "UTGameState.h"

#include "UTFlagRunPvEScoreboard.generated.h"

UCLASS()
class UNREALTOURNAMENT_API UUTFlagRunPvEScoreboard : public UUTFlagRunScoreboard
{
	GENERATED_BODY()
public:
	UUTFlagRunPvEScoreboard(const FObjectInitializer& OI)
		: Super(OI)
	{
		GameMessageText = NSLOCTEXT("UTPvEScoreboard", "ScoreboardHeader", "{Difficulty} {GameName} in {MapName}");
		DefendTitle = NSLOCTEXT("UTScoreboard", "Defending", "SIEGE - {Difficulty}");
		
		DefendLines.Empty();
		DefendLines.Add(NSLOCTEXT("UTPvEScoreboard", "DefenseLine1", "* You are defending.  Your goal is to keep the enemy from bringing"));
		DefendLines.Add(NSLOCTEXT("UTPvEScoreboard", "DefenseLine1b", "  the flag to your base for as long as possible."));
		DefendLines.Add(FText::GetEmpty());
		DefendLines.Add(NSLOCTEXT("UTPvEScoreboard", "DefenseLine2", "* You have 3 lives to start. Extra lives are gained by team kill count."));
		DefendLines.Add(FText::GetEmpty());
		DefendLines.Add(NSLOCTEXT("UTPvEScoreboard", "DefenseLine3", "* Enemy minions respawn endlessly. Elite monsters appear "));
		DefendLines.Add(NSLOCTEXT("UTPvEScoreboard", "DefenseLine3b", "   periodically, but have limited respawns. "));
		DefendLines.Add(FText::GetEmpty());
		DefendLines.Add(NSLOCTEXT("UTPvEScoreboard", "DefenseLine5", "* The enemy flag carrier can power up Rally Points to cause monsters"));
		DefendLines.Add(NSLOCTEXT("UTPvEScoreboard", "DefenseLine5b", "  to spawn there for a limited time. "));

		BlueTeamName = NSLOCTEXT("CTFPvERewardMessage", "BlueTeamName", "PLAYERS");
		RedTeamName = NSLOCTEXT("CTFPvERewardMessage", "RedTeamName", "MONSTERS");
		TeamScorePrefix = NSLOCTEXT("CTFPvERewardMessage", "TeamScorePrefix", "");
		TeamScorePostfix = NSLOCTEXT("CTFPvERewardMessage", "TeamScorePostfix", " Overrun the Base!");
		DefenseScorePostfix = NSLOCTEXT("CTFPvERewardMessage", "DefenseScoreBonusPostfix", " Successfully Defend!");
	}

	virtual FText GetRoundTitle(bool bIsOnDefense) const override
	{
		AUTFlagRunPvEGameState* GS = GetWorld()->GetGameState<AUTFlagRunPvEGameState>();
		FFormatNamedArguments Args;
		Args.Add("Difficulty", GetBotSkillName(GS->GameDifficulty));
		return FText::Format(DefendTitle, Args);
	}

	virtual FText GetScoringSummaryTitle(bool bIsOnDefense) const override
	{
		return GetRoundTitle(bIsOnDefense);
	}

	virtual void GetTitleMessageArgs(FFormatNamedArguments& Args) const override
	{
		Super::GetTitleMessageArgs(Args);
		AUTFlagRunPvEGameState* GS = GetWorld()->GetGameState<AUTFlagRunPvEGameState>();
		if (GS != nullptr)
		{
			Args.Add("Difficulty", GetBotSkillName(GS->GameDifficulty));
		}
	}

	virtual void GetScoringStars(int32& NumStars, FLinearColor& StarColor) const override
	{
		AUTFlagRunGameState* GS = GetWorld()->GetGameState<AUTFlagRunGameState>();
		if (GS != nullptr)
		{
			NumStars = GS->BonusLevel;
			switch (GS->BonusLevel)
			{
				case 0:
				case 1:
					StarColor = BRONZECOLOR;
					break;
				case 2:
					StarColor = SILVERCOLOR;
					break;
				default:
					StarColor = GOLDCOLOR;
					break;
			}
		}
	}

	virtual bool ShowScorePanel() override
	{
		AUTFlagRunPvEGameState* GS = GetWorld()->GetGameState<AUTFlagRunPvEGameState>();
		if (GS != nullptr && GS->HasMatchEnded() && GS->IsLineUpActive())
		{
			return false;
		}
		else
		{
			return Super::ShowScorePanel();
		}
	}

	virtual bool ShouldDrawScoringStats() override
	{
		// match rules for drawing the player score panel
		return UTGameState && ((UTGameState->GetMatchState() == MatchState::MatchIntermission) || UTGameState->HasMatchEnded()) && ShowScorePanel();
	}
	virtual void DrawStatsLeft(float DeltaTime, float& YPos, float XOffset, float ScoreWidth, float PageBottom) override
	{
		UUTTeamScoreboard::DrawStatsLeft(DeltaTime, YPos, XOffset, ScoreWidth, PageBottom);
	}
	virtual void DrawStatsRight(float DeltaTime, float& YPos, float XOffset, float ScoreWidth, float PageBottom) override
	{
		UUTTeamScoreboard::DrawStatsRight(DeltaTime, YPos, XOffset, ScoreWidth, PageBottom);
	}

	virtual void DrawTeamScoreBreakdown(float DeltaTime, float& YPos, float XOffset, float ScoreWidth, float PageBottom) override
	{
		Canvas->SetLinearDrawColor(FLinearColor::White);
		FStatsFontInfo StatsFontInfo;
		StatsFontInfo.TextRenderInfo.bEnableShadow = true;
		StatsFontInfo.TextRenderInfo.bClipText = true;
		StatsFontInfo.TextFont = UTHUDOwner->TinyFont;
		bHighlightStatsLineTopValue = false;

		float XL, SmallYL;
		Canvas->TextSize(UTHUDOwner->TinyFont, "TEST", XL, SmallYL, RenderScale, RenderScale);
		StatsFontInfo.TextHeight = SmallYL;
		float MedYL;
		Canvas->TextSize(UTHUDOwner->SmallFont, TeamScoringHeader.ToString(), XL, MedYL, RenderScale, RenderScale);
		Canvas->DrawText(UTHUDOwner->SmallFont, TeamScoringHeader, XOffset + 0.5f*(ScoreWidth - XL), YPos, RenderScale, RenderScale, StatsFontInfo.TextRenderInfo);
		YPos += 1.1f * MedYL;

		DrawTeamStats(DeltaTime, YPos, XOffset, ScoreWidth, PageBottom, StatsFontInfo);
	}

	virtual void DrawClockTeamStatsLine(FText StatsName, FName StatsID, float DeltaTime, float XOffset, float& YPos, const FStatsFontInfo& StatsFontInfo, float ScoreWidth, bool bSkipEmpty) override
	{
		int32 HighlightIndex = 0;
		int32 BlueTeamValue = UTGameState->Teams[1]->GetStatsValue(StatsID);
		if (bSkipEmpty && BlueTeamValue == 0)
		{
			return;
		}

		FText ClockStringBlue = UTHUDOwner->ConvertTime(FText::GetEmpty(), FText::GetEmpty(), BlueTeamValue, false);
		DrawTextStatsLine(StatsName, TEXT(""), ClockStringBlue.ToString(), DeltaTime, XOffset, YPos, StatsFontInfo, ScoreWidth, 0);
	}

	virtual void DrawTeamStats(float DeltaTime, float& YPos, float XOffset, float ScoreWidth, float MaxHeight, const FStatsFontInfo& StatsFontInfo) override
	{
		if (UTGameState == nullptr || UTGameState->Teams.Num() < 2 || UTGameState->Teams[1] == nullptr)
		{
			return;
		}

		DrawStatsLine(NSLOCTEXT("UTScoreboard", "TeamKills", "Kills"), -1, UTGameState->Teams[1]->GetStatsValue(NAME_TeamKills), DeltaTime, XOffset, YPos, StatsFontInfo, ScoreWidth);

		float SectionSpacing = 0.6f * StatsFontInfo.TextHeight;
		YPos += SectionSpacing;

		// find top scorer
		//AUTPlayerState* TopScorerBlue = FindTopTeamScoreFor(1);

		// find top kills && KD
		AUTPlayerState* TopKillerBlue = FindTopTeamKillerFor(1);

		//DrawPlayerStatsLine(NSLOCTEXT("UTScoreboard", "TopScorer", "Top Scorer"), nullptr, TopScorerBlue, DeltaTime, XOffset, YPos, StatsFontInfo, ScoreWidth, 0);
		DrawPlayerStatsLine(NSLOCTEXT("UTScoreboard", "TopDefender", "Top Defender"), nullptr, UTGameState->Teams[1]->TopDefender, DeltaTime, XOffset, YPos, StatsFontInfo, ScoreWidth, 0);
		DrawPlayerStatsLine(NSLOCTEXT("UTScoreboard", "TopSupport", "Top Support"), nullptr, UTGameState->Teams[1]->TopSupporter, DeltaTime, XOffset, YPos, StatsFontInfo, ScoreWidth, 0);
		DrawPlayerStatsLine(NSLOCTEXT("UTScoreboard", "TopKills", "Top Kills"), nullptr, TopKillerBlue, DeltaTime, XOffset, YPos, StatsFontInfo, ScoreWidth, 0);
		YPos += SectionSpacing;

		DrawStatsLine(NSLOCTEXT("UTScoreboard", "BeltPickups", "Shield Belt Pickups"), -1, UTGameState->Teams[1]->GetStatsValue(NAME_ShieldBeltCount), DeltaTime, XOffset, YPos, StatsFontInfo, ScoreWidth);
		DrawStatsLine(NSLOCTEXT("UTScoreboard", "LargeArmorPickups", "Large Armor Pickups"), -1, UTGameState->Teams[1]->GetStatsValue(NAME_ArmorVestCount), DeltaTime, XOffset, YPos, StatsFontInfo, ScoreWidth);
		DrawStatsLine(NSLOCTEXT("UTScoreboard", "MediumArmorPickups", "Medium Armor Pickups"), -1, UTGameState->Teams[1]->GetStatsValue(NAME_ArmorPadsCount), DeltaTime, XOffset, YPos, StatsFontInfo, ScoreWidth);

		int32 TeamStat1 = UTGameState->Teams[1]->GetStatsValue(NAME_UDamageCount);
		if (TeamStat1 > 0)
		{
			DrawStatsLine(NSLOCTEXT("UTScoreboard", "UDamagePickups", "UDamage Pickups"), -1, TeamStat1, DeltaTime, XOffset, YPos, StatsFontInfo, ScoreWidth);
			DrawClockTeamStatsLine(NSLOCTEXT("UTScoreboard", "UDamage", "UDamage Control"), NAME_UDamageTime, DeltaTime, XOffset, YPos, StatsFontInfo, ScoreWidth, true);
		}
		TeamStat1 = UTGameState->Teams[1]->GetStatsValue(NAME_BerserkCount);
		if (TeamStat1 > 0)
		{
			DrawStatsLine(NSLOCTEXT("UTScoreboard", "BerserkPickups", "Berserk Pickups"), -1, TeamStat1, DeltaTime, XOffset, YPos, StatsFontInfo, ScoreWidth);
			DrawClockTeamStatsLine(NSLOCTEXT("UTScoreboard", "Berserk", "Berserk Control"), NAME_BerserkTime, DeltaTime, XOffset, YPos, StatsFontInfo, ScoreWidth, true);
		}
		TeamStat1 = UTGameState->Teams[1]->GetStatsValue(NAME_InvisibilityCount);
		if (TeamStat1 > 0)
		{
			DrawStatsLine(NSLOCTEXT("UTScoreboard", "InvisibilityPickups", "Invisibility Pickups"), -1, TeamStat1, DeltaTime, XOffset, YPos, StatsFontInfo, ScoreWidth);
			DrawClockTeamStatsLine(NSLOCTEXT("UTScoreboard", "Invisibility", "Invisibility Control"), NAME_InvisibilityTime, DeltaTime, XOffset, YPos, StatsFontInfo, ScoreWidth, true);
		}
		TeamStat1 = UTGameState->Teams[1]->GetStatsValue(NAME_KegCount);
		if (TeamStat1 > 0)
		{
			DrawStatsLine(NSLOCTEXT("UTScoreboard", "KegPickups", "Keg Pickups"), -1, TeamStat1, DeltaTime, XOffset, YPos, StatsFontInfo, ScoreWidth);
		}
		TeamStat1 = UTGameState->Teams[1]->GetStatsValue(NAME_HelmetCount);
		if (TeamStat1 > 0)
		{
			DrawStatsLine(NSLOCTEXT("UTScoreboard", "SmallArmorPickups", "Small Armor Pickups"), -1, TeamStat1, DeltaTime, XOffset, YPos, StatsFontInfo, ScoreWidth);
		}
	}

	virtual void DrawTeamPanel(float RenderDelta, float& YOffset) override
	{
		YOffset += 119.f * RenderScale;
	}

	virtual void DrawScoreHeaders(float RenderDelta, float& YOffset) override
	{
		float Height = 23.f * RenderScale;
		int32 ColumnCnt = ((UTGameState && UTGameState->bTeamGame) || ActualPlayerCount > 16) ? 2 : 1;
		float ColumnHeaderAdjustY = ColumnHeaderY * RenderScale;
		float XOffset = Canvas->ClipX - ScaledCellWidth - ScaledEdgeSize;
		
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

		YOffset += Height + 4.f * RenderScale;
	}

	virtual void DrawPlayer(int32 Index, AUTPlayerState* PlayerState, float RenderDelta, float XOffset, float YOffset) override
	{
		if (PlayerState->GetTeamNum() == 1)
		{
			Super::DrawPlayer(Index, PlayerState, RenderDelta, XOffset, YOffset);
		}
	}

	virtual void DrawScoringPlays(float DeltaTime, float& YPos, float XOffset, float ScoreWidth, float MaxHeight) override
	{}

	virtual bool ShowScoringInfo() override
	{
		AUTFlagRunGameState* GS = GetWorld()->GetGameState<AUTFlagRunGameState>();
		return GS != nullptr && (GS->IsMatchIntermission() || GS->HasMatchEnded()) && !IsBeforeFirstRound() && (!UTHUDOwner || !UTHUDOwner->bDisplayMatchSummary);
	}

	virtual void DrawScoreAnnouncement(float DeltaTime) override
	{
		AUTFlagRunPvEGameState* GS = GetWorld()->GetGameState<AUTFlagRunPvEGameState>();
		if (GS == nullptr || ScoringTeam == nullptr)
		{
			return;
		}

		float PoundStart = 1.2f;
		float PoundInterval = 0.5f;
		float WooshStart = 3.1f;
		float WooshTime = 0.25f;
		float WooshInterval = 0.4f;
		float CurrentTime = GetWorld()->GetTimeSeconds() - ScoreReceivedTime;

		UFont* InFont = UTHUDOwner->LargeFont;
		FText EmphasisText = (ScoringTeam && (ScoringTeam->TeamIndex == 0)) ? RedTeamName : BlueTeamName;
		float YL, EmphasisXL;
		Canvas->StrLen(InFont, EmphasisText.ToString(), EmphasisXL, YL);
		float YPos = (LastScorePanelYOffset > 0.f) ? LastScorePanelYOffset - 2.f : 0.25f*Canvas->ClipY;
		float TopY = YPos;

		FText ScorePrefix = (Reason == 0) ? TeamScorePrefix : DefenseScorePrefix;
		FText ScorePostfix = (Reason == 0) ? TeamScorePostfix : DefenseScorePostfix;
		float PostXL;
		Canvas->StrLen(InFont, ScorePostfix.ToString(), PostXL, YL);

		float ScoreWidth = RenderScale * 1.2f * (PostXL + EmphasisXL);
		float ScoreHeight = 3.5f * YL;
		float XOffset = 0.5f*(Canvas->ClipX - ScoreWidth);
		DrawFramedBackground(XOffset, YPos, ScoreWidth, RenderScale * ScoreHeight);

		FFontRenderInfo TextRenderInfo;
		TextRenderInfo.bEnableShadow = true;
		TextRenderInfo.bClipText = true;

		// Draw scoring team string
		FLinearColor EmphasisColor = (ScoringTeam && (ScoringTeam->TeamIndex == 0)) ? REDHUDCOLOR : BLUEHUDCOLOR;
		float ScoreX = 0.5f * (Canvas->ClipX - RenderScale * (EmphasisXL + PostXL));

		Canvas->SetLinearDrawColor(ScoringTeam->TeamColor);
		Canvas->DrawText(InFont, EmphasisText, ScoreX, YPos, RenderScale, RenderScale, TextRenderInfo);
		ScoreX += EmphasisXL*RenderScale;

		Canvas->SetLinearDrawColor(FLinearColor::White);
		Canvas->DrawText(InFont, ScorePostfix, ScoreX, YPos, RenderScale, RenderScale, TextRenderInfo);

		YPos += YL*RenderScale;

		if (ScoringTeam->TeamIndex == 0)
		{
			float TimeXL;
			FString TimeString = UUTGameEngine::ConvertTime(NSLOCTEXT("UTPvEScoreboard", "SurvivalTime", "Survival Time: "), FText::GetEmpty(), GetWorld()->GetGameState<AUTGameState>()->ElapsedTime, false, true, false).ToString();
			Canvas->StrLen(InFont, TimeString, TimeXL, YL);
			ScoreX = 0.5f * (Canvas->ClipX - RenderScale * TimeXL);
			Canvas->SetLinearDrawColor(FLinearColor::White);
			Canvas->DrawText(InFont, TimeString, ScoreX, YPos, RenderScale, RenderScale, TextRenderInfo);
			YPos += YL * RenderScale;
		}

		{
			float KillsXL;
			FString KillString = FText::Format(NSLOCTEXT("UTPvEScoreboard", "Kills", "Total Team Kills: {0}"), FText::AsNumber(GS->Teams[1]->GetStatsValue(NAME_TeamKills))).ToString();
			Canvas->StrLen(InFont, KillString, KillsXL, YL);
			ScoreX = 0.5f * (Canvas->ClipX - RenderScale * KillsXL);
			Canvas->SetLinearDrawColor(FLinearColor::White);
			Canvas->DrawText(InFont, KillString, ScoreX, YPos, RenderScale, RenderScale, TextRenderInfo);
			YPos += YL * RenderScale;
		}

		// Draw scoring player string
		if (ScoringPlayer != nullptr)
		{
			float DeliveredXL;
			Canvas->StrLen(UTHUDOwner->SmallFont, DeliveredPrefix.ToString(), DeliveredXL, YL);
			YPos = TopY + RenderScale * (ScoreHeight - 1.1f*YL);
			EmphasisText = FText::FromString(ScoringPlayer->PlayerName);
			Canvas->StrLen(UTHUDOwner->SmallFont, EmphasisText.ToString(), EmphasisXL, YL);

			ScoreX = 0.5f * (Canvas->ClipX - RenderScale * (EmphasisXL + DeliveredXL));
			Canvas->SetLinearDrawColor(FLinearColor::White);
			Canvas->DrawText(UTHUDOwner->SmallFont, DeliveredPrefix, ScoreX, YPos, RenderScale, RenderScale, TextRenderInfo);
			ScoreX += DeliveredXL*RenderScale;
			Canvas->SetLinearDrawColor(ScoringTeam->TeamColor);
			Canvas->DrawText(UTHUDOwner->SmallFont, EmphasisText, ScoreX, YPos, RenderScale, RenderScale, TextRenderInfo);
		}
	}
};