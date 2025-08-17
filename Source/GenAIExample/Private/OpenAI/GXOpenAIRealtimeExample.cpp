// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "OpenAI/GXOpenAIRealtimeExample.h"

#include "Components/RealtimeAudioCaptureComponent.h"
#include "Models/OpenAI/GenOAIRealtime.h"
#include "Sound/SoundWaveProcedural.h"
#include "Sound/SoundSubmix.h"
#include "Engine/Engine.h"
#include "TimerManager.h"
#include "Misc/FileHelper.h"
#include "Utilities/GenAIAudioUtils.h"
#include "AudioDevice.h"

AGXOpenAIRealtimeExample::AGXOpenAIRealtimeExample()
{
    PrimaryActorTick.bCanEverTick = true;
    CurrentState = ERealtimeConversationState::Idle;
    DisplayMicRms = 0.0f;

    AudioCapture = CreateDefaultSubobject<URealtimeAudioCaptureComponent>(TEXT("AudioCapture"));
    AudioCapture->SetupAttachment(RootComponent);
    AudioCapture->bAutoActivate = false;

    AIAudioPlayer = CreateDefaultSubobject<UAudioComponent>(TEXT("AIAudioPlayer"));
    AIAudioPlayer->SetupAttachment(RootComponent);
    AIAudioPlayer->bAutoActivate = false;
    
    RecordingSubmix = nullptr;
}

void AGXOpenAIRealtimeExample::BeginPlay()
{
    Super::BeginPlay();
    
    RealtimeService = UGenOAIRealtime::CreateRealtimeService(this);
    if (RealtimeService)
    {
        RealtimeService->OnConnectedBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleRealtimeConnected);
        RealtimeService->OnConnectionErrorBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleRealtimeConnectionError);
        RealtimeService->OnDisconnectedBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleRealtimeDisconnected);
        RealtimeService->OnAudioResponseBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleRealtimeAudioResponse);
        RealtimeService->OnAudioTranscriptDeltaBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleRealtimeTranscriptDelta);
    }

    if (AudioCapture)
    {
        AudioCapture->OnAudioGenerated.AddUObject(this, &AGXOpenAIRealtimeExample::HandleAudioGenerated);
    }
}

void AGXOpenAIRealtimeExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ToggleConversation(false);
    Super::EndPlay(EndPlayReason);
}

void AGXOpenAIRealtimeExample::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    // --- THREAD-SAFE UI UPDATES ONLY ---
    
    // Process any pending transcript updates that came from the network thread
    FString TranscriptDelta;
    while (PendingTranscriptDeltas.Dequeue(TranscriptDelta))
    {
        FullUserTranscript += TranscriptDelta;
        OnUserTranscriptUpdated.Broadcast(FullUserTranscript);
    }

    // Update the on-screen debug message with the latest RMS value from the audio thread
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Green, FString::Printf(TEXT("Current Mic RMS: %.4f"), DisplayMicRms.load()));
    }

    // Check if the AI has finished speaking
    if (CurrentState == ERealtimeConversationState::SpeakingAI && AIAudioPlayer && !AIAudioPlayer->IsPlaying())
    {
        SetState(ERealtimeConversationState::Connected_Ready);
    }
}

void AGXOpenAIRealtimeExample::ToggleConversation(bool bShouldStart, const FString& Model, const FString& SystemPrompt)
{
    if (bShouldStart)
    {
        if (CurrentState != ERealtimeConversationState::Idle) return;
        if (!RecordingSubmix)
        {
            UE_LOG(LogTemp, Error, TEXT("ToggleConversation FAILED: RecordingSubmix is not assigned in the editor."));
            return;
        }
        if (RealtimeService)
        {
            SetState(ERealtimeConversationState::Connecting);
            CachedSystemPrompt = SystemPrompt;
            RealtimeService->ConnectToServer(Model, false);
        }
    }
    else
    {
        if (CurrentState == ERealtimeConversationState::Idle) return;
        if (RealtimeService)
        {
            RealtimeService->DisconnectFromServer();
        }
    }
}

void AGXOpenAIRealtimeExample::HandleRealtimeConnected(const FString& SessionId)
{
    UE_LOG(LogTemp, Log, TEXT("Realtime VAD Example: Connected with Session ID: %s"), *SessionId);
    
    if (RealtimeService && !CachedSystemPrompt.IsEmpty())
    {
        RealtimeService->SetSystemInstructions(CachedSystemPrompt);
    }

    AIResponseWave = NewObject<USoundWaveProcedural>();
    AIResponseWave->SetSampleRate(24000);
    AIResponseWave->NumChannels = 1;
    AIResponseWave->Duration = INDEFINITELY_LOOPING_DURATION;
    AIResponseWave->SoundGroup = SOUNDGROUP_Voice;
    AIResponseWave->bLooping = false;
    AIAudioPlayer->SetSound(AIResponseWave);
    
    AudioCapture->SetSubmixSend(RecordingSubmix, 1.0f);
    AudioCapture->SetVolumeMultiplier(0.0f);
    
    AudioCapture->Activate();
    SetState(ERealtimeConversationState::Connected_Ready);
}

