// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "OpenAI/GXOpenAIStructuredOpExample.h"
#include "Data/OpenAI/GenOAIChatStructs.h" // Contains FGenOAIStructuredChatSettings
#include "Data/GenAIMessageStructs.h"    // Contains FGenChatMessage

AGXOpenAIStructuredOpExample::AGXOpenAIStructuredOpExample()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AGXOpenAIStructuredOpExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Cancel any active request when the actor is destroyed
    if (ActiveStructuredOpRequest.IsValid())
    {
        ActiveStructuredOpRequest->Cancel();
    }
    Super::EndPlay(EndPlayReason);
}

void AGXOpenAIStructuredOpExample::RequestStructuredOperation(const FString& UserMessage, const FString& ModelName, const FString& Schema, const FString& SystemPrompt)
{
    // If there's an ongoing request, cancel it before starting a new one.
    if (ActiveStructuredOpRequest.IsValid())
    {
        ActiveStructuredOpRequest->Cancel();
    }

    // Set up the chat settings for the structured operation
    FGenOAIStructuredChatSettings StructuredChatSettings;
    StructuredChatSettings.ChatSettings.Model = ModelName;
    StructuredChatSettings.SchemaJson = Schema;

    if (!SystemPrompt.IsEmpty())
    {
        StructuredChatSettings.ChatSettings.Messages.Add(FGenChatMessage(TEXT("system"), SystemPrompt));
    }

    // **CORRECTED PART**
    // Correctly construct the FGenChatMessage using the provided constructor,
    // which handles the FGenAIMessageContent wrapper internally.
    // The role is passed as an FString, e.g., "user".
    StructuredChatSettings.ChatSettings.Messages.Add(FGenChatMessage(TEXT("user"), UserMessage));
    
    // Create and activate the asynchronous action
    UGenOAIStructuredOpService* AsyncAction = UGenOAIStructuredOpService::RequestOpenAIStructuredOutput(this, StructuredChatSettings);

    if (AsyncAction)
    {
        // Bind our callback function to the OnComplete delegate
        AsyncAction->OnComplete.AddDynamic(this, &AGXOpenAIStructuredOpExample::OnStructuredOpCompleted);
        ActiveStructuredOpRequest = AsyncAction;
        UE_LOG(LogTemp, Log, TEXT("Requesting structured operation..."));
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to create RequestOpenAIStructuredOutput async action."));
        OnUIStructuredOpResponse.Broadcast(TEXT(""), TEXT("Failed to create async action."), false);
    }
}

void AGXOpenAIStructuredOpExample::OnStructuredOpCompleted(const FString& Response, const FString& Error, bool bSuccess)
{
    if (bSuccess)
    {
        UE_LOG(LogTemp, Log, TEXT("Structured Operation Successful. Response: %s"), *Response);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Structured Operation Failed. Error: %s"), *Error);
    }

    // Broadcast the result to the UI or any other Blueprint listeners
    OnUIStructuredOpResponse.Broadcast(Response, Error, bSuccess);

    // Clear the weak pointer as the action is now complete
    ActiveStructuredOpRequest.Reset();
}