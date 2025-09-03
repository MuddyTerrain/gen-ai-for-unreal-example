// Copyright 2025, Muddy Terrain Games, All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/OpenAI/GenOAIChatStructs.h" // Correct struct for chat settings
#include "Http.h"                          // For TWeakObjectPtr and request management
#include "XAI/GXXAIChatExample.h"
#include "GXOpenAIChatExample.generated.h"

// -- BLUEPRINT-FRIENDLY DELEGATES FOR UI --

struct FGenOpenAIStreamEvent;


UCLASS()
class GENAIEXAMPLE_API AGXOpenAIChatExample : public AActor
{
    GENERATED_BODY()

public:
    AGXOpenAIChatExample();

protected:
    // Called when the actor is being destroyed
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    /**
     * @brief Sends a user message for a complete, non-streaming response.
     * @param UserMessage The text from the user.
     * @param ModelName The name of the OpenAI chat model to use.
     * @param Image (Optional) A UTexture2D asset for multimodal chat.
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI|OpenAI Examples")
    void RequestNonStreamingChat(const FString& UserMessage, const FString& ModelName, const FString& SystemPrompt = TEXT(""), UTexture2D* Image = nullptr);

    /**
     * @brief Sends a user message for a streaming response.
     * @param UserMessage The text from the user.
     * @param ModelName The name of the OpenAI chat model to use.
     * @param Image (Optional) A UTexture2D asset for multimodal chat.
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI|OpenAI Examples")
    void RequestStreamingChat(const FString& UserMessage, const FString& ModelName, const FString& SystemPrompt = TEXT(""), UTexture2D* Image = nullptr);
    
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

    /** Handles the response from the streaming chat request. */
    void OnStreamingChatEvent(const FGenOpenAIStreamEvent& StreamEvent);

    // -- STATE MANAGEMENT --

    /** Stores the conversation history. Uses the correct FGenChatMessage struct. */
    TArray<FGenChatMessage> ConversationHistory;
     
    /** Keeps track of the active HTTP requests to allow cancellation. */
    FHttpRequestPtr ActiveRequestNonStreaming;
    FHttpRequestPtr ActiveRequestStreaming;
};