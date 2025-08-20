// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "OpenAI/GXOpenAIRealtimeExample.h"
#include "Components/RealtimeAudioCaptureComponent.h"
#include "Models/OpenAI/GenOAIRealtime.h"
#include "Sound/SoundWaveProcedural.h"
#include "Sound/SoundSubmix.h"
#include "Engine/Engine.h"
#include "Utilities/GenAIAudioUtils.h"
#include "Async/Async.h"

DEFINE_LOG_CATEGORY_STATIC(LogRealtimeFSM, Log, All);

AGXOpenAIRealtimeExample::AGXOpenAIRealtimeExample()
{
    PrimaryActorTick.bCanEverTick = true;
    CurrentState = ERealtimeConversationState::Idle;
    AudioCapture = CreateDefaultSubobject<URealtimeAudioCaptureComponent>(TEXT("AudioCapture"));
    AIAudioPlayer = CreateDefaultSubobject<UAudioComponent>(TEXT("AIAudioPlayer"));
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
        RealtimeService->OnUserTranscriptDeltaBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleUserTranscriptDelta);
        RealtimeService->OnAssistantTranscriptDeltaBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleAssistantTranscriptDelta);
        RealtimeService->OnServerSpeechStartedBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleServerSpeechStarted);
        RealtimeService->OnServerSpeechStoppedBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleServerSpeechStopped);
    }

    if (AudioCapture)
    {
        AudioCapture->OnAudioGenerated.AddUObject(this, &AGXOpenAIRealtimeExample::HandleAudioGenerated);
    }
    
    if (AIAudioPlayer)
    {
        AIAudioPlayer->OnAudioFinished.AddDynamic(this, &AGXOpenAIRealtimeExample::OnAIAudioFinished);
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

    FString UserDelta;
    while (PendingUserTranscriptDeltas.Dequeue(UserDelta))
    {
        FullUserTranscript += UserDelta;
        OnUserTranscriptUpdated.Broadcast(FullUserTranscript);
    }

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
}

void AGXOpenAIRealtimeExample::ToggleConversation(bool bShouldStart, const FString& Model, const FString& SystemPrompt)
{
    if (bShouldStart)
    {
        if (CurrentState != ERealtimeConversationState::Idle || !RealtimeService) return;
        
        SetState(ERealtimeConversationState::Connecting);
        
        FGenOAIRealtimeSessionSettings Settings;
        Settings.Model = Model;
        Settings.SystemInstructions = SystemPrompt;
        Settings.bEnableServerVAD = bEnableServerVAD;
        Settings.ServerVADThreshold = ServerVADThreshold;
        Settings.ServerVADSilenceMs = ServerVADSilenceMs;
        Settings.ServerVADPrefixPaddingMs = ServerVADPrefixPaddingMs;
        Settings.bServerVADCreateResponse = bServerVADCreateResponse;
        Settings.bServerVADInterruptResponse = bServerVADInterruptResponse;
        
        RealtimeService->ConnectWithSettings(Settings, false);
    }
    else
    {
        if (CurrentState == ERealtimeConversationState::Idle || !RealtimeService) return;
        RealtimeService->DisconnectFromServer();
    }
}

void AGXOpenAIRealtimeExample::HandleRealtimeConnected(const FString& SessionId)
{
    AIResponseWave = NewObject<USoundWaveProcedural>();
    AIResponseWave->SetSampleRate(24000);
    AIResponseWave->NumChannels = 1;
    AIResponseWave->Duration = INDEFINITELY_LOOPING_DURATION;
    AIResponseWave->SoundGroup = SOUNDGROUP_Voice;
    AIResponseWave->bLooping = false;
    AIAudioPlayer->SetSound(AIResponseWave);
    
    AudioCapture->Activate();
    
    SetState(ERealtimeConversationState::Connected_Ready);
}

void AGXOpenAIRealtimeExample::HandleRealtimeConnectionError(int32 StatusCode, const FString& Reason, bool bWasClean)
{
    UE_LOG(LogRealtimeFSM, Error, TEXT("Connection Error: %s"), *Reason);
    SetState(ERealtimeConversationState::Idle);
}

