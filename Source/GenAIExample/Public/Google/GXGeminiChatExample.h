// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenAIExampleDelegates.h"
#include "GameFramework/Actor.h"
#include "Http.h"

#if WITH_GENAI_MODULE
#include "Data/Google/GenGeminiChatStructs.h"
#include "Data/Google/GenGeminiStreamStructs.h"
#endif

#include "GXGeminiChatExample.generated.h"

UCLASS()
class GENAIEXAMPLE_API AGXGeminiChatExample : public AActor
{
    GENERATED_BODY()

public:
    AGXGeminiChatExample();

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    /**
     * @brief Sends a user message for a complete, non-streaming response from Gemini.
     * @param UserMessage The text from the user.
     * @param ModelName The name of the Google Gemini model to use.
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI|Google Examples")
    void RequestNonStreamingChat(const FString& UserMessage, const FString& ModelName, const FString& SystemPrompt = TEXT(""));

    /**
     * @brief Sends a user message for a streaming response from Gemini.
     * @param UserMessage The text from the user.
     * @param ModelName The name of the Google Gemini model to use.
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI|Google Examples")
    void RequestStreamingChat(const FString& UserMessage, const FString& ModelName, const FString& SystemPrompt = TEXT(""));

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

#if WITH_GENAI_MODULE
private:
    // -- CORE PLUGIN INTEGRATION --

    /** Handles the response from the streaming chat request. */
    void OnStreamingChatEvent(EGoogleGeminiStreamEventType EventType, const FGeminiGenerateContentResponseChunk& Chunk, const FString& ErrorMessage, bool bSuccess);

    // -- STATE MANAGEMENT --

    /** Stores the conversation history. */
    TArray<FGenGeminiMessage> ConversationHistory;

    /** Keeps track of the active HTTP requests to allow cancellation. */
    FHttpRequestPtr ActiveRequestNonStreaming;
    FHttpRequestPtr ActiveRequestStreaming;
    
    /** Accumulates the full response from a streaming request. */
    FString AccumulatedStreamedResponse;
#endif
};

