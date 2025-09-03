// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenAIExampleDelegates.h"
#include "GameFramework/Actor.h"
#include "Data/XAI/GenXAIChatStructs.h" // For XAI-specific structs
#include "Http.h"
#include "GXXAIChatExample.generated.h"

// Forward declaration for the stream event enum
enum class EXAIStreamEventType : uint8;


UCLASS()
class GENAIEXAMPLE_API AGXXAIChatExample : public AActor
{
	GENERATED_BODY()

public:
	AGXXAIChatExample();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/**
	 * @brief Sends a user message for a complete, non-streaming response.
	 * @param UserMessage The text from the user.
	 * @param ModelName The name of the XAI chat model to use (e.g., "grok-1.5-flash").
	 * @param Image (Optional) A UTexture2D asset for multimodal chat.
	 */
	UFUNCTION(BlueprintCallable, Category = "GenAI|XAI Examples")
	void RequestNonStreamingChat(const FString& UserMessage, const FString& ModelName, const FString& SystemPrompt = TEXT(""), UTexture2D* Image = nullptr);

	/**
	 * @brief Sends a user message for a streaming response.
	 * @param UserMessage The text from the user.
	 * @param ModelName The name of the XAI chat model to use (e.g., "grok-1.5-flash").
	 * @param Image (Optional) A UTexture2D asset for multimodal chat.
	 */
	UFUNCTION(BlueprintCallable, Category = "GenAI|XAI Examples")
	void RequestStreamingChat(const FString& UserMessage, const FString& ModelName, const FString& SystemPrompt = TEXT(""), UTexture2D* Image = nullptr);

	/** Clears the chat history and resets the system prompt. */
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
	/**
	 * @brief Handles raw streaming events directly from the GenXAIChatStream class.
	 * @param EventType The kind of event (e.g., ContentDelta, Completion).
	 * @param Payload The FString data associated with the event.
	 * @param bSuccess True if the stream is still considered successful.
	 */
	void OnStreamingChatEvent(EXAIStreamEventType EventType, const FString& Payload, bool bSuccess);

	// -- STATE MANAGEMENT --

	/** Stores the full conversation history using the XAI-specific message struct. */
	TArray<FGenXAIMessage> ConversationHistory;

	/** Keeps track of active requests to allow for cancellation. */
	FHttpRequestPtr ActiveRequestNonStreaming;
	FHttpRequestPtr ActiveRequestStreaming;
};
