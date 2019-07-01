// (ɔ) The_Cowboy 1000 BC - 2019 AD. All rights reversed.

using UnrealBuildTool;

// A word (actually a line) from Epic.
// This module must be loaded "PreLoadingScreen" in the .uproject file, otherwise it will not hook in time!

public class UnrealTournamentFullScreenMovie : ModuleRules
{
    // Constructor
    public UnrealTournamentFullScreenMovie(ReadOnlyTargetRules Target) : base(Target)
    {
        /// <summary>
        /// Explicit private PCH for this module. Implies that this module will not use a shared PCH.
        /// </summary>
        /// Since UnrealEngine 4.21
        PrivatePCHHeaderFile = "Public/UnrealTournamentFullScreenMovie.h";

        PrivateIncludePaths.Add("UnrealTournamentFullScreenMovie/Private");// What is the deal with relative path convention
        PrivateDependencyModuleNames.AddRange(new string[] { 
            "Core",
            "CoreUObject",
            "MoviePlayer",
            "Slate",
            "SlateCore",
            "InputCore"
        }); 
    }
}