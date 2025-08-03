// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "OpenAI/GXOpenAIChatExample.h"
#include "Models/OpenAI/GenOAIChat.h"         // For non-streaming
#include "Models/OpenAI/GenOAIChatStream.h"  // For streaming
#include "Utilities/GenUtils.h"
#include "Misc/Paths.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/EnumProperty.h"

/**
 * @brief Converts a model name string to the corresponding EOpenAIChatModel enum value.
 *
 * This allows for flexible model selection from Blueprints or external configuration files.
 * It iterates through the known enum values and compares them against the input string.
 * If no match is found, it defaults to 'Custom', allowing the system to use the
 * input string as a custom model identifier.
 *
 * @param ModelName The model name to convert.
 * @return The corresponding EOpenAIChatModel enum value.
 */
static EOpenAIChatModel StringToOpenAIChatModel(const FString& ModelName)
{
    const UEnum* EnumPtr = StaticEnum<EOpenAIChatModel>();
    if (EnumPtr)
    {
        for (int32 i = 0; i < EnumPtr->NumEnums() - 1; ++i)
        {
            if (ModelName.Equals(OpenAIChatModelToString(static_cast<EOpenAIChatModel>(EnumPtr->GetValueByIndex(i))), ESearchCase::IgnoreCase))
            {
                return static_cast<EOpenAIChatModel>(EnumPtr->GetValueByIndex(i));
            }
        }
    }
    // If no standard model matches, treat it as a custom model identifier.
    return EOpenAIChatModel::Custom;
}


AGXOpenAIChatExample::AGXOpenAIChatExample()
{
    PrimaryActorTick.bCanEverTick = false;
    // Set a default system message to guide the AI's behavior
    ConversationHistory.Add(FGenChatMessage(TEXT("system"), TEXT("You are a helpful assistant integrated into an Unreal Engine application.")));
}

void AGXOpenAIChatExample::ClearConversation()
{
    ConversationHistory.Empty();
    ConversationHistory.Add(FGenChatMessage(TEXT("system"), TEXT("You are a helpful assistant integrated into an Unreal Engine application.")));
}

void AGXOpenAIChatExample::RequestNonStreamingChat(const FString& UserMessage, const FString& ModelName, const FString& ImagePath)
{
    // 1. Construct the message content (text and optional image)
    TArray<FGenAIMessageContent> MessageContent;
    MessageContent.Add(FGenAIMessageContent::FromText(UserMessage));
    
    if (!ImagePath.IsEmpty() && FPaths::FileExists(ImagePath))
    {
        MessageContent.Add(FGenAIMessageContent::FromImagePath(ImagePath, EGenAIImageDetail::Auto));
    }

    // 2. Add the complete user message to our history
    ConversationHistory.Add(FGenChatMessage(TEXT("user"), MessageContent));

    // 3. Configure the chat settings
    FGenOAIChatSettings ChatSettings;
    
    // Set the model from the input string
    ChatSettings.Model = StringToOpenAIChatModel(ModelName);
    if (ChatSettings.Model == EOpenAIChatModel::Custom)
    {
        ChatSettings.CustomModelName = ModelName;
    }

    ChatSettings.Messages = ConversationHistory;
    ChatSettings.MaxTokens = 1500;
    ChatSettings.bStream = false;

    // 4. Send the request using the static function and a lambda for the response
    ActiveRequestNonStreaming = UGenOAIChat::SendChatRequest(
        ChatSettings,
        FOnChatCompletionResponse::CreateLambda(
            [this](const FString& Response, const FString& ErrorMessage, bool bSuccess)
            {
                if (!UGenUtils::IsContextStillValid(this)) return;

                if (bSuccess)
                {
                    // Add AI's response to history and broadcast to UI
                    ConversationHistory.Add(FGenChatMessage(TEXT("assistant"), Response));
                    OnUINonStreamingResponse.Broadcast(Response, true);
                }
                else
                {
                    // Broadcast error to UI
                    OnUINonStreamingResponse.Broadcast(ErrorMessage, false);
                    // Important: If the call failed, remove the user message we added
                    // so we don't have a one-sided conversation in our history.
                    ConversationHistory.Pop();
                }
                ActiveRequestNonStreaming.Reset();
            })
    );
}

