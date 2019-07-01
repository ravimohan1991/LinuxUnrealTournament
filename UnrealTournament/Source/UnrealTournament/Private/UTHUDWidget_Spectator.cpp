// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "UnrealTournament.h"
#include "UTHUDWidget_Spectator.h"
#include "UTCarriedObject.h"
#include "UTFlagRunGameState.h"
#include "UTDemoRecSpectator.h"
#include "UTLineUpHelper.h"

UUTHUDWidget_Spectator::UUTHUDWidget_Spectator(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DesignedResolution = 1080;
	Position=FVector2D(0,0);
	Size=FVector2D(1920.0f,54.0f);
	ScreenPosition=FVector2D(0.0f, 0.94f);
	Origin=FVector2D(0.0f,0.0f);
}

bool UUTHUDWidget_Spectator::ShouldDraw_Implementation(bool bShowScores)
{
	if (UTHUDOwner && Cast<AUTDemoRecSpectator>(UTHUDOwner->UTPlayerOwner))
	{
		return true;
	}

	if (UTHUDOwner && UTHUDOwner->UTPlayerOwner && UTGameState)
	{

		UUTLocalPlayer* UTLocalPlayer = Cast<UUTLocalPlayer>(UTHUDOwner->UTPlayerOwner->Player);
		if (UTLocalPlayer && UTLocalPlayer->AreMenusOpen() && !UTGameState->HasMatchStarted())
		{
			return false;
		}

		if (UTHUDOwner->UTPlayerOwner->UTPlayerState && (UTGameState->GetMatchState() != MatchState::PlayerIntro))
		{
			AUTPlayerState* PS = UTHUDOwner->UTPlayerOwner->UTPlayerState;
			if (UTGameState->IsMatchIntermission() || UTGameState->HasMatchEnded() || !UTGameState->HasMatchStarted())
			{
				return true;
			}
			return (PS->bOnlySpectator || PS->bOutOfLives || (UTCharacterOwner ? UTCharacterOwner->IsDead() : (UTHUDOwner->UTPlayerOwner->GetPawn() == NULL)));
		}
	}
	return false;
}

void UUTHUDWidget_Spectator::DrawSimpleMessage(FText SimpleMessage, float DeltaTime, FText ViewingMessage)
{
	if (SimpleMessage.IsEmpty() || !UTHUDOwner->MediumFont || !UTHUDOwner->SmallFont)
	{
		return;
	}
	float MessageWidth, YL;
	Canvas->StrLen(UTHUDOwner->MediumFont, SimpleMessage.ToString(), MessageWidth, YL);

	bool bViewingMessage = !ViewingMessage.IsEmpty();
	RenderScale = Canvas->ClipY / 1080.f;
	float Scaling = RenderScale;
	float ScreenWidth = Canvas->ClipX;
	float BackgroundWidth = ScreenWidth;
	float TextPosition = 200.f;
	float MessageOffset = 0.f;
	float YOffset = 0.f;
	float LogoWidth = 150.5f;
	float Height = 54.f;
	if (bViewingMessage)
	{
		Height = YL;
		Scaling = RenderScale* FMath::Max(1.f, 3.f - 6.f*(GetWorld()->GetTimeSeconds() - ViewCharChangeTime));
		float XL;
		Canvas->StrLen(UTHUDOwner->SmallFont, ViewingMessage.ToString(), XL, YL);
		BackgroundWidth = FMath::Max(XL, MessageWidth);
		BackgroundWidth = Scaling * (FMath::Max(BackgroundWidth, 128.f) + 64.f);
		MessageOffset = (ScreenWidth - BackgroundWidth) * (UTGameState->HasMatchEnded() ? 0.5f : 1.f);
		TextPosition = 32.f*RenderScale + MessageOffset;
		YOffset = -96.f*RenderScale;
		Height += YL;
	}
	else
	{
		BackgroundWidth = RenderScale * (MessageWidth + LogoWidth + 128.f);
		MessageOffset = 0.5f * (Canvas->ClipX - BackgroundWidth);
		TextPosition = MessageOffset + 220.f*RenderScale;
	}

	// Draw the Background
	bMaintainAspectRatio = false;
	bScaleByDesignedResolution = false;
	DrawTexture(UTHUDOwner->ScoreboardAtlas, MessageOffset, YOffset, BackgroundWidth, Scaling * Height, 4, 2, 124, 128, 1.0);
	if (bViewingMessage)
	{
		YOffset += 24.f*RenderScale;
		DrawText(ViewingMessage, TextPosition, YOffset, UTHUDOwner->SmallFont, Scaling, 1.f, GetMessageColor(), ETextHorzPos::Left, ETextVertPos::Center);
		YOffset += 10.f*RenderScale;
	}
	else
	{
		bMaintainAspectRatio = true;
		// Draw the Logo and spacer bar
		DrawTexture(UTHUDOwner->ScoreboardAtlas, TextPosition - 190.f*RenderScale, 27.f*RenderScale, LogoWidth*RenderScale, 49.f*RenderScale, 162, 14, 301, 98.0, 1.0f, FLinearColor::White, FVector2D(0.f, 0.5f));
		DrawTexture(UTHUDOwner->ScoreboardAtlas, TextPosition - 19.f*RenderScale, 27.f*RenderScale, 4.f*RenderScale, 49.5f*RenderScale, 488, 13, 4, 99, 1.0f, FLinearColor::White, FVector2D(0.f, 0.5f));
	}
	DrawText(SimpleMessage, TextPosition, YOffset + 20.f*RenderScale, UTHUDOwner->MediumFont, RenderScale, 1.f, GetMessageColor(), ETextHorzPos::Left, ETextVertPos::Center);
}

