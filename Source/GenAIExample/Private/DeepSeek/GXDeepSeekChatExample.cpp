// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "DeepSeek/GXDeepSeekChatExample.h"
#include "Models/DeepSeek/GenDSeekChat.h"
#include "Models/DeepSeek/GenDSeekChatStream.h"
#include "Data/GenAIMessageStructs.h"
#include "Data/DeepSeek/GenDeepSeekStructs.h"
#include "Utilities/GenUtils.h"


AGXDeepSeekChatExample::AGXDeepSeekChatExample()
{
    PrimaryActorTick.bCanEverTick = false;
    ConversationHistory.Add(FGenChatMessage(TEXT("system"), {FGenAIMessageContent::FromText(TEXT("You are a helpful assistant integrated into an Unreal Engine application."))}));
}

void AGXDeepSeekChatExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
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
    if (ActiveRequestNonStreaming.IsValid()) return;

    ConversationHistory.Add(FGenChatMessage(TEXT("user"), {FGenAIMessageContent::FromText(UserMessage)}));

    FGenDeepSeekChatSettings ChatSettings;
    ChatSettings.Model = ModelName;  // Set model directly as string
    ChatSettings.Messages = ConversationHistory;

    ActiveRequestNonStreaming = UGenDSeekChat::SendChatRequest(
        ChatSettings,
        FOnDSeekChatCompletionResponse::CreateLambda(
            [this](const FString& Response, const FString& Error, bool bSuccess)
            {
                if (!UGenUtils::IsContextStillValid(this)) return;

                if (bSuccess)
                {
                    ConversationHistory.Add(FGenChatMessage(TEXT("assistant"), {FGenAIMessageContent::FromText(Response)}));
                    OnUINonStreamingResponse.Broadcast(Response, true);
                }
                else
                {
                    OnUINonStreamingResponse.Broadcast(Error, false);
                    ConversationHistory.Pop();
                }
                ActiveRequestNonStreaming.Reset();
            })
    );
}

void AGXDeepSeekChatExample::RequestStreamingChat(const FString& UserMessage, const FString& ModelName)
{
    if (ActiveRequestStreaming.IsValid()) return;

    ConversationHistory.Add(FGenChatMessage(TEXT("user"), {FGenAIMessageContent::FromText(UserMessage)}));

    FGenDeepSeekChatSettings ChatSettings;
    ChatSettings.Model = ModelName;  // Set model directly as string
    ChatSettings.Messages = ConversationHistory;

    // This delegate is of type FOnDSeekChatStreamResponse, which we now correctly handle in OnStreamingChatEvent.
    ActiveRequestStreaming = UGenDSeekChatStream::SendStreamChatRequest(ChatSettings, FOnDSeekChatStreamResponse::CreateUObject(this, &AGXDeepSeekChatExample::OnStreamingChatEvent));
}

void AGXDeepSeekChatExample::ClearConversation()
{
    ConversationHistory.Empty();
    ConversationHistory.Add(FGenChatMessage(TEXT("system"), {FGenAIMessageContent::FromText(TEXT("You are a helpful assistant integrated into an Unreal Engine application."))}));
}

void AGXDeepSeekChatExample::OnStreamingChatEvent(EDeepSeekStreamEventType EventType, const FString& Payload, bool bSuccess)
{
    if (!UGenUtils::IsContextStillValid(this)) return;

    if (!bSuccess)
    {
        // For any kind of failure, broadcast the error, pop the user message, and reset.
        OnUIStreamingError.Broadcast(Payload);
        ConversationHistory.Pop();
        ActiveRequestStreaming.Reset();
        return;
    }

    switch (EventType)
    {
        case EDeepSeekStreamEventType::ContentUpdate:
            // The payload is a delta chunk of the message.
            OnUIStreamingResponseDelta.Broadcast(Payload);
            break;

        case EDeepSeekStreamEventType::Completion:
            // The payload is the final, complete message.
            ConversationHistory.Add(FGenChatMessage(TEXT("assistant"), {FGenAIMessageContent::FromText(Payload)}));
            OnUIStreamingResponseCompleted.Broadcast(Payload);
            ActiveRequestStreaming.Reset();
            break;

        case EDeepSeekStreamEventType::Error:
             // This case is now handled by the initial !bSuccess check, but we keep it for clarity.
             OnUIStreamingError.Broadcast(Payload);
             ConversationHistory.Pop();
             ActiveRequestStreaming.Reset();
             break;
        
        case EDeepSeekStreamEventType::ReasoningUpdate:
            // Ignored for this example.
            break;
    }
}