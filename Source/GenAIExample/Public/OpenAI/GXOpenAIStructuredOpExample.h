// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GenAIExampleDelegates.h" // Include our central delegates
#include "Models/OpenAI/GenOAIStructuredOpService.h" // Required for the async action
#include "GXOpenAIStructuredOpExample.generated.h"

UCLASS()
class GENAIEXAMPLE_API AGXOpenAIStructuredOpExample : public AActor
{
	GENERATED_BODY()

public:
	AGXOpenAIStructuredOpExample();

protected:
	// Called when the actor is being destroyed to handle cleanup
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/**
	 * @brief Executes a structured operation request using the async action.
	 * @param UserMessage The prompt to send to the model.
	 * @param ModelName The name of the OpenAI model to use (e.g., "gpt-4o").
	 * @param Schema The JSON schema defining the desired output structure.
	 */
	UFUNCTION(BlueprintCallable, Category = "GenAI|OpenAI Examples")
	void RequestStructuredOperation(const FString& UserMessage, const FString& ModelName, const FString& Schema);

	/** Delegate for Blueprints to receive the result of the structured operation. */
	UPROPERTY(BlueprintAssignable, Category = "GenAI | Events")
	FOnUIStructuredOpResponse OnUIStructuredOpResponse;

private:
	/**
	 * @brief Callback function bound to the OnComplete delegate of the async action.
	 * @param Response The JSON string response from the AI.
	 * @param Error The error message, if any.
	 * @param bSuccess Whether the request was successful.
	 */
	UFUNCTION()
	void OnStructuredOpCompleted(const FString& Response, const FString& Error, bool bSuccess);

	/** A handle to the active async action, used for cancellation. */
	TWeakObjectPtr<UGenOAIStructuredOpService> ActiveStructuredOpRequest;
};