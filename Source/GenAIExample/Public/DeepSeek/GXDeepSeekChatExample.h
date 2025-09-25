// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenAIExampleDelegates.h"
#include "GameFramework/Actor.h"
#include "Http.h"

#if WITH_GENAI_MODULE
#include "Data/GenAIMessageStructs.h"
#include "Data/DeepSeek/GenDeepSeekStructs.h"
#endif

#include "GXDeepSeekChatExample.generated.h"

UCLASS()
class GENAIEXAMPLE_API AGXDeepSeekChatExample : public AActor
{
    GENERATED_BODY()

public:
    AGXDeepSeekChatExample();

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    UFUNCTION(BlueprintCallable, Category = "GenAI|DeepSeek Examples")
    void RequestNonStreamingChat(const FString& UserMessage, const FString& ModelName, const FString& SystemPrompt = TEXT(""));

    UFUNCTION(BlueprintCallable, Category = "GenAI|DeepSeek Examples")
    void RequestStreamingChat(const FString& UserMessage, const FString& ModelName, const FString& SystemPrompt = TEXT(""));
    
    UFUNCTION(BlueprintCallable, Category = "GenAI | UI Example")
    void ClearConversation();

    // -- DELEGATES FOR BLUEPRINT UI --

    UPROPERTY(BlueprintAssignable, Category = "GenAI | Events")
    FOnUINonStreamingResponse OnUINonStreamingResponse;

    UPROPERTY(BlueprintAssignable, Category = "GenAI | Events")
    FOnUIStreamingResponseDelta OnUIStreamingResponseDelta;

    UPROPERTY(BlueprintAssignable, Category = "GenAI | Events")
    FOnUIStreamingResponseCompleted OnUIStreamingResponseCompleted;

    UPROPERTY(BlueprintAssignable, Category = "GenAI | Events")
    FOnUIStreamingError OnUIStreamingError;

#if WITH_GENAI_MODULE
private:
    /**
     * @brief Handles raw streaming events directly from the GenDSeekChatStream class.
     * @param EventType The kind of event (e.g., ContentUpdate, Completion).
     * @param Payload The FString data associated with the event (delta content, full message, or error).
     * @param bSuccess True if the stream is still considered successful.
     */
    void OnStreamingChatEvent(EDeepSeekStreamEventType EventType, const FString& Payload, bool bSuccess);

    // -- STATE MANAGEMENT --
    TArray<FGenChatMessage> ConversationHistory;
    FHttpRequestPtr ActiveRequestNonStreaming;
    FHttpRequestPtr ActiveRequestStreaming;
#endif
};