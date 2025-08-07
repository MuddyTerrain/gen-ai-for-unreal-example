// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "OpenAI/GXOpenAIAudioExample.h"
#include "Models/OpenAI/GenOAITextToSpeech.h"
#include "Models/OpenAI/GenOAITranscription.h"
#include "Utilities/GenUtils.h"
#include "Misc/Paths.h"

EOpenAITTSModel AGXOpenAIAudioExample::StringToOpenAITTSModel(const FString& ModelName)
{
    if (ModelName == TEXT("tts-1")) return EOpenAITTSModel::TTS_1;
    if (ModelName == TEXT("tts-1-hd")) return EOpenAITTSModel::TTS_1_HD;
    return EOpenAITTSModel::Custom;
}

EOpenAITranscriptionModel AGXOpenAIAudioExample::StringToOpenAITranscriptionModel(const FString& ModelName)
{
    if (ModelName == TEXT("whisper-1")) return EOpenAITranscriptionModel::Whisper_1;
    return EOpenAITranscriptionModel::Custom;
}



AGXOpenAIAudioExample::AGXOpenAIAudioExample()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AGXOpenAIAudioExample::RequestTextToSpeech(const FString& TextToSpeak, const FString& ModelName)
{
    // Configure TTS settings
    FGenOAITextToSpeechSettings TTSSettings;
    TTSSettings.Model = StringToOpenAITTSModel(ModelName);
    if (TTSSettings.Model == EOpenAITTSModel::Custom)
    {
        TTSSettings.CustomModelName = ModelName;
    }
    
    TTSSettings.InputText = TextToSpeak;
    TTSSettings.Voice = EGenAIVoice::Alloy;  // Using Alloy as default voice

    // Create a weak pointer to handle potential destruction during async operation
    TWeakObjectPtr<AGXOpenAIAudioExample> WeakThis(this);
    
    // Send the request using the static function
    ActiveTTSRequest = UGenOAITextToSpeech::SendTextToSpeechRequest(
        TTSSettings,
        FOnTTSCompletionResponse::CreateLambda(
            [WeakThis](const TArray<uint8>& AudioData, const FString& Error, bool bSuccess)
            {
                if (!WeakThis.IsValid()) return;
                
                WeakThis->OnUITTSResponse.Broadcast(AudioData, Error, bSuccess);
                WeakThis->ActiveTTSRequest.Reset();
            }));
}

void AGXOpenAIAudioExample::RequestTranscriptionFromFile(const FString& AudioFilePath, const FString& ModelName, const FString& Prompt, const FString& Language)
{
    if (!FPaths::FileExists(AudioFilePath))
    {
        OnUITranscriptionResponse.Broadcast(TEXT("File not found: ") + AudioFilePath, false);
        return;
    }

    // Configure transcription settings
    FGenOAITranscriptionSettings TranscriptionSettings;
    TranscriptionSettings.Model = StringToOpenAITranscriptionModel(ModelName);
    if (TranscriptionSettings.Model == EOpenAITranscriptionModel::Custom)
    {
        TranscriptionSettings.CustomModelName = ModelName;
    }
    TranscriptionSettings.Prompt = Prompt;
    TranscriptionSettings.Language = Language;

    // Create a weak pointer to handle potential destruction during async operation
    TWeakObjectPtr<AGXOpenAIAudioExample> WeakThis(this);

    // Send the request using the static function
    ActiveTranscriptionRequest = UGenOAITranscription::SendTranscriptionRequest(
        AudioFilePath,
        TranscriptionSettings,
        FOnTranscriptionCompletionResponse::CreateLambda(
            [WeakThis](const FString& Transcript, const FString& Error, bool bSuccess)
            {
                if (!WeakThis.IsValid()) return;
                if (bSuccess)
                {
                    WeakThis->OnUITranscriptionResponse.Broadcast(Transcript, true);
                }
                else
                {
                    WeakThis->OnUITranscriptionResponse.Broadcast(Error, false);
                }
                WeakThis->ActiveTranscriptionRequest.Reset();
            }));
}

void AGXOpenAIAudioExample::RequestTranscriptionFromData(const TArray<uint8>& AudioData, const FString& ModelName, const FString& Prompt, const FString& Language)
{
    if (AudioData.Num() == 0)
    {
        OnUITranscriptionResponse.Broadcast(TEXT("No audio data provided"), false);
        return;
    }

    // Configure transcription settings
    FGenOAITranscriptionSettings TranscriptionSettings;
    TranscriptionSettings.Model = StringToOpenAITranscriptionModel(ModelName);
    if (TranscriptionSettings.Model == EOpenAITranscriptionModel::Custom)
    {
        TranscriptionSettings.CustomModelName = ModelName;
    }
    TranscriptionSettings.Prompt = Prompt;
    TranscriptionSettings.Language = Language;

    // Create a weak pointer to handle potential destruction during async operation
    TWeakObjectPtr<AGXOpenAIAudioExample> WeakThis(this);

    // Send the request using the static function
    ActiveTranscriptionRequest = UGenOAITranscription::SendTranscriptionRequestFromData(
        AudioData,
        TranscriptionSettings,
        FOnTranscriptionCompletionResponse::CreateLambda(
            [WeakThis](const FString& Transcript, const FString& Error, bool bSuccess)
            {
                if (!WeakThis.IsValid()) return;
                if (bSuccess)
                {
                    WeakThis->OnUITranscriptionResponse.Broadcast(Transcript, true);
                }
                else
                {
                    WeakThis->OnUITranscriptionResponse.Broadcast(Error, false);
                }
                WeakThis->ActiveTranscriptionRequest.Reset();
            }));
}

void AGXOpenAIAudioExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Cancel any pending requests when the actor is destroyed
    if (ActiveTTSRequest.IsValid() && ActiveTTSRequest->GetStatus() == EHttpRequestStatus::Processing)
    {
        ActiveTTSRequest->CancelRequest();
    }
    
    if (ActiveTranscriptionRequest.IsValid() && ActiveTranscriptionRequest->GetStatus() == EHttpRequestStatus::Processing)
    {
        ActiveTranscriptionRequest->CancelRequest();
    }

    Super::EndPlay(EndPlayReason);
}
