#pragma once

#include "CoreMinimal.h"
#include "GenAIExampleDelegates.h"
#include "GameFramework/Actor.h"
#include "Data/Google/GenGoogleAudioStructs.h"
#include "Http.h"
#include "GXGoogleAudioExample.generated.h"

UCLASS()
class GENAIEXAMPLE_API AGXGoogleAudioExample : public AActor
{
    GENERATED_BODY()

public:
    AGXGoogleAudioExample();

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    /**
     * @brief Sends text to be converted to speech.
     * @param TextToSpeak The text to convert to speech.
     * @param ModelName The name of the Google TTS model to use.
     * @param VoiceName The name of the voice to use.
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI|Google Examples")
    void RequestTextToSpeech(const FString& TextToSpeak, const FString& ModelName = TEXT("gemini-2.5-pro-preview-tts"), const FString& VoiceName = TEXT("Zephyr"));

    /**
     * @brief Request transcription of audio from a file.
     * @param AudioFilePath The path to the audio file to transcribe.
     * @param ModelName The name of the Google model to use.
     * @param Prompt Optional prompt to guide transcription.
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI|Google Examples")
    void RequestTranscriptionFromFile(const FString& AudioFilePath, const FString& ModelName = TEXT("gemini-1.0-pro"), const FString& Prompt = TEXT(""));

    /**
     * @brief Request transcription of audio from raw data.
     * @param AudioData The raw audio data to transcribe.
     * @param ModelName The name of the Google model to use.
     * @param Prompt Optional prompt to guide transcription.
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI|Google Examples")
    void RequestTranscriptionFromData(const TArray<uint8>& AudioData, const FString& ModelName = TEXT("gemini-1.0-pro"), const FString& Prompt = TEXT(""));

    // -- DELEGATES FOR BLUEPRINT UI --
    
    /** Called when TTS request completes */
    UPROPERTY(BlueprintAssignable, Category = "GenAI|Events")
    FOnUITTSResponse OnUITTSResponse;

    /** Called when transcription request completes */
    UPROPERTY(BlueprintAssignable, Category = "GenAI|Events")
    FOnUITranscriptionResponse OnUITranscriptionResponse;

private:
    /** Helper function to convert string model names to enum types */
    static EGoogleTTSModel StringToGoogleTTSModel(const FString& ModelName);
    static EGoogleModels StringToGoogleModel(const FString& ModelName);
    static EGoogleAIVoice StringToGoogleVoice(const FString& VoiceName);

    /** Keeps track of active HTTP requests to allow cancellation */
    TSharedPtr<IHttpRequest> ActiveTTSRequest;
    TSharedPtr<IHttpRequest> ActiveTranscriptionRequest;
};
