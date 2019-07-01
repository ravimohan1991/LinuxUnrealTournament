// (ɔ) The_Cowboy 1000 BC - 2019 AD. All rights reversed.

using UnrealBuildTool;
using System.Collections.Generic;
using System.IO;

public class UnrealTournamentEditorTarget : TargetRules
{
    bool IsLicenseeBuild()
    {
        return true;
    }

    public UnrealTournamentEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;

        ExtraModuleNames.AddRange(new string[] { 
            "UnrealTournament",
            "UnrealTournamentEditor"
        });

        if(!IsLicenseeBuild())
        {
            ExtraModuleNames.Add("OnlineSubsystemMcp");
        }

        ExtraModuleNames.Add("OnlineSubsystemNull");
    }
}