void AGXOpenAIRealtimeExample::HandleRealtimeDisconnected()
{
    SetState(ERealtimeConversationState::Idle);
}

void AGXOpenAIRealtimeExample::HandleRealtimeAudioResponse(const TArray<uint8>& AudioData)
{
    AsyncTask(ENamedThreads::GameThread, [this, AudioData]()
    {
        if (CurrentState == ERealtimeConversationState::WaitingForAI)
        {
            SetState(ERealtimeConversationState::SpeakingAI);
        }
        
        if (CurrentState == ERealtimeConversationState::SpeakingAI && AIResponseWave)
        {
            AIResponseWave->QueueAudio(AudioData.GetData(), AudioData.Num());
            if (AIAudioPlayer && !AIAudioPlayer->IsPlaying())
            {
                AIAudioPlayer->Play();
            }
        }
    });
}

void AGXOpenAIRealtimeExample::HandleAudioGenerated(const float* InAudio, int32 NumSamples)
{
    if (CurrentState != ERealtimeConversationState::Idle && CurrentState != ERealtimeConversationState::Connecting)
    {
        if (RealtimeService)
        {
            
            const TArray<uint8> ConvertedAudio = UGenAIAudioUtils::ConvertAudioToPCM16Mono24kHz(InAudio, NumSamples, 2);
            if(ConvertedAudio.Num() > 0)
            {
                UE_LOG(LogRealtimeFSM, Verbose, TEXT("Sending %d samples of audio to server."), NumSamples);
                
                RealtimeService->SendAudioToServer(ConvertedAudio);
            }
        }
    }
}

void AGXOpenAIRealtimeExample::HandleServerSpeechStarted(const FString& ItemId)
{
    AsyncTask(ENamedThreads::GameThread, [this]()
    {
        if (CurrentState == ERealtimeConversationState::SpeakingAI)
        {
            UE_LOG(LogRealtimeFSM, Warning, TEXT("Barge-in: Stopping current AI speech."));
            if (AIAudioPlayer) AIAudioPlayer->Stop();
            if (AIResponseWave) AIResponseWave->ResetAudio();
        }
        
        FullUserTranscript.Empty();
        OnUserTranscriptUpdated.Broadcast(FullUserTranscript);
        AssistantTranscript.Empty();
        OnAssistantTranscriptUpdated.Broadcast(AssistantTranscript);
        
        SetState(ERealtimeConversationState::UserIsSpeaking);
    });
}

void AGXOpenAIRealtimeExample::HandleServerSpeechStopped(const FString& ItemId)
{
    AsyncTask(ENamedThreads::GameThread, [this]()
    {
        if (CurrentState == ERealtimeConversationState::UserIsSpeaking)
        {
            SetState(ERealtimeConversationState::WaitingForAI);
        }
    });
}

void AGXOpenAIRealtimeExample::OnAIAudioFinished()
{
    AsyncTask(ENamedThreads::GameThread, [this]()
    {
        if (CurrentState == ERealtimeConversationState::SpeakingAI)
        {
            SetState(ERealtimeConversationState::Connected_Ready);
        }
    });
}

void AGXOpenAIRealtimeExample::HandleUserTranscriptDelta(const FString& TranscriptDelta) { PendingUserTranscriptDeltas.Enqueue(TranscriptDelta); }
void AGXOpenAIRealtimeExample::HandleAssistantTranscriptDelta(const FString& TranscriptDelta) { PendingAssistantTranscriptDeltas.Enqueue(TranscriptDelta); }

void AGXOpenAIRealtimeExample::SetState(ERealtimeConversationState NewState)
{
    if (CurrentState == NewState) return;

    UE_LOG(LogRealtimeFSM, Display, TEXT("STATE CHANGE: From '%s' to '%s'"), *UEnum::GetValueAsString(CurrentState), *UEnum::GetValueAsString(NewState));
    CurrentState = NewState;
    OnStateChanged.Broadcast(NewState);

    if (NewState == ERealtimeConversationState::Idle)
    {
        if (AudioCapture && AudioCapture->IsActive()) AudioCapture->Deactivate();
        if (AIAudioPlayer && AIAudioPlayer->IsPlaying()) AIAudioPlayer->Stop();
    }
}