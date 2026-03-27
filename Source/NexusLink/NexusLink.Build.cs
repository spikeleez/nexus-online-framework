// Copyright (c) 2026 spikeleez. All rights reserved.

using UnrealBuildTool;

public class NexusLink : ModuleRules
{
	public NexusLink (ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

        // Opt-in to using the "Include-What-You-Use" mode as per engine plugin standard.
        // Implies that PCHUsage is set to UseExplicitOrSharedPCHs.
        IWYUSupport = IWYUSupport.Full;

        PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
            "CoreUObject",
            "Engine",
			"DeveloperSettings",
			"OnlineSubsystem",
            "OnlineSubsystemUtils",
			"CoreOnline"
        });
		
		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Networking",
			"Sockets",
			"SlateCore",
			"Slate"
		});

        // Steam support (conditionally linked — only used at runtime if Steam OSS is active).
        if (Target.Platform == UnrealTargetPlatform.Win64 ||
            Target.Platform == UnrealTargetPlatform.Linux ||
            Target.Platform == UnrealTargetPlatform.Mac)
        {
            AddEngineThirdPartyPrivateStaticDependencies(Target, "Steamworks");
            PublicDefinitions.Add("NEXUSLINK_WITH_STEAM=1");
        }
        else
        {
            PublicDefinitions.Add("NEXUSLINK_WITH_STEAM=0");
        }
    }
}
