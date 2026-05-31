using UnrealBuildTool;

public class ZonefallWeather : ModuleRules
{
	public ZonefallWeather(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Niagara"
		});
	}
}
