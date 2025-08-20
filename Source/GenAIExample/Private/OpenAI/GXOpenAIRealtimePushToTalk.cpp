// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "OpenAI/GXOpenAIRealtimePushToTalk.h"
#include "Components/RealtimeAudioCaptureComponent.h"
#include "Models/OpenAI/GenOAIRealtime.h"
#include "Sound/SoundWaveProcedural.h"
#include "Sound/SoundSubmix.h"
#include "Utilities/GenAIAudioUtils.h"
#include "Async/Async.h"

DEFINE_LOG_CATEGORY_STATIC(LogPushToTalk, Log, All);

AGXOpenAIRealtimePushToTalk::AGXOpenAIRealtimePushToTalk()
{
    PrimaryActorTick.bCanEverTick = false;
    CurrentState = EPushToTalkState::Idle;
    AudioCapture = CreateDefaultSubobject<URealtimeAudioCaptureComponent>(TEXT("AudioCapture"));
    AIAudioPlayer = CreateDefaultSubobject<UAudioComponent>(TEXT("AIAudioPlayer"));
}

void AGXOpenAIRealtimePushToTalk::BeginPlay()
{
    Super::BeginPlay();
    
    RealtimeService = UGenOAIRealtime::CreateRealtimeService(this);
    if (RealtimeService)
    {
        RealtimeService->OnConnectedBP.AddDynamic(this, &AGXOpenAIRealtimePushToTalk::HandleRealtimeConnected);
        RealtimeService->OnConnectionErrorBP.AddDynamic(this, &AGXOpenAIRealtimePushToTalk::HandleRealtimeConnectionError);
        RealtimeService->OnDisconnectedBP.AddDynamic(this, &AGXOpenAIRealtimePushToTalk::HandleRealtimeDisconnected);
        RealtimeService->OnAudioResponseBP.AddDynamic(this, &AGXOpenAIRealtimePushToTalk::HandleRealtimeAudioResponse);
        RealtimeService->OnAssistantTranscriptDeltaBP.AddDynamic(this, &AGXOpenAIRealtimePushToTalk::HandleAssistantTranscriptDelta);
    }

    if (AudioCapture)
    {
        AudioCapture->OnAudioGenerated.AddUObject(this, &AGXOpenAIRealtimePushToTalk::HandleAudioGenerated);
    }
    
    if (AIAudioPlayer)
    {
        AIAudioPlayer->OnAudioFinished.AddDynamic(this, &AGXOpenAIRealtimePushToTalk::OnAIAudioFinished);
    }
}

void AGXOpenAIRealtimePushToTalk::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    Disconnect();
    Super::EndPlay(EndPlayReason);
}

void AGXOpenAIRealtimePushToTalk::Connect(const FString& Model, const FString& SystemPrompt)
{
    if (CurrentState != EPushToTalkState::Idle || !RealtimeService) return;
    
    SetState(EPushToTalkState::Connecting);
    
    // For push-to-talk, we disable server VAD entirely.
    FGenOAIRealtimeSessionSettings Settings;
    Settings.Model = Model;
    Settings.SystemInstructions = SystemPrompt;
    Settings.bEnableServerVAD = false; // The key change for manual control
    
    RealtimeService->ConnectWithSettings(Settings, false);
}

void AGXOpenAIRealtimePushToTalk::Disconnect()
{
    if (CurrentState == EPushToTalkState::Idle || !RealtimeService) return;
    RealtimeService->DisconnectFromServer();
}

void AGXOpenAIRealtimePushToTalk::StartRecording()
{
    if (CurrentState != EPushToTalkState::Connected_Ready) return;

    UE_LOG(LogPushToTalk, Log, TEXT("Starting audio recording..."));
    AccumulatedAudioBuffer.Empty();
    AudioCapture->Activate();
    SetState(EPushToTalkState::Recording);
}

