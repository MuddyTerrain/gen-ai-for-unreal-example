// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenAIExampleDelegates.h"
#include "GameFramework/Actor.h"
#include "Http.h"
#include "GXGoogleImageExample.generated.h"

UCLASS()
class GENAIEXAMPLE_API AGXGoogleImageExample : public AActor
{
	GENERATED_BODY()

public:
	AGXGoogleImageExample();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

public:
	/**
	 * @brief Requests an image from Google's Imagen models.
	 * @param Prompt A description of the desired image.
	 * @param ModelName The name of the model to use (e.g., "imagen-3.0-generate-preview-06-06").
	 */
	UFUNCTION(BlueprintCallable, Category = "GenAI|Google Examples")
	void RequestGoogleImage(const FString& Prompt, const FString& ModelName);

	/**
	 * @brief Requests an image edit from Google's Gemini models.
	 * @param Prompt A description of the desired edit.
	 * @param Image The image to edit.
	 * @param ModelName The name of the model to use (e.g., "gemini-2.5-flash-image-preview").
	 */
	UFUNCTION(BlueprintCallable, Category = "GenAI|Google Examples")
	void RequestGoogleImageEdit(const FString& Prompt, UTexture2D* Image, const FString& ModelName);

	UPROPERTY(BlueprintAssignable, Category = "GenAI | Events")
	FOnImageGenerated OnImageGenerated;

	UPROPERTY(BlueprintAssignable, Category = "GenAI | Events")
	FOnImageGenerationError OnImageGenerationError;

private:
	void OnImageResponse(const TArray<uint8>& ImageBytes, const FString& Error, bool bSuccess);

	FHttpRequestPtr ActiveRequest;
};
