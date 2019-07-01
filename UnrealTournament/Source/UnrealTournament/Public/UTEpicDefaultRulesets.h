// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.
#pragma once

#include "UTATypes.h"
#include "UTReplicatedGameRuleset.h"
#include "UTEpicDefaultRulesets.generated.h"


UCLASS()
class UNREALTOURNAMENT_API UUTEpicDefaultRulesets : public UObject
{
	GENERATED_UCLASS_BODY()

public:
	static void GetEpicRulesets(TArray<FString>& Rules)
	{

		// Menu Visible Rulesets

		Rules.Add(EEpicDefaultRuleTags::FlagRun);
		Rules.Add(EEpicDefaultRuleTags::FlagRunVSAI);
		Rules.Add(EEpicDefaultRuleTags::Deathmatch);
		Rules.Add(EEpicDefaultRuleTags::Siege);
		Rules.Add(EEpicDefaultRuleTags::CTF);
		Rules.Add(EEpicDefaultRuleTags::TDM);
		Rules.Add(EEpicDefaultRuleTags::BIGCTF);
		Rules.Add(EEpicDefaultRuleTags::COMPCTF);
		Rules.Add(EEpicDefaultRuleTags::TEAMSHOWDOWN);
		Rules.Add(EEpicDefaultRuleTags::DUEL);
		Rules.Add(EEpicDefaultRuleTags::iCTF);

		// TODO For testing.. these should be always pushed via the MCP and not hard coded.  

		// Quickplay Rulesets

		Rules.Add(EEpicDefaultRuleTags::QuickPlay_Deathmatch);
		Rules.Add(EEpicDefaultRuleTags::QuickPlay_FlagRun);
		Rules.Add(EEpicDefaultRuleTags::QuickPlay_FlagRunVSAINormal);
		Rules.Add(EEpicDefaultRuleTags::QuickPlay_FlagRunVSAIHard);

		// Ranked Rules

		Rules.Add(EEpicDefaultRuleTags::Ranked_DUEL);
		Rules.Add(EEpicDefaultRuleTags::Ranked_TEAMSHOWDOWN);
		Rules.Add(EEpicDefaultRuleTags::Ranked_CTF);
		Rules.Add(EEpicDefaultRuleTags::Ranked_FlagRun);
	}

