// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "Google/GXGoogleAudioExample.h"
#include "Models/Google/GenGoogleTextToSpeech.h"
#include "Models/Google/GenGoogleTranscription.h"
#include "Utilities/GenUtils.h"
#include "Misc/Paths.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Utilities/GenAIAudioUtils.h"
#include "Misc/DateTime.h"
#include "Sound/SoundWave.h"

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
    RecordingSubmix = nullptr;
}

void AGXGoogleAudioExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clear any pending timers
    GetWorld()->GetTimerManager().ClearTimer(FileWriteDelayTimer);

    // Cancel any pending HTTP requests
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

void AGXGoogleAudioExample::RequestTextToSpeech(const FString& TextToSpeak, const FString& ModelName, const FString& VoiceName)
{
    FGoogleTextToSpeechSettings TTSSettings;
    TTSSettings.Model = StringToGoogleTTSModel(ModelName);
    if (TTSSettings.Model == EGoogleTTSModel::Custom)
    {
        TTSSettings.CustomModelName = ModelName;
    }
    TTSSettings.InputText = TextToSpeak;
    TTSSettings.SpeechConfig.Voice = StringToGoogleVoice(VoiceName);

    TWeakObjectPtr<AGXGoogleAudioExample> WeakThis(this);
    
    ActiveTTSRequest = UGenGoogleTextToSpeech::SendTextToSpeechRequest(
        TTSSettings,
        FOnGoogleTTSCompletionResponse::CreateLambda(
            [WeakThis](const TArray<uint8>& AudioData, const FString& Error, bool bSuccess)
            {
                if (!WeakThis.IsValid()) return;
                
                if (bSuccess)
                {
                    USoundWave* SoundWave = UGenAIAudioUtils::ConvertPCMAudioToSoundWave(AudioData);
                    if (SoundWave)
                    {
                        WeakThis->OnUITTSResponse.Broadcast(SoundWave, FString(), true);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("TTS Success, but failed to convert audio data to SoundWave."));
                        WeakThis->OnUITTSResponse.Broadcast(nullptr, TEXT("Failed to create SoundWave from audio data."), false);
                    }
                }
                else
                {
                    WeakThis->OnUITTSResponse.Broadcast(nullptr, Error, false);
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
        return;
    }

    FGoogleTranscriptionSettings TranscriptionSettings;
    TranscriptionSettings.Model = StringToGoogleModel(ModelName);
    if (TranscriptionSettings.Model == EGoogleModels::Custom)
    {
        TranscriptionSettings.CustomModelName = ModelName;
    }
    TranscriptionSettings.Prompt = Prompt;

    TWeakObjectPtr<AGXGoogleAudioExample> WeakThis(this);

    ActiveTranscriptionRequest = UGenGoogleTranscription::SendTranscriptionRequest(
        AudioFilePath,
        TranscriptionSettings,
        FOnGoogleTranscriptionCompletionResponse::CreateLambda(
            [WeakThis](const FString& Transcript, const FString& Error, bool bSuccess)
            {
                if (!WeakThis.IsValid()) return;
                WeakThis->OnUITranscriptionResponse.Broadcast(bSuccess ? Transcript : Error, bSuccess);
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

    FGoogleTranscriptionSettings TranscriptionSettings;
    TranscriptionSettings.Model = StringToGoogleModel(ModelName);
    if (TranscriptionSettings.Model == EGoogleModels::Custom)
    {
        TranscriptionSettings.CustomModelName = ModelName;
    }
    TranscriptionSettings.Prompt = Prompt;

    TWeakObjectPtr<AGXGoogleAudioExample> WeakThis(this);

    ActiveTranscriptionRequest = UGenGoogleTranscription::SendTranscriptionRequestFromData(
        AudioData,
        TranscriptionSettings,
        FOnGoogleTranscriptionCompletionResponse::CreateLambda(
            [WeakThis](const FString& Transcript, const FString& Error, bool bSuccess)
            {
                if (!WeakThis.IsValid()) return;
                WeakThis->OnUITranscriptionResponse.Broadcast(bSuccess ? Transcript : Error, bSuccess);
                WeakThis->ActiveTranscriptionRequest.Reset();
            }));
}

void AGXGoogleAudioExample::StartRecording(const FString& ModelName, const FString& Prompt)
{
    if (!RecordingSubmix)
    {
        UE_LOG(LogTemp, Warning, TEXT("StartRecording: No RecordingSubmix specified. Please assign it in the editor."));
        return;
    }

    TranscriptionModelName = ModelName;
    TranscriptionPrompt = Prompt;

    UAudioMixerBlueprintLibrary::StartRecordingOutput(this, 0.f, RecordingSubmix);
    UE_LOG(LogTemp, Log, TEXT("Started recording submix '%s'"), *RecordingSubmix->GetName());
}

void AGXGoogleAudioExample::StopRecordingAndTranscribe()
{
    if (!RecordingSubmix)
    {
        UE_LOG(LogTemp, Warning, TEXT("StopRecordingAndTranscribe: No RecordingSubmix specified."));
        OnUITranscriptionResponse.Broadcast(TEXT("Recording submix is not set."), false);
        return;
    }

    const FString BaseFileName = FString::Printf(TEXT("mic_recording_%s"), *FDateTime::Now().ToString());
    const FString SaveDirectory = FPaths::ProjectSavedDir() + TEXT("BouncedWavFiles/");

    UAudioMixerBlueprintLibrary::StopRecordingOutput(this, EAudioRecordingExportType::WavFile, BaseFileName, SaveDirectory, RecordingSubmix);
    
    UE_LOG(LogTemp, Log, TEXT("Stop recording command issued. Waiting 0.2s for file to save..."));

    GetWorld()->GetTimerManager().SetTimer(
        FileWriteDelayTimer,
        [this, BaseFileName]() { ProcessRecordedFile(BaseFileName); },
        0.2f,
        false);
}

void AGXGoogleAudioExample::ProcessRecordedFile(FString BaseFileName)
{
    UE_LOG(LogTemp, Log, TEXT("Timer finished. Processing file: %s.wav"), *BaseFileName);

    const FString FileNameWithExtension = BaseFileName + TEXT(".wav");
    
    TArray<uint8> RawPCMData = UGenAIAudioUtils::GetPCMDataFromWavFile(FileNameWithExtension);

    if (RawPCMData.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to read PCM data from %s."), *FileNameWithExtension);
        OnUITranscriptionResponse.Broadcast(TEXT("Failed to read recorded audio file after delay."), false);
        return;
    }

    TArray<uint8> ConvertedPCMData = UGenAIAudioUtils::ConvertAudioToPCM16Mono24kHz(RawPCMData);
    TArray<uint8> WavData = UGenAIAudioUtils::ConvertPCMToWavData(ConvertedPCMData, 24000, 1);

    if (WavData.Num() == 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to convert processed PCM to WAV format."));
        OnUITranscriptionResponse.Broadcast(TEXT("Failed to process recorded audio."), false);
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("Sending %d bytes of WAV data for transcription."), WavData.Num());
    RequestTranscriptionFromData(WavData, TranscriptionModelName, TranscriptionPrompt);
}