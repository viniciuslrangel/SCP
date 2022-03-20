using UnrealBuildTool;

public class SCPEditor : ModuleRules
{
	public SCPEditor(ReadOnlyTargetRules target) : base(target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.Add("SCP");

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"Slate",
			"UnrealEd",
			"EditorStyle",
			"DesktopPlatform",
			"RawMesh"
		});

		PrivateDependencyModuleNames.AddRange(new[]
		{
			"InputCore",
			"SlateCore",
			"PropertyEditor",
			"LevelEditor",
			"MainFrame",
			"ToolMenus",
			"DeveloperSettings",
			"Kismet"
		});
	}
}