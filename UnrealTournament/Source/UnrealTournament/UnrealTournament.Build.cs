// (ɔ) The_Cowboy 1000 BC - 2019 AD. All rights reversed.

using UnrealBuildTool;
using System.IO;

public class UnrealTournament : ModuleRules
{
    bool IsLicenseeBuild()
    {
        return true; //!Directory.Exists("Runtime/NotForLicensees");
    }

    public UnrealTournament(ReadOnlyTargetRules Target) : base(Target)
    {
        /// <summary>
        /// If true and unity builds are enabled, this module will build without unity.
        /// Unity sucks!
        /// </summary>
        bFasterWithoutUnity = true;

        /// <summary>
        /// Precompiled header usage for this module
        /// </summary>
        //PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs; Enabled in the workable Linux compile 

        /// <summary>
        /// Overrides BuildConfiguration.MinFilesUsingPrecompiledHeader if non-zero.
        /// </summary>
        // This is a game module with only a small number of source files, so go ahead and force a precompiled header
        // to be generated to make incremental changes to source files as fast as possible for small projects.
        MinFilesUsingPrecompiledHeaderOverride = 1;

        /// <summary>
        /// Explicit private PCH for this module. Implies that this module will not use a shared PCH.
        /// </summary>
        /// Since UnrealEngine 4.21
        PrivatePCHHeaderFile = "Public/UnrealTournament.h";

        /// <summary>
        /// List of all paths to this module's internal include files, not exposed to other modules (at least one include to the 'Private' path, more if we want to avoid relative paths)
        /// </summary>
        PrivateIncludePaths.AddRange(new string[] { 
            "UnrealTournament/Private/Slate",
            "UnrealTournament/Private/Slate/Base",
            "UnrealTournament/Private/Slate/Dialogs",
            "UnrealTournament/Private/Slate/Menus",
            "UnrealTournament/Private/Slate/Panels",
            //"UnrealTournament/Private/Slate/Toasts",
            "UnrealTournament/Private/Slate/Widgets",
            "UnrealTournament/Private/Slate/UIWindows",
            //"UnrealTournament/ThirdParty/sqlite"
        });

        // Let's not forget the Windows!
        if(Target.Platform == UnrealTargetPlatform.Win64)
        {
            /// <summary>
            /// List of system/library paths (directory of .lib files) - typically used for External (third party) modules
            /// </summary>
            PublicLibraryPaths.Add("UnrealTournament/ThirdParty/sqlite/Windows");
            PublicAdditionalLibraries.Add("sqlite3.lib");

            //PublicAdditionalLibraries.Add("bcrypt.lib"); Don't know why it is there when not included in the Windows directory
        }

        /// <summary>
        /// Addition modules this module may require at run-time 
        /// </summary>
         DynamicallyLoadedModuleNames.AddRange(new string[] {
            "UTReplayStreamer",
         }); 

        /// <summary>
        /// List of public dependency module names (no path needed) (automatically does the private/public include). These are modules that are required by our public source files.
        /// </summary>
        PublicDependencyModuleNames.AddRange(new string[] { 
            "Core",// All Present in UnrealEngine
            "CoreUObject",
            "Engine",
            "InputCore",
            "GameplayTasks",
            "AIModule",
            "OnlineSubsystem",// From Engine/Plugins
            "OnlineSubsystemUtils",// From Engine/Plugins
            "RenderCore",
            "Navmesh",
            "WebBrowser",
            "NetworkReplayStreaming",
            "InMemoryNetworkReplayStreaming",
            "Json",
            "JsonUtilities",
            "HTTP",
            "UMG",
            "Party",// From Engine/Plugins
            "Lobby",// From Engine/Plugins
            "Qos",// From Engine/Plugins
            "BlueprintContext",// From UnrealTournament (Finally!)
            "EngineSettings",
            "Landscape",//From Engine/Content. What module is that? might be troublesome for compilation
            "Foliage",// From Engine/Content
            "PerfCounters",
            "PakFile",
            "UnrealTournamentFullScreenMovie",// Also from UnrealTournament
	    "SourceControl"	
        });

        /// <summary>
        /// List of private dependency module names.  These are modules that our private code depends on but nothing in our public
        /// include files depend on.
        /// </summary>
        PrivateDependencyModuleNames.AddRange(new string[] { 
            "Slate", // All Present in UnrealEngine
            "SlateCore", 
            "FriendsAndChat",
            "Sockets",
            "Analytics",
            "AnalyticsET"
        });

        // Loading modules for Server build.
        if(Target.Type != TargetRules.TargetType.Server)
        {
            PublicDependencyModuleNames.AddRange(new string[] { 
                "AppFramework",
                "RHI",
                "SlateRHIRenderer",
                "MoviePlayer"
            });
        }

        // Loading modules for Editor build.
        if(Target.Type != TargetRules.TargetType.Editor)
        {
            PublicDependencyModuleNames.AddRange(new string[] { 
                
            });
        }

        CircularlyReferencedDependentModules.Add("BlueprintContext");

        PublicDefinitions.Add("WITH_PROFILE=0");
        PublicDefinitions.Add("WITH_SOCIAL=0");
	PublicDefinitions.Add("WITH_QOSREPORTER=0");

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                    "GithubStubs",
            }
        );
    }
}
