// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/AudioComponent.h"
#include "GXOpenAIRealtimeExample.generated.h"

// Forward declarations
class UGenOAIRealtime;
class URealtimeAudioCaptureComponent;
class USoundWaveProcedural;

/** Defines the current state of the real-time voice conversation. */
UENUM(BlueprintType)
enum class ERealtimeConversationState : uint8
{
    Idle			UMETA(DisplayName = "Idle"),
    Connecting		UMETA(DisplayName = "Connecting..."),
    Connected_Ready UMETA(DisplayName = "Ready (Listening for Speech)"),
    UserIsSpeaking	UMETA(DisplayName = "User Speaking"),
    WaitingForAI	UMETA(DisplayName = "Waiting for AI"),
    SpeakingAI		UMETA(DisplayName = "AI Speaking")
};

// UI Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRealtimeStateChanged, ERealtimeConversationState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUserTranscriptUpdated, const FString&, Transcript);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIResponseUpdated, const FString&, AIResponse);

/**
 * An example actor demonstrating a full, hands-free voice conversation with Voice Activity Detection (VAD)
 * using the OpenAI Realtime API service.
 */
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
    //~ VAD Settings (Editable in Blueprint)
    //~=============================================================================

    /** The volume threshold to trigger speech detection. Increase if background noise is an issue. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|VAD Settings")
    float VoiceActivationThreshold = 0.02f;

    /** How long the user must be silent (in seconds) before their speech is sent to the AI. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|VAD Settings")
    float SilenceTimeout = 1.0f;

    //~=============================================================================
    //~ Conversation Control
    //~=============================================================================

    /** Starts or stops the voice conversation. */
    UFUNCTION(BlueprintCallable, Category = "GenAI|OpenAI|Realtime Example")
    void ToggleConversation(bool bShouldStart, const FString& Model = TEXT("gpt-4o"), const FString& SystemPrompt = TEXT("You are a helpful and friendly voice assistant."));

    //~=============================================================================
    //~ Blueprint Delegates for UI
    //~=============================================================================

    UPROPERTY(BlueprintAssignable, Category = "GenAI|OpenAI|Realtime Example")
    FOnRealtimeStateChanged OnStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "GenAI|OpenAI|Realtime Example")
    FOnUserTranscriptUpdated OnUserTranscriptUpdated;

    UPROPERTY(BlueprintAssignable, Category = "GenAI|OpenAI|Realtime Example")
    FOnAIResponseUpdated OnAIResponseUpdated;

private:
    //~=============================================================================
    //~ Internal Handlers & Logic
    //~=============================================================================
    
    UFUNCTION() void HandleRealtimeConnected(const FString& SessionId);
    UFUNCTION() void HandleRealtimeConnectionError(int32 StatusCode, const FString& Reason, bool bWasClean);
    UFUNCTION() void HandleRealtimeDisconnected();
    UFUNCTION() void HandleRealtimeTextResponse(const FString& Text);
    UFUNCTION() void HandleRealtimeAudioResponse(const TArray<uint8>& AudioData);
    UFUNCTION() void HandleRealtimeTranscriptDelta(const FString& TranscriptDelta);

    void HandleAudioGenerated(const float* InAudio, int32 NumSamples);
    void OnSilenceDetected();
    void SetState(ERealtimeConversationState NewState);

    //~=============================================================================
    //~ Components and Services
    //~=============================================================================

    UPROPERTY() TObjectPtr<UGenOAIRealtime> RealtimeService;
    UPROPERTY() TObjectPtr<URealtimeAudioCaptureComponent> AudioCapture;
    UPROPERTY() TObjectPtr<UAudioComponent> AIAudioPlayer;
    UPROPERTY() TObjectPtr<USoundWaveProcedural> AIResponseWave;

    //~=============================================================================
    //~ Internal State
    //~=============================================================================

    UPROPERTY() ERealtimeConversationState CurrentState;
    FTimerHandle SilenceTimer;
    FString FullUserTranscript;
    FString FullAIResponse;
    FString CachedSystemPrompt;
};