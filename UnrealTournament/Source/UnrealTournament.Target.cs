// (ɔ) The_Cowboy 1000 BC - 2019 AD. All rights reversed.

using UnrealBuildTool;
using System.Collections.Generic;
using System.IO;

public class UnrealTournamentTarget : TargetRules 
{ 
    bool IsLicenseeBuild()
    {
        return true; //!Directory.Exists("Runtime/NotForLicensees");
    }

    public UnrealTournamentTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;

        // Turn on shipping checks and logging
        bUseLoggingInShipping = true;
        bUseChecksInShipping = true;

        // TargetRules interface
        /// <summary>
        /// List of additional modules to be compiled into the target.
        /// </summary>
        ExtraModuleNames.AddRange(new string[]{
        "UnrealTournament",
        "UnrealTournamentFullScreenMovie"
        });

        // 
        if (!IsLicenseeBuild()) 
        {
            ExtraModuleNames.Add("OnlineSubsystemMcp");
        }

        ExtraModuleNames.Add("OnlineSubsystemNull");
    }
}