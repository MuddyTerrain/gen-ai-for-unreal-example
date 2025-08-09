#pragma once

#include "CoreMinimal.h"
#include "GenAIExampleDelegates.h"
#include "GameFramework/Actor.h"
#include "Data/Anthropic/GenClaudeChatStructs.h"
#include "Http.h"
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
     * @param Image (Optional) A UTexture2D asset for multimodal chat.
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI|Claude Examples")
    void RequestNonStreamingChat(const FString& UserMessage, const FString& ModelName, UTexture2D* Image = nullptr);


    /** Clears the chat history. */
    UFUNCTION(BlueprintCallable, Category = "GenAI|Claude Examples")
    void ClearConversation();

    // -- DELEGATES FOR BLUEPRINT UI --
    UPROPERTY(BlueprintAssignable, Category = "GenAI|Events")
    FOnUINonStreamingResponse OnUINonStreamingResponse;

private:

    /** Stores the conversation history using Claude's message format */
    TArray<FGenClaudeChatMessage> ConversationHistory;
     
    /** Keeps track of the active HTTP requests to allow cancellation. */
    TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> ActiveRequestNonStreaming;
};
