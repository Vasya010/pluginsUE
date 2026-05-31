// Some copyright should be here...

using UnrealBuildTool;

public class CombatCoreEditor : ModuleRules
{
    public CombatCoreEditor(ReadOnlyTargetRules Target) : base(Target)
    {
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				// ... add other public dependencies that you statically link with here ...
			}
			);



        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "Core",
            "CoreUObject",
            "Engine",

            "CombatCore",

            // Edytor / UI
            "UnrealEd",
            "Slate",
            "SlateCore",
            "EditorStyle",
            "ToolMenus",
            "EditorWidgets",
            "Projects",

            // Asset tools + toolkit
            "AssetTools",
            "LevelEditor",

            // Details panel
            "PropertyEditor",

            // Viewport + preview scene
            "InputCore",
            "RenderCore",
            "RHI",
            "AdvancedPreviewScene",
            "EditorFramework",

            "AnimationCore",
            "AnimGraphRuntime"

        });


        DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
