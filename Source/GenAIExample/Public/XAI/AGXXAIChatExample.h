// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenAIChatExample.generated.h"

/**
 * This class demonstrates how to use the GenAI plugin for chat-based AI interactions.
 */
UCLASS(Blueprintable)
class GENAIEXAMPLES_API AGXXAIChatExample : public AActor
{
	GENERATED_BODY()

public:
	/**
	 * @brief Sends a user message for a complete, non-streaming response.
	 * @param UserMessage The text from the user.
	 * @param ModelName The name of the XAI chat model to use.
	 */
	UFUNCTION(BlueprintCallable, Category = "GenAI|XAI Examples")
	void RequestNonStreamingChat(const FString& UserMessage, const FString& ModelName);

	/**
	 * @brief Sends a user message for a streaming response.
	 * @param UserMessage The text from the user.
	 * @param ModelName The name of the XAI chat model to use.
	 */
	UFUNCTION(BlueprintCallable, Category = "GenAI|XAI Examples")
	void RequestStreamingChat(const FString& UserMessage, const FString& ModelName);

	/** Clears the chat history. */
	UFUNCTION(BlueprintCallable, Category = "GenAI | UI Example")
	void ClearChatHistory();

	// -- RESPONSE HANDLING --

	/** Handles the response from the non-streaming chat request. */
	void OnNonStreamingResponse(const FString& Response, const FString& Error, bool bSuccess);

	/** Handles the response from the streaming chat request. */
	void OnStreamingResponse(const FGenXAIStreamEvent& StreamEvent);

	// -- STATE MANAGEMENT --

	/** Stores the conversation history. */
	UPROPERTY(BlueprintReadOnly, Category = "GenAI | State")
	TArray<FString> ChatHistory;

	/** The name of the XAI chat model to use. */
	UPROPERTY(BlueprintReadOnly, Category = "GenAI | State")
	FString ModelName;

	/** Whether the last request was successful. */
	UPROPERTY(BlueprintReadOnly, Category = "GenAI | State")
	bool bLastRequestSuccessful;
};
