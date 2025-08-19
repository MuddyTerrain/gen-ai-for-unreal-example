// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/AudioComponent.h"
#include "AGXOpenAIRealtimePushToTalk.generated.h"

class UGenOAIRealtime;
class URealtimeAudioCaptureComponent;
class USoundWaveProcedural;
class USoundSubmix;

// Simplified states for Push-to-Talk
UENUM(BlueprintType)
enum class EPushToTalkState : uint8
{
    Idle,
    Connecting,
    Connected_Ready,
    Recording,
    WaitingForAI,
    SpeakingAI
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPushToTalkStateChanged, EPushToTalkState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPushToTalkTranscriptUpdated, const FString&, Transcript);

UCLASS()
class GENAIEXAMPLE_API AGXOpenAIRealtimePushToTalk : public AActor
{
    GENERATED_BODY()

public:
    AGXOpenAIRealtimePushToTalk();

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    //~=============================================================================
    //~ Components & Settings
    //~=============================================================================
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|Audio Settings")
    USoundSubmix* RecordingSubmix;

    //~=============================================================================
    //~ Conversation Control
    //~=============================================================================
    UFUNCTION(BlueprintCallable, Category = "GenAI|PushToTalk")
    void Connect(const FString& Model = TEXT("gpt-4o"), const FString& SystemPrompt = TEXT("You are a helpful assistant."));

    UFUNCTION(BlueprintCallable, Category = "GenAI|PushToTalk")
    void Disconnect();
    
    UFUNCTION(BlueprintCallable, Category = "GenAI|PushToTalk")
    void StartRecording();

    UFUNCTION(BlueprintCallable, Category = "GenAI|PushToTalk")
    void StopRecordingAndProcess();

    //~=============================================================================
    //~ Blueprint Delegates for UI
    //~=============================================================================
    UPROPERTY(BlueprintAssignable, Category = "GenAI|PushToTalk")
    FOnPushToTalkStateChanged OnStateChanged;
    
    UPROPERTY(BlueprintAssignable, Category = "GenAI|PushToTalk")
    FOnPushToTalkTranscriptUpdated OnAssistantTranscriptUpdated;

private:
    // Realtime Service Handlers
    UFUNCTION() void HandleRealtimeConnected(const FString& SessionId);
    UFUNCTION() void HandleRealtimeConnectionError(int32 StatusCode, const FString& Reason, bool bWasClean);
    UFUNCTION() void HandleRealtimeDisconnected();
    UFUNCTION() void HandleRealtimeAudioResponse(const TArray<uint8>& AudioData);
    UFUNCTION() void HandleAssistantTranscriptDelta(const FString& TranscriptDelta);
    UFUNCTION() void OnAIAudioFinished();

    // Audio Capture Handler
    void HandleAudioGenerated(const float* InAudio, int32 NumSamples);

    void SetState(EPushToTalkState NewState);

    UPROPERTY() TObjectPtr<UGenOAIRealtime> RealtimeService;
    UPROPERTY(VisibleAnywhere) TObjectPtr<URealtimeAudioCaptureComponent> AudioCapture;
    UPROPERTY() TObjectPtr<UAudioComponent> AIAudioPlayer;
    UPROPERTY() TObjectPtr<USoundWaveProcedural> AIResponseWave;

    EPushToTalkState CurrentState;
    TArray<uint8> AccumulatedAudioBuffer;
    FString AssistantTranscript;
};
