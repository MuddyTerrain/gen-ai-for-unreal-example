# Gemini Project Overview: GenAIExample for Unreal Engine

## Project Overview

This repository contains `GenAIExample`, an Unreal Engine project designed to showcase the capabilities of the **GenAIForUnreal** plugin. The project serves as a practical demonstration and testing ground for the plugin, providing a collection of C++ and Blueprint examples for various generative AI features.

The core of this repository is the `GenAIForUnreal` plugin, which integrates multiple third-party Generative AI APIs (including Google Gemini, OpenAI, Anthropic, and DeepSeek) into the Unreal Engine. It provides a unified interface for developers to add advanced AI capabilities to their games and applications.

### Key Technologies

*   **Engine:** Unreal Engine (versions 5.1+)
*   **Core Plugin:** `GenAIForUnreal`
*   **UI:** Unreal Motion Graphics (UMG) for example widgets.
*   **APIs:** The plugin uses REST (`HTTP`, `Json`) and `WebSockets` to communicate with generative AI services.
*   **Core UE Modules:** `Core`, `Engine`, `InputCore`, `UMG`, `AudioCapture`, `ImageWrapper`.

### Features Demonstrated

Based on the project structure, the examples cover:

*   **Text Chat:** Standard and streaming chat responses.
*   **Image Generation:** Creating images from text prompts.
*   **Multimodal Chat:** Combining text and images in a single prompt.
*   **Audio Interaction:** Audio-to-text (transcription) and text-to-audio (synthesis) conversations.
*   **Structured Output:** Forcing the AI to respond in a specific JSON format.
*   **Realtime API:** Conversations with AI, in realtime. 

## Building and Running

This is a standard Unreal Engine project. To build and run it:

1.  **Generate Project Files:** Right-click the `GenAIExample.uproject` file and select "Generate Visual Studio project files".
2.  **Build the Project:** Open the generated `GenAIExample.sln` in Visual Studio and build the solution (Build > Build Solution).
3.  **Run the Editor:** Set `GenAIExample` as the startup project and run it (Debug > Start Debugging) to open the Unreal Editor.
4.  **Explore Examples:** The main example levels and assets can be found in the `Content/BlueprintsExample/` and `Content/CppExample/` directories.

### Testing

The plugin contains a `GenAITests` module, indicating the presence of automated tests. These can be run using Unreal Engine's **Session Frontend** tool and its **Automation** tab.

## Development Conventions

*   **Project Structure:** The repository is structured as a main project (`GenAIExample`) that uses and demonstrates a plugin (`GenAIForUnreal`). All core AI logic resides within the plugin, while the project contains only example implementations.
*   **API Keys:** API keys for the various AI services are expected to be managed through the plugin's settings within the Unreal Editor, which likely uses the `Plugins/GenAIForUnreal/Content/secureconfig.bin` file for secure storage.
*   **Adding Features:** New AI provider integrations or core features should be added to the `GenAIForUnreal` plugin source.
*   **Adding Examples:** New examples demonstrating plugin features should be added to the `GenAIExample` project, either in C++ under `Source/GenAIExample/` or as Blueprint assets in `Content/`.
*   **Coding Style:** The C++ code adheres to the standard Unreal Engine coding style (PascalCase, `UCLASS`/`USTRUCT` macros, etc.).