void UUTHUDWidget_Spectator::Draw_Implementation(float DeltaTime)
{
	Super::Draw_Implementation(DeltaTime);

	FText ShortMessage;
	FText SpectatorMessage = GetSpectatorMessageText(ShortMessage);
	DrawSimpleMessage(SpectatorMessage, DeltaTime, ShortMessage);
}

FText UUTHUDWidget_Spectator::GetSpectatorMessageText(FText& ShortMessage)
{
	FText SpectatorMessage;
	ShortMessage = FText::GetEmpty();
	if (UTGameState)
	{
		bool bDemoRecSpectator = UTHUDOwner->UTPlayerOwner && Cast<AUTDemoRecSpectator>(UTHUDOwner->UTPlayerOwner);
		AUTPlayerState* UTPS = UTHUDOwner->UTPlayerOwner->UTPlayerState;

		if (bDemoRecSpectator)
		{
			AActor* ViewActor = UTHUDOwner->UTPlayerOwner->GetViewTarget();
			AUTCharacter* ViewCharacter = Cast<AUTCharacter>(ViewActor);
			if (!ViewCharacter)
			{
				AUTCarriedObject* Flag = Cast<AUTCarriedObject>(ViewActor);
				if (Flag && Flag->Holder)
				{
					ViewCharacter = Cast<AUTCharacter>(Flag->GetAttachmentReplication().AttachParent);
				}
			}
            if (ViewCharacter && ViewCharacter->GetPlayerState())
			{
				FFormatNamedArguments Args;
                Args.Add("PlayerName", FText::AsCultureInvariant(ViewCharacter->GetPlayerState()->GetPlayerName()));
				ShortMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "NowViewing", "Now viewing");
				SpectatorMessage = FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "SpectatorPlayerWatching", "{PlayerName}"), Args);
			}
		}
		else if (!UTGameState->HasMatchStarted())
		{
			// Look to see if we are waiting to play and if we must be ready.  If we aren't, just exit cause we don
			if (UTGameState->IsMatchInCountdown())
			{
				if (UTPS && UTPS->RespawnChoiceA && UTPS->RespawnChoiceB)
				{
					SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "Choose Start", "Choose your start position");
				}
				else
				{
					SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "MatchStarting", "Match is about to start");
				}
			}
			else if (UTHUDOwner->UTPlayerOwner && UTHUDOwner->UTPlayerOwner->UTPlayerState && UTHUDOwner->UTPlayerOwner->UTPlayerState->bIsWarmingUp)
			{
				if (UTCharacterOwner ? UTCharacterOwner->IsDead() : (UTHUDOwner->UTPlayerOwner->GetPawn() == NULL))
				{
					if (UTPS->RespawnTime > 0.0f)
					{
						FFormatNamedArguments Args;
						static const FNumberFormattingOptions RespawnTimeFormat = FNumberFormattingOptions()
							.SetMinimumFractionalDigits(0)
							.SetMaximumFractionalDigits(0);
						Args.Add("RespawnTime", FText::AsNumber(UTHUDOwner->UTPlayerOwner->UTPlayerState->RespawnTime + 0.5f, &RespawnTimeFormat));
						SpectatorMessage = FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "RespawnWaitMessage", "You can respawn in {RespawnTime}..."), Args);
					}
					else
					{
						SpectatorMessage = (UTGameState->ForceRespawnTime > 0.3f) ? NSLOCTEXT("UUTHUDWidget_Spectator", "RespawnMessage", "Press [FIRE] to respawn...") : FText::GetEmpty();
					}
				}
				else
				{
					UUTLocalPlayer* LP = UTHUDOwner->UTPlayerOwner ? UTHUDOwner->UTPlayerOwner->GetUTLocalPlayer() : nullptr;
					ShortMessage = (LP && LP->AreMenusOpen())
						? NSLOCTEXT("UUTHUDWidget_Spectator", "ClickLeave", "Click on LEAVE WARM UP to leave")
						: NSLOCTEXT("UUTHUDWidget_Spectator", "PressEnter", "Press [ENTER] to leave");
					SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "Warmup", "Warm Up");
					if (UTHUDOwner->UTPlayerOwner->UTPlayerState->bIsMatchHost)
					{ 
						ShortMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "PressEnter", "Host, Press [ENTER] to");
						SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "StartMatch", "START MATCH");
					}
				}
			}
			else if (UTPS && UTPS->bCaster)
			{
				SpectatorMessage = (UTGameState->AreAllPlayersReady())
					? NSLOCTEXT("UUTHUDWidget_Spectator", "WaitingForCaster", "All players are ready. Press [Enter] to start match.")
					: NSLOCTEXT("UUTHUDWidget_Spectator", "WaitingForReady", "Waiting for players to warm up.");
			}
			else if (UTPS && UTPS->bIsMatchHost)
			{
				SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "WaitingForHost", "Host, Press [ENTER] to start match.");
			}
			else if (UTPS && UTPS->bOnlySpectator)
			{
				SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "WaitingForReady", "Waiting for players.");
			}
			else if (UTHUDOwner->GetScoreboard() && UTHUDOwner->GetScoreboard()->IsInteractive())
			{
				SpectatorMessage = (UTHUDOwner->GetNetMode() == NM_Standalone) 
					? NSLOCTEXT("UUTHUDWidget_Spectator", "StartMatchFromMenu", "Click on START MATCH to begin.")
					: NSLOCTEXT("UUTHUDWidget_Spectator", "WaitingForPlayersMenu", "Click on JOIN WARM UP to join warm up.");
			}
			else if (UTHUDOwner->GetNetMode() == NM_Standalone)
			{
				SpectatorMessage = (UTHUDOwner->GetNetMode() == NM_Standalone)
					? NSLOCTEXT("UUTHUDWidget_Spectator", "StartMatchFromFire", "Press [FIRE] to begin.")
					: NSLOCTEXT("UUTHUDWidget_Spectator", "WaitingForPlayers", "Press [ENTER] to warm up.");
			}
		}
		else if (!UTGameState->HasMatchEnded())
		{
			if (UTGameState->IsMatchIntermission())
			{
				int32 IntermissionTime = UTGameState->GetIntermissionTime();
				if (IntermissionTime > 0)
				{
					FFormatNamedArguments Args;
					if ((IntermissionTime > 6) && UTGameState->IsLineUpActive() && UTPS->GetUTCharacter() && AUTLineUpHelper::IsControllerInLineup(UTPlayerOwner))
					{
						if (UTGameState->ActiveLineUpHelper && UTGameState->ActiveLineUpHelper->CanInitiateGroupTaunt(UTPS) && !UTPS->ActiveGroupTaunt)
						{
							Args.Add("GroupTaunt", UTHUDOwner->GroupTauntLabel);
							SpectatorMessage = FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "GroupTaunt", "Press {GroupTaunt} to start a group taunt."), Args);
						}
						else
						{
							Args.Add("Emote1", UTHUDOwner->TauntOneLabel);
							Args.Add("Emote2", UTHUDOwner->TauntTwoLabel);
							SpectatorMessage = FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "GroupTaunt", "Press {Emote1} or {Emote2} to play your emotes."), Args);
						}
					}
					else if (IntermissionTime < 10)
					{
						Args.Add("Time", FText::AsNumber(IntermissionTime));
						AUTFlagRunGameState* BlitzGameState = Cast<AUTFlagRunGameState>(UTGameState);
						SpectatorMessage = BlitzGameState
							? FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "Intermission", "Game resumes in {Time}"), Args)
							: FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "HalfTime", "HALFTIME - Game resumes in {Time}"), Args);
					}
				}
			}
			else if (UTPS && (UTPS->bOnlySpectator || UTPS->bOutOfLives))
			{
				AActor* ViewActor = UTHUDOwner->UTPlayerOwner->GetViewTarget();
				AUTCharacter* ViewCharacter = Cast<AUTCharacter>(ViewActor);
				if (!ViewCharacter)
				{
					AUTCarriedObject* Flag = Cast<AUTCarriedObject>(ViewActor);
					if (Flag && Flag->Holder)
					{
						ViewCharacter = Cast<AUTCharacter>(Flag->GetAttachmentReplication().AttachParent);
					}
				}
                if (ViewCharacter && ViewCharacter->GetPlayerState())
				{
                    if (LastViewedPS != ViewCharacter->GetPlayerState())
					{
						ViewCharChangeTime = ViewCharacter->GetWorld()->GetTimeSeconds();
                        LastViewedPS = Cast<AUTPlayerState>(ViewCharacter->GetPlayerState());
					}
					FFormatNamedArguments Args;
                    Args.Add("PlayerName", FText::AsCultureInvariant(ViewCharacter->GetPlayerState()->GetPlayerName()));
					ShortMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "NowViewing", "Now viewing");
					SpectatorMessage = FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "SpectatorPlayerWatching", "{PlayerName}"), Args);
				}
				else
				{
					LastViewedPS = NULL;
				}
			}
			else if (UTPS && (UTCharacterOwner ? UTCharacterOwner->IsDead() : (UTHUDOwner->UTPlayerOwner->GetPawn() == NULL)))
			{
				if (UTPS->RespawnTime > 0.0f)
				{
					FFormatNamedArguments Args;
					static const FNumberFormattingOptions RespawnTimeFormat = FNumberFormattingOptions()
						.SetMinimumFractionalDigits(0)
						.SetMaximumFractionalDigits(0);
					Args.Add("RespawnTime", FText::AsNumber(UTHUDOwner->UTPlayerOwner->UTPlayerState->RespawnTime + 0.5f, &RespawnTimeFormat));
					SpectatorMessage = FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "RespawnWaitMessage", "You can respawn in {RespawnTime}..."), Args);
				}
				else if (!UTPS->bOutOfLives)
				{
					if (UTPS->RespawnChoiceA != nullptr)
					{
						SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "ChooseRespawnMessage", "Choose a respawn point with [FIRE] or [ALT-FIRE]");
					}
					else
					{
						SpectatorMessage = ((UTGameState->ForceRespawnTime > 0.3f) || (UTHUDOwner->UTPlayerOwner && UTHUDOwner->UTPlayerOwner->bPlayerIsWaiting)) ? NSLOCTEXT("UUTHUDWidget_Spectator", "RespawnMessage", "Press [FIRE] to respawn...") : FText::GetEmpty();
					}
				}
			}
		}
		else
		{
			AUTCharacter* ViewCharacter = Cast<AUTCharacter>(UTHUDOwner->UTPlayerOwner->GetViewTarget());
            AUTPlayerState* PS = ViewCharacter ? Cast<AUTPlayerState>(ViewCharacter->GetPlayerState()) : NULL;
			if (UTHUDOwner && UTHUDOwner->bDisplayMatchSummary)
			{
				if (UTGameState->GetNetMode() != NM_Standalone)
				{
					const AUTGameMode* DefaultGame = Cast<AUTGameMode>(UTGameState->GetDefaultGameMode());
					if (DefaultGame && (DefaultGame->MatchSummaryTime - (GetWorld()->GetTimeSeconds() - UTHUDOwner->MatchSummaryTime) < 10))
					{
						int32 RemainingDelay = DefaultGame->MatchSummaryTime - (GetWorld()->GetTimeSeconds() - UTHUDOwner->MatchSummaryTime);
						if (RemainingDelay <= 0)
						{
							SpectatorMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "MapVoteInitMessage", "Initializing Map Vote");
						}
						else
						{
							FFormatNamedArguments Args;
							static const FNumberFormattingOptions RespawnTimeFormat = FNumberFormattingOptions()
								.SetMinimumFractionalDigits(0)
								.SetMaximumFractionalDigits(0);
							Args.Add("TimeToMapVote", FText::AsNumber(RemainingDelay, &RespawnTimeFormat));
							SpectatorMessage = FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "MapVoteWaitMessage", "Map Vote in {TimeToMapVote}..."), Args);
						}
					}
				}
			}
			else if (PS)
			{
				FFormatNamedArguments Args;
                Args.Add("PlayerName", FText::AsCultureInvariant(PS->GetPlayerName()));
				ShortMessage = NSLOCTEXT("UUTHUDWidget_Spectator", "NowViewing", "Now viewing");
				if (UTGameState->bTeamGame && PS && PS->Team && (!UTGameState->GameModeClass || !UTGameState->GameModeClass->GetDefaultObject<AUTTeamGameMode>() || UTGameState->GameModeClass->GetDefaultObject<AUTTeamGameMode>()->bAnnounceTeam))
				{
					SpectatorMessage = (PS->Team->TeamIndex == 0)
						? FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "SpectatorPlayerWatchingRed", "Red Team Led by {PlayerName}"), Args)
						: FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "SpectatorPlayerWatchingBlue", "Blue Team Led by {PlayerName}"), Args);
				}
				else
				{
					SpectatorMessage = FText::Format(NSLOCTEXT("UUTHUDWidget_Spectator", "SpectatorPlayerWatching", "{PlayerName}"), Args);
				}
			}
		}
	}
	return SpectatorMessage;
}

float UUTHUDWidget_Spectator::GetDrawScaleOverride()
{
	return 1.0;
}
