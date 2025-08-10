// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GenAIExampleDelegates.generated.h"

// -- SHARED BLUEPRINT-FRIENDLY DELEGATES FOR ALL UI EXAMPLES --
// Defining these in one central place prevents LNK2005 linker errors.

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUINonStreamingResponse, const FString&, Message, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIStreamingResponseDelta, const FString&, DeltaContent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIStreamingResponseCompleted, const FString&, FullMessage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnUIStreamingError, const FString&, ErrorMessage);

// -- Image Generation Delegates --
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnImageGenerated, UTexture2D*, GeneratedTexture, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnImageGenerationError, const FString&, ErrorMessage);


// -- Text-to-Speech/Transcription Delegates --

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnUITTSResponse, USoundWave*, AudioWave, const FString&, Error, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnUITranscriptionResponse, const FString&, TranscribedText, bool, bSuccess);


/**
 * 
 */
UCLASS()
class GENAIEXAMPLE_API UGenAIExampleDelegates : public UObject
{
	GENERATED_BODY()
};
