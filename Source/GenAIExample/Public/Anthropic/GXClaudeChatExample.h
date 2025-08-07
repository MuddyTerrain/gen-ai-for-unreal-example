#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/Anthropic/GenClaudeChatStructs.h"
#include "Http.h"
#include "XAI/GXXAIChatExample.h"
#include "GXClaudeChatExample.generated.h"

struct FGenClaudeStreamEvent;

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
     * @param ImagePath (Optional) The full local path to an image for multimodal chat.
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI|Claude Examples")
    void RequestNonStreamingChat(const FString& UserMessage, const FString& ModelName, const FString& ImagePath);

    // /**
    //  * @brief Sends a user message for a streaming response using Claude.
    //  * @param UserMessage The text from the user.
    //  * @param ModelName The name of the Claude model to use.
    //  * @param ImagePath (Optional) The full local path to an image for multimodal chat.
    //  */
    // UFUNCTION(BlueprintCallable, Category = "GenAI|Claude Examples")
    // void RequestStreamingChat(const FString& UserMessage, const FString& ModelName, const FString& ImagePath);
    //
    /** Clears the chat history. */
    UFUNCTION(BlueprintCallable, Category = "GenAI|Claude Examples")
    void ClearConversation();

    // -- DELEGATES FOR BLUEPRINT UI --
    UPROPERTY(BlueprintAssignable, Category = "GenAI|Events")
    FOnUINonStreamingResponse OnUINonStreamingResponse;

    UPROPERTY(BlueprintAssignable, Category = "GenAI|Events")
    FOnUIStreamingResponseDelta OnUIStreamingResponseDelta;

    UPROPERTY(BlueprintAssignable, Category = "GenAI|Events")
    FOnUIStreamingResponseCompleted OnUIStreamingResponseCompleted;

    UPROPERTY(BlueprintAssignable, Category = "GenAI|Events")
    FOnUIStreamingError OnUIStreamingError;

private:
    // /** Handles the response from the streaming chat request. */
    // void OnStreamingChatEvent(const FGenClaudeStreamEvent& StreamEvent);

    /** Stores the conversation history using Claude's message format */
    TArray<FGenClaudeChatMessage> ConversationHistory;
     
    /** Keeps track of the active HTTP requests to allow cancellation. */
    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveRequestNonStreaming;
    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveRequestStreaming;
};
