#pragma once

#include "CoreMinimal.h"
#include "GenAIExampleDelegates.h"
#include "GameFramework/Actor.h"
#include "Data/OpenAI/GenOAIAudioStructs.h"
#include "Http.h"
#include "GXOpenAIAudioExample.generated.h"

UCLASS()
class GENAIEXAMPLE_API AGXOpenAIAudioExample : public AActor
{
    GENERATED_BODY()

public:
    AGXOpenAIAudioExample();

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    /**
     * @brief Sends text to be converted to speech.
     * @param TextToSpeak The text to convert to speech.
     * @param ModelName The name of the OpenAI TTS model to use.
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI|OpenAI Examples")
    void RequestTextToSpeech(const FString& TextToSpeak, const FString& ModelName = TEXT("tts-1"));

    /**
     * @brief Request transcription of audio from a file.
     * @param AudioFilePath The path to the audio file to transcribe.
     * @param ModelName The name of the OpenAI model to use.
     * @param Prompt Optional prompt to guide transcription.
     * @param Language Optional language code (e.g., "en" for English).
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI|OpenAI Examples")
    void RequestTranscriptionFromFile(const FString& AudioFilePath, const FString& ModelName = TEXT("whisper-1"), const FString& Prompt = TEXT(""), const FString& Language = TEXT(""));

    /**
     * @brief Request transcription of audio from raw data.
     * @param AudioData The raw audio data to transcribe.
     * @param ModelName The name of the OpenAI model to use.
     * @param Prompt Optional prompt to guide transcription.
     * @param Language Optional language code (e.g., "en" for English).
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI|OpenAI Examples")
    void RequestTranscriptionFromData(const TArray<uint8>& AudioData, const FString& ModelName = TEXT("whisper-1"), const FString& Prompt = TEXT(""), const FString& Language = TEXT(""));

    // -- DELEGATES FOR BLUEPRINT UI --
    
    /** Called when TTS request completes */
    UPROPERTY(BlueprintAssignable, Category = "GenAI|Events")
    FOnUITTSResponse OnUITTSResponse;

    /** Called when transcription request completes */
    UPROPERTY(BlueprintAssignable, Category = "GenAI|Events")
    FOnUITranscriptionResponse OnUITranscriptionResponse;

private:
    /** Helper function to convert string model names to enum types */
    static EOpenAITTSModel StringToOpenAITTSModel(const FString& ModelName);
    static EOpenAITranscriptionModel StringToOpenAITranscriptionModel(const FString& ModelName);

    /** Keeps track of active HTTP requests to allow cancellation */
    TSharedPtr<IHttpRequest> ActiveTTSRequest;
    TSharedPtr<IHttpRequest> ActiveTranscriptionRequest;
};
