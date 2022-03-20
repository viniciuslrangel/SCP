using UnrealBuildTool;
using System.Collections.Generic;

public class SCPTarget : TargetRules
{
	public SCPTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.Add("SCP");
	}
}
