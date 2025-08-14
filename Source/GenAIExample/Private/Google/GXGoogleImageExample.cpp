// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "Google/GXGoogleImageExample.h"

#include "Models/Google/GenGoogleImageGeneration.h"
#include "Data/Google/GenGoogleImageStructs.h"
#include "Utilities/GenUtils.h"
#include "ImageUtils.h" // Include for FImageUtils

AGXGoogleImageExample::AGXGoogleImageExample()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AGXGoogleImageExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (ActiveRequest.IsValid() && ActiveRequest->GetStatus() == EHttpRequestStatus::Processing)
	{
		ActiveRequest->CancelRequest();
	}
	Super::EndPlay(EndPlayReason);
}

void AGXGoogleImageExample::RequestGoogleImage(const FString& Prompt, const FString& ModelName)
{
	if (ActiveRequest.IsValid()) return;

	FGenGoogleImageSettings Settings;
	Settings.Prompt = Prompt;
	Settings.Model = ModelName;  // Set model directly as string
	Settings.AspectRatio = EGenGoogleImageAspectRatio::Ratio_1_1;
	Settings.NumberOfImages = 1;

	ActiveRequest = UGenGoogleImageGeneration::SendImageGenerationRequest(Settings, FOnGoogleImageGenerationCompletionResponse::CreateUObject(this, &AGXGoogleImageExample::OnImageResponse));
}

void AGXGoogleImageExample::OnImageResponse(const TArray<uint8>& ImageBytes, const FString& Error, bool bSuccess)
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
