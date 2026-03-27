// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class NexusLinkEditor : ModuleRules
{
	public NexusLinkEditor (ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

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
