// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "Google/GXGoogleAudioExample.h"
#include "Models/Google/GenGoogleTextToSpeech.h"
#include "Models/Google/GenGoogleTranscription.h"
#include "Utilities/GenUtils.h"
#include "Misc/Paths.h"

EGoogleTTSModel AGXGoogleAudioExample::StringToGoogleTTSModel(const FString& ModelName)
{
    if (ModelName == TEXT("gemini-2.5-flash-preview-tts")) return EGoogleTTSModel::Gemini_2_5_Flash_Preview_TTS;
    if (ModelName == TEXT("gemini-2.5-pro-preview-tts")) return EGoogleTTSModel::Gemini_2_5_Pro_Preview_TTS;
    return EGoogleTTSModel::Custom;
}

EGoogleModels AGXGoogleAudioExample::StringToGoogleModel(const FString& ModelName)
{
    return UGenUtils::StringToModel<EGoogleModels>(ModelName, GoogleModelToString, EGoogleModels::Custom);
}

EGoogleAIVoice AGXGoogleAudioExample::StringToGoogleVoice(const FString& VoiceName)
{
    // Default to Zephyr if not found
    EGoogleAIVoice Voice = EGoogleAIVoice::Zephyr;
    for (int32 i = 0; i < static_cast<int32>(EGoogleAIVoice::Sulafat) + 1; ++i)
    {
        if (VoiceName == GoogleAIVoiceToString(static_cast<EGoogleAIVoice>(i)))
        {
            Voice = static_cast<EGoogleAIVoice>(i);
            break;
        }
    }
    return Voice;
}

AGXGoogleAudioExample::AGXGoogleAudioExample()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AGXGoogleAudioExample::RequestTextToSpeech(const FString& TextToSpeak, const FString& ModelName, const FString& VoiceName)
{
    // Configure TTS settings
    FGoogleTextToSpeechSettings TTSSettings;
    TTSSettings.Model = StringToGoogleTTSModel(ModelName);
    if (TTSSettings.Model == EGoogleTTSModel::Custom)
    {
        TTSSettings.CustomModelName = ModelName;
    }
    
    TTSSettings.InputText = TextToSpeak;
    TTSSettings.SpeechConfig.Voice = StringToGoogleVoice(VoiceName);

    // Create a weak pointer to handle potential destruction during async operation
    TWeakObjectPtr<AGXGoogleAudioExample> WeakThis(this);
    
    // Send the request using the static function
    ActiveTTSRequest = UGenGoogleTextToSpeech::SendTextToSpeechRequest(
        TTSSettings,
        FOnGoogleTTSCompletionResponse::CreateLambda(
            [WeakThis](const TArray<uint8>& AudioData, const FString& Error, bool bSuccess)
            {
                if (!WeakThis.IsValid()) return;
                
                if (bSuccess)
                {
                    WeakThis->OnUITTSResponse.Broadcast(AudioData, Error, true);
                }
                else
                {
                    TArray<uint8> EmptyData;
                    WeakThis->OnUITTSResponse.Broadcast(EmptyData, Error, false);
                    UE_LOG(LogTemp, Warning, TEXT("TTS Error: %s"), *Error);
                }
                
                WeakThis->ActiveTTSRequest.Reset();
            }));
}

void AGXGoogleAudioExample::RequestTranscriptionFromFile(const FString& AudioFilePath, const FString& ModelName, const FString& Prompt)
{
    if (!FPaths::FileExists(AudioFilePath))
    {
        OnUITranscriptionResponse.Broadcast(TEXT("File not found: ") + AudioFilePath, false); 
    }

    // Configure transcription settings
    FGoogleTranscriptionSettings TranscriptionSettings;
    TranscriptionSettings.Model = StringToGoogleModel(ModelName);
    if (TranscriptionSettings.Model == EGoogleModels::Custom)
    {
        TranscriptionSettings.CustomModelName = ModelName;
    }
    TranscriptionSettings.Prompt = Prompt;

    // Create a weak pointer to handle potential destruction during async operation
    TWeakObjectPtr<AGXGoogleAudioExample> WeakThis(this);

    // Send the request using the static function
    ActiveTranscriptionRequest = UGenGoogleTranscription::SendTranscriptionRequest(
        AudioFilePath,
        TranscriptionSettings,
        FOnGoogleTranscriptionCompletionResponse::CreateLambda(
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

void AGXGoogleAudioExample::RequestTranscriptionFromData(const TArray<uint8>& AudioData, const FString& ModelName, const FString& Prompt)
{
    if (AudioData.Num() == 0)
    {
        OnUITranscriptionResponse.Broadcast(TEXT("No audio data provided"), false);
        return;
    }

    // Configure transcription settings
    FGoogleTranscriptionSettings TranscriptionSettings;
    TranscriptionSettings.Model = StringToGoogleModel(ModelName);
    if (TranscriptionSettings.Model == EGoogleModels::Custom)
    {
        TranscriptionSettings.CustomModelName = ModelName;
    }
    TranscriptionSettings.Prompt = Prompt;

    // Create a weak pointer to handle potential destruction during async operation
    TWeakObjectPtr<AGXGoogleAudioExample> WeakThis(this);

    // Send the request using the static function
    ActiveTranscriptionRequest = UGenGoogleTranscription::SendTranscriptionRequestFromData(
        AudioData,
        TranscriptionSettings,
        FOnGoogleTranscriptionCompletionResponse::CreateLambda(
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

void AGXGoogleAudioExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
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