void AGXOpenAIRealtimeExample::HandleRealtimeConnectionError(int32 StatusCode, const FString& Reason, bool bWasClean)
{
    UE_LOG(LogTemp, Error, TEXT("Realtime VAD Example: Connection Error %d: %s"), StatusCode, *Reason);
    if(AudioCapture && AudioCapture->IsActive()) AudioCapture->Deactivate();
    SetState(ERealtimeConversationState::Idle);
}

void AGXOpenAIRealtimeExample::HandleRealtimeDisconnected()
{
    UE_LOG(LogTemp, Log, TEXT("Realtime VAD Example: Disconnected."));
    GetWorld()->GetTimerManager().ClearTimer(SilenceTimer);
    if (AudioCapture && AudioCapture->IsActive()) AudioCapture->Deactivate();
    if (AIAudioPlayer && AIAudioPlayer->IsPlaying()) AIAudioPlayer->Stop();
    SetState(ERealtimeConversationState::Idle);
}

void AGXOpenAIRealtimeExample::HandleRealtimeAudioResponse(const TArray<uint8>& AudioData)
{
    if (CurrentState == ERealtimeConversationState::WaitingForAI)
    {
        SetState(ERealtimeConversationState::SpeakingAI);
    }
    
    if (AIResponseWave)
    {
        AIResponseWave->QueueAudio(AudioData.GetData(), AudioData.Num());
        if (!AIAudioPlayer->IsPlaying())
        {
            AIAudioPlayer->Play();
        }
    }
}

void AGXOpenAIRealtimeExample::HandleRealtimeTranscriptDelta(const FString& TranscriptDelta)
{
    PendingTranscriptDeltas.Enqueue(TranscriptDelta);
}

void AGXOpenAIRealtimeExample::HandleAudioGenerated(const float* InAudio, int32 NumSamples)
{
    // This function runs on the audio thread.
    const int32 Channels = 2; // set to your capture format
    TArray<uint8> ConvertedAudioBuffer = UGenAIAudioUtils::ConvertAudioToPCM16Mono24kHz(InAudio, NumSamples, Channels);

    // Calculate RMS on the RAW audio for a more responsive VAD
    float Rms = 0.0f;
    for (int32 i = 0; i < NumSamples; ++i)
    {
        Rms += InAudio[i] * InAudio[i];
    }
    Rms = FMath::Sqrt(Rms / FMath::Max(1, NumSamples));
    DisplayMicRms.store(Rms);

    // --- Client-side gating ONLY (server handles actual VAD & commit). ---
    if (Rms > VoiceActivationThreshold)
    {
        if (CurrentState == ERealtimeConversationState::Connected_Ready)
        {
            AsyncTask(ENamedThreads::GameThread, [this]()
            {
                FullUserTranscript.Empty();
                OnUserTranscriptUpdated.Broadcast(FullUserTranscript);
                AccumulatedAudioBuffer.Empty();
                SetState(ERealtimeConversationState::UserIsSpeaking);
            });
        }
        // Clear any pending silence timer
        AsyncTask(ENamedThreads::GameThread, [this]()
        {
            GetWorld()->GetTimerManager().ClearTimer(SilenceTimer);
        });
    }
    else if (CurrentState == ERealtimeConversationState::UserIsSpeaking)
    {
        // Start silence timer if not already active
        AsyncTask(ENamedThreads::GameThread, [this]()
        {
            if (!GetWorld()->GetTimerManager().IsTimerActive(SilenceTimer))
            {
                GetWorld()->GetTimerManager().SetTimer(SilenceTimer, this, &AGXOpenAIRealtimeExample::OnSilenceDetected, SilenceTimeout, false);
            }
        });
    }

    // If speaking, stream audio to server (server VAD will decide when to stop/commit internally)
    if (CurrentState == ERealtimeConversationState::UserIsSpeaking)
    {
        if (RealtimeService) RealtimeService->SendAudioToServer(ConvertedAudioBuffer);
        AccumulatedAudioBuffer.Append(ConvertedAudioBuffer);
    }
}

void AGXOpenAIRealtimeExample::OnSilenceDetected()
{
    if (CurrentState != ERealtimeConversationState::UserIsSpeaking) return;

    if (AccumulatedAudioBuffer.Num() == 0)
    {
        SetState(ERealtimeConversationState::Connected_Ready);
        return;
    }

    // Save utterance for debugging (optional)
    TArray<uint8> WavData = UGenAIAudioUtils::ConvertPCMToWavData(AccumulatedAudioBuffer, 24000, 1);
    const FString FilePath = FPaths::ProjectSavedDir() + TEXT("BouncedWavFiles/last_sent_utterance.wav");
    FFileHelper::SaveArrayToFile(WavData, *FilePath);
    UE_LOG(LogTemp, Log, TEXT("Saved last user utterance to: %s (server-side VAD will auto-commit)"), *FilePath);

    // Do NOT manually commit or request a response here: server VAD auto commits & creates response.
    SetState(ERealtimeConversationState::WaitingForAI);
}

void AGXOpenAIRealtimeExample::SetState(ERealtimeConversationState NewState)
{
    // This function is now only called from the Game Thread
    if (CurrentState != NewState)
    {
        UE_LOG(LogTemp, Warning, TEXT("STATE CHANGE: From '%s' to '%s'"), *UEnum::GetValueAsString(CurrentState), *UEnum::GetValueAsString(NewState));
        CurrentState = NewState;
        OnStateChanged.Broadcast(NewState);
    }
}
