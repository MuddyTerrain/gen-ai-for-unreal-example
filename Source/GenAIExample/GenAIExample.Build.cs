// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class GenAIExample : ModuleRules
{
	public GenAIExample(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "UMG" });

		PrivateDependencyModuleNames.AddRange(new string[] { "AudioMixer", "AudioCapture" });

		// Check if the GenAIForUnreal plugin directory exists as a project or engine plugin
		string ProjectGenAIPluginPath = Path.Combine(ModuleDirectory, "..", "..", "Plugins", "GenAIForUnreal");
		string EngineGenAIPluginPath = Path.Combine(EngineDirectory, "Plugins", "Fab", "GenAIForUnreal");
		if (Directory.Exists(ProjectGenAIPluginPath) || Directory.Exists(EngineGenAIPluginPath))
		{
			PublicDependencyModuleNames.Add("GenAI");
			PublicDefinitions.Add("WITH_GENAI_MODULE=1");
		}
		else
		{
			PublicDefinitions.Add("WITH_GENAI_MODULE=0");
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
