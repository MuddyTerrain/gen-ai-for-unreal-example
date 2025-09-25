// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenAIExampleDelegates.h"
#include "GameFramework/Actor.h"
#if WITH_GENAI_MODULE
#include "Data/Google/GenGoogleAudioStructs.h"
#include "Http.h"
#endif
#include "Sound/SoundSubmix.h"
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
    //~=============================================================================
    //~ TTS and Transcription Requests
    //~=============================================================================
    
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

    //~=============================================================================
    //~ Live Audio Recording
    //~=============================================================================

    /** The submix to capture audio from for transcription. This must be assigned in the editor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|Google Examples", meta = (ToolTip = "The submix to capture audio from for transcription."))
    USoundSubmix* RecordingSubmix;
    
    /**
     * @brief Starts recording audio from the specified submix.
     * @param ModelName The transcription model to use when recording stops.
     * @param Prompt The prompt to use when recording stops.
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI|Google Examples", meta=(ToolTip="Starts recording audio from the specified submix."))
    void StartRecording(const FString& ModelName = TEXT("gemini-1.0-pro"), const FString& Prompt = TEXT(""));

    /**
     * @brief Stops the active recording and sends the captured audio for transcription.
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI|Google Examples", meta=(ToolTip="Stops recording and sends the audio for transcription."))
    void StopRecordingAndTranscribe();
    
    // -- DELEGATES FOR BLUEPRINT UI --
    
    /** Called when TTS request completes with a playable SoundWave */
    UPROPERTY(BlueprintAssignable, Category = "GenAI|Events")
    FOnUITTSResponse OnUITTSResponse;

    /** Called when transcription request completes */
    UPROPERTY(BlueprintAssignable, Category = "GenAI|Events")
    FOnUITranscriptionResponse OnUITranscriptionResponse;

#if WITH_GENAI_MODULE
private:
    static EGoogleAIVoice StringToGoogleVoice(const FString& VoiceName);
    
    /** The actual file processing logic, called after a delay to avoid race conditions. */
    void ProcessRecordedFile(FString BaseFileName);

    /** Keeps track of active HTTP requests to allow cancellation */
    TSharedPtr<IHttpRequest> ActiveTTSRequest;
    TSharedPtr<IHttpRequest> ActiveTranscriptionRequest;

    /** Timer handle for the delay between stopping recording and processing the file. */
    FTimerHandle FileWriteDelayTimer;
    
    // -- Internal state for recording --
    FString TranscriptionModelName;
    FString TranscriptionPrompt;
#endif
};