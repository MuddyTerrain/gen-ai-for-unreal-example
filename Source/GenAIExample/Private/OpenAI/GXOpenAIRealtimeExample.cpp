// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "OpenAI/GXOpenAIRealtimeExample.h"

#include "Components/RealtimeAudioCaptureComponent.h"
#include "Models/OpenAI/GenOAIRealtime.h"
#include "Sound/SoundWaveProcedural.h"

AGXOpenAIRealtimeExample::AGXOpenAIRealtimeExample()
{
    PrimaryActorTick.bCanEverTick = true; // VAD needs Tick
    CurrentState = ERealtimeConversationState::Idle;

    AudioCapture = CreateDefaultSubobject<URealtimeAudioCaptureComponent>(TEXT("AudioCapture"));
    AudioCapture->SetupAttachment(RootComponent);
    AudioCapture->bAutoActivate = false;

    AIAudioPlayer = CreateDefaultSubobject<UAudioComponent>(TEXT("AIAudioPlayer"));
    AIAudioPlayer->SetupAttachment(RootComponent);
    AIAudioPlayer->bAutoActivate = false;
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

    // If the AI has finished speaking, return to the ready state to listen for the user.
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
    
    AudioCapture->Activate(); // Start listening immediately
    SetState(ERealtimeConversationState::Connected_Ready);
}

void AGXOpenAIRealtimeExample::HandleRealtimeConnectionError(int32 StatusCode, const FString& Reason, bool bWasClean)
{
    UE_LOG(LogTemp, Error, TEXT("Realtime VAD Example: Connection Error %d: %s"), StatusCode, *Reason);
    AudioCapture->Deactivate();
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
        // First audio chunk received, transition to speaking state.
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
    FullUserTranscript += TranscriptDelta;
    OnUserTranscriptUpdated.Broadcast(FullUserTranscript);
}

void AGXOpenAIRealtimeExample::HandleAudioGenerated(const float* InAudio, int32 NumSamples)
{
    if (CurrentState != ERealtimeConversationState::Connected_Ready && CurrentState != ERealtimeConversationState::UserIsSpeaking)
    {
        return;
    }

    // Calculate RMS to get the volume of the audio chunk
    float Rms = 0.0f;
    for (int32 i = 0; i < NumSamples; ++i)
    {
        Rms += InAudio[i] * InAudio[i];
    }
    Rms = FMath::Sqrt(Rms / NumSamples);

    // VAD Logic
    if (Rms > VoiceActivationThreshold)
    {
        // User is speaking
        if (CurrentState == ERealtimeConversationState::Connected_Ready)
        {
            // This is the beginning of speech
            FullUserTranscript.Empty();
            OnUserTranscriptUpdated.Broadcast(FullUserTranscript);
            RealtimeService->ClearInputBuffer();
            SetState(ERealtimeConversationState::UserIsSpeaking);
        }
        
        // We are in the UserIsSpeaking state, so clear the silence timer
        GetWorld()->GetTimerManager().ClearTimer(SilenceTimer);
    }
    else if (CurrentState == ERealtimeConversationState::UserIsSpeaking)
    {
        // User was speaking, but is now quiet. Start the silence timer if it's not already running.
        if (!GetWorld()->GetTimerManager().IsTimerActive(SilenceTimer))
        {
            GetWorld()->GetTimerManager().SetTimer(SilenceTimer, this, &AGXOpenAIRealtimeExample::OnSilenceDetected, SilenceTimeout, false);
        }
    }

    // If we are in the speaking state, stream the audio data
    if(CurrentState == ERealtimeConversationState::UserIsSpeaking)
    {
        TArray<int16> PcmData;
        PcmData.SetNumUninitialized(NumSamples);
        for (int32 i = 0; i < NumSamples; ++i)
        {
            PcmData[i] = static_cast<int16>(FMath::Clamp(InAudio[i] * 32767.0f, -32768.0f, 32767.0f));
        }
        TArray<uint8> AudioBuffer;
        AudioBuffer.Append(reinterpret_cast<uint8*>(PcmData.GetData()), NumSamples * sizeof(int16));
        RealtimeService->SendAudioToServer(AudioBuffer);
    }
}

void AGXOpenAIRealtimeExample::OnSilenceDetected()
{
    if (CurrentState != ERealtimeConversationState::UserIsSpeaking) return;

    UE_LOG(LogTemp, Log, TEXT("Silence detected. Committing audio and requesting AI response."));
    if (RealtimeService)
    {
        RealtimeService->CommitAndRequestResponse();
    }
    SetState(ERealtimeConversationState::WaitingForAI);
}

void AGXOpenAIRealtimeExample::SetState(ERealtimeConversationState NewState)
{
    if (CurrentState != NewState)
    {
        CurrentState = NewState;
        OnStateChanged.Broadcast(CurrentState);
    }
}
