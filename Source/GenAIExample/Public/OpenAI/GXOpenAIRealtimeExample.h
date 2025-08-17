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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|VAD Settings", meta=(ToolTip="If true, use local RMS threshold; if false rely entirely on server VAD and stream continuously."))
    bool bUseLocalRMSGate = false;

    // Server-side VAD configuration (applied after connect via ConfigureServerVAD)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GenAI|VAD Settings") bool bEnableServerVAD = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GenAI|VAD Settings") float ServerVADThreshold = 0.5f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GenAI|VAD Settings") int32 ServerVADSilenceMs = 400;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GenAI|VAD Settings") int32 ServerVADPrefixPaddingMs = 300;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GenAI|VAD Settings") bool bServerVADCreateResponse = true;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GenAI|VAD Settings") bool bServerVADInterruptResponse = true;

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

    // Additional realtime session settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GenAI|Realtime Settings") FString Voice = TEXT("alloy");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GenAI|Realtime Settings") float Temperature = 0.8f;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GenAI|Realtime Settings") FString OutputAudioFormat = TEXT("pcm16");
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GenAI|Realtime Settings", meta=(ToolTip="If set, server will transcribe user input audio with this model (e.g. gpt-4o-mini-transcribe).")) FString InputTranscriptionModel;
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GenAI|Realtime Settings", meta=(ToolTip="If false, assistant audio won't be played (transcripts still processed).")) bool bStreamAssistantAudio = true;

    // Assistant transcript delegate
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssistantTranscriptUpdated, const FString&, Transcript);
    UPROPERTY(BlueprintAssignable, Category="GenAI|OpenAI|Realtime Example") FOnAssistantTranscriptUpdated OnAssistantTranscriptUpdated;

private:
    //~=============================================================================
    //~ Internal Handlers & Logic
    //~=============================================================================
    
    UFUNCTION() void HandleRealtimeConnected(const FString& SessionId);
    UFUNCTION() void HandleRealtimeConnectionError(int32 StatusCode, const FString& Reason, bool bWasClean);
    UFUNCTION() void HandleRealtimeDisconnected();
    UFUNCTION() void HandleRealtimeAudioResponse(const TArray<uint8>& AudioData);
    UFUNCTION() void HandleRealtimeTranscriptDelta(const FString& TranscriptDelta); // legacy assistant delta
    UFUNCTION() void HandleUserTranscriptDelta(const FString& TranscriptDelta);
    UFUNCTION() void HandleAssistantTranscriptDelta(const FString& TranscriptDelta);
    UFUNCTION() void HandleServerSpeechStarted(const FString& ItemId);
    UFUNCTION() void HandleServerSpeechStopped(const FString& ItemId);
    UFUNCTION() void HandleServerAudioDone(const FString& ResponseId);

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
    FString AssistantTranscript;
    
    /** A buffer to hold all the audio chunks for a single user utterance, used for saving to a file. */
    TArray<uint8> AccumulatedAudioBuffer;


    /** Thread-safe queue to hold raw audio chunks from the audio thread. */
    TQueue<TArray<float>, EQueueMode::Mpsc> PendingAudioChunks;

    /** Atomic float to safely store the latest mic volume from the audio thread for display only. */
    std::atomic<float> DisplayMicRms;

    TQueue<FString, EQueueMode::Mpsc> PendingAssistantTranscriptDeltas;
    TQueue<FString, EQueueMode::Mpsc> PendingUserTranscriptDeltas;
    bool bExtendedLogging = false;
};
