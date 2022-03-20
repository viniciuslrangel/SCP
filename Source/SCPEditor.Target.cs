using UnrealBuildTool;

public class SCPEditorTarget : TargetRules
{
	public SCPEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange(new[] { "SCP", "SCPEditor" });
	}
}
