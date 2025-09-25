// Copyright 2025, Muddy Terrain Games, All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GenAIExampleDelegates.h"
#include "GameFramework/Actor.h"
#if WITH_GENAI_MODULE
#include "Data/Anthropic/GenClaudeChatStructs.h"
#include "Http.h"
#endif
#include "GXClaudeChatExample.generated.h"

UCLASS()
class GENAIEXAMPLE_API AGXClaudeChatExample : public AActor
{
    GENERATED_BODY()

public:
    AGXClaudeChatExample();

protected:
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
    /**
     * @brief Sends a user message for a complete, non-streaming response using Claude.
     * @param UserMessage The text from the user.
     * @param ModelName The name of the Claude model to use.
     * @param Image (Optional) A UTexture2D asset for multimodal chat.
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI|Claude Examples")
    void RequestNonStreamingChat(const FString& UserMessage, const FString& ModelName, const FString& SystemPrompt = TEXT(""), UTexture2D* Image = nullptr);


    /** Clears the chat history. */
    UFUNCTION(BlueprintCallable, Category = "GenAI|Claude Examples")
    void ClearConversation();

    // -- DELEGATES FOR BLUEPRINT UI --
    UPROPERTY(BlueprintAssignable, Category = "GenAI|Events")
    FOnUINonStreamingResponse OnUINonStreamingResponse;

#if WITH_GENAI_MODULE
private:

    /** Stores the conversation history using Claude's message format */
    TArray<FGenClaudeChatMessage> ConversationHistory;
     
    /** Keeps track of the active HTTP requests to allow cancellation. */
    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveRequestNonStreaming;
#endif
};
