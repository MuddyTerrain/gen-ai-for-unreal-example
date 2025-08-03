// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/GenAIMessageStructs.h" // For FGenChatMessage
#include "Http.h" // For TWeakObjectPtr and request management
#include "GXDeepSeekChatExample.generated.h"

// -- BLUEPRINT-FRIENDLY DELEGATES FOR UI --

struct FGenDeepSeekStreamEvent;

// Broadcasts the final, complete AI response for non-streaming calls
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUINonStreamingResponse, const FString&, Message, bool, bSuccess);

// Broadcasts a piece of a streaming response (a "delta")
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIStreamingResponseDelta, const FString&, DeltaContent);

// Signals that the entire streaming response is complete
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIStreamingResponseCompleted, const FString&, FullMessage);

// Signals an error occurred during streaming
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIStreamingError, const FString&, ErrorMessage);


UCLASS()
class GENAIEXAMPLE_API AGXDeepSeekChatExample : public AActor
{
    GENERATED_BODY()

public:
    AGXDeepSeekChatExample();

protected:
    // Called when the actor is being destroyed
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    /**
     * @brief Sends a user message for a complete, non-streaming response.
     * @param UserMessage The text from the user.
     * @param ModelName The name of the DeepSeek chat model to use (e.g., "deepseek-chat").
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI|DeepSeek Examples")
    void RequestNonStreamingChat(const FString& UserMessage, const FString& ModelName);

    /**
     * @brief Sends a user message for a streaming response.
     * @param UserMessage The text from the user.
     * @param ModelName The name of the DeepSeek chat model to use (e.g., "deepseek-chat").
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI|DeepSeek Examples")
    void RequestStreamingChat(const FString& UserMessage, const FString& ModelName);
    
    /** Clears the chat history. */
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

private:
    // -- CORE PLUGIN INTEGRATION --
    
    /** Handles incoming events from the streaming chat request. */
    void OnStreamingChatEvent(const FGenDeepSeekStreamEvent& StreamEvent);

    // -- STATE MANAGEMENT --

    /** Stores the full conversation history, including system, user, and assistant messages. */
    TArray<FGenChatMessage> ConversationHistory;
     
    /** Keeps track of the active HTTP requests to allow for cancellation. */
    FHttpRequestPtr ActiveRequestNonStreaming;
    FHttpRequestPtr ActiveRequestStreaming;
};