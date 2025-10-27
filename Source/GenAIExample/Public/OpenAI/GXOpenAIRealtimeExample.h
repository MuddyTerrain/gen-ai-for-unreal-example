// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/AudioComponent.h"
#include "Containers/Queue.h"
#include <atomic>

#if WITH_GENAI_MODULE
#include "Components/RealtimeAudioCaptureComponent.h"
#include "Models/OpenAI/GenOAIRealtime.h"
#endif

#include "GXOpenAIRealtimeExample.generated.h"

class URealtimeAudioCaptureComponent;
class UGenOAIRealtime;
class USoundWaveProcedural;
class USoundSubmix;

UENUM(BlueprintType)
enum class ERealtimeConversationState : uint8
{
    Idle,
    Connecting,
    Connected_Ready,
    UserIsSpeaking,
    WaitingForAI,
    SpeakingAI
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRealtimeStateChanged, ERealtimeConversationState, NewState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUserTranscriptUpdated, const FString&, Transcript);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAssistantTranscriptUpdated, const FString&, Transcript);


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
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GenAI|Audio Settings")
    USoundSubmix* RecordingSubmix;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GenAI|VAD Settings")
    bool bEnableServerVAD = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GenAI|VAD Settings")
    float ServerVADThreshold = 0.5f;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GenAI|VAD Settings")
    int32 ServerVADSilenceMs = 400;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GenAI|VAD Settings")
    int32 ServerVADPrefixPaddingMs = 300;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GenAI|VAD Settings")
    bool bServerVADCreateResponse = true;
    
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="GenAI|VAD Settings")
    bool bServerVADInterruptResponse = true;

    UFUNCTION(BlueprintCallable, Category = "GenAI|OpenAI|Realtime Example")
    void ToggleConversation(bool bShouldStart, const FString& Model = TEXT("gpt-4o"), const FString& SystemPrompt = TEXT("You are a helpful and friendly voice assistant."));

    UPROPERTY(BlueprintAssignable, Category = "GenAI|OpenAI|Realtime Example")
    FOnRealtimeStateChanged OnStateChanged;

    UPROPERTY(BlueprintAssignable, Category = "GenAI|OpenAI|Realtime Example")
    FOnUserTranscriptUpdated OnUserTranscriptUpdated;
    
    UPROPERTY(BlueprintAssignable, Category="GenAI|OpenAI|Realtime Example")
    FOnAssistantTranscriptUpdated OnAssistantTranscriptUpdated;
    
private:
    UFUNCTION() void HandleRealtimeConnected(const FString& SessionId);
    UFUNCTION() void HandleRealtimeConnectionError(int32 StatusCode, const FString& Reason, bool bWasClean);
    UFUNCTION() void HandleRealtimeDisconnected();
    UFUNCTION() void HandleRealtimeAudioResponse(const TArray<uint8>& AudioData);
    UFUNCTION() void HandleUserTranscriptDelta(const FString& TranscriptDelta);
    UFUNCTION() void HandleAssistantTranscriptDelta(const FString& TranscriptDelta);
    UFUNCTION() void HandleServerSpeechStarted(const FString& ItemId);
    UFUNCTION() void HandleServerSpeechStopped(const FString& ItemId);
    UFUNCTION() void OnAIAudioFinished();

    void HandleAudioGenerated(const float* InAudio, int32 NumSamples);
    void SetState(ERealtimeConversationState NewState);

    UPROPERTY()
    TObjectPtr<UObject> RealtimeService;
    
    UPROPERTY(VisibleAnywhere, Category = "Components")
    TObjectPtr<URealtimeAudioCaptureComponent> AudioCapture;
    
    UPROPERTY()
    TObjectPtr<UAudioComponent> AIAudioPlayer;
    UPROPERTY() TObjectPtr<USoundWaveProcedural> AIResponseWave;

    UPROPERTY() ERealtimeConversationState CurrentState;
    FString FullUserTranscript;
    FString AssistantTranscript;
    
    std::atomic<float> DisplayMicRms;
    TQueue<FString, EQueueMode::Mpsc> PendingUserTranscriptDeltas;
    TQueue<FString, EQueueMode::Mpsc> PendingAssistantTranscriptDeltas;

};