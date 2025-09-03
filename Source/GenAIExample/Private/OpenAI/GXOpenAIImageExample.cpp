// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "OpenAI/GXOpenAIImageExample.h"

#include "Models/OpenAI/GenOAIImageGeneration.h"
#include "Data/OpenAI/GenOAIImageStructs.h"
#include "Utilities/GenUtils.h"
#include "ImageUtils.h"

AGXOpenAIImageExample::AGXOpenAIImageExample()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AGXOpenAIImageExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ActiveRequest.IsValid() && ActiveRequest->GetStatus() == EHttpRequestStatus::Processing)
	{
		ActiveRequest->CancelRequest();
	}
	Super::EndPlay(EndPlayReason);
}

void AGXOpenAIImageExample::RequestOpenAIImage(const FString& Prompt, const FString& ModelName)
{
	if (ActiveRequest.IsValid()) return;

	FGenOAIImageSettings Settings;
	Settings.Prompt = Prompt;
	Settings.Model = ModelName;  // Set model directly as string
	Settings.Size = EGenAIImageSize::Size1024x1024;
	Settings.Quality = EGenAIImageQuality::Standard;
	Settings.N = 1;

	if (ModelName == TEXT("gpt-image-1"))
	{
		Settings.Quality = EGenAIImageQuality::Medium;
	}

	ActiveRequest = UGenOAIImageGeneration::SendImageGenerationRequest(Settings, FOnImageGenerationCompletionResponse::CreateUObject(this, &AGXOpenAIImageExample::OnImageResponse));
}

void AGXOpenAIImageExample::OnImageResponse(const TArray<uint8>& ImageBytes, const FString& Error, bool bSuccess)
{
	if (bSuccess)
	{
		// Use the engine's built-in utility to create the texture
		UTexture2D* Texture = FImageUtils::ImportBufferAsTexture2D(ImageBytes);
		OnImageGenerated.Broadcast(Texture, true);
	}
	else
	{
		OnImageGenerationError.Broadcast(Error);
	}
	ActiveRequest.Reset();
}