void AGXOpenAIRealtimePushToTalk::StopRecordingAndProcess()
{
    if (CurrentState != EPushToTalkState::Recording) return;
    
    UE_LOG(LogPushToTalk, Log, TEXT("Stopping recording. Processing %d bytes of audio."), AccumulatedAudioBuffer.Num());
    AudioCapture->Deactivate();

    if (RealtimeService && AccumulatedAudioBuffer.Num() > 0)
    {
        // Send the entire captured audio chunk in one go
        RealtimeService->SendAudioToServer(AccumulatedAudioBuffer);
        // Manually commit the buffer and request a response
        RealtimeService->CommitAndRequestResponse();
        SetState(EPushToTalkState::WaitingForAI);
    }
    else
    {
        // If no audio was captured, just go back to ready
        SetState(EPushToTalkState::Connected_Ready);
    }
}

void AGXOpenAIRealtimePushToTalk::HandleRealtimeConnected(const FString& SessionId)
{
    AIResponseWave = NewObject<USoundWaveProcedural>();
    AIResponseWave->SetSampleRate(24000);
    AIResponseWave->NumChannels = 1;
    AIResponseWave->Duration = INDEFINITELY_LOOPING_DURATION;
    AIAudioPlayer->SetSound(AIResponseWave);
    
    SetState(EPushToTalkState::Connected_Ready);
}

void AGXOpenAIRealtimePushToTalk::HandleRealtimeConnectionError(int32 StatusCode, const FString& Reason, bool bWasClean)
{
    UE_LOG(LogPushToTalk, Error, TEXT("Connection Error: %s"), *Reason);
    SetState(EPushToTalkState::Idle);
}

void AGXOpenAIRealtimePushToTalk::HandleRealtimeDisconnected()
{
    SetState(EPushToTalkState::Idle);
}

void AGXOpenAIRealtimePushToTalk::HandleRealtimeAudioResponse(const TArray<uint8>& AudioData)
{
    AsyncTask(ENamedThreads::GameThread, [this, AudioData]()
    {
        if (CurrentState == EPushToTalkState::WaitingForAI)
        {
            SetState(EPushToTalkState::SpeakingAI);
        }
        
        if (CurrentState == EPushToTalkState::SpeakingAI && AIResponseWave)
        {
            AIResponseWave->QueueAudio(AudioData.GetData(), AudioData.Num());
            if (AIAudioPlayer && !AIAudioPlayer->IsPlaying())
            {
                AIAudioPlayer->Play();
            }
        }
    });
}

void AGXOpenAIRealtimePushToTalk::HandleAudioGenerated(const float* InAudio, int32 NumSamples)
{
    if (CurrentState == EPushToTalkState::Recording)
    {
        const TArray<uint8> ConvertedAudio = UGenAIAudioUtils::ConvertAudioToPCM16Mono24kHz(InAudio, NumSamples, 2);
        if(ConvertedAudio.Num() > 0)
        {
            AccumulatedAudioBuffer.Append(ConvertedAudio);
        }
    }
}

void AGXOpenAIRealtimePushToTalk::OnAIAudioFinished()
{
    AsyncTask(ENamedThreads::GameThread, [this]()
    {
        if (CurrentState == EPushToTalkState::SpeakingAI)
        {
            SetState(EPushToTalkState::Connected_Ready);
        }
    });
}

void AGXOpenAIRealtimePushToTalk::HandleAssistantTranscriptDelta(const FString& TranscriptDelta) 
{
    AsyncTask(ENamedThreads::GameThread, [this, TranscriptDelta]()
    {
        AssistantTranscript += TranscriptDelta;
        OnAssistantTranscriptUpdated.Broadcast(AssistantTranscript);
    });
}

void AGXOpenAIRealtimePushToTalk::SetState(EPushToTalkState NewState)
{
    if (CurrentState == NewState) return;

    UE_LOG(LogPushToTalk, Display, TEXT("STATE CHANGE: From '%s' to '%s'"),
        *UEnum::GetValueAsString(CurrentState),
        *UEnum::GetValueAsString(NewState));
    CurrentState = NewState;
    OnStateChanged.Broadcast(NewState);

    if (NewState == EPushToTalkState::Idle)
    {
        if (AudioCapture && AudioCapture->IsActive()) AudioCapture->Deactivate();
        if (AIAudioPlayer && AIAudioPlayer->IsPlaying()) AIAudioPlayer->Stop();
    }
    else if (NewState == EPushToTalkState::SpeakingAI)
    {
        AssistantTranscript.Empty();
        OnAssistantTranscriptUpdated.Broadcast(AssistantTranscript);
    }
}
