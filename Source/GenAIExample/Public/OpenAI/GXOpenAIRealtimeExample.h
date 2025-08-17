// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/AudioComponent.h"
#include "Containers/Queue.h"
#include <atomic>
#include "GXOpenAIRealtimeExample.generated.h"

// Forward declarations
class UGenOAIRealtime;
class URealtimeAudioCaptureComponent;
class USoundWaveProcedural;
class USoundSubmix;

/** Defines the current state of the real-time voice conversation. */
UENUM(BlueprintType)
enum class ERealtimeConversationState : uint8
{
    Idle          UMETA(DisplayName = "Idle"),
    Connecting     UMETA(DisplayName = "Connecting..."),
    Connected_Ready UMETA(DisplayName = "Ready (Listening for Speech)"),
    UserIsSpeaking  UMETA(DisplayName = "User Speaking"),
    WaitingForAI    UMETA(DisplayName = "Waiting for AI"),
    SpeakingAI     UMETA(DisplayName = "AI Speaking")
};

// UI Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRealtimeStateChanged, ERealtimeConversationState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUserTranscriptUpdated, const FString&, Transcript);

UCLASS()
class GENAIEXAMPLE_API AGXOpenAIRealtimeExample : public AActor
{
    GENERATED_BODY()

public:
    AGXOpenAIRealtimeExample();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void Tick(float DeltaSeconds) override;

public:
    //~=============================================================================
    //~ Audio Settings (Editable in Blueprint)
    //~=============================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|Audio Settings")
    USoundSubmix* RecordingSubmix;
    
    //~=============================================================================
    //~ VAD Settings (Editable in Blueprint)
    //~=============================================================================

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|VAD Settings")
    float VoiceActivationThreshold = 0.02f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|VAD Settings")
    float SilenceTimeout = 1.0f;

    //~=============================================================================
    //~ Conversation Control
    //~=============================================================================

    UFUNCTION(BlueprintCallable, Category = "GenAI|OpenAI|Realtime Example")
    void ToggleConversation(bool bShouldStart, const FString& Model = TEXT("gpt-4o"), const FString& SystemPrompt = TEXT("You are a helpful and friendly voice assistant."));

    //~=============================================================================
    //~ Blueprint Delegates for UI
    //~=============================================================================

    UPROPERTY(BlueprintAssignable, Category = "GenAI|OpenAI|Realtime Example")
    FOnRealtimeStateChanged OnStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "GenAI|OpenAI|Realtime Example")
    FOnUserTranscriptUpdated OnUserTranscriptUpdated;

private:
    //~=============================================================================
    //~ Internal Handlers & Logic
    //~=============================================================================
    
    UFUNCTION() void HandleRealtimeConnected(const FString& SessionId);
    UFUNCTION() void HandleRealtimeConnectionError(int32 StatusCode, const FString& Reason, bool bWasClean);
    UFUNCTION() void HandleRealtimeDisconnected();
    UFUNCTION() void HandleRealtimeAudioResponse(const TArray<uint8>& AudioData);
    UFUNCTION() void HandleRealtimeTranscriptDelta(const FString& TranscriptDelta);

    void HandleAudioGenerated(const float* InAudio, int32 NumSamples);
    void OnSilenceDetected();
    void SetState(ERealtimeConversationState NewState);

    //~=============================================================================
    //~ Components and Services
    //~=============================================================================

    UPROPERTY() TObjectPtr<UGenOAIRealtime> RealtimeService;
    UPROPERTY(VisibleAnywhere) 
    TObjectPtr<URealtimeAudioCaptureComponent> AudioCapture;
    UPROPERTY() TObjectPtr<UAudioComponent> AIAudioPlayer;
    UPROPERTY() TObjectPtr<USoundWaveProcedural> AIResponseWave;

    //~=============================================================================
    //~ Internal State & Threading
    //~=============================================================================

    UPROPERTY() ERealtimeConversationState CurrentState;
    FTimerHandle SilenceTimer;
    FString FullUserTranscript;
    FString CachedSystemPrompt;
    
    /** A buffer to hold all the audio chunks for a single user utterance, used for saving to a file. */
    TArray<uint8> AccumulatedAudioBuffer;

    /** Thread-safe queue to hold transcript deltas received from the network thread. */
    TQueue<FString, EQueueMode::Mpsc> PendingTranscriptDeltas;

    /** Thread-safe queue to hold raw audio chunks from the audio thread. */
    TQueue<TArray<float>, EQueueMode::Mpsc> PendingAudioChunks;

    /** Atomic float to safely store the latest mic volume from the audio thread for display only. */
    std::atomic<float> DisplayMicRms;
};
