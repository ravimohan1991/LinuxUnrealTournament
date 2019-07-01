// (ɔ) The_Cowboy 1000 BC - 2019 AD. All rights reversed.

using UnrealBuildTool;
using System;

public class BlueprintContext : ModuleRules
{
    bool IsLicenseeBuild()
    {
        return true;//!Directory.Exists("Runtime/NotForLicensees");
    }

    public BlueprintContext(ReadOnlyTargetRules Target) : base(Target)
    {

        /// <summary>
        /// Explicit private PCH for this module. Implies that this module will not use a shared PCH.
        /// </summary>
        /// Since UnrealEngine 4.21
        PrivatePCHHeaderFile = "Public/BlueprintContext.h";

        /// <summary>
        /// List of public dependency module names (no path needed) (automatically does the private/public include). These are modules that are required by our public source files.
        /// </summary>
        PublicDependencyModuleNames.AddRange(new string[] { 
            "Core",// All Present in UnrealEngine
            "CoreUObject",
            "Engine",
            "Slate",
            "SlateCore",
            "UMG",
            "UnrealTournament",// From UnrealTournament
            "OnlineSubsystem",// From Engine/Plugins
            "GameplayTags",
            "Party"// From Engine/Plugins
        });
    
        if(!IsLicenseeBuild())
        {
            /// <summary>
            /// List of modules names (no path needed) with header files that our module's public headers needs access to, but we don't need to "import" or link against.
            /// </summary>
           // PublicIncludePathModuleNames.Add("Social"); Not present in the UnrealEngine atm.

            //PublicDependencyModuleNames.AddRange(new string[] { Not present in the UnrealEngine atm. 
             //   "McpProfileSys",
             //   "Social",
             //   "OnlineSybsystemMcp",
             //   "GameSubCatalog"
            //});
        }

        /// <summary>
        /// List of private dependency module names.  These are modules that our private code depends on but nothing in our public
        /// include files depend on.
        /// </summary>
        PrivateDependencyModuleNames.AddRange(new string[] { 
            "OnlineSubsystemUtils",// From Engine/Plugins
            "InputCore",
            "GameplayAbilities",// From Engine/Plugins
            "Qos",// From Engine/Plugins
            "Json"
        });

        /// <summary>
        /// (This setting is currently not need as we discover all files from the 'Public' folder) List of all paths to include files that are exposed to other modules
        /// </summary>
        // Try diabling this
        PublicIncludePaths.AddRange(new string[] { 
            "BlueprintContext/Public"
        });

        /// <summary>
        /// List of all paths to this module's internal include files, not exposed to other modules (at least one include to the 'Private' path, more if we want to avoid relative paths)
        /// </summary>
        PrivateIncludePaths.AddRange(new string[] {
            "BlueprintContext/Private"
        });

        // Game modules usually are forced to get their own PCH and use non-unity if they have less than a certain number
        // of source files.  We don't want to slow down full builds for our game modules which are not frequently-iterated on
        MinSourceFilesForUnityBuildOverride = 1;
       // MinFilesUsingPrecompiledHeaderOverride = BuildConfiguration.MinFilesUsingPrecompiledHeader; Not int the buildtool atm
    }
}
