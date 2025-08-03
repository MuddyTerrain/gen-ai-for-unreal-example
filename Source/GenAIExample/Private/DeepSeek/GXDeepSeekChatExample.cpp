// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "DeepSeek/GXDeepSeekChatExample.h"
#include "Models/DeepSeek/GenDSeekChat.h"
#include "Models/DeepSeek/GenDSeekChatStream.h"
#include "Data/GenAIMessageStructs.h"
#include "Data/DeepSeek/GenDeepSeekStructs.h"
#include "Utilities/GenUtils.h" // Required for context validation
#include "UObject/UObjectGlobals.h" // Required for StaticEnum

/**
 * @brief Converts a model name string to the corresponding EDeepSeekModels enum value.
 *
 * This allows for flexible model selection from Blueprints. If no standard model
 * name matches, it defaults to EDeepSeekModels::Chat.
 *
 * @param ModelName The model name to convert.
 * @return The corresponding EDeepSeekModels enum value.
 */
static EDeepSeekModels StringToDeepSeekModel(const FString& ModelName)
{
    const UEnum* EnumPtr = StaticEnum<EDeepSeekModels>();
    if (EnumPtr)
    {
        for (int32 i = 0; i < EnumPtr->NumEnums() - 1; ++i)
        {
            // Note: We need a ToString function for EDeepSeekModels similar to the OpenAI version.
            // Assuming one exists, e.g., DeepSeekModelToString(). If not, this needs implementation.
            // For now, we'll compare against the display name as a fallback.
            FString EnumName = EnumPtr->GetDisplayNameTextByIndex(i).ToString();
            if (ModelName.Equals(EnumName, ESearchCase::IgnoreCase))
            {
                return static_cast<EDeepSeekModels>(EnumPtr->GetValueByIndex(i));
            }
        }
    }
    // Default if no match is found
    return EDeepSeekModels::Chat;
}


AGXDeepSeekChatExample::AGXDeepSeekChatExample()
{
    PrimaryActorTick.bCanEverTick = false;
    // Set a default system message to guide the AI's behavior
    ConversationHistory.Add(FGenChatMessage(TEXT("system"), {FGenAIMessageContent::FromText(TEXT("You are a helpful assistant integrated into an Unreal Engine application."))}));
}

void AGXDeepSeekChatExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Cancel any in-flight requests to prevent crashes on actor destruction
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

void AGXDeepSeekChatExample::RequestNonStreamingChat(const FString& UserMessage, const FString& ModelName)
{
    if (ActiveRequestNonStreaming.IsValid())
    {
        // Don't start a new request if one is already active.
        return;
    }

    // Add user message to history before sending
    ConversationHistory.Add(FGenChatMessage(TEXT("user"), {FGenAIMessageContent::FromText(UserMessage)}));

    FGenDSeekChatSettings ChatSettings;
    ChatSettings.Model = UGenUtils::StringToModel<EDeepSeekModels>(ModelName, DeepSeekModelToString, EDeepSeekModels::Unknown);
    ChatSettings.Messages = ConversationHistory;

    // Send the request using a lambda for the callback to ensure robust handling
    ActiveRequestNonStreaming = UGenDSeekChat::SendChatRequest(
        ChatSettings,
        FOnDSeekChatCompletionResponse::CreateLambda(
            [this](const FString& Response, const FString& Error, bool bSuccess)
            {
                // Ensure the actor context is still valid before processing the response
                if (!UGenUtils::IsContextStillValid(this)) return;

                if (bSuccess)
                {
                    // Add AI's response to history and broadcast to UI
                    ConversationHistory.Add(FGenChatMessage(TEXT("assistant"), {FGenAIMessageContent::FromText(Response)}));
                    OnUINonStreamingResponse.Broadcast(Response, true);
                }
                else
                {
                    // Broadcast error to UI and remove the user message we added to keep history consistent
                    OnUINonStreamingResponse.Broadcast(Error, false);
                    ConversationHistory.Pop();
                }
                // Reset the handle now that the request is complete
                ActiveRequestNonStreaming.Reset();
            })
    );
}

void AGXDeepSeekChatExample::RequestStreamingChat(const FString& UserMessage, const FString& ModelName)
{
    if (ActiveRequestStreaming.IsValid())
    {
        // Don't start a new request if one is already active.
        return;
    }

    // Add user message to history
    ConversationHistory.Add(FGenChatMessage(TEXT("user"), {FGenAIMessageContent::FromText(UserMessage)}));

    FGenDSeekChatSettings ChatSettings;
    ChatSettings.Model = UGenUtils::StringToModel<EDeepSeekModels>(ModelName, DeepSeekModelToString, EDeepSeekModels::Unknown);
    ChatSettings.Messages = ConversationHistory;

    // Send the request, binding our handler function to the delegate
    ActiveRequestStreaming = UGenDSeekChatStream::SendStreamChatRequest(ChatSettings, FOnDSeekChatStreamResponse::CreateUObject(this, &AGXDeepSeekChatExample::OnStreamingChatEvent));
}

void AGXDeepSeekChatExample::ClearConversation()
{
    ConversationHistory.Empty();
    // Re-add the initial system message after clearing
    ConversationHistory.Add(FGenChatMessage(TEXT("system"), {FGenAIMessageContent::FromText(TEXT("You are a helpful assistant integrated into an Unreal Engine application."))}));
}

void AGXDeepSeekChatExample::OnStreamingChatEvent(const FGenDeepSeekStreamEvent& StreamEvent)
{
    // Ensure the actor context is still valid
    if (!UGenUtils::IsContextStillValid(this)) return;

    if (!StreamEvent.bSuccess)
    {
        OnUIStreamingError.Broadcast(StreamEvent.Error);
        // If the stream fails, pop the user's message and reset the request
        ConversationHistory.Pop();
        ActiveRequestStreaming.Reset();
        return;
    }

    switch (StreamEvent.EventType)
    {
    case EDeepSeekStreamEventType::ContentUpdate:
        OnUIStreamingResponseDelta.Broadcast(StreamEvent.Content);
        break;
    case EDeepSeekStreamEventType::Completion:
        // The full message is in the 'Content' field upon completion
        ConversationHistory.Add(FGenChatMessage(TEXT("assistant"), {FGenAIMessageContent::FromText(StreamEvent.Content)}));
        OnUIStreamingResponseCompleted.Broadcast(StreamEvent.Content);
        ActiveRequestStreaming.Reset(); // Reset handle on success
        break;
    case EDeepSeekStreamEventType::Error:
        OnUIStreamingError.Broadcast(StreamEvent.Error);
        ConversationHistory.Pop(); // Pop user message on error
        ActiveRequestStreaming.Reset(); // Reset handle on error
        break;
    case EDeepSeekStreamEventType::ReasoningUpdate:
        // Ignored to align with the OpenAI example's functionality
        break;
    }
}