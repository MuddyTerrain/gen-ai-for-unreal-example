// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "Google/GXGeminiChatExample.h"
#include "Models/Google/GenGeminiChat.h"
#include "Models/Google/GenGeminiChatStream.h"
#include "Utilities/GenUtils.h"


AGXGeminiChatExample::AGXGeminiChatExample()
{
    PrimaryActorTick.bCanEverTick = false;
    // Set a default system message to guide the AI's behavior.
    // Note: Gemini API prefers the 'user'/'model' role cycle. A system prompt is often the first 'user' message.
    ConversationHistory.Add(FGenGeminiMessage(TEXT("user"), FString(TEXT("You are a helpful assistant integrated into an Unreal Engine application. Please keep your responses concise."))));
    ConversationHistory.Add(FGenGeminiMessage(TEXT("model"), FString(TEXT("Okay, I will be a helpful and concise assistant."))));
}

void AGXGeminiChatExample::ClearConversation()
{
    ConversationHistory.Empty();
    ConversationHistory.Add(FGenGeminiMessage(TEXT("user"), FString(TEXT("You are a helpful assistant integrated into an Unreal Engine application. Please keep your responses concise."))));
    ConversationHistory.Add(FGenGeminiMessage(TEXT("model"), FString(TEXT("Okay, I will be a helpful and concise assistant."))));
    AccumulatedStreamedResponse.Empty();
}

void AGXGeminiChatExample::RequestNonStreamingChat(const FString& UserMessage, const FString& ModelName)
{
    // 1. Add the user message to our history
    ConversationHistory.Add(FGenGeminiMessage(TEXT("user"), UserMessage));

    // 2. Configure the chat settings
    FGenGoogleChatSettings ChatSettings;
    ChatSettings.Model = ModelName;
    ChatSettings.Messages = ConversationHistory;
    ChatSettings.MaxOutputTokens = 2048;
    ChatSettings.Temperature = 0.7f;

    // 3. Send the request
    ActiveRequestNonStreaming = UGenGeminiChat::SendChatRequest(
        ChatSettings,
        FOnGeminiChatCompletionResponse::CreateLambda(
            [this](const FString& Response, const FString& ErrorMessage, bool bSuccess)
            {
                if (!UGenUtils::IsContextStillValid(this)) return;

                if (bSuccess)
                {
                    ConversationHistory.Add(FGenGeminiMessage(TEXT("model"), Response));
                    OnUINonStreamingResponse.Broadcast(Response, true);
                }
                else
                {
                    OnUINonStreamingResponse.Broadcast(ErrorMessage, false);
                    ConversationHistory.Pop(); // Remove the user message on failure
                }
                ActiveRequestNonStreaming.Reset();
            })
    );
}

void AGXGeminiChatExample::RequestStreamingChat(const FString& UserMessage, const FString& ModelName)
{
    // 1. Add to history
    ConversationHistory.Add(FGenGeminiMessage(TEXT("user"), UserMessage));
    AccumulatedStreamedResponse.Empty();

    // 2. Configure settings
    FGenGoogleChatSettings ChatSettings;
    ChatSettings.Model = ModelName;
    ChatSettings.Messages = ConversationHistory;

    // 3. Send the request
    ActiveRequestStreaming = UGenGeminiChatStream::SendStreamChatRequest(
        ChatSettings,
        FOnGeminiChatStreamResponse::CreateUObject(this, &AGXGeminiChatExample::OnStreamingChatEvent)
    );
}

void AGXGeminiChatExample::OnStreamingChatEvent(EGoogleGeminiStreamEventType EventType, const FGeminiGenerateContentResponseChunk& Chunk, const FString& ErrorMessage, bool bSuccess)
{
    if (!UGenUtils::IsContextStillValid(this)) return;

    if (!bSuccess)
    {
        OnUIStreamingError.Broadcast(ErrorMessage);
        ConversationHistory.Pop();
        ActiveRequestStreaming.Reset();
        return;
    }

    switch (EventType)
    {
        case EGoogleGeminiStreamEventType::ChunkReceived:
            if (Chunk.Candidates.Num() > 0 && Chunk.Candidates[0].Content.Parts.Num() > 0)
            {
                const FString& DeltaContent = Chunk.Candidates[0].Content.Parts[0].Text;
                if (!DeltaContent.IsEmpty())
                {
                    AccumulatedStreamedResponse += DeltaContent;
                    OnUIStreamingResponseDelta.Broadcast(DeltaContent);
                }
            }
            break;

        case EGoogleGeminiStreamEventType::Completed:
            ConversationHistory.Add(FGenGeminiMessage(TEXT("model"), AccumulatedStreamedResponse));
            OnUIStreamingResponseCompleted.Broadcast(TEXT("")); // Final full message is already accumulated
            ActiveRequestStreaming.Reset();
            break;

        case EGoogleGeminiStreamEventType::Error:
            OnUIStreamingError.Broadcast(ErrorMessage);
            ConversationHistory.Pop();
            ActiveRequestStreaming.Reset();
            break;
        
        default:
            break;
    }
}

void AGXGeminiChatExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
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
