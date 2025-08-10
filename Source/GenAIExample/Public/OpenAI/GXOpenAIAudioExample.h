// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenAIExampleDelegates.h"
#include "GameFramework/Actor.h"
#include "Data/OpenAI/GenOAIAudioStructs.h"
#include "Http.h"
#include "Sound/SoundSubmix.h"
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
    //~=============================================================================
    //~ TTS and Transcription Requests
    //~=============================================================================

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

    //~=============================================================================
    //~ Live Audio Recording
    //~=============================================================================

    /** The submix to capture audio from for transcription. This must be assigned in the editor. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|OpenAI Examples", meta = (ToolTip = "The submix to capture audio from for transcription."))
    USoundSubmix* RecordingSubmix;

    /**
     * @brief Starts recording audio from the specified submix.
     * @param ModelName The transcription model to use when recording stops.
     * @param Prompt The prompt to use when recording stops.
     * @param Language The language of the audio.
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI|OpenAI Examples", meta=(ToolTip="Starts recording audio from the specified submix."))
    void StartRecording(const FString& ModelName = TEXT("whisper-1"), const FString& Prompt = TEXT(""), const FString& Language = TEXT("en"));

    /**
     * @brief Stops the active recording and sends the captured audio for transcription.
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI|OpenAI Examples", meta=(ToolTip="Stops recording and sends the audio for transcription."))
    void StopRecordingAndTranscribe();


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
    FString TranscriptionLanguage;
};