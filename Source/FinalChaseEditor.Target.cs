using UnrealBuildTool;
using System.Collections.Generic;

public class FinalChaseEditorTarget : TargetRules
{
    public FinalChaseEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("FinalChase");
    }
}
