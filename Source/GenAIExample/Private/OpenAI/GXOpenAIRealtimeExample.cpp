// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "OpenAI/GXOpenAIRealtimeExample.h"

#include "Models/OpenAI/GenOAIRealtime.h"
#include "AudioCapture/Public/AudioCaptureComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "Utilities/GenAIAudioUtils.h" // Your audio utilities

AGXOpenAIRealtimeExample::AGXOpenAIRealtimeExample()
{
    PrimaryActorTick.bCanEverTick = false;
    CurrentState = ERealtimeConversationState::Idle;

    // Create the audio capture component to get microphone input
    AudioCapture = CreateDefaultSubobject<UAudioCaptureComponent>(TEXT("AudioCapture"));
    AudioCapture->SetupAttachment(RootComponent);

    // Create the audio player component to play the AI's voice
    AIAudioPlayer = CreateDefaultSubobject<UAudioComponent>(TEXT("AIAudioPlayer"));
    AIAudioPlayer->SetupAttachment(RootComponent);
    AIAudioPlayer->bAutoActivate = false;
}

void AGXOpenAIRealtimeExample::BeginPlay()
{
    Super::BeginPlay();
    
    // Create the Realtime Service object that will manage the WebSocket
    RealtimeService = UGenOAIRealtime::CreateRealtimeService(this);

    // Bind to its delegates to receive events from the server
    if (RealtimeService)
    {
        RealtimeService->OnConnectedBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleRealtimeConnected);
        RealtimeService->OnConnectionErrorBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleRealtimeConnectionError);
        RealtimeService->OnDisconnectedBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleRealtimeDisconnected);
        RealtimeService->OnTextResponseBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleRealtimeTextResponse);
        RealtimeService->OnAudioResponseBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleRealtimeAudioResponse);
        RealtimeService->OnAudioTranscriptDeltaBP.AddDynamic(this, &AGXOpenAIRealtimeExample::HandleRealtimeTranscriptDelta);
    }

    // Bind to our own audio capture component's delegate
    if (AudioCapture)
    {
        //AudioCapture->OnAudioCapture.AddUObject(this, &AGXOpenAIRealtimeExample::OnAudioCaptured);
    }
}

void AGXOpenAIRealtimeExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    EndConversation(); // Ensure disconnection on EndPlay
    Super::EndPlay(EndPlayReason);
}

void AGXOpenAIRealtimeExample::StartConversation(const FString& Model, const FString& SystemPrompt)
{
    if (CurrentState != ERealtimeConversationState::Idle)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot start conversation, already in progress. End the current one first."));
        return;
    }
    
    if (RealtimeService)
    {
        SetState(ERealtimeConversationState::Connecting);
        CachedSystemPrompt = SystemPrompt;
        RealtimeService->ConnectToServer(Model, false);
    }
}

void AGXOpenAIRealtimeExample::EndConversation()
{
    if (RealtimeService && CurrentState != ERealtimeConversationState::Idle)
    {
        RealtimeService->DisconnectFromServer();
    }
    // The Disconnected handler will clean up the state
}

void AGXOpenAIRealtimeExample::StartSpeaking()
{
    // Allow user to interrupt the AI
    if (CurrentState != ERealtimeConversationState::Connected && CurrentState != ERealtimeConversationState::SpeakingAI)
    {
        UE_LOG(LogTemp, Warning, TEXT("Cannot start speaking, not in a connected or AI-speaking state."));
        return;
    }

    // Clear previous user transcript for the UI
    FullUserTranscript.Empty();
    OnUserTranscriptUpdated.Broadcast(FullUserTranscript);
    
    // Clear the server's audio buffer to enable barge-in
    RealtimeService->ClearInputBuffer();
    
    // Stop AI playback if it's currently talking
    if (AIAudioPlayer->IsPlaying())
    {
        AIAudioPlayer->Stop();
    }

    // Start capturing from the microphone
    if (AudioCapture)
    {
        AudioCapture->Start();
    }

    SetState(ERealtimeConversationState::Listening);
}

