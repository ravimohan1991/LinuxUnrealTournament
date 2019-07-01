// (ɔ) The_Cowboy 1000 BC - 2019 AD. All rights reversed.

using UnrealBuildTool;
using System.IO;

public class UnrealTournamentEditor : ModuleRules
{
    bool IsLicenseeBuild()
    {
        return true;
    }

    public UnrealTournamentEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        bFasterWithoutUnity = true;
        MinFilesUsingPrecompiledHeaderOverride = 1;
        PrivatePCHHeaderFile = "Public/UnrealTournamentEditor.h";

        PublicDependencyModuleNames.AddRange(new string[]{ 
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "UnrealEd",
            "Matinee",
            "Slate",
            "SlateCore",
            "SlateRHIRenderer",
            "UnrealTournament",
            "OnlineSubsystem",
            "OnlineSubsystemUtils",
            "BlueprintContext",
            "BlueprintContextEditor",
            "PakFile",
            "StreamingFile",
            "NetworkFile",
            "PerfCounters",
            "UMGEditor",
            "UMG" });

        if (!IsLicenseeBuild())
        {
            PublicDependencyModuleNames.AddRange(new string[]
                {
                    "McpProfileSys",
                    "GameSubCatalog",
                    "GameSubCatalogEditor",
                    "UTMcpProfile",
                    "LootTables",
                }
            );
        }
        else
        {
            PublicDependencyModuleNames.Add("GithubStubs");
        }
    }
}