using UnrealBuildTool;

public class NexusLink : ModuleRules
{
	public NexusLink (ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
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
	}
}
