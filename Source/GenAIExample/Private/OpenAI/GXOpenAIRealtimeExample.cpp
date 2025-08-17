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
#include "Settings/GenAISettings.h"

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

    bExtendedLogging = GetDefault<UGenAISettings>()->bEnableExtendedLogging;
    
    RealtimeService = UGenOAIRealtime::CreateRealtimeService(this);
    if (RealtimeService)
    {
        RealtimeService->OnConnectedBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleRealtimeConnected);
        RealtimeService->OnConnectionErrorBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleRealtimeConnectionError);
        RealtimeService->OnDisconnectedBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleRealtimeDisconnected);
        RealtimeService->OnAudioResponseBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleRealtimeAudioResponse);
        // Legacy assistant transcript (streaming) kept for backward compat; route to assistant handler
        RealtimeService->OnAudioTranscriptDeltaBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleRealtimeTranscriptDelta);
        // New explicit user / assistant transcript streams
        RealtimeService->OnUserTranscriptDeltaBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleUserTranscriptDelta);
        RealtimeService->OnAssistantTranscriptDeltaBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleAssistantTranscriptDelta);
        RealtimeService->OnServerSpeechStartedBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleServerSpeechStarted);
        RealtimeService->OnServerSpeechStoppedBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleServerSpeechStopped);
        RealtimeService->OnAudioDoneBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleServerAudioDone);
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

    // Process user transcript deltas
    FString UserDelta;
    while (PendingUserTranscriptDeltas.Dequeue(UserDelta))
    {
        FullUserTranscript += UserDelta;
        OnUserTranscriptUpdated.Broadcast(FullUserTranscript);
    }
    // Process assistant transcript deltas
    FString AssistantDelta;
    bool bAssistantChanged = false;
    while (PendingAssistantTranscriptDeltas.Dequeue(AssistantDelta))
    {
        AssistantTranscript += AssistantDelta;
        bAssistantChanged = true;
    }
    if (bAssistantChanged)
    {
        OnAssistantTranscriptUpdated.Broadcast(AssistantTranscript);
    }

    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(1, 0.0f, FColor::Green, FString::Printf(TEXT("Current Mic RMS: %.4f"), DisplayMicRms.load()));
    }

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
            FGenOAIRealtimeSessionSettings Settings; // picks default model if empty
            Settings.Model = Model;
            Settings.SystemInstructions = SystemPrompt;
            Settings.Temperature = Temperature; // Pass the actor's temperature setting
            Settings.InputTranscriptionModel = InputTranscriptionModel; // optional
            Settings.bEnableServerVAD = bEnableServerVAD;
            Settings.ServerVADThreshold = ServerVADThreshold;
            Settings.ServerVADSilenceMs = ServerVADSilenceMs;
            Settings.ServerVADPrefixPaddingMs = ServerVADPrefixPaddingMs;
            Settings.bServerVADCreateResponse = bServerVADCreateResponse;
            Settings.bServerVADInterruptResponse = bServerVADInterruptResponse;
            Settings.bStreamAssistantAudio = bStreamAssistantAudio; // may be used later if we implement suppression upstream
            RealtimeService->ConnectWithSettings(Settings, false);
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
    if (bExtendedLogging) UE_LOG(LogTemp, Log, TEXT("Realtime Example: Connected Session %s"), *SessionId);


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
    if (bExtendedLogging) UE_LOG(LogTemp, Error, TEXT("Realtime Example: Connection Error %d: %s"), StatusCode, *Reason);
    if(AudioCapture && AudioCapture->IsActive()) AudioCapture->Deactivate();
    SetState(ERealtimeConversationState::Idle);
}

void AGXOpenAIRealtimeExample::HandleRealtimeDisconnected()
{
    if (bExtendedLogging) UE_LOG(LogTemp, Log, TEXT("Realtime Example: Disconnected."));
    GetWorld()->GetTimerManager().ClearTimer(SilenceTimer);
    if (AudioCapture && AudioCapture->IsActive()) AudioCapture->Deactivate();
    if (AIAudioPlayer && AIAudioPlayer->IsPlaying()) AIAudioPlayer->Stop();
    SetState(ERealtimeConversationState::Idle);
}