void AGXOpenAIChatExample::RequestStreamingChat(const FString& UserMessage, const FString& ModelName, const FString& ImagePath)
{
    // 1. Construct the message content
    TArray<FGenAIMessageContent> MessageContent;
    MessageContent.Add(FGenAIMessageContent::FromText(UserMessage));

    if (!ImagePath.IsEmpty() && FPaths::FileExists(ImagePath))
    {
        MessageContent.Add(FGenAIMessageContent::FromImagePath(ImagePath, EGenAIImageDetail::Auto));
    }
    
    // 2. Add to history
    ConversationHistory.Add(FGenChatMessage(TEXT("user"), MessageContent));

    // 3. Configure settings
    FGenOAIChatSettings ChatSettings;
    
    // Set the model from the input string
    ChatSettings.Model = StringToOpenAIChatModel(ModelName);
    if (ChatSettings.Model == EOpenAIChatModel::Custom)
    {
        ChatSettings.CustomModelName = ModelName;
    }

    ChatSettings.Messages = ConversationHistory;
    ChatSettings.bStream = true; // Implicitly handled, but good for clarity

    // 4. Send the request, binding our handler function to the delegate
    ActiveRequestStreaming = UGenOAIChatStream::SendStreamChatRequest(
        ChatSettings, 
        FOnOpenAIChatStreamResponse::CreateUObject(this, &AGXOpenAIChatExample::OnStreamingChatEvent)
    );
}

void AGXOpenAIChatExample::OnStreamingChatEvent(const FGenOpenAIStreamEvent& StreamEvent)
{
    if (!UGenUtils::IsContextStillValid(this)) return;

    if (!StreamEvent.bSuccess)
    {
        // Broadcast a specific error event for the UI
        OnUIStreamingError.Broadcast(StreamEvent.ErrorMessage);
        // Clean up on failure
        ConversationHistory.Pop(); 
        ActiveRequestStreaming.Reset();
        return;
    }

    switch (StreamEvent.EventType)
    {
        case EOpenAIStreamEventType::ResponseOutputTextDelta:
            // This is a chunk of text. Broadcast it to the UI.
            if (!StreamEvent.DeltaContent.IsEmpty())
            {
                OnUIStreamingResponseDelta.Broadcast(StreamEvent.DeltaContent);
            }
            break;

        case EOpenAIStreamEventType::ResponseCompleted:
            // The stream is done. The 'DeltaContent' now holds the *full* message.
            UE_LOG(LogTemp, Log, TEXT("Stream complete. Full message: %s"), *StreamEvent.DeltaContent);
            ConversationHistory.Add(FGenChatMessage(TEXT("assistant"), StreamEvent.DeltaContent));
            OnUIStreamingResponseCompleted.Broadcast(StreamEvent.DeltaContent);
            ActiveRequestStreaming.Reset();
            break;

        case EOpenAIStreamEventType::ResponseFailed:
        case EOpenAIStreamEventType::Error:
             OnUIStreamingError.Broadcast(StreamEvent.ErrorMessage);
             ConversationHistory.Pop();
             ActiveRequestStreaming.Reset();
             break;
        
        // Other cases like ResponseCreated can be ignored for UI purposes
        default:
            break;
    }
}

void AGXOpenAIChatExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Cancel any in-flight requests when the actor is destroyed to prevent crashes.
    if (ActiveRequestNonStreaming.IsValid() && ActiveRequestNonStreaming->GetStatus() == EHttpRequestStatus::Processing)
    {
        ActiveRequestNonStreaming->CancelRequest();
    }
    if (ActiveRequestStreaming.IsValid() && ActiveRequestStreaming->GetStatus() == EHttpRequestStatus::Processing)
    {
        ActiveRequestStreaming->CancelRequest();
    }
    
    Super::EndPlay(EndPlayReason);
}