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
		string projectGenAiPluginPath = Path.Combine(ModuleDirectory, "..", "..", "Plugins", "GenAIForUnreal");
		string engineGenAiPluginPath = Path.Combine(EngineDirectory, "Plugins", "Fab", "GenAIForUnreal");
		if (Directory.Exists(projectGenAiPluginPath) || Directory.Exists(engineGenAiPluginPath))
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
