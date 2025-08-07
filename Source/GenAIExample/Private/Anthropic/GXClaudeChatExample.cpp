// Copyright 2025, Muddy Terrain Games, All Rights Reserved.

#include "Anthropic/GXClaudeChatExample.h"
#include "Models/Anthropic/GenClaudeChat.h"
#include "Utilities/GenUtils.h"
#include "Misc/Paths.h"

/**
 * @brief Converts a model name string to the corresponding EClaudeModels enum value.
 * If no match is found, defaults to Claude_Opus_4.
 */
static EClaudeModels StringToClaudeModel(const FString& ModelName)
{
    return UGenUtils::StringToModel<EClaudeModels>(ModelName, ClaudeModelToString, EClaudeModels::Claude_Opus_4);
}

AGXClaudeChatExample::AGXClaudeChatExample()
{
    PrimaryActorTick.bCanEverTick = false;
    // Set a default system message
    ConversationHistory.Add(FGenClaudeChatMessage(TEXT("system"), TEXT("You are Claude, a helpful AI assistant integrated into an Unreal Engine application.")));
}

void AGXClaudeChatExample::ClearConversation()
{
    ConversationHistory.Empty();
    ConversationHistory.Add(FGenClaudeChatMessage(TEXT("system"), TEXT("You are Claude, a helpful AI assistant integrated into an Unreal Engine application.")));
}

void AGXClaudeChatExample::RequestNonStreamingChat(const FString& UserMessage, const FString& ModelName, const FString& ImagePath)
{
    // 1. Construct the message content (text and optional image)
    TArray<FGenAIMessageContent> MessageContent;
    MessageContent.Add(FGenAIMessageContent::FromText(UserMessage));
    
    if (!ImagePath.IsEmpty() && FPaths::FileExists(ImagePath))
    {
        MessageContent.Add(FGenAIMessageContent::FromImagePath(ImagePath, EGenAIImageDetail::Auto));
    }

    // 2. Add the user message to history
    ConversationHistory.Add(FGenClaudeChatMessage(TEXT("user"), MessageContent));

    // 3. Configure chat settings
    FGenClaudeChatSettings ChatSettings;
    ChatSettings.Model = StringToClaudeModel(ModelName);
    ChatSettings.Messages = ConversationHistory;
    ChatSettings.MaxTokens = 1500;
    ChatSettings.bStreamResponse = false;

    // 4. Send the request
    ActiveRequestNonStreaming = UGenClaudeChat::SendChatRequest(
        ChatSettings,
        FOnClaudeChatCompletionResponse::CreateLambda(
            [this](const FString& Response, const FString& ErrorMessage, bool bSuccess)
            {
                if (!UGenUtils::IsContextStillValid(this)) return;

                if (bSuccess)
                {
                    // Add AI's response to history and broadcast to UI
                    ConversationHistory.Add(FGenClaudeChatMessage(TEXT("assistant"), Response));
                    OnUINonStreamingResponse.Broadcast(Response, true);
                }
                else
                {
                    // Broadcast error to UI
                    OnUINonStreamingResponse.Broadcast(ErrorMessage, false);
                    // Remove the user message on failure
                    ConversationHistory.Pop();
                }
                ActiveRequestNonStreaming.Reset();
            })
    );
}

// void AGXClaudeChatExample::RequestStreamingChat(const FString& UserMessage, const FString& ModelName, const FString& ImagePath)
// {
//     // 1. Construct the message content
//     TArray<FGenAIMessageContent> MessageContent;
//     MessageContent.Add(FGenAIMessageContent::FromText(UserMessage));
//
//     if (!ImagePath.IsEmpty() && FPaths::FileExists(ImagePath))
//     {
//         MessageContent.Add(FGenAIMessageContent::FromImagePath(ImagePath, EGenAIImageDetail::Auto));
//     }
//     
//     // 2. Add to history
//     ConversationHistory.Add(FGenClaudeChatMessage(TEXT("user"), MessageContent));
//
//     // 3. Configure settings
//     FGenClaudeChatSettings ChatSettings;
//     ChatSettings.Model = StringToClaudeModel(ModelName);
//     ChatSettings.Messages = ConversationHistory;
//     ChatSettings.bStreamResponse = true;
//
//     // 4. Send the streaming request
//     ActiveRequestStreaming = UGenClaudeChatStream::SendStreamChatRequest(
//         ChatSettings,
//         FOnClaudeStreamResponse::CreateUObject(this, &AGXClaudeChatExample::OnStreamingChatEvent)
//     );
// }
//
// void AGXClaudeChatExample::OnStreamingChatEvent(const FGenClaudeStreamEvent& StreamEvent)
// {
//     if (!UGenUtils::IsContextStillValid(this)) return;
//
//     if (!StreamEvent.bSuccess)
//     {
//         OnUIStreamingError.Broadcast(StreamEvent.ErrorMessage);
//         ConversationHistory.Pop();
//         ActiveRequestStreaming.Reset();
//         return;
//     }
//
//     switch (StreamEvent.EventType)
//     {
//         case EClaudeStreamEventType::ResponseOutputTextDelta:
//             if (!StreamEvent.DeltaContent.IsEmpty())
//             {
//                 OnUIStreamingResponseDelta.Broadcast(StreamEvent.DeltaContent);
//             }
//             break;
//
//         case EClaudeStreamEventType::ResponseCompleted:
//             ConversationHistory.Add(FGenClaudeChatMessage(TEXT("assistant"), StreamEvent.DeltaContent));
//             OnUIStreamingResponseCompleted.Broadcast(StreamEvent.DeltaContent);
//             ActiveRequestStreaming.Reset();
//             break;
//
//         case EClaudeStreamEventType::ResponseFailed:
//         case EClaudeStreamEventType::Error:
//             OnUIStreamingError.Broadcast(StreamEvent.ErrorMessage);
//             ConversationHistory.Pop();
//             ActiveRequestStreaming.Reset();
//             break;
//             
//         default:
//             break;
//     }
// }

void AGXClaudeChatExample::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    // Cancel any active requests
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