void AGXOpenAIRealtimeExample::HandleRealtimeAudioResponse(const TArray<uint8>& AudioData)
{
    if (!bStreamAssistantAudio)
    {
        // Suppress playback but still mark state if first chunk
        if (CurrentState == ERealtimeConversationState::WaitingForAI)
        {
            SetState(ERealtimeConversationState::SpeakingAI);
            AssistantTranscript.Empty();
            OnAssistantTranscriptUpdated.Broadcast(AssistantTranscript);
        }
        return; // ignore audio queueing
    }

    if (CurrentState == ERealtimeConversationState::WaitingForAI)
    {
        AssistantTranscript.Empty();
        OnAssistantTranscriptUpdated.Broadcast(AssistantTranscript);
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
    // Treat legacy transcript delta as assistant transcript stream
    PendingAssistantTranscriptDeltas.Enqueue(TranscriptDelta);
}

void AGXOpenAIRealtimeExample::HandleUserTranscriptDelta(const FString& TranscriptDelta)
{
    PendingUserTranscriptDeltas.Enqueue(TranscriptDelta);
}

void AGXOpenAIRealtimeExample::HandleAssistantTranscriptDelta(const FString& TranscriptDelta)
{
    PendingAssistantTranscriptDeltas.Enqueue(TranscriptDelta);
}

void AGXOpenAIRealtimeExample::HandleAudioGenerated(const float* InAudio, int32 NumSamples)
{
    const int32 Channels = 2;
    TArray<uint8> ConvertedAudioBuffer = UGenAIAudioUtils::ConvertAudioToPCM16Mono24kHz(InAudio, NumSamples, Channels);

    // Calculate RMS with bounds checking
    float Rms = 0.0f;
    if (NumSamples > 0)
    {
        for (int32 i = 0; i < NumSamples; ++i) 
        {
            Rms += InAudio[i] * InAudio[i];
        }
        Rms = FMath::Sqrt(Rms / NumSamples);
    }
    DisplayMicRms.store(Rms);

    if (bUseLocalRMSGate)
    {
        if (Rms > VoiceActivationThreshold)
        {
            if (CurrentState == ERealtimeConversationState::Connected_Ready || CurrentState == ERealtimeConversationState::SpeakingAI)
            {
                AsyncTask(ENamedThreads::GameThread, [this]()
                {
                    // Interrupt AI if speaking
                    if (CurrentState == ERealtimeConversationState::SpeakingAI && AIAudioPlayer && AIAudioPlayer->IsPlaying())
                    {
                        AIAudioPlayer->Stop();
                    }
                    FullUserTranscript.Empty();
                    OnUserTranscriptUpdated.Broadcast(FullUserTranscript);
                    AccumulatedAudioBuffer.Empty();
                    SetState(ERealtimeConversationState::UserIsSpeaking);
                });
            }
            AsyncTask(ENamedThreads::GameThread, [this]() 
            { 
                if (IsValid(this) && GetWorld())
                {
                    GetWorld()->GetTimerManager().ClearTimer(SilenceTimer); 
                }
            });
        }
        else if (CurrentState == ERealtimeConversationState::UserIsSpeaking)
        {
            AsyncTask(ENamedThreads::GameThread, [this]()
            {
                if (IsValid(this) && GetWorld() && !GetWorld()->GetTimerManager().IsTimerActive(SilenceTimer))
                {
                    GetWorld()->GetTimerManager().SetTimer(SilenceTimer, this, &AGXOpenAIRealtimeExample::OnSilenceDetected, SilenceTimeout, false);
                }
            });
        }
    }

    // Always stream while ready, user speaking, or AI speaking (for server VAD barge-in)
    if (CurrentState == ERealtimeConversationState::Connected_Ready ||
        CurrentState == ERealtimeConversationState::UserIsSpeaking ||
        CurrentState == ERealtimeConversationState::SpeakingAI)
    {
        if (RealtimeService) 
        {
            RealtimeService->SendAudioToServer(ConvertedAudioBuffer);
        }
        AccumulatedAudioBuffer.Append(ConvertedAudioBuffer);
    }
}

void AGXOpenAIRealtimeExample::HandleServerSpeechStarted(const FString& ItemId)
{
    if (bExtendedLogging) UE_LOG(LogTemp, Log, TEXT("Realtime Example: Server speech started (Item %s)"), *ItemId);

    if (CurrentState == ERealtimeConversationState::SpeakingAI)
    {
        if (AIAudioPlayer && AIAudioPlayer->IsPlaying()) AIAudioPlayer->Stop();
        FullUserTranscript.Empty();
        OnUserTranscriptUpdated.Broadcast(FullUserTranscript);
        AssistantTranscript.Empty();
        OnAssistantTranscriptUpdated.Broadcast(AssistantTranscript);
        AccumulatedAudioBuffer.Empty();
        SetState(ERealtimeConversationState::UserIsSpeaking);
        return;
    }
    if (CurrentState == ERealtimeConversationState::Connected_Ready || CurrentState == ERealtimeConversationState::WaitingForAI)
    {
        FullUserTranscript.Empty();
        OnUserTranscriptUpdated.Broadcast(FullUserTranscript);
        AccumulatedAudioBuffer.Empty();
        SetState(ERealtimeConversationState::UserIsSpeaking);
    }
}

void AGXOpenAIRealtimeExample::HandleServerSpeechStopped(const FString& ItemId)
{
    if (bExtendedLogging) UE_LOG(LogTemp, Log, TEXT("Realtime Example: Server speech stopped (Item %s)"), *ItemId);

    if (CurrentState == ERealtimeConversationState::UserIsSpeaking)
    {
        // Awaiting model response once server stops speech (auto commit)
        SetState(ERealtimeConversationState::WaitingForAI);
    }
}

void AGXOpenAIRealtimeExample::HandleServerAudioDone(const FString& ResponseId)
{
    if (bExtendedLogging) UE_LOG(LogTemp, Log, TEXT("Realtime Example: Audio done (Response %s)"), *ResponseId);

    if (CurrentState == ERealtimeConversationState::SpeakingAI)
    {
        if (!AIAudioPlayer || !AIAudioPlayer->IsPlaying())
        {
            SetState(ERealtimeConversationState::Connected_Ready);
        }
    }
}

void AGXOpenAIRealtimeExample::OnSilenceDetected()
{
    if (!bUseLocalRMSGate) return; // Only relevant when using local RMS gating
    if (CurrentState != ERealtimeConversationState::UserIsSpeaking) return;

    // If no audio gathered, just revert to ready
    if (AccumulatedAudioBuffer.Num() == 0)
    {
        SetState(ERealtimeConversationState::Connected_Ready);
        return;
    }

    // Optional debug save
    TArray<uint8> WavData = UGenAIAudioUtils::ConvertPCMToWavData(AccumulatedAudioBuffer, 24000, 1);
    const FString FilePath = FPaths::ProjectSavedDir() + TEXT("BouncedWavFiles/last_sent_utterance.wav");
    FFileHelper::SaveArrayToFile(WavData, *FilePath);

    // Transition to waiting for AI (server VAD or manual commit logic outside when disabled)
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
