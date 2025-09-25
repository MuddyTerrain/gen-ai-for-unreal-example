// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "Google/GXGoogleImageExample.h"

#if WITH_GENAI_MODULE
#include "Models/Google/GenGoogleImageGeneration.h"
#include "Data/Google/GenGoogleImageStructs.h"
#include "Utilities/GenUtils.h"
#include "ImageUtils.h" // Include for FImageUtils
#include "IImageWrapperModule.h"
#include "IImageWrapper.h"
#endif

AGXGoogleImageExample::AGXGoogleImageExample()
{
#if WITH_GENAI_MODULE
	PrimaryActorTick.bCanEverTick = false;
#endif
}

void AGXGoogleImageExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
#if WITH_GENAI_MODULE
	if (ActiveRequest.IsValid() && ActiveRequest->GetStatus() == EHttpRequestStatus::Processing)
	{
		ActiveRequest->CancelRequest();
	}
#endif
	Super::EndPlay(EndPlayReason);
}

void AGXGoogleImageExample::RequestGoogleImage(const FString& Prompt, const FString& ModelName)
{
#if WITH_GENAI_MODULE
	if (ActiveRequest.IsValid()) return;

	FGenGoogleImageSettings Settings;
	Settings.Prompt = Prompt;
	Settings.Model = ModelName;  // Set model directly as string
	Settings.AspectRatio = EGenGoogleImageAspectRatio::Ratio_1_1;
	Settings.NumberOfImages = 1;

	ActiveRequest = UGenGoogleImageGeneration::SendImageGenerationRequest(Settings, FOnGoogleImageGenerationCompletionResponse::CreateUObject(this, &AGXGoogleImageExample::OnImageResponse));
#else
	UE_LOG(LogTemp, Warning, TEXT("GenAI module is not available. RequestGoogleImage will do nothing."));
#endif
}

void AGXGoogleImageExample::RequestGoogleImageEdit(const FString& Prompt, UTexture2D* Image, const FString& ModelName)
{
#if WITH_GENAI_MODULE
	if (ActiveRequest.IsValid()) return;
	if (!Image)
	{
		OnImageGenerationError.Broadcast("Input image is null.");
		return;
	}

	FGenGoogleImageSettings Settings;
	Settings.Prompt = Prompt;
	Settings.Model = ModelName;

	TArray<uint8> ImageData;
	if (Image->GetPlatformData() && Image->GetPlatformData()->Mips.Num() > 0)
	{
		FTexture2DMipMap& Mip = Image->GetPlatformData()->Mips[0];
		void* TextureData = Mip.BulkData.Lock(LOCK_READ_ONLY);
		
		IImageWrapperModule& ImageWrapperModule = FModuleManager::LoadModuleChecked<IImageWrapperModule>(FName("ImageWrapper"));
		TSharedPtr<IImageWrapper> ImageWrapper = ImageWrapperModule.CreateImageWrapper(EImageFormat::PNG);

		if (ImageWrapper.IsValid() && ImageWrapper->SetRaw(TextureData, Mip.BulkData.GetBulkDataSize(), Image->GetSizeX(), Image->GetSizeY(), ERGBFormat::BGRA, 8))
		{
			TArray64<uint8> CompressedData = ImageWrapper->GetCompressed(100);
			ImageData.Append(CompressedData.GetData(), CompressedData.Num());
		}

		Mip.BulkData.Unlock();
	}

	if (ImageData.Num() == 0)
	{
		OnImageGenerationError.Broadcast("Failed to convert image to bytes.");
		return;
	}
	
	Settings.ImageBytes = ImageData;
	Settings.MimeType = TEXT("image/png");

	ActiveRequest = UGenGoogleImageGeneration::SendImageGenerationRequest(Settings, FOnGoogleImageGenerationCompletionResponse::CreateUObject(this, &AGXGoogleImageExample::OnImageResponse));
#else
	UE_LOG(LogTemp, Warning, TEXT("GenAI module is not available. RequestGoogleImageEdit will do nothing."));
#endif
}

#if WITH_GENAI_MODULE
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
#endif