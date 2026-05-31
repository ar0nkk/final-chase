using UnrealBuildTool;
using System.Collections.Generic;

public class FinalChaseTarget : TargetRules
{
    public FinalChaseTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("FinalChase");
    }
}
