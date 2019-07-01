// (ɔ) The_Cowboy 1000 BC - 2019 AD. All rights reversed.

namespace UnrealBuildTool.Rules 
{ 
    public class UTReplayStreamer : ModuleRules 
    { 
        public UTReplayStreamer(ReadOnlyTargetRules Target) : base(Target)
        {
            /// <summary>
            /// Explicit private PCH for this module. Implies that this module will not use a shared PCH.
            /// </summary>
            /// Since UnrealEngine 4.21
            PrivatePCHHeaderFile = "Private/UTReplayStreamer.h";

            PrivateIncludePaths.Add("UTReplayStreamer/Private");
            PrivateDependencyModuleNames.AddRange(new string[] { 
                "Core",// All Present in UnrealEngine
                "CoreUObject",
                "Engine",
                "OnlineSubsystem",// From Engine/Plugins
                "OnlineSubsystemUtils",// From Engine/Plugins
                "Json",
                "HTTP",
                "NetworkReplayStreaming",
                "HttpNetworkReplayStreaming",
                "NullNetworkReplayStreaming"
            });
        }
    }
}