void AGXOpenAIRealtimeExample::StopSpeaking()
{
    if (CurrentState != ERealtimeConversationState::Listening)
    {
        return; // Not currently listening, so nothing to do
    }
    
    // Stop capturing microphone audio
    if (AudioCapture)
    {
        AudioCapture->Stop();
    }

    // Tell the server we are done speaking and want a response
    if (RealtimeService)
    {
        RealtimeService->CommitAndRequestResponse();
    }

    SetState(ERealtimeConversationState::WaitingForAI);
}

//~=============================================================================
//~ Private Handlers
//~=============================================================================

void AGXOpenAIRealtimeExample::HandleRealtimeConnected(const FString& SessionId)
{
    UE_LOG(LogTemp, Log, TEXT("Realtime Example: Connected with Session ID: %s"), *SessionId);
    
    // Set the system prompt now that we are connected
    if (RealtimeService && !CachedSystemPrompt.IsEmpty())
    {
        RealtimeService->SetSystemInstructions(CachedSystemPrompt);
    }

    // Create the procedural sound wave that will receive the AI's audio response
    AIResponseWave = NewObject<USoundWaveProcedural>();
    AIResponseWave->SetSampleRate(24000); // OpenAI Realtime uses 24kHz
    AIResponseWave->NumChannels = 1;
    AIResponseWave->Duration = INDEFINITELY_LOOPING_DURATION;
    AIResponseWave->SoundGroup = SOUNDGROUP_Voice;
    AIResponseWave->bLooping = false;

    // Assign our new sound wave to the audio component so it can be played
    AIAudioPlayer->SetSound(AIResponseWave);
    
    SetState(ERealtimeConversationState::Connected);
}

void AGXOpenAIRealtimeExample::HandleRealtimeConnectionError(int32 StatusCode, const FString& Reason, bool bWasClean)
{
    UE_LOG(LogTemp, Error, TEXT("Realtime Example: Connection Error %d: %s"), StatusCode, *Reason);
    SetState(ERealtimeConversationState::Idle);
}

void AGXOpenAIRealtimeExample::HandleRealtimeDisconnected()
{
    UE_LOG(LogTemp, Log, TEXT("Realtime Example: Disconnected."));
    // if (AudioCapture && AudioCapture->IsCapturing())
    // {
    //     AudioCapture->Stop();
    // }
    if (AIAudioPlayer && AIAudioPlayer->IsPlaying())
    {
        AIAudioPlayer->Stop();
    }
    SetState(ERealtimeConversationState::Idle);
}

void AGXOpenAIRealtimeExample::HandleRealtimeTextResponse(const FString& Text)
{
    if (CurrentState == ERealtimeConversationState::WaitingForAI)
    {
        // This is the first piece of text from the AI, so change state
        SetState(ERealtimeConversationState::SpeakingAI);
        FullAIResponse.Empty();
    }

    FullAIResponse += Text;
    OnAIResponseUpdated.Broadcast(FullAIResponse);
}

void AGXOpenAIRealtimeExample::HandleRealtimeAudioResponse(const TArray<uint8>& AudioData)
{
    if (AIResponseWave)
    {
        // Add the incoming audio chunk to the playback queue
        AIResponseWave->QueueAudio(AudioData.GetData(), AudioData.Num());

        // Start playing if we haven't already
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

void AGXOpenAIRealtimeExample::OnAudioCaptured(const float* InAudio, int32 NumSamples)
{
    if (!RealtimeService || CurrentState != ERealtimeConversationState::Listening)
    {
        return;
    }
    
    // Convert the raw float audio from the capture component to 16-bit PCM
    TArray<int16> PcmData;
    PcmData.SetNumUninitialized(NumSamples);
    for (int32 i = 0; i < NumSamples; ++i)
    {
        // Clamp and convert to int16 range
        PcmData[i] = static_cast<int16>(FMath::Clamp(InAudio[i] * 32767.0f, -32768.0f, 32767.0f));
    }
    
    // Create a byte array view of the PCM data to send to the service
    TArray<uint8> AudioBuffer;
    AudioBuffer.Append(reinterpret_cast<uint8*>(PcmData.GetData()), NumSamples * sizeof(int16));

    // Send the processed audio chunk to the server
    RealtimeService->SendAudioToServer(AudioBuffer);
}

void AGXOpenAIRealtimeExample::SetState(ERealtimeConversationState NewState)
{
    if (CurrentState != NewState)
    {
        CurrentState = NewState;
        OnStateChanged.Broadcast(CurrentState);
    }
}