	static void InsureEpicDefaults(FUTGameRuleset* NewRuleset)
	{
		// TODO: This should pull from a file that is pushed from the MCP if the MCP is available

		if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::Deathmatch, ESearchCase::IgnoreCase))
		{
			NewRuleset->Categories.Empty(); 
			NewRuleset->Categories.Add(TEXT("Featured"));

			NewRuleset->Title = TEXT("Deathmatch");
			NewRuleset->Tooltip = TEXT("Standard free-for-all Deathmatch.");
			NewRuleset->Description = TEXT("Standard free-for-all deathmatch.\n\n<UT.Hub.RulesText_Small>TimeLimit : %TimeLimit% minutes</>\n<UT.Hub.RulesText_Small>Maximum players : %MaxPlayers%</>");
			NewRuleset->MaxPlayers = 6;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_DM.GB_DM'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTDMGameMode");
			NewRuleset->GameOptions = FString(TEXT("?TimeLimit=10?GoalScore=0"));
			NewRuleset->bTeamGame = false;
						
			NewRuleset->OptionFlags = GAME_OPTION_FLAGS_All;

			NewRuleset->MaxTeamCount = -1;
			NewRuleset->MaxTeamSize = -1;
			NewRuleset->MaxPartySize = 5;

			NewRuleset->EpicMaps = "/Game/RestrictedAssets/Maps/DM-Outpost23";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/DM-Chill";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/DM-Underland";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Lea/DM-Lea";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Unsaved/DM-Unsaved";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Backspace/DM-Backspace";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Salt/DM-Salt";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Batrankus/DM-Batrankus";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/DM-BioTower";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Spacer";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Cannon";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Deadfall";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Focus";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-NickTest1";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Solo";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Decktest";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-ASDF";

			NewRuleset->DefaultMap = "/Game/RestrictedAssets/Maps/DM-Chill";
		}
		else if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::TDM, ESearchCase::IgnoreCase))
		{
			NewRuleset->Categories.Empty(); 
			NewRuleset->Categories.Add(TEXT("Classic"));

			NewRuleset->Title = TEXT("Team Deathmatch");
			NewRuleset->Tooltip = TEXT("Red versus blue team deathmatch.");
			NewRuleset->Description = TEXT("Red versus blue team deathmatch.\n\n<UT.Hub.RulesText_Small>TimeLimit : %timelimit% minutes</>\n<UT.Hub.RulesText_Small>Maximum players: %maxplayers%</>");
			NewRuleset->MaxPlayers = 10;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_TDM.GB_TDM'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTTeamDMGameMode");
			NewRuleset->GameOptions = FString(TEXT("?TimeLimit=20?GoalScore=0"));
			NewRuleset->bTeamGame = true;

			NewRuleset->OptionFlags = GAME_OPTION_FLAGS_All;


			NewRuleset->MaxTeamCount = 2;
			NewRuleset->MaxTeamSize = 5;
			NewRuleset->MaxPartySize = 5;

			NewRuleset->EpicMaps = "/Game/RestrictedAssets/Maps/DM-Outpost23";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/DM-Underland";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/DM-Chill";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Unsaved/DM-Unsaved";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Backspace/DM-Backspace";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Salt/DM-Salt";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Batrankus/DM-Batrankus";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Lea/DM-Lea";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Spacer";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Cannon";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Deadfall";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Temple";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Focus";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-NickTest1";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Solo";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Decktest";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-ASDF";


			NewRuleset->DefaultMap = "/Game/RestrictedAssets/Maps/DM-Chill";
		}
		else if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::TEAMSHOWDOWN, ESearchCase::IgnoreCase))
		{
			NewRuleset->Categories.Empty();
			NewRuleset->Categories.Add(TEXT("Competitive"));

			NewRuleset->Title = TEXT("Showdown");
			NewRuleset->Tooltip = TEXT("Red versus blue team showdown.");
			NewRuleset->Description = TEXT("Red versus blue team showdown.\n\n<UT.Hub.RulesText_Small>TimeLimit : %timelimit% minute rounds</>\n<UT.Hub.RulesText_Small>Scoring : First to %goalscore% wins</>\n<UT.Hub.RulesText_Small>Maximum players: %maxplayers%</>");
			NewRuleset->MaxPlayers = 6;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_TDM.GB_TDM'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTTeamShowdownGame");
			NewRuleset->GameOptions = FString(TEXT("?TimeLimit=2?GoalScore=5"));
			NewRuleset->bTeamGame = true;

			NewRuleset->OptionFlags = GAME_OPTION_FLAGS_All;

			NewRuleset->MaxTeamCount = 2;
			NewRuleset->MaxTeamSize = 3;
			NewRuleset->MaxPartySize = 3;

			NewRuleset->EpicMaps = "/Game/RestrictedAssets/Maps/DM-Chill";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/DM-Underland";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Unsaved/DM-Unsaved";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Backspace/DM-Backspace";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Salt/DM-Salt";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Batrankus/DM-Batrankus";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Lea/DM-Lea";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Spacer";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Cannon";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Temple";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Focus";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Decktest";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-ASDF";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Solo";

			NewRuleset->DefaultMap = "/Game/RestrictedAssets/Maps/DM-Chill";
		}
		else if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::DUEL, ESearchCase::IgnoreCase))
		{
			NewRuleset->Categories.Empty(); 
			NewRuleset->Categories.Add(TEXT("Competitive"));

			NewRuleset->Title = TEXT("Duel");
			NewRuleset->Tooltip = TEXT("One vs one test of deathmatch skill.");
			NewRuleset->Description = TEXT("One vs one test of deathmatch skill.\n\n<UT.Hub.RulesText_Small>TimeLimit : %timelimit% minutes</>\n<UT.Hub.RulesText_Small>Maximum players: %maxplayers%</>");
			NewRuleset->MaxPlayers = 2;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_Duel.GB_Duel'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTDuelGame");
			NewRuleset->GameOptions = FString(TEXT("?TimeLimit=10?GoalScore=0"));
			NewRuleset->bTeamGame = true;
			NewRuleset->bCompetitiveMatch = true;

			NewRuleset->OptionFlags = GAME_OPTION_FLAGS_All;

			NewRuleset->MaxTeamCount = 2;
			NewRuleset->MaxTeamSize = 1;
			NewRuleset->MaxPartySize = 1;

			NewRuleset->EpicMaps = "/Game/RestrictedAssets/Maps/WIP/DM-ASDF";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/DM-Underland";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Lea/DM-Lea";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Unsaved/DM-Unsaved";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Backspace/DM-Backspace";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Salt/DM-Salt";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Batrankus/DM-Batrankus";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Solo";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Decktest";
			NewRuleset->DefaultMap = "/Game/RestrictedAssets/Maps/WIP/DM-ASDF";
		}
		else if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::SHOWDOWN, ESearchCase::IgnoreCase))
		{
			NewRuleset->Categories.Empty(); 
			NewRuleset->Categories.Add(TEXT("Competitive"));
			NewRuleset->bCompetitiveMatch = true;
			NewRuleset->Title = TEXT("1v1 Showdown");
			NewRuleset->Tooltip = TEXT("New School one vs one test of deathmatch skill.");
			NewRuleset->Description = TEXT("New School one vs one test of deathmatch skill.\n\n<UT.Hub.RulesText_Small>TimeLimit : %timelimit% minute rounds</>\n<UT.Hub.RulesText_Small>Maximum players : %maxplayers%</>");
			NewRuleset->MaxPlayers = 2;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_Duel.GB_Duel'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTShowdownGame");
			NewRuleset->GameOptions = FString(TEXT("?Timelimit=2?GoalScore=5"));
			NewRuleset->bTeamGame = true;

			NewRuleset->OptionFlags = GAME_OPTION_FLAGS_All;

			NewRuleset->MaxTeamCount = 2;
			NewRuleset->MaxTeamSize = 1;
			NewRuleset->MaxPartySize = 1;

			NewRuleset->EpicMaps = "/Game/RestrictedAssets/Maps/WIP/DM-ASDF";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/DM-Chill";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/DM-Underland";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Lea/DM-Lea";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Unsaved/DM-Unsaved";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Backspace/DM-Backspace";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Salt/DM-Salt";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Batrankus/DM-Batrankus";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Solo";

			NewRuleset->DefaultMap = "/Game/RestrictedAssets/Maps/WIP/DM-ASDF";
		}
		else if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::CTF, ESearchCase::IgnoreCase))
		{
			NewRuleset->Categories.Empty(); 
			NewRuleset->Categories.Add(TEXT("Classic"));

			NewRuleset->Title = TEXT("Capture the Flag");
			NewRuleset->Tooltip = TEXT("Capture the Flag.");
			NewRuleset->Description = TEXT("Capture the Flag, with guns.\n\n<UT.Hub.RulesText_Small>TimeLimit : %timelimit% minutes with halftime</>\n<UT.Hub.RulesText_Small>Mercy Rule : On</>\n<UT.Hub.RulesText_Small>Maximum players : %maxplayers%</>");
			NewRuleset->MaxPlayers = 10;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_CTF.GB_CTF'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTCTFGameMode");
			NewRuleset->GameOptions = FString(TEXT("?TimeLimit=20?GoalScore=0"));
			NewRuleset->bTeamGame = true;

			NewRuleset->OptionFlags = GAME_OPTION_FLAGS_All;

			NewRuleset->MaxTeamCount = 2;
			NewRuleset->MaxTeamSize = 5;
			NewRuleset->MaxPartySize = 5;

			NewRuleset->EpicMaps = "/Game/RestrictedAssets/Maps/CTF-TitanPass";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/CTF-Face";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Pistola/CTF-Pistola";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Polaris/CTF-Polaris";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/CTF-Blank";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/CTF-Quick";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/CTF-Plaza";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/CTF-BigRock";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/CTF-Volcano";
			NewRuleset->DefaultMap = "/Game/RestrictedAssets/Maps/CTF-TitanPass";
		}
		else if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::BIGCTF, ESearchCase::IgnoreCase))
		{
			NewRuleset->Categories.Empty(); 
			NewRuleset->Categories.Add(TEXT("Classic"));

			NewRuleset->Title = TEXT("Big CTF");
			NewRuleset->Tooltip = TEXT("Capture the Flag with large teams.");
			NewRuleset->Description = TEXT("Capture the Flag with large teams.\n\n<UT.Hub.RulesText_Small>TimeLimit : %timelimit% minutes with halftime</>\n<UT.Hub.RulesText_Small>Mercy Rule : On</>\n<UT.Hub.RulesText_Small>Maximum players : %maxplayers%</>");
			NewRuleset->MaxPlayers = 20;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_LargeCTF.GB_LargeCTF'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTCTFGameMode");
			NewRuleset->GameOptions = FString(TEXT("?TimeLimit=20?GoalScore=0"));
			NewRuleset->bTeamGame = true;

			NewRuleset->OptionFlags = GAME_OPTION_FLAGS_All;

			NewRuleset->MaxTeamCount = 2;
			NewRuleset->MaxTeamSize = 10;
			NewRuleset->MaxPartySize = 5;

			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/CTF-Face";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/CTF-TitanPass";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/CTF-Volcano";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/CTF-BigRock";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/CTF-Dam";
			NewRuleset->DefaultMap = "/Game/RestrictedAssets/Maps/CTF-Face";
		}
		else if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::COMPCTF, ESearchCase::IgnoreCase))
		{
			NewRuleset->Categories.Empty();
			NewRuleset->Categories.Add(TEXT("Competitive"));

			NewRuleset->Title = TEXT("Competitive CTF");
			NewRuleset->Tooltip = TEXT("Capture the Flag with competition rules.");
			NewRuleset->Description = TEXT("Capture the Flag, with guns.\n\n<UT.Hub.RulesText_Small>TimeLimit : %timelimit% minutes with halftime</>\n<UT.Hub.RulesText_Small>Mercy Rule : Off</>\n<UT.Hub.RulesText_Small>Maximum players : %maxplayers%</>");
			NewRuleset->MaxPlayers = 10;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_CTF.GB_CTF'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTCTFGameMode");
			NewRuleset->GameOptions = FString(TEXT("?TimeLimit=20?GoalScore=0?MercyScore=0"));
			NewRuleset->bCompetitiveMatch = true;
			NewRuleset->bTeamGame = true;

			NewRuleset->OptionFlags = GAME_OPTION_FLAGS_All;

			NewRuleset->MaxTeamCount = 2;
			NewRuleset->MaxTeamSize = 5;
			NewRuleset->MaxPartySize = 5;

			NewRuleset->EpicMaps = "/Game/RestrictedAssets/Maps/CTF-TitanPass";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/CTF-Face";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Pistola/CTF-Pistola";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Polaris/CTF-Polaris";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/CTF-Blank";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/CTF-Quick";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/CTF-Plaza";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/CTF-BigRock";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/CTF-Volcano";
			NewRuleset->DefaultMap = "/Game/RestrictedAssets/Maps/CTF-TitanPass";
		}
		else if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::iCTF, ESearchCase::IgnoreCase))
		{
			NewRuleset->Categories.Empty(); 
			NewRuleset->Categories.Add(TEXT("Classic"));

			NewRuleset->Title = TEXT("Instagib CTF");
			NewRuleset->Tooltip = TEXT("Instagib CTF");
			NewRuleset->Description = TEXT("Capture the Flag with Instagib rifles.\n\n<UT.Hub.RulesText_Small>Mutators : Instagib</>\n<UT.Hub.RulesText_Small>TimeLimit : %timelimit% minutes with halftime</>\n<UT.Hub.RulesText_Small>Mercy Rule : On</>\n<UT.Hub.RulesText_Small>Maximum players : %maxplayers%</>");
			NewRuleset->MaxPlayers = 20;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_InstagibCTF.GB_InstagibCTF'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTCTFGameMode");
			NewRuleset->GameOptions = FString(TEXT("?TimeLimit=20?GoalScore=0?Mutator=Instagib"));
			NewRuleset->bTeamGame = true;

			NewRuleset->OptionFlags = GAME_OPTION_FLAGS_All;

			NewRuleset->MaxTeamCount = 2;
			NewRuleset->MaxTeamSize = 5;
			NewRuleset->MaxPartySize = 5;

			NewRuleset->MaxMapsInList=16;

			NewRuleset->EpicMaps = "/Game/RestrictedAssets/Maps/CTF-TitanPass";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/CTF-Face";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Pistola/CTF-Pistola";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Polaris/CTF-Polaris";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/CTF-Blank";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/CTF-BigRock";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/CTF-Volcano";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/CTF-Quick";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/CTF-Plaza";

			NewRuleset->DefaultMap = "/Game/RestrictedAssets/Maps/CTF-TitanPass";
		}		
		else if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::QuickPlay_Deathmatch, ESearchCase::IgnoreCase))
		{
			NewRuleset->Title = TEXT("Deathmatch");

			NewRuleset->Categories.Empty(); 
			NewRuleset->Categories.Add(TEXT("QuickPlay"));
			NewRuleset->MaxPlayers = 6;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_DM.GB_DM'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTDMGameMode");
			NewRuleset->GameOptions = FString(TEXT("?TimeLimit=10?GoalScore=0"));
			NewRuleset->bTeamGame = false;
			NewRuleset->MaxTeamCount = 1;
			NewRuleset->MaxTeamSize = 6;
			NewRuleset->MaxPartySize = 5;

			NewRuleset->EpicMaps = "/Game/RestrictedAssets/Maps/DM-Outpost23";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/DM-Chill";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/DM-Underland";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/DM-Outpost23";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Backspace/DM-Backspace";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Salt/DM-Salt";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Batrankus/DM-Batrankus";
			
			NewRuleset->DefaultMap = "/Game/RestrictedAssets/Maps/DM-Chill";
			NewRuleset->bHideFromUI = true;
		}

		else if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::QuickPlay_FlagRun, ESearchCase::IgnoreCase))
		{
			NewRuleset->Title = TEXT("Blitz");

			NewRuleset->Categories.Empty();
			NewRuleset->Categories.Add(TEXT("QuickPlay"));
			NewRuleset->MaxPlayers = 10;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_CTF.GB_CTF'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTFlagRunGame");
			NewRuleset->GameOptions = FString(TEXT(""));
			NewRuleset->bTeamGame = true;
			NewRuleset->MaxTeamCount = 2;
			NewRuleset->MaxTeamSize = 5;
			NewRuleset->MaxPartySize = 5;
 
			NewRuleset->EpicMaps ="/Game/RestrictedAssets/Maps/WIP/FR-Fort";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/FR-MeltDown";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/FR-Loh";

			NewRuleset->DefaultMap = "/Game/RestrictedAssets/Maps/WIP/FR-Fort";
			NewRuleset->bHideFromUI = true;
		}

		else if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::QuickPlay_FlagRunVSAINormal, ESearchCase::IgnoreCase))
		{
			NewRuleset->Title = TEXT("Blitz vs Bots");

			NewRuleset->Categories.Empty();
			NewRuleset->Categories.Add(TEXT("QuickPlay"));
			NewRuleset->MaxPlayers = 5;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_CTF.GB_CTF'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTFlagRunGame");
			NewRuleset->GameOptions = FString(TEXT("?Difficulty=2.0?VSAI=1"));
			NewRuleset->bTeamGame = true;
			NewRuleset->MaxTeamCount = 1;
			NewRuleset->MaxTeamSize = 5;
			NewRuleset->MaxPartySize = 5;

			NewRuleset->EpicMaps = "/Game/RestrictedAssets/Maps/WIP/FR-Fort";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/FR-MeltDown";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/FR-Loh";

			NewRuleset->DefaultMap = "/Game/RestrictedAssets/Maps/WIP/FR-Fort";
			NewRuleset->bHideFromUI = true;
		}
		
		else if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::QuickPlay_FlagRunVSAIHard, ESearchCase::IgnoreCase))
		{

			NewRuleset->Title = TEXT("Blitz vs Bots");


			NewRuleset->Categories.Empty();
			NewRuleset->Categories.Add(TEXT("QuickPlay"));
			NewRuleset->MaxPlayers = 5;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_CTF.GB_CTF'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTFlagRunGame");
			NewRuleset->GameOptions = FString(TEXT("?Difficulty=4.0?VSAI=1?RequireFull=0"));
			NewRuleset->bTeamGame = true;
			NewRuleset->MaxTeamCount = 1;
			NewRuleset->MaxTeamSize = 5;
			NewRuleset->MaxPartySize = 5;

			NewRuleset->EpicMaps = "/Game/RestrictedAssets/Maps/WIP/FR-Fort";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/FR-MeltDown";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/FR-Loh";

			NewRuleset->DefaultMap = "/Game/RestrictedAssets/Maps/WIP/FR-Fort";
			NewRuleset->bHideFromUI = true;
		}
		
		else if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::Ranked_DUEL, ESearchCase::IgnoreCase))
		{
			NewRuleset->Title = TEXT("Duel");

			NewRuleset->Categories.Empty(); 
			NewRuleset->Categories.Add(TEXT("Ranked"));
			NewRuleset->MaxPlayers = 2;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_Duel.GB_Duel'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTDuelGame");
			NewRuleset->GameOptions = FString(TEXT("?TimeLimit=10?GoalScore=0"));
			NewRuleset->bTeamGame = true;
			NewRuleset->bCompetitiveMatch = true;
			NewRuleset->MaxTeamCount = 2;
			NewRuleset->MaxTeamSize = 1;
			NewRuleset->MaxPartySize = 1;
			NewRuleset->EpicMaps = "/Game/RestrictedAssets/Maps/WIP/DM-ASDF";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Solo";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Lea/DM-Lea";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-DeckTest";
			
			NewRuleset->DefaultMap = "/Game/RestrictedAssets/Maps/WIP/DM-ASDF";
			NewRuleset->bHideFromUI = true;
		}

		else if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::Ranked_TEAMSHOWDOWN, ESearchCase::IgnoreCase))
		{
			NewRuleset->Title = TEXT("Showdown");

			NewRuleset->Categories.Empty();
			NewRuleset->Categories.Add(TEXT("Ranked"));

			NewRuleset->MaxPlayers = 6;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_TDM.GB_TDM'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTTeamShowdownGame");
			NewRuleset->GameOptions = FString(TEXT("?TimeLimit=2?GoalScore=5"));
			NewRuleset->bTeamGame = true;
			NewRuleset->MaxTeamCount = 2;
			NewRuleset->MaxTeamSize = 3;
			NewRuleset->MaxPartySize = 3;

			NewRuleset->EpicMaps = "/Game/RestrictedAssets/Maps/DM-Chill";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/DM-Underland";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Unsaved/DM-Unsaved";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Backspace/DM-Backspace";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Salt/DM-Salt";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Batrankus/DM-Batrankus";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Lea/DM-Lea";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Spacer";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Cannon";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Temple";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Focus";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Decktest";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-ASDF";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/DM-Solo";

			NewRuleset->DefaultMap = "/Game/RestrictedAssets/Maps/DM-Chill";
			NewRuleset->bHideFromUI = true;
		}
		
		else if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::Ranked_CTF, ESearchCase::IgnoreCase))
		{
			NewRuleset->Title = TEXT("CTF");
			NewRuleset->Categories.Empty(); 
			NewRuleset->Categories.Add(TEXT("Ranked"));
			NewRuleset->MaxPlayers = 10;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_CTF.GB_CTF'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTCTFGameMode");
			NewRuleset->GameOptions = FString(TEXT("?TimeLimit=20?GoalScore=0"));
			NewRuleset->bTeamGame = true;

			NewRuleset->MaxTeamCount = 2;
			NewRuleset->MaxTeamSize = 5;
			NewRuleset->MaxPartySize = 5;

			NewRuleset->EpicMaps = "/Game/RestrictedAssets/Maps/CTF-TitanPass";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/CTF-Face";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/EpicInternal/Pistola/CTF-Pistola";

			NewRuleset->DefaultMap = "/Game/RestrictedAssets/Maps/CTF-TitanPass";
			NewRuleset->bHideFromUI = true;
		}
		
		else if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::Ranked_FlagRun, ESearchCase::IgnoreCase))
		{
			NewRuleset->Title = TEXT("Blitz");
			NewRuleset->Categories.Empty();
			NewRuleset->Categories.Add(TEXT("Ranked"));

			NewRuleset->MaxPlayers = 10;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_CTF.GB_CTF'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTFlagRunGame");
			NewRuleset->GameOptions = FString(TEXT(""));
			NewRuleset->bTeamGame = true;
			NewRuleset->MaxTeamCount = 2;
			NewRuleset->MaxTeamSize = 5;
			NewRuleset->MaxPartySize = 5;
 
			NewRuleset->EpicMaps ="/Game/RestrictedAssets/Maps/WIP/FR-Fort";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/FR-MeltDown";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/FR-Loh";

			NewRuleset->DefaultMap = "/Game/RestrictedAssets/Maps/WIP/FR-Fort";
			NewRuleset->bHideFromUI = true;
		}
	

		else if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::FlagRun, ESearchCase::IgnoreCase))
		{
			NewRuleset->Categories.Empty();
			NewRuleset->Categories.Add(TEXT("Featured"));

			NewRuleset->Title = TEXT("Blitz");
			NewRuleset->Tooltip = TEXT("Attackers must deliver their flag to the enemy base.");
			NewRuleset->Description = TEXT("Blitz.\n<UT.Hub.RulesText_Small>Maximum players : %maxplayers%</>");
			NewRuleset->MaxPlayers = 10;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_CTF.GB_CTF'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTFlagRunGame");
			NewRuleset->GameOptions = FString(TEXT(""));
			NewRuleset->bTeamGame = true;
			NewRuleset->MaxMapsInList = 16;

			NewRuleset->OptionFlags = GAME_OPTION_FLAGS_All;

			NewRuleset->EpicMaps ="/Game/RestrictedAssets/Maps/WIP/FR-Fort";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/FR-MeltDown";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/FR-Loh";
		}
		else if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::FlagRunVSAI, ESearchCase::IgnoreCase))
		{
			NewRuleset->Categories.Empty();
			NewRuleset->Categories.Add(TEXT("Featured"));

			NewRuleset->Title = TEXT("Blitz vs AI");
			NewRuleset->Tooltip = TEXT("Co-op vs AI.  Attackers must deliver their flag to the enemy base.");
			NewRuleset->Description = TEXT("Blitz Coop vs AI.\n<UT.Hub.RulesText_Small>Maximum players : %maxplayers%</>");
			NewRuleset->MaxPlayers = 5;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_CTF.GB_CTF'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTFlagRunGame");
			NewRuleset->GameOptions = FString(TEXT("?VSAI=1"));
			NewRuleset->bTeamGame = true;
			NewRuleset->MaxMapsInList = 16;

			NewRuleset->OptionFlags = GAME_OPTION_FLAGS_BotSkill;

			NewRuleset->EpicMaps = "/Game/RestrictedAssets/Maps/WIP/FR-Fort";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/FR-MeltDown";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/FR-Loh";
		}

		else if (NewRuleset->UniqueTag.Equals(EEpicDefaultRuleTags::Siege, ESearchCase::IgnoreCase))
		{
			NewRuleset->Categories.Empty();
			NewRuleset->Categories.Add(TEXT("Featured"));

			NewRuleset->Title = TEXT("Siege");
			NewRuleset->Tooltip = TEXT("Prototype PVE mode.  Defend your base against hordes of incoming enemies trying to deliver their flag.");
			NewRuleset->Description = TEXT("PROTOTYPE PVE mode.  Defend your base against hordes of incoming enemies trying to deliver their flag..\n<UT.Hub.RulesText_Small>Maximum players : %maxplayers%</>");
			NewRuleset->MaxPlayers = 5;
			NewRuleset->DisplayTexture = TEXT("Texture2D'/Game/RestrictedAssets/UI/GameModeBadges/GB_CTF.GB_CTF'");
			NewRuleset->GameMode = TEXT("/Script/UnrealTournament.UTFlagRunPvEGame");
			NewRuleset->GameOptions = FString(TEXT(""));
			NewRuleset->bTeamGame = true;
			NewRuleset->MaxMapsInList = 16;

			NewRuleset->OptionFlags = GAME_OPTION_FLAGS_BotSkill + GAME_OPTION_FLAGS_RequireFull;

			NewRuleset->EpicMaps = "/Game/RestrictedAssets/Maps/WIP/FR-Fort";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/FR-MeltDown";
			NewRuleset->EpicMaps = NewRuleset->EpicMaps + ",/Game/RestrictedAssets/Maps/WIP/FR-Loh";
		}


	}
};



