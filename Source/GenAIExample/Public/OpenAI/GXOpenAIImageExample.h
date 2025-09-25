// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenAIExampleDelegates.h"
#include "GameFramework/Actor.h"
#if WITH_GENAI_MODULE
#include "Http.h"
#endif
#include "GXOpenAIImageExample.generated.h"

UCLASS()
class GENAIEXAMPLE_API AGXOpenAIImageExample : public AActor
{
	GENERATED_BODY()

public:
	AGXOpenAIImageExample();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/**
	 * @brief Requests an image from OpenAI's DALL-E models.
	 * @param Prompt A description of the desired image.
	 * @param ModelName The name of the model to use (e.g., "dall-e-3").
	 */
	UFUNCTION(BlueprintCallable, Category = "GenAI|OpenAI Examples")
	void RequestOpenAIImage(const FString& Prompt, const FString& ModelName);

	UPROPERTY(BlueprintAssignable, Category = "GenAI | Events")
	FOnImageGenerated OnImageGenerated;

	UPROPERTY(BlueprintAssignable, Category = "GenAI | Events")
	FOnImageGenerationError OnImageGenerationError;

#if WITH_GENAI_MODULE
private:
	void OnImageResponse(const TArray<uint8>& ImageBytes, const FString& Error, bool bSuccess);

	FHttpRequestPtr ActiveRequest;
#endif
};
