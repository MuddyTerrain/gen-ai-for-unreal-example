// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "OpenAI/GXOpenAIAudioExample.h"

#include "AudioCaptureComponent.h"
#include "Models/OpenAI/GenOAITextToSpeech.h"
#include "Models/OpenAI/GenOAITranscription.h"
#include "Misc/Paths.h"
#include "Sound/SoundWave.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Utilities/GenAIAudioUtils.h"
#include "Data/OpenAI/GenOAIAudioStructs.h"
#include "Components/SceneComponent.h"


AGXOpenAIAudioExample::AGXOpenAIAudioExample()
{
    PrimaryActorTick.bCanEverTick = false;

    // Create a root component
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

    // Create and attach the audio capture component
    AudioCapture = CreateDefaultSubobject<UAudioCaptureComponent>(TEXT("AudioCaptureComponent"));
    if (AudioCapture)
    {
        AudioCapture->SetupAttachment(RootComponent);
        AudioCapture->SoundSubmix = RecordingSubmix;

    }
    
    RecordingSubmix = nullptr;
}

void AGXOpenAIAudioExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Clear any pending timers
    GetWorld()->GetTimerManager().ClearTimer(FileWriteDelayTimer);
    
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

void AGXOpenAIAudioExample::RequestTextToSpeech(const FString& TextToSpeak, const FString& ModelName)
{
    FGenOAITextToSpeechSettings TTSSettings;
    TTSSettings.Model = ModelName;  // Set model directly as string
    TTSSettings.InputText = TextToSpeak;
    TTSSettings.Voice = EGenAIVoice::Alloy;

    TWeakObjectPtr<AGXOpenAIAudioExample> WeakThis(this);
    
    ActiveTTSRequest = UGenOAITextToSpeech::SendTextToSpeechRequest(
        TTSSettings,
        FOnTTSCompletionResponse::CreateLambda(
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

void AGXOpenAIAudioExample::RequestTranscriptionFromFile(const FString& AudioFilePath, const FString& ModelName, const FString& Prompt, const FString& Language)
{
    if (!FPaths::FileExists(AudioFilePath))
    {
        OnUITranscriptionResponse.Broadcast(TEXT("File not found: ") + AudioFilePath, false);
        return;
    }

    FGenOAITranscriptionSettings TranscriptionSettings;
    TranscriptionSettings.Model = ModelName;  // Set model directly as string
    TranscriptionSettings.Prompt = Prompt;
    TranscriptionSettings.Language = Language;

    TWeakObjectPtr<AGXOpenAIAudioExample> WeakThis(this);

    ActiveTranscriptionRequest = UGenOAITranscription::SendTranscriptionRequest(
        AudioFilePath,
        TranscriptionSettings,
        FOnTranscriptionCompletionResponse::CreateLambda(
            [WeakThis](const FString& Transcript, const FString& Error, bool bSuccess)
            {
                if (!WeakThis.IsValid()) return;
                WeakThis->OnUITranscriptionResponse.Broadcast(bSuccess ? Transcript : Error, bSuccess);
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

    FGenOAITranscriptionSettings TranscriptionSettings;
    TranscriptionSettings.Model = ModelName;  // Set model directly as string
    TranscriptionSettings.Prompt = Prompt;
    TranscriptionSettings.Language = Language;

    TWeakObjectPtr<AGXOpenAIAudioExample> WeakThis(this);

    ActiveTranscriptionRequest = UGenOAITranscription::SendTranscriptionRequestFromData(
        AudioData,
        TranscriptionSettings,
        FOnTranscriptionCompletionResponse::CreateLambda(
            [WeakThis](const FString& Transcript, const FString& Error, bool bSuccess)
            {
                if (!WeakThis.IsValid()) return;
                WeakThis->OnUITranscriptionResponse.Broadcast(bSuccess ? Transcript : Error, bSuccess);
                WeakThis->ActiveTranscriptionRequest.Reset();
            }));
}

void AGXOpenAIAudioExample::StartRecording(const FString& ModelName, const FString& Prompt, const FString& Language)
{
    if (!RecordingSubmix)
    {
        UE_LOG(LogTemp, Warning, TEXT("StartRecording: No RecordingSubmix specified. Please assign it in the editor."));
        return;
    }
    
    if (!AudioCapture)
    {
        UE_LOG(LogTemp, Warning, TEXT("StartRecording: AudioCapture component is not valid."));
        return;
    }

    TranscriptionModelName = ModelName;
    TranscriptionPrompt = Prompt;
    TranscriptionLanguage = Language;
    
    AudioCapture->SoundSubmix = RecordingSubmix;
    // Route the audio capture to the recording submix
    AudioCapture->SetSubmixSend(RecordingSubmix, 1.0f);

    // Start capturing audio from the microphone
    AudioCapture->Start();

    UAudioMixerBlueprintLibrary::StartRecordingOutput(this, 0.f, RecordingSubmix);
    UE_LOG(LogTemp, Log, TEXT("Started recording submix '%s' and audio capture."), *RecordingSubmix->GetName());
}

void AGXOpenAIAudioExample::StopRecordingAndTranscribe()
{
    if (!RecordingSubmix)
    {
        UE_LOG(LogTemp, Warning, TEXT("StopRecordingAndTranscribe: No RecordingSubmix specified."));
        OnUITranscriptionResponse.Broadcast(TEXT("Recording submix is not set."), false);
        return;
    }

    if (!AudioCapture)
    {
        UE_LOG(LogTemp, Warning, TEXT("StopRecordingAndTranscribe: AudioCapture component is not valid."));
        return;
    }
    
    // Stop capturing audio from the microphone first
    AudioCapture->Stop();

    // Use a fixed file name to overwrite the previous recording
    const FString BaseFileName = TEXT("mic_recording_overwrite");
    const FString SaveDirectory = FPaths::ProjectSavedDir() + TEXT("BouncedWavFiles/");

    UAudioMixerBlueprintLibrary::StopRecordingOutput(this, EAudioRecordingExportType::WavFile, BaseFileName, SaveDirectory, RecordingSubmix);
    
    UE_LOG(LogTemp, Log, TEXT("Stop recording command issued. Waiting 0.2s for file to save..."));

    GetWorld()->GetTimerManager().SetTimer(
        FileWriteDelayTimer,
        [this, BaseFileName]() { ProcessRecordedFile(BaseFileName); },
        0.2f,
        false);
}

void AGXOpenAIAudioExample::ProcessRecordedFile(FString BaseFileName)
{
    UE_LOG(LogTemp, Log, TEXT("Timer finished. Processing file: %s.wav"), *BaseFileName);

    const FString FileNameWithExtension = FPaths::ProjectSavedDir() + TEXT("BouncedWavFiles/") + BaseFileName + TEXT(".wav");
    
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
    RequestTranscriptionFromData(WavData, TranscriptionModelName, TranscriptionPrompt, TranscriptionLanguage);
}
