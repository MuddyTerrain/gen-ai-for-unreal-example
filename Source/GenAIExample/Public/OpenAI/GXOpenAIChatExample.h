#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/OpenAI/GenOAIChatStructs.h" // Correct struct for chat settings
#include "Http.h"                          // For TWeakObjectPtr and request management
#include "GXOpenAIChatExample.generated.h"

// -- BLUEPRINT-FRIENDLY DELEGATES FOR UI --

struct FGenOpenAIStreamEvent;
// Broadcasts the final, complete AI response for non-streaming calls
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUINonStreamingResponse, const FString&, Message, bool, bSuccess);

// Broadcasts a piece of a streaming response (a "delta")
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIStreamingResponseDelta, const FString&, DeltaContent);

// Signals that the entire streaming response is complete
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIStreamingResponseCompleted, const FString&, FullMessage);

// Signals an error occurred during streaming
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIStreamingError, const FString&, ErrorMessage);


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
     * @param ImagePath (Optional) The full local path to an image for multimodal chat.
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI | UI Example")
    void RequestNonStreamingChat(const FString& UserMessage, const FString& ImagePath = TEXT(""));

    /**
     * @brief Sends a user message for a streaming response.
     * @param UserMessage The text from the user.
     * @param ImagePath (Optional) The full local path to an image for multimodal chat.
     */
    UFUNCTION(BlueprintCallable, Category = "GenAI | UI Example")
    void RequestStreamingChat(const FString& UserMessage, const FString& ImagePath = TEXT(""));

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