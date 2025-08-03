// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "XAI/AGXXAIChatExample.h"
#include "Models/XAI/GenXAIChat.h"
#include "Models/XAI/GenXAIChatStream.h"
#include "Data/XAI/GenXAIChatStructs.h"
#include "Data/XAI/GenXAIChatStreamStructs.h"

AGXXAIChatExample::AGXXAIChatExample()
{
}

void AGXXAIChatExample::BeginPlay()
{
    Super::BeginPlay();
}

void AGXXAIChatExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (ActiveRequestNonStreaming.IsValid())
    {
        ActiveRequestNonStreaming->CancelRequest();
    }

    if (ActiveRequestStreaming.IsValid())
    {
        ActiveRequestStreaming->CancelRequest();
    }

    Super::EndPlay(EndPlayReason);
}

void AGXXAIChatExample::RequestNonStreamingChat(const FString& UserMessage, const FString& ModelName)
{
    if (ActiveRequestNonStreaming.IsValid())
    {
        ActiveRequestNonStreaming->CancelRequest();
    }

    FGenXAIChatSettings ChatSettings;
    ChatSettings.Model = StringToXAIChatModel(ModelName);
    if (ChatSettings.Model == EXAIChatModel::Custom)
    {
        ChatSettings.CustomModelName = ModelName;
    }
    
    // Add user message to settings and history
    ConversationHistory.Add(FGenXAIMessage(TEXT("user"), UserMessage));
    ChatSettings.Messages = ConversationHistory;

    ActiveRequestNonStreaming = UGenXAIChat::SendChatRequest(ChatSettings, FOnXAIChatCompletionResponse::CreateUObject(this, &AGXXAIChatExample::OnNonStreamingResponse));
}

void AGXXAIChatExample::RequestStreamingChat(const FString& UserMessage, const FString& ModelName)
{
    if (ActiveRequestStreaming.IsValid())
    {
        ActiveRequestStreaming->CancelRequest();
    }

    FGenXAIChatSettings ChatSettings;
    ChatSettings.Model = StringToXAIChatModel(ModelName);
    if (ChatSettings.Model == EXAIChatModel::Custom)
    {
        ChatSettings.CustomModelName = ModelName;
    }

    // Add user message to settings and history
    ConversationHistory.Add(FGenXAIMessage(TEXT("user"), UserMessage));
    ChatSettings.Messages = ConversationHistory;

    ActiveRequestStreaming = UGenXAIChatStream::SendStreamChatRequest(ChatSettings, FOnXAIStreamResponse::CreateUObject(this, &AGXXAIChatExample::OnStreamingResponse));
}

void AGXXAIChatExample::ClearConversation()
{
    ConversationHistory.Empty();
}

void AGXXAIChatExample::OnStreamingResponse(const FGenXAIStreamEvent& StreamEvent)
{
    if (!StreamEvent.bSuccess)
    {
        OnUIStreamError.Broadcast(StreamEvent.Error);
        return;
    }

    switch (StreamEvent.EventType)
    {
    case EXAIStreamEventType::ContentDelta:
        OnUIStreamContentUpdate.Broadcast(StreamEvent.Content);
        break;
    case EXAIStreamEventType::Completion:
        ConversationHistory.Add(FGenXAIMessage(TEXT("assistant"), StreamEvent.Content));
        OnUIStreamCompleted.Broadcast(StreamEvent.Content);
        break;
    case EXAIStreamEventType::Error:
        OnUIStreamError.Broadcast(StreamEvent.Error);
        break;
    }
}
