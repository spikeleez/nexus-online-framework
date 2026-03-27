// Copyright (c) 2026 spikeleez. All rights reserved.

using UnrealBuildTool;

public class NexusLinkEditor : ModuleRules
{
	public NexusLinkEditor (ReadOnlyTargetRules Target) : base(Target)
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
            "Slate",
            "SlateCore",
            "NexusLink"
        });

        PrivateDependencyModuleNames.AddRange(new string[]
        {

        });
    }
}
