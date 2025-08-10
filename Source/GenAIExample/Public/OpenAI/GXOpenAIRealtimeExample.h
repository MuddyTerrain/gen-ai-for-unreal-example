// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/AudioComponent.h"
#include "GXOpenAIRealtimeExample.generated.h"

// Forward declarations to keep header dependencies clean
class UGenOAIRealtime;
class UAudioCaptureComponent;
class USoundWaveProcedural;

/** Defines the current state of the real-time voice conversation. */
UENUM(BlueprintType)
enum class ERealtimeConversationState : uint8
{
    Idle			UMETA(DisplayName = "Idle"),
    Connecting		UMETA(DisplayName = "Connecting..."),
    Connected		UMETA(DisplayName = "Connected & Ready"),
    Listening		UMETA(DisplayName = "Listening (User Speaking)"),
    WaitingForAI	UMETA(DisplayName = "Waiting for AI"),
    SpeakingAI		UMETA(DisplayName = "AI Speaking")
};

// Delegate to broadcast state changes to the UI
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRealtimeStateChanged, ERealtimeConversationState, NewState);
// Delegate for the user's live transcribed speech
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUserTranscriptUpdated, const FString&, Transcript);
// Delegate for the AI's complete text response
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAIResponseUpdated, const FString&, AIResponse);


/**
 * An example actor demonstrating a full, back-and-forth voice conversation
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

public:
    //~=============================================================================
    //~ Connection and Session Management
    //~=============================================================================

    /** Connects to the Realtime service and starts a session. */
    UFUNCTION(BlueprintCallable, Category = "GenAI|OpenAI|Realtime Example")
    void StartConversation(const FString& Model = TEXT("gpt-4o"), const FString& SystemPrompt = TEXT("You are a helpful and friendly voice assistant."));

    /** Disconnects from the Realtime service and ends the session. */
    UFUNCTION(BlueprintCallable, Category = "GenAI|OpenAI|Realtime Example")
    void EndConversation();

    //~=============================================================================
    //~ User Interaction (Push-to-Talk)
    //~=============================================================================

    /** Call this when the user starts speaking (e.g., push-to-talk key is pressed). */
    UFUNCTION(BlueprintCallable, Category = "GenAI|OpenAI|Realtime Example")
    void StartSpeaking();

    /** Call this when the user stops speaking (e.g., push-to-talk key is released). */
    UFUNCTION(BlueprintCallable, Category = "GenAI|OpenAI|Realtime Example")
    void StopSpeaking();

    //~=============================================================================
    //~ Blueprint Delegates for UI
    //~=============================================================================

    /** Broadcasts whenever the conversation state changes. */
    UPROPERTY(BlueprintAssignable, Category = "GenAI|OpenAI|Realtime Example")
    FOnRealtimeStateChanged OnStateChanged;

    /** Broadcasts the live transcript of the user's speech as it's understood by the AI. */
    UPROPERTY(BlueprintAssignable, Category = "GenAI|OpenAI|Realtime Example")
    FOnUserTranscriptUpdated OnUserTranscriptUpdated;

    /** Broadcasts the AI's text response as it's being generated. */
    UPROPERTY(BlueprintAssignable, Category = "GenAI|OpenAI|Realtime Example")
    FOnAIResponseUpdated OnAIResponseUpdated;

private:
    //~=============================================================================
    //~ Internal Handlers for Realtime Service Events
    //~=============================================================================
    
    UFUNCTION()
    void HandleRealtimeConnected(const FString& SessionId);
    
    UFUNCTION()
    void HandleRealtimeConnectionError(int32 StatusCode, const FString& Reason, bool bWasClean);
    
    UFUNCTION()
    void HandleRealtimeDisconnected();
    
    UFUNCTION()
    void HandleRealtimeTextResponse(const FString& Text);
    
    UFUNCTION()
    void HandleRealtimeAudioResponse(const TArray<uint8>& AudioData);
    
    UFUNCTION()
    void HandleRealtimeTranscriptDelta(const FString& TranscriptDelta);

    /** Called by the AudioCaptureComponent with new raw microphone data. */
    void OnAudioCaptured(const float* InAudio, int32 NumSamples);

    /** Helper function to change the current state and broadcast the change. */
    void SetState(ERealtimeConversationState NewState);

    //~=============================================================================
    //~ Components and Services
    //~=============================================================================

    /** The underlying OpenAI Realtime service that handles the WebSocket connection. */
    UPROPERTY()
    TObjectPtr<UGenOAIRealtime> RealtimeService;

    /** Component to capture live microphone audio. */
    UPROPERTY()
    TObjectPtr<UAudioCaptureComponent> AudioCapture;

    /** Component to play back the AI's audio response. */
    UPROPERTY()
    TObjectPtr<UAudioComponent> AIAudioPlayer;

    /** The procedural sound wave that holds the AI's streaming audio response. */
    UPROPERTY()
    TObjectPtr<USoundWaveProcedural> AIResponseWave;

    //~=============================================================================
    //~ Internal State
    //~=============================================================================

    UPROPERTY()
    ERealtimeConversationState CurrentState;

    FString FullUserTranscript;
    FString FullAIResponse;
    FString CachedSystemPrompt;
};